#pragma once

#include "SceneIR.h"
#include "FormatDescriptor.h"

class GeometryProcessor {
public:
	static void processMesh(ModelImport::MeshData& mesh, const FormatDescriptor& desc);

	static void generateSmoothNormals(ModelImport::MeshData& mesh);
	static void flipWindingOrder(ModelImport::MeshData& mesh);
	static void sanitizeFinite(ModelImport::MeshData& mesh);

	static BBox computeSceneBounds(const ModelImport::ImportSceneIR& scene);

private:
	static void accumulateNodeBounds(const ModelImport::NodeData* node, const glm::mat4& parentTransform,
		const std::vector<ModelImport::MeshData>& meshes, BBox& outBBox);
};
