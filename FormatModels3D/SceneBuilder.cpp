#include "SceneBuilder.h"

bool SceneBuilder::build(const ModelImport::ImportSceneIR& ir, const FormatDescriptor& desc,
	const ModelLoadingContext& ctx, IScene* scene, IMaterialCreator* materialCreator,
	IProgressCallback* callback)
{
	if (!scene || !materialCreator || ir.isEmpty()) {
		if (callback)
			callback->onProgress(1.0);
		return false;
	}

	if (callback && !callback->onProgress(0.15))
		return false;

	// collect only materials that are actually referenced by meshes in the node hierarchy
	std::set<uint32_t> usedMaterialIndices;
	int totalMeshes = 0;
	auto inspectNodes = [&](auto& self, const ModelImport::NodeData* node) -> void {
		if (!node) return;
		for (uint32_t meshIdx : node->meshIndices) {
			if (meshIdx < ir.meshes.size()) {
				const auto& m = ir.meshes[meshIdx];
				usedMaterialIndices.insert(m.materialIndex);
				if (!m.vertices.empty() && !m.indices.empty()) {
					totalMeshes++;
				}
			}
		}
		for (const auto& child : node->children)
			self(self, child.get());
	};
	inspectNodes(inspectNodes, ir.root.get());

	// create only used materials
	std::map<uint32_t, IMaterial*> createdMaterials;
	int matIdx = 1;
	for (uint32_t mIndex : usedMaterialIndices) {
		auto it = ir.materials.find(mIndex);
		if (it == ir.materials.end())
			continue;

		const auto& matData = it->second;
		QString mtrlName;

		if ((desc.flags & FormatDescriptor::IgnoreMaterialName) || matData.name.trimmed().isEmpty()) {
			mtrlName = QFileInfo(ctx.sourceFileName).fileName() + "_" + QString::number(matIdx);
		} else {
			mtrlName = matData.name;
		}

		auto material = materialCreator->create(mtrlName, matData.parameters, ctx);
		createdMaterials[mIndex] = material;
		matIdx++;
	}

	if (callback && !callback->onProgress(0.25))
		return false;

	// build node hierarchy
	INode* rootCreatedNode = nullptr;
	int processedMeshes = 0;
	if (!buildNodeHierarchy(ir.root.get(), &scene->root(), true, ir, desc, ctx,
			createdMaterials, rootCreatedNode, callback, processedMeshes, totalMeshes)) {
		return false;
	}

	// put on the floor if requested
	if (ctx.putOnTheFloor && !ir.aabb.isNull() && rootCreatedNode) {
		glm::mat4 mat = rootCreatedNode->transformation();
		mat[3][2] -= ir.aabb.min.z;
		rootCreatedNode->setTransformation(mat);
	}

	if (callback)
		callback->onProgress(1.0);

	return true;
}

bool SceneBuilder::buildNodeHierarchy(const ModelImport::NodeData* nodeData, INode* parentNode, bool isRoot,
	const ModelImport::ImportSceneIR& ir, const FormatDescriptor& desc, const ModelLoadingContext& ctx,
	const std::map<uint32_t, IMaterial*>& createdMaterials, INode*& outRootNode,
	IProgressCallback* callback, int& processedMeshes, int totalMeshes)
{
	if (!nodeData || !parentNode)
		return true;

	QString nodeName = isRoot ? QFileInfo(ctx.sourceFileName).fileName() : nodeData->name;
	INode* destNode = parentNode->addChildNode(nodeName, SceneElement_MeshNode);
	if (isRoot)
		outRootNode = destNode;

	glm::mat4 transform = nodeData->localTransform;
	if (isRoot && (desc.flags & FormatDescriptor::UpY)) {
		transform = glm::mat4(
			1,  0, 0, 0,
			0,  0, 1, 0,
			0, -1, 0, 0,
			0,  0, 0, 1
		) * transform;
	}

	destNode->setTransformation(transform);

	auto meshNode = qobject_cast<IMeshNode*>(destNode);

	for (uint32_t meshIdx : nodeData->meshIndices) {
		if (meshIdx >= ir.meshes.size())
			continue;

		const auto& meshData = ir.meshes[meshIdx];
		if (meshData.vertices.empty() || meshData.indices.empty())
			continue;

		IMaterial* material = nullptr;
		auto it = createdMaterials.find(meshData.materialIndex);
		if (it != createdMaterials.end())
			material = it->second;

		auto geometry = meshNode->addGeometry(material);

		geometry->setVertices(meshData.vertices);
		geometry->setUVset(0, meshData.uv0);
		if (!meshData.uv1.empty())
			geometry->setUVset(1, meshData.uv1);
		geometry->setIndices(meshData.indices);

		processedMeshes++;
		if (callback && totalMeshes > 0) {
			double progress = 0.25 + 0.70 * (static_cast<double>(processedMeshes) / totalMeshes);
			if (!callback->onProgress(progress))
				return false;
		}
	}

	for (const auto& child : nodeData->children) {
		if (!buildNodeHierarchy(child.get(), destNode, false, ir, desc, ctx,
				createdMaterials, outRootNode, callback, processedMeshes, totalMeshes)) {
			return false;
		}
	}

	return true;
}
