#include "Plugin.h"
#include "ModelLoaders.h"
#include "../Shared/Interfaces/ApplicationContext.h"
#include "../Shared/Utils/MetaTypes.h"

namespace {

template <typename T>
void registerModelLoader(IModelLoaderRegistry& registry)
{
	const auto& desc = T::descriptor();
	registry.registerModelLoader({ desc.id, desc.description, desc.extensions });
}

} // namespace

FormatModels3DPlugin::FormatModels3DPlugin()
{
	customRegisterMetaType<AssimpLoader3DS>();
	customRegisterMetaType<AssimpLoaderBlender>();
	customRegisterMetaType<AssimpLoaderCollada>();
	customRegisterMetaType<AssimpLoaderDXF>();
	customRegisterMetaType<AssimpLoaderFbx>();
	customRegisterMetaType<AssimpLoaderGLTF>();
	customRegisterMetaType<AssimpLoaderOBJ>();
	customRegisterMetaType<AssimpLoaderPLY>();
	customRegisterMetaType<AssimpLoaderSTL>();
}

FormatModels3DPlugin::~FormatModels3DPlugin()
{
}

void FormatModels3DPlugin::activateInContext(IApplicationContext *appContext)
{
	IModelLoaderRegistry & mlRegistry = *appContext->modelLoadersRegistry();
	registerModelLoader<AssimpLoader3DS>(mlRegistry);
	registerModelLoader<AssimpLoaderBlender>(mlRegistry);
	registerModelLoader<AssimpLoaderCollada>(mlRegistry);
	registerModelLoader<AssimpLoaderDXF>(mlRegistry);
	registerModelLoader<AssimpLoaderFbx>(mlRegistry);
	registerModelLoader<AssimpLoaderGLTF>(mlRegistry);
	registerModelLoader<AssimpLoaderOBJ>(mlRegistry);
	registerModelLoader<AssimpLoaderPLY>(mlRegistry);
	registerModelLoader<AssimpLoaderSTL>(mlRegistry);
}
