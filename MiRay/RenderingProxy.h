#pragma once

#include "SimpleParams.h"
#include <QObject>
#include <QFuture>
#include <QQuickImageProvider>
#include <QSettings>
#include <QQmlContext>
#include <QImage>
#include <QDebug>

class ICoreInstance;
class IApplicationContext;

struct RenderingContext {
	IApplicationContext * appContext;
	ICoreInstance * core;

	int numIterations;
	int maxDuration;

	int width;
	int height;
	eRenderingAlgorithm algorithm;

	float maxIntensity;
	float photonRadius;
	bool denoise;
	bool extraChannels;
};

class ColorImageProvider : public QQuickImageProvider
{
	QImage m_image;

public:
	ColorImageProvider() : QQuickImageProvider(QQuickImageProvider::Image)
	{
	}

	void setImage(const QImage & image)
	{
		m_image = image;
	}

	QImage requestImage(const QString & id, QSize * size, const QSize & requestedSize) override
	{
		if (size)
			*size = m_image.size();

		return m_image;
	}
};

class IImageManager;
void saveRenderingBuffers(const RenderingBuffers & originalBuffers, bool extraChannels, bool denoised, const QString & fileName, const IImageManager * imageManager);

class RenderingProxy : public QObject
{
	Q_OBJECT

	QSettings & m_settings;
	QQmlContext * m_context;
	ColorImageProvider * m_imageProvider;
	RenderingBuffers m_renderingBuffers;
	RenderingContext m_ctx;

	int m_totalIterationCount;
	bool m_denoised;
	volatile bool m_stop;
	QFuture<int> m_future;

public:
	RenderingProxy(QSettings & settings, QQmlContext * context, const RenderingContext & ctx, ColorImageProvider * imageProvider, QObject * parent = nullptr);
	~RenderingProxy() override;

	Q_INVOKABLE void stop();
	Q_INVOKABLE void renderMore();
	Q_INVOKABLE void save();

	int rendering(const RenderingContext & ctx);

signals:
	void progress(float progress);
	void previewChanged(const QString & imageId);
	void renderingFinished();
};
