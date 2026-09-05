#pragma once

namespace ModelImport {

struct MeshData {
	std::string name;
	uint32_t materialIndex = 0;
	std::vector<Vertex> vertices;
	std::vector<glm::vec2> uv0;
	std::vector<glm::vec2> uv1;
	std::vector<uint32_t> indices;
};

struct MaterialData {
	uint32_t index = 0;
	QString name;
	QJsonObject parameters;
};

struct NodeData {
	QString name;
	glm::mat4 localTransform{1.0f};
	std::vector<uint32_t> meshIndices;
	std::vector<std::unique_ptr<NodeData>> children;
};

struct ImportSceneIR { // intermediate scene representation
	std::unique_ptr<NodeData> root;
	std::vector<MeshData> meshes;
	std::map<uint32_t, MaterialData> materials;
	BBox aabb;

	bool isEmpty() const {
		return !root || (meshes.empty() && root->children.empty());
	}
};

} // namespace ModelImport
