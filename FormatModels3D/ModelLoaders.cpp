#include "ModelLoaders.h"
#include "AssetResolver.h"
#include "SceneIR.h"
#include "GeometryProcessor.h"
#include "SceneBuilder.h"
#include "../Shared/Utils/Log.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/IOSystem.hpp>
#include <assimp/defaultiostream.h>
#include <assimp/config.h>

#ifdef _WIN32
#include <string_view>
#include <windows.h>
#endif

namespace {

#ifdef _WIN32
std::wstring utf8ToUtf16(std::string_view str)
{
	if (str.empty())
		return {};

	const auto length = static_cast<int>(str.length());
	const int requiredSize = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), length, nullptr, 0);
	if (requiredSize <= 0)
		return {};

	std::wstring convertedString(requiredSize, L'\0');
	::MultiByteToWideChar(CP_UTF8, 0, str.data(), length, convertedString.data(), requiredSize);
	return convertedString;
}
#endif

class UnicodeIOStream : public Assimp::DefaultIOStream {
public:
	UnicodeIOStream(FILE* file, const std::string& filename)
		: Assimp::DefaultIOStream(file, filename)
	{}
};

class UnicodeIOSystem : public Assimp::IOSystem {
public:
	bool Exists(const char* fileName) const override
	{
		FILE* file = nullptr;
#ifdef _WIN32
		std::wstring wstrFile = utf8ToUtf16(fileName);
		file = ::_wfopen(wstrFile.c_str(), L"rb");
#else
		file = ::fopen(fileName, "rb");
#endif
		if (!file)
			return false;
		::fclose(file);
		return true;
	}

	char getOsSeparator() const override
	{
#ifdef _WIN32
		return '\\';
#else
		return '/';
#endif
	}

	Assimp::IOStream* Open(const char* fileName, const char* mode = "rb") override
	{
		FILE* file = nullptr;
#ifdef _WIN32
		std::wstring wstrFile = utf8ToUtf16(fileName);
		std::wstring wstrMode = utf8ToUtf16(mode);
		file = ::_wfopen(wstrFile.c_str(), wstrMode.c_str());
#else
		file = ::fopen(fileName, mode);
#endif
		if (!file)
			return nullptr;
		return new UnicodeIOStream(file, fileName);
	}

	void Close(Assimp::IOStream* file) override
	{
		delete file;
	}
};

void readColor(QJsonObject& params, const char* target, const aiMaterial* material, const char* colorKey, int type, int idx)
{
	aiColor3D clr(0.f, 0.f, 0.f);
	if (AI_SUCCESS == material->Get(colorKey, type, idx, clr)) {
		params[target] = QColor(F2B(clr.r), F2B(clr.g), F2B(clr.b), 255).name();
	}
}

QString getTexture(const aiMaterial* material, aiTextureType type, const AssetResolver& resolver)
{
	if (material->GetTextureCount(type) == 0)
		return QString();

	aiString path;
	if (material->GetTexture(type, 0, &path) != aiReturn_SUCCESS)
		return QString();

	QString rawPath = QDir::toNativeSeparators(QString::fromUtf8(path.C_Str()));
	return resolver.resolveAsset(rawPath);
}

std::unique_ptr<ModelImport::NodeData> convertNodeRecursive(const aiNode* node)
{
	if (!node)
		return nullptr;

	auto nodeData = std::make_unique<ModelImport::NodeData>();
	nodeData->name = QString::fromUtf8(node->mName.C_Str());

	auto mat = glm::make_mat4(node->mTransformation[0]);
	nodeData->localTransform = glm::transpose(mat);

	nodeData->meshIndices.reserve(node->mNumMeshes);
	for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
		nodeData->meshIndices.push_back(node->mMeshes[i]);
	}

	nodeData->children.reserve(node->mNumChildren);
	for (unsigned int i = 0; i < node->mNumChildren; ++i) {
		auto child = convertNodeRecursive(node->mChildren[i]);
		if (child)
			nodeData->children.push_back(std::move(child));
	}

	return nodeData;
}

} // namespace

GenericModelLoader::GenericModelLoader(const FormatDescriptor& desc)
	: m_desc(desc)
{
}

