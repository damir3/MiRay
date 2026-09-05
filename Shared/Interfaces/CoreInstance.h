#pragma once

#include "Image.h"

enum eRenderingAlgorithm {
	RenderingAlgorithm_VolumePathTracing = 0,	// Volume Path Tracing
	RenderingAlgorithm_UPBP,					// Unifying points, beams, and paths in volumetric light transport simulation
};

struct RenderArea {
	glm::ivec2 frameSize = glm::ivec2(0);
	glm::ivec2 tilePos = glm::ivec2(0);
	glm::ivec2 tileSize = glm::ivec2(0);
};

struct RenderingBuffers {
	ImagePtr frameBuffer; // ImageFormat_RGBA + ImageDataType_Float
	ImagePtr denoisedBuffer; // ImageFormat_RGBA + ImageDataType_Float
	ImagePtr normalBuffer; // ImageFormat_RGB + ImageDataType_Float
	ImagePtr depthBuffer; // ImageFormat_Grayscale + ImageDataType_Float
	ImagePtr albedoBuffer; // ImageFormat_RGB + ImageDataType_Float
	ImagePtr objectsBuffer; // ImageFormat_RGB + ImageDataType_Float
	ImagePtr materialsBuffer; // ImageFormat_RGB + ImageDataType_Float
	std::vector< std::pair< std::string, ImagePtr > > layerBuffers; // ImageFormat_RGBA + ImageDataType_Float
};

struct RenderingParameters {
	eRenderingAlgorithm algorithm = RenderingAlgorithm_VolumePathTracing;
	float maxIntensity = std::numeric_limits<float>::max();
	float photonScale = 1.f;
	const RenderArea *renderArea = nullptr;
};

struct SavingParameters {
	bool waitUntilDone = true;
	bool storeFileName = true;

	bool collectResources = false;
	QSet<QString> collectionBlacklist;

	static SavingParameters forSceneSaving() { return SavingParameters(); }
	static SavingParameters forSceneExport() { SavingParameters p; p.storeFileName = false; return p; }
	static SavingParameters forResourcesCollection() { SavingParameters p; p.storeFileName = false; p.collectResources = true; return p; }
	static SavingParameters forResourcesCollectionWithBlacklist(const QSet<QString> &blacklist) { SavingParameters p; p.storeFileName = false; p.collectResources = true; p.collectionBlacklist = blacklist; return p; }

	SavingParameters &async(bool b) { waitUntilDone = !b; return *this; }
	SavingParameters &collect(bool b) { collectResources = b; return *this; }
	SavingParameters &saveName(bool b) { storeFileName = b; return *this; }
};

struct LoadingParameters {
	bool newScene = true;
	bool waitUntilDone = true;

	static LoadingParameters forSceneLoading() { return LoadingParameters(); }
	static LoadingParameters forSceneImport() { LoadingParameters p; p.newScene = false;  return p; }

	LoadingParameters &async(bool b) { waitUntilDone = !b; return *this; }
	LoadingParameters &import(bool b) { newScene = !b; return *this; }
};

enum eBufferType {
	BufferType_Normal =    (1 << 0),
	BufferType_Depth =     (1 << 1),
	BufferType_Albedo =    (1 << 2),
	BufferType_Objects =   (1 << 3),
	BufferType_Materials = (1 << 4),
	BufferType_Layers =    (1 << 5),
	BufferType_Denoised =  (1 << 6)
};

class ICoreRenderer;

class SHAREDLIB_EXPORT ICoreInstance : public QObject
{
	Q_OBJECT
public:
	virtual ~ICoreInstance() {}

	virtual ICoreRenderer *createRenderer(float devicePixelRatio, bool offscreen) = 0;
	virtual class IScene *getScene() const = 0;

	virtual bool loadFile(const QString &fileName, const LoadingParameters &params) = 0; // throws std::runtime_error
	virtual void saveFile(const QString &fileName, const SavingParameters &params) = 0; // throws std::runtime_error
	virtual void cancelProgress() = 0;
	virtual QString fileName() const = 0;

	virtual bool isLoading() const = 0;

	virtual void createRenderingBuffers(RenderingBuffers & buffers, int width, int height, uint32_t bufferTypes) const = 0;

	virtual int render(const RenderingBuffers &buffers, const RenderingParameters &params, int iterations, int startFrame, volatile bool &stop) = 0; // returns the number of steps performed
	virtual bool denoise(const RenderingBuffers & buffers) = 0;

signals:
	void progressStarted(const QString &operation);
	void progressUpdated(float progress); // 0..1
	void progressFinished();
	void sceneLoaded();
	void sceneSaved();
	void sceneLoadingFailed(const QString &error);
	void sceneSavingFailed(const QString &error);
};
