#pragma once

#include "SceneIR.h"
#include "FormatDescriptor.h"
#include "../Shared/Interfaces/ModelLoader.h"
#include "../Shared/Interfaces/Scene.h"
#include "../Shared/Interfaces/Node.h"
#include "../Shared/Interfaces/MeshNode.h"
#include "../Shared/Interfaces/Geometry.h"
#include "../Shared/Interfaces/SerializationContext.h"

class SceneBuilder {
public:
	static bool build(const ModelImport::ImportSceneIR& ir, const FormatDescriptor& desc,
		const ModelLoadingContext& ctx, IScene* scene, IMaterialCreator* materialCreator,
		IProgressCallback* callback);

private:
	static bool buildNodeHierarchy(const ModelImport::NodeData* nodeData, INode* parentNode, bool isRoot,
		const ModelImport::ImportSceneIR& ir, const FormatDescriptor& desc, const ModelLoadingContext& ctx,
		const std::map<uint32_t, IMaterial*>& createdMaterials, INode*& outRootNode,
		IProgressCallback* callback, int& processedMeshes, int totalMeshes);
};
