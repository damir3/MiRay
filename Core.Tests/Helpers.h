#pragma once

#include "../Shared/Interfaces/CoreInstance.h"
#include "../Shared/Interfaces/Scene.h"
#include "../Shared/Interfaces/Material.h"
#include "../Shared/Interfaces/Geometry.h"

class SceneEventsHelper : public QObject
{
	Q_OBJECT

public:
	enum SceneEvent {
		SCENE_EVENT_NODE_CHANGED,
		SCENE_EVENT_BEFORE_SCENE_UPDATE,
		SCENE_EVENT_AFTER_SCENE_UPDATE,
	};

	struct Event {
		SceneEvent	type;

		// SCENE_EVENT_NODE_CHANGED
		const INode *node;
		eNodeChanged what;

		// SCENE_EVENT_BEFORE_SCENE_UPDATE
		// SCENE_EVENT_AFTER_SCENE_UPDATE
		SceneModification	mod;
	};

	SceneEventsHelper(IScene *s);

	size_t getEventCount() const;
	const Event &getEvent(size_t i) const;

private:
	std::vector<Event>	m_events;

private slots:
	void onNodeChanged(const INode *node, eNodeChanged what);
	void onBeforeSceneUpdate(const SceneModification &mod);
	void onAfterSceneUpdate(const SceneModification &mod);
};

class MaterialEventsHelper : public QObject
{
	Q_OBJECT

public:
	struct Event {
		bool any;
		const IMaterial *material;
	};

	std::vector<Event>	m_events;

public slots:
	void onAnyMaterialChanged(const IMaterial *material);
	void onSpecificMaterialChanged();
};

class TestScene : public ::testing::Test
{
	QSharedPointer<Application>	  m_app;
	QSharedPointer<ICoreInstance> m_core;
	QSharedPointer<ICoreInstance> m_core2;

protected:

	void SetUp() override
	{
		int argc = 0;
		char **argv = nullptr;
		m_app = QSharedPointer<Application>(new Application(argc, argv));
		m_app->init(eLoggingMode::None);

		m_core = QSharedPointer<ICoreInstance>(m_app->createCoreInstance());
	}

	void TearDown() override
	{
		m_core2.clear();
		m_core.clear();
		m_app.clear();
	}

	void resetCore()
	{
		m_core = QSharedPointer<ICoreInstance>(m_app->createCoreInstance());
	}

	void createCore2()
	{
		m_core2 = QSharedPointer<ICoreInstance>(m_app->createCoreInstance());
	}

	Application *app() { return m_app.data(); }

	ICoreInstance *core() { return m_core.data(); }
	ICoreInstance *core2() { return m_core2.data(); }

	IScene *scene() { return m_core->getScene(); }
	IScene *scene2() { return m_core2->getScene(); }

	IMaterialManager *matmanager() { return &scene()->materialManager(); }
	IMaterialManager *matmanager2() { return &scene2()->materialManager(); }
};

vec3 anglesToVector(float yaw, float pitch);
std::vector<Vertex> createGeometryVertices(int count);
std::vector<vec2> createGeometryUVset(int count, const vec2 & offset);
std::vector<uint32_t> createGeometryIndices(int count);
