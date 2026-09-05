#include "Helpers.h"
#include "../Shared/Interfaces/MeshNode.h"

SceneEventsHelper::SceneEventsHelper(IScene *s)
{
	// connect(s, SIGNAL(nodeChanged(const INode *, eNodeChanged)), this, SLOT(onNodeChanged(const INode *, eNodeChanged)));
	connect(s, SIGNAL(beforeSceneUpdate(const SceneModification &)), this, SLOT(onBeforeSceneUpdate(const SceneModification &)));
	connect(s, SIGNAL(afterSceneUpdate(const SceneModification &)), this, SLOT(onAfterSceneUpdate(const SceneModification &)));
}

size_t SceneEventsHelper::getEventCount() const
{
	return m_events.size();
}

const SceneEventsHelper::Event &SceneEventsHelper::getEvent(size_t i) const
{
	return m_events[i];
}

void SceneEventsHelper::onNodeChanged(const INode *node, eNodeChanged what)
{
	Event ev;
	ev.type = SCENE_EVENT_NODE_CHANGED;
	ev.node = node;
	ev.what = what;
	m_events.push_back(ev);
}

void SceneEventsHelper::onBeforeSceneUpdate(const SceneModification &mod)
{
	Event ev;
	ev.type = SCENE_EVENT_BEFORE_SCENE_UPDATE;
	ev.mod = mod;
	m_events.push_back(ev);
}

void SceneEventsHelper::onAfterSceneUpdate(const SceneModification &mod)
{
	Event ev;
	ev.type = SCENE_EVENT_AFTER_SCENE_UPDATE;
	ev.mod = mod;
	m_events.push_back(ev);
}

// ------------------------------------------------------------------------ //

void MaterialEventsHelper::onAnyMaterialChanged(const IMaterial *material)
{
	Event ev;
	ev.any = true;
	ev.material = material;
	m_events.push_back(ev);
}

void MaterialEventsHelper::onSpecificMaterialChanged()
{
	Event ev;
	ev.any = false;
	ev.material = qobject_cast<IMaterial *>(sender());
	m_events.push_back(ev);
}

// ------------------------------------------------------------------------ //


vec3 anglesToVector(float yaw, float pitch)
{
	float cx = cosf(pitch);
	float sx = sinf(pitch);
	float cz = cosf(yaw);
	float sz = sinf(yaw);
	return vec3(cz*cx, sz*cx, sx);
}

std::vector<Vertex> createGeometryVertices(int count)
{
	std::vector<Vertex> verts(count);
	for (int i = 0; i < count; i++)
	{
		auto ni = i % 31;
		auto vi = i % 51;
		auto n = glm::normalize(anglesToVector(ni*0.05f, ni*0.03f));
		verts[i].pos = vec3(vi*1.2f, vi*2.3f, vi*3.4f);
		verts[i].normal = n;
	}
	return verts;
}

std::vector<vec2> createGeometryUVset(int count, const vec2 & offset)
{
	std::vector<vec2> verts(count);
	for (int i = 0; i < count; i++)
	{
		auto ti = i % 41;
		verts[i] = vec2(ti*0.123f, ti*0.234f) + offset;
	}
	return verts;
}

std::vector<uint32_t> createGeometryIndices(int count)
{
	std::vector<uint32_t> indices(count);
	for (int i = 0; i < count; i++)
		indices[i] = i;
	return indices;
}