bool GenericModelLoader::canLoad(const ModelLoadingContext& ctx) const
{
	QString suffix = QFileInfo(ctx.sourceFileName).suffix().toLower();
	for (const auto& ext : m_desc.extensions) {
		if (suffix.compare(ext, Qt::CaseInsensitive) == 0)
			return true;
	}
	return false;
}

bool GenericModelLoader::loadScene(const QString& filePath, const AssetResolver& resolver, ModelImport::ImportSceneIR& outScene, IProgressCallback* callback, float unitScale) const
{
	if (callback && !callback->onProgress(0.0))
		return false;

	Assimp::Importer importer;
	importer.SetIOHandler(new UnicodeIOSystem());
	importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);
	importer.SetPropertyBool(AI_CONFIG_IMPORT_COLLADA_USE_COLLADA_NAMES, true);

	int flags = aiProcess_PopulateArmatureData | aiProcess_Triangulate;
	if (!(m_desc.flags & FormatDescriptor::FlipFacing)) {
		flags |= aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_FlipWindingOrder;
	} else {
		flags |= aiProcess_JoinIdenticalVertices;
	}

	if (unitScale > 0.0f && std::abs(unitScale - 1.0f) > 1e-5f) {
		importer.SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, unitScale);
		flags |= aiProcess_GlobalScale;
	}

	const aiScene* aiScene = nullptr;
	try {
		aiScene = importer.ReadFile(filePath.toUtf8().constData(), flags);
	} catch (...) {
		LogError() << "Assimp exception loading file:" << filePath;
		return false;
	}

	if (!aiScene || !aiScene->mRootNode) {
		LogError() << "Assimp failed to load file:" << filePath;
		LogError() << importer.GetErrorString();
		return false;
	}

	if (callback && !callback->onProgress(0.1))
		return false;

	// convert meshes
	outScene.meshes.resize(aiScene->mNumMeshes);
	for (unsigned int m = 0; m < aiScene->mNumMeshes; ++m) {
		const aiMesh* mesh = aiScene->mMeshes[m];
		auto& meshData = outScene.meshes[m];
		meshData.name = mesh->mName.C_Str();
		meshData.materialIndex = mesh->mMaterialIndex;

		if ((mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) != aiPrimitiveType_TRIANGLE)
			continue;

		meshData.vertices.reserve(mesh->mNumVertices);
		meshData.uv0.reserve(mesh->mNumVertices);
		if (mesh->mTextureCoords[1])
			meshData.uv1.reserve(mesh->mNumVertices);

		for (unsigned int vi = 0; vi < mesh->mNumVertices; ++vi) {
			Vertex v;
			v.pos = glm::vec3(mesh->mVertices[vi].x, mesh->mVertices[vi].y, mesh->mVertices[vi].z);

			if (mesh->mNormals) {
				v.normal = glm::vec3(mesh->mNormals[vi].x, mesh->mNormals[vi].y, mesh->mNormals[vi].z);
			} else {
				v.normal = glm::vec3(0.f);
			}

			glm::vec2 uv0 = mesh->mTextureCoords[0]
				? glm::vec2(mesh->mTextureCoords[0][vi].x, mesh->mTextureCoords[0][vi].y)
				: glm::vec2(0.f);

			meshData.vertices.push_back(v);
			meshData.uv0.push_back(uv0);

			if (mesh->mTextureCoords[1]) {
				meshData.uv1.push_back(glm::vec2(mesh->mTextureCoords[1][vi].x, mesh->mTextureCoords[1][vi].y));
			}
		}

		meshData.indices.reserve(mesh->mNumFaces * 3);
		for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
			const auto& face = mesh->mFaces[f];
			for (unsigned int i = 0; i < face.mNumIndices; ++i) {
				meshData.indices.push_back(face.mIndices[i]);
			}
		}

		GeometryProcessor::processMesh(meshData, m_desc);
	}

	// convert materials
	for (unsigned int m = 0; m < aiScene->mNumMaterials; ++m) {
		const aiMaterial* material = aiScene->mMaterials[m];
		ModelImport::MaterialData matData;
		matData.index = m;

		aiString name;
		material->Get(AI_MATKEY_NAME, name);
		matData.name = QString::fromUtf8(name.C_Str());

		QJsonObject params;
		readColor(params, MATERIAL_PARAMETER_DIFFUSE_COLOR, material, AI_MATKEY_COLOR_DIFFUSE);
		readColor(params, MATERIAL_PARAMETER_SPECULAR_COLOR, material, AI_MATKEY_COLOR_SPECULAR);
		readColor(params, MATERIAL_PARAMETER_AMBIENT_COLOR, material, AI_MATKEY_COLOR_AMBIENT);
		readColor(params, MATERIAL_PARAMETER_EMISSIVE_COLOR, material, AI_MATKEY_COLOR_EMISSIVE);

		QString diffuseTex = getTexture(material, aiTextureType_DIFFUSE, resolver);
		if (!diffuseTex.isEmpty()) {
			params[MATERIAL_PARAMETER_DIFFUSE_TEXTURE] = diffuseTex;
			if (m_desc.flags & FormatDescriptor::TextureOverColor) {
				params[MATERIAL_PARAMETER_DIFFUSE_COLOR] = QColor(255, 255, 255).name();
				params[MATERIAL_PARAMETER_AMBIENT_COLOR] = QColor(255, 255, 255).name();
			}
		}

		QString specTex = getTexture(material, aiTextureType_SPECULAR, resolver);
		if (!specTex.isEmpty()) {
			params[MATERIAL_PARAMETER_SPECULAR_TEXTURE] = specTex;
			if (m_desc.flags & FormatDescriptor::TextureOverColor)
				params[MATERIAL_PARAMETER_SPECULAR_COLOR] = QColor(255, 255, 255).name();
		}

		QString glossTex = getTexture(material, aiTextureType_SHININESS, resolver);
		if (!glossTex.isEmpty())
			params[MATERIAL_PARAMETER_GLOSSINESS_TEXTURE] = glossTex;

		QString normalsTex = getTexture(material, aiTextureType_NORMALS, resolver);
		if (!normalsTex.isEmpty())
			params[MATERIAL_PARAMETER_BUMP_NORMAL_MAP] = normalsTex;

		QString bumpTex = getTexture(material, aiTextureType_HEIGHT, resolver);
		if (!bumpTex.isEmpty())
			params[MATERIAL_PARAMETER_BUMP_TEXTURE] = bumpTex;

		float bumpScaling = 0.f;
		if (AI_SUCCESS == material->Get(AI_MATKEY_BUMPSCALING, bumpScaling))
			params[MATERIAL_PARAMETER_BUMP_FACTOR] = bumpScaling;

		float ior = 1.f;
		if (AI_SUCCESS == material->Get(AI_MATKEY_REFRACTI, ior))
			params[MATERIAL_PARAMETER_REFRACTION_N] = ior;

		if (!(m_desc.flags & FormatDescriptor::IgnoreOpacity)) {
			float opacity = 1.f;
			if (AI_SUCCESS == material->Get(AI_MATKEY_OPACITY, opacity))
				params[MATERIAL_PARAMETER_OPACITY_FACTOR] = opacity;
		}

		matData.parameters = params;
		outScene.materials[m] = matData;
	}

	// convert node hierarchy
	outScene.root = convertNodeRecursive(aiScene->mRootNode);

	// compute scene bounds
	outScene.aabb = GeometryProcessor::computeSceneBounds(outScene);

	return true;
}

void GenericModelLoader::load(const ModelLoadingContext& ctx, IScene* scene, IMaterialCreator* materialCreator, IProgressCallback* callback)
{
	AssetResolver resolver(ctx.sourceFolder);
	ModelImport::ImportSceneIR ir;

	float effectiveScale = ctx.unitScale > 0.0f ? ctx.unitScale : m_desc.unitScale;

	if (!loadScene(ctx.sourceFileName, resolver, ir, callback, effectiveScale)) {
		if (callback)
			callback->onProgress(1.0);
		return;
	}

	SceneBuilder::build(ir, m_desc, ctx, scene, materialCreator, callback);
}
