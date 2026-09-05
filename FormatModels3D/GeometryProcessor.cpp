#include "GeometryProcessor.h"

void GeometryProcessor::processMesh(ModelImport::MeshData& mesh, const FormatDescriptor& desc)
{
	if (desc.flags & FormatDescriptor::FlipFacing) {
		// check if mesh has normals already
		bool hasValidNormals = false;
		for (const auto& v : mesh.vertices) {
			if (glm::length2(v.normal) > 1e-6f) {
				hasValidNormals = true;
				break;
			}
		}

		if (!hasValidNormals) {
			generateSmoothNormals(mesh);
		}

		flipWindingOrder(mesh);
	}

	sanitizeFinite(mesh);
}

void GeometryProcessor::generateSmoothNormals(ModelImport::MeshData& mesh)
{
	if (mesh.vertices.empty() || mesh.indices.empty())
		return;

	for (auto& v : mesh.vertices) {
		v.normal = glm::vec3(0.f);
	}

	const size_t numTriangles = mesh.indices.size() / 3;
	for (size_t t = 0; t < numTriangles; ++t) {
		const auto idx0 = mesh.indices[t * 3 + 0];
		const auto idx1 = mesh.indices[t * 3 + 1];
		const auto idx2 = mesh.indices[t * 3 + 2];

		if (idx0 >= mesh.vertices.size() || idx1 >= mesh.vertices.size() || idx2 >= mesh.vertices.size())
			continue;

		const auto & v0 = mesh.vertices[idx0].pos;
		const auto & v1 = mesh.vertices[idx1].pos;
		const auto & v2 = mesh.vertices[idx2].pos;

		const auto edge1 = v1 - v0;
		const auto edge2 = v2 - v0;
		const auto normal = glm::cross(edge1, edge2);

		mesh.vertices[idx0].normal += normal;
		mesh.vertices[idx1].normal += normal;
		mesh.vertices[idx2].normal += normal;
	}

	for (auto& v : mesh.vertices) {
		float len2 = glm::length2(v.normal);
		if (len2 > 1e-12f) {
			v.normal = glm::normalize(v.normal);
		} else {
			v.normal = glm::vec3(0.f, 0.f, 1.f);
		}
	}
}

void GeometryProcessor::flipWindingOrder(ModelImport::MeshData& mesh)
{
	const size_t numTriangles = mesh.indices.size() / 3;
	for (size_t t = 0; t < numTriangles; ++t) {
		std::swap(mesh.indices[t * 3 + 0], mesh.indices[t * 3 + 2]);
	}
}

void GeometryProcessor::sanitizeFinite(ModelImport::MeshData& mesh)
{
	for (auto& v : mesh.vertices) {
		if (!std::isfinite(v.pos.x) || !std::isfinite(v.pos.y) || !std::isfinite(v.pos.z)) {
			v.pos = glm::vec3(0.f);
		}
		if (!std::isfinite(v.normal.x) || !std::isfinite(v.normal.y) || !std::isfinite(v.normal.z)) {
			v.normal = glm::vec3(0.f, 0.f, 1.f);
		}
	}

	for (auto& uv : mesh.uv0) {
		if (!std::isfinite(uv.x) || !std::isfinite(uv.y)) {
			uv = glm::vec2(0.f);
		}
	}

	for (auto& uv : mesh.uv1) {
		if (!std::isfinite(uv.x) || !std::isfinite(uv.y)) {
			uv = glm::vec2(0.f);
		}
	}
}

void GeometryProcessor::accumulateNodeBounds(const ModelImport::NodeData* node,
	const glm::mat4& parentTransform, const std::vector<ModelImport::MeshData>& meshes, BBox& outBBox)
{
	if (!node)
		return;

	glm::mat4 globalTransform = parentTransform * node->localTransform;

	for (uint32_t meshIdx : node->meshIndices) {
		if (meshIdx < meshes.size()) {
			const auto& mesh = meshes[meshIdx];
			for (const auto& v : mesh.vertices) {
				outBBox.addToBounds(glm::transformCoord(v.pos, globalTransform));
			}
		}
	}

	for (const auto& child : node->children) {
		accumulateNodeBounds(child.get(), globalTransform, meshes, outBBox);
	}
}

BBox GeometryProcessor::computeSceneBounds(const ModelImport::ImportSceneIR& scene)
{
	BBox bbox;
	bbox.clear();

	if (scene.root) {
		accumulateNodeBounds(scene.root.get(), glm::mat4(1.0f), scene.meshes, bbox);
	}

	return bbox;
}
