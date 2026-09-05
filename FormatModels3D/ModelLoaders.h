#pragma once

#include "FormatDescriptor.h"
#include "../Shared/Interfaces/ModelLoader.h"

class AssetResolver;
namespace ModelImport {
struct ImportSceneIR;
}

class GenericModelLoader : public IModelLoader, public ICallbackModelLoader {
	Q_OBJECT
	Q_INTERFACES(ICallbackModelLoader)

protected:
	const FormatDescriptor& m_desc;

	bool loadScene(const QString& filePath, const AssetResolver& resolver, ModelImport::ImportSceneIR& outScene, IProgressCallback* callback, float unitScale) const;

public:
	explicit GenericModelLoader(const FormatDescriptor& desc);

	bool canLoad(const ModelLoadingContext& ctx) const override;
	bool needsAutoSetup() const override { return true; }

	void load(const ModelLoadingContext& ctx, IScene* scene, IMaterialCreator* materialCreator, IProgressCallback* callback) override;
};

class AssimpLoader3DS : public GenericModelLoader {
	Q_OBJECT
	Q_INTERFACES(ICallbackModelLoader)
public:
	static const FormatDescriptor& descriptor() { return FormatDescriptors::get3DS(); }
	AssimpLoader3DS() : GenericModelLoader(descriptor()) {}
	static const char* uid() { return descriptor().id; }
};

class AssimpLoaderBlender : public GenericModelLoader {
	Q_OBJECT
	Q_INTERFACES(ICallbackModelLoader)
public:
	static const FormatDescriptor& descriptor() { return FormatDescriptors::getBlender(); }
	AssimpLoaderBlender() : GenericModelLoader(descriptor()) {}
	static const char* uid() { return descriptor().id; }
};

using AssimpLoaderBlend = AssimpLoaderBlender;

class AssimpLoaderCollada : public GenericModelLoader {
	Q_OBJECT
	Q_INTERFACES(ICallbackModelLoader)
public:
	static const FormatDescriptor& descriptor() { return FormatDescriptors::getCollada(); }
	AssimpLoaderCollada() : GenericModelLoader(descriptor()) {}
	static const char* uid() { return descriptor().id; }
};

class AssimpLoaderDXF : public GenericModelLoader {
	Q_OBJECT
	Q_INTERFACES(ICallbackModelLoader)
public:
	static const FormatDescriptor& descriptor() { return FormatDescriptors::getDXF(); }
	AssimpLoaderDXF() : GenericModelLoader(descriptor()) {}
	static const char* uid() { return descriptor().id; }
};

class AssimpLoaderFbx : public GenericModelLoader {
	Q_OBJECT
	Q_INTERFACES(ICallbackModelLoader)
public:
	static const FormatDescriptor& descriptor() { return FormatDescriptors::getFBX(); }
	AssimpLoaderFbx() : GenericModelLoader(descriptor()) {}
	static const char* uid() { return descriptor().id; }
};

class AssimpLoaderGLTF : public GenericModelLoader {
	Q_OBJECT
	Q_INTERFACES(ICallbackModelLoader)
public:
	static const FormatDescriptor& descriptor() { return FormatDescriptors::getGLTF(); }
	AssimpLoaderGLTF() : GenericModelLoader(descriptor()) {}
	static const char* uid() { return descriptor().id; }
};

class AssimpLoaderOBJ : public GenericModelLoader {
	Q_OBJECT
	Q_INTERFACES(ICallbackModelLoader)
public:
	static const FormatDescriptor& descriptor() { return FormatDescriptors::getOBJ(); }
	AssimpLoaderOBJ() : GenericModelLoader(descriptor()) {}
	static const char* uid() { return descriptor().id; }
};

class AssimpLoaderPLY : public GenericModelLoader {
	Q_OBJECT
	Q_INTERFACES(ICallbackModelLoader)
public:
	static const FormatDescriptor& descriptor() { return FormatDescriptors::getPLY(); }
	AssimpLoaderPLY() : GenericModelLoader(descriptor()) {}
	static const char* uid() { return descriptor().id; }
};

class AssimpLoaderSTL : public GenericModelLoader {
	Q_OBJECT
	Q_INTERFACES(ICallbackModelLoader)
public:
	static const FormatDescriptor& descriptor() { return FormatDescriptors::getSTL(); }
	AssimpLoaderSTL() : GenericModelLoader(descriptor()) {}
	static const char* uid() { return descriptor().id; }
};
