#pragma once

#include <QObject>
#include <QFuture>
#include <QPointer>
#include <QStringList>
#include "../Shared/Interfaces/CoreInstance.h"

class IMaterial;

class PreviewRenderer : public QObject
{
	Q_OBJECT

	class IApplicationContext & m_appCtx;

	std::shared_ptr<ICoreInstance> m_core;
	RenderingBuffers m_buffers;
	IScene * m_scene = nullptr;

	QPointer<IMaterial> m_originalMaterial;

	int m_passCount = 0;
	bool m_stop = true;
	QFuture<void> m_future;

	QStringList m_sceneNames;
	QString m_sceneName;

	void updatePreviewMaterial();
	void processMaterial();

public:
	PreviewRenderer(class IApplicationContext & appCtx);
	~PreviewRenderer();

	const QStringList getScenes() const { return m_sceneNames; }
	const QString & scene() const { return m_sceneName; }
	void setScene(const QString & name);

	void setMaterial(const IMaterial* material);
	void stop();

	bool renderStep();

	float progress() const;

signals:
	void rendered(const IMaterial *);
	void progressChanged();
};
