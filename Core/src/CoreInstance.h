/*
	Copyright (C) 2013-2020 Damir Sagidullin

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
	documentation files (the "Software"), to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
	and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions
	of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
	TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
	CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.

	(The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
*/

#pragma once

#include "../../Shared/Interfaces/ModelLoader.h"

class IApplicationContext;
class ICoreRenderer;
class BaseRenderer;
class SceneImportCommand;
class Scene;

#include <OpenImageDenoise/oidn.hpp>

class CoreInstance
	: public ICoreInstance
	, public IProgressCallback
{
	Q_OBJECT

	IApplicationContext	&m_ctx;
	class CoreRenderer	*m_renderer;
	std::unique_ptr<Scene>			m_scene;
	std::unique_ptr<BaseRenderer>	m_renderers[2];

	QString				m_fileName;
	bool				m_loadedWell;
	bool				m_savedWell;
	QString				m_savingError;

	QFuture<void>		m_loading;
	QFuture<void>		m_saving;
	QScopedPointer<SceneImportCommand> m_importCommand;

	mutable oidn::DeviceRef	m_oidnDevice;
	mutable oidn::FilterRef	m_oidnFilter;

	void backgroundLoad(ModelLoadingContext ctx);
	void backgroundSave(ModelSavingContext ctx);

	bool onProgress(double progress) override;

private slots:
	void updateSaverProgress(float);
	void onBeforeImageUpdate(const QString&);
	void onAfterImageUpdate(const QString&);

public:
	CoreInstance(IApplicationContext &ctx);
	~CoreInstance();

	ILog &log();

	IApplicationContext &appContext() const { return m_ctx; }
	ICoreRenderer *createRenderer(float devicePixelRatio, bool offscreen) override;
	IScene *getScene() const override;
	ICoreRenderer *renderWidget() const;
	BaseRenderer *renderer() const { return m_renderers[RenderingAlgorithm_VolumePathTracing].get(); }
	Scene & scene() const { return *m_scene; }
	bool isLoading() const override { return m_loading.isRunning(); }

	void initOIDN();
	oidn::DeviceRef& oidnDevice() { return m_oidnDevice; }

	bool loadFile(const QString &fileName, const LoadingParameters &params) override;
	void saveFile(const QString &fileName, const SavingParameters &params) override;
	void cancelProgress() override;
	QString fileName() const override;

	void createRenderingBuffers(RenderingBuffers & buffers, int width, int height, uint32_t bufferTypes) const override;

	int render(const RenderingBuffers &buffers, const RenderingParameters &params, int iterationCount, int startIteration, volatile bool &stop) override;
	bool denoise(const RenderingBuffers & buffers) override;
};
