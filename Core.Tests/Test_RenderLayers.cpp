#include "../Shared/Interfaces/MaterialManager.h"
#include "../Shared/Interfaces/CoreInstance.h"
#include "../Shared/Interfaces/Scene.h"
#include "../Shared/Interfaces/Material.h"
#include "../Shared/Interfaces/Camera.h"
#include "../Shared/Interfaces/Geometry.h"
#include "../Shared/Interfaces/MeshNode.h"
#include "../Shared/Interfaces/RenderLayerManager.h"
#include "../Shared/Interfaces/RenderLayer.h"
#include "../Shared/Interfaces/ModelSaver.h"
#include "../Shared/Interfaces/SerializationContext.h"
#include "../Core/src/CoreInstance.h"

#include "Helpers.h"

class TestRenderLayers : public ::testing::Test
{
	QSharedPointer<Application>	m_spApp;
	QSharedPointer<ICoreInstance>	m_spCore;

	void SetUp() override
	{
		int argc = 0;
		char **argv = nullptr;
		m_spApp = QSharedPointer<Application>(new Application(argc, argv));
		m_spApp->init(eLoggingMode::None);

		m_spCore = QSharedPointer<ICoreInstance>(m_spApp->createCoreInstance());
	}

	void TearDown() override
	{
		m_spCore.clear();
		m_spApp.clear();
	}

protected:

	Application *app()
	{
		return m_spApp.data();
	}

	CoreInstance *core()
	{
		return static_cast<CoreInstance *>(m_spCore.data());
	}

	IScene *scene()
	{
		return m_spCore->getScene();
	}

	IRenderLayerManager *rlmanager()
	{
		return &scene()->renderLayerManager();
	}
};


QByteArray dumpToString(IRenderLayerManager *m)
{
	auto c = m->count();
	QString str;
	for (int i = 0; i < c; i++)
		str += m->get(i)->name().get();
	return str.toUtf8();
}

QByteArray getValue(IMeshNode * mn)
{
	return mn->renderLayer().getTitle(mn->renderLayer().getIndex()).toUtf8();
}

TEST_F(TestRenderLayers, CreateRemoveMove)
{
	auto l0 = rlmanager()->create(0);
	auto l1 = rlmanager()->create(1);
	auto l2 = rlmanager()->create(2);
	ASSERT_STREQ("Layer 1Layer 2Layer 3", dumpToString(rlmanager()));
	l0->name().set("a");
	l1->name().set("b");
	l2->name().set("c");
	ASSERT_STREQ("abc", dumpToString(rlmanager()));
	ASSERT_EQ(0, rlmanager()->getIndex(l0));
	ASSERT_EQ(1, rlmanager()->getIndex(l1));
	ASSERT_EQ(2, rlmanager()->getIndex(l2));
	auto lx = rlmanager()->create(0);
	ASSERT_STREQ("Layer 1abc", dumpToString(rlmanager()));
	lx->name().set("a");
	ASSERT_STREQ("Layer 1abc", dumpToString(rlmanager()));
	lx->name().set("d");
	ASSERT_STREQ("dabc", dumpToString(rlmanager()));
	rlmanager()->move(0, 3);
	ASSERT_STREQ("abcd", dumpToString(rlmanager()));
	rlmanager()->move(3, 0);
	ASSERT_STREQ("dabc", dumpToString(rlmanager()));
	scene()->undoStack().undo();
	ASSERT_STREQ("abcd", dumpToString(rlmanager()));
	rlmanager()->create();
	ASSERT_STREQ("abcdLayer 5", dumpToString(rlmanager()));
	rlmanager()->remove(2);
	ASSERT_STREQ("abdLayer 5", dumpToString(rlmanager()));
	scene()->undoStack().undo();
	ASSERT_STREQ("abcdLayer 5", dumpToString(rlmanager()));
	scene()->undoStack().undo();
	ASSERT_STREQ("abcd", dumpToString(rlmanager()));
}

TEST_F(TestRenderLayers, MeshNodes)
{
	auto l0 = rlmanager()->create(0);
	auto l3 = rlmanager()->create(1);
	auto l1 = rlmanager()->create(1);
	auto l2 = rlmanager()->create(2);
	ASSERT_STREQ("Layer 1Layer 2 1Layer 3Layer 2", dumpToString(rlmanager()));
	l0->name()._set("a");
	l1->name()._set("b");
	l2->name()._set("c");
	l3->name()._set("d");
	ASSERT_STREQ("abcd", dumpToString(rlmanager()));
	ASSERT_EQ(0, rlmanager()->getIndex(l0));
	ASSERT_EQ(1, rlmanager()->getIndex(l1));
	ASSERT_EQ(2, rlmanager()->getIndex(l2));
	ASSERT_EQ(3, rlmanager()->getIndex(l3));

	INode *root = &scene()->root();
	auto m1 = qobject_cast<IMeshNode *>(root->addChildNode("1", SceneElement_MeshNode));
	auto m2 = qobject_cast<IMeshNode *>(root->addChildNode("2", SceneElement_MeshNode));
	auto m3 = qobject_cast<IMeshNode *>(root->addChildNode("3", SceneElement_MeshNode));
	auto m4 = qobject_cast<IMeshNode *>(root->addChildNode("4", SceneElement_MeshNode));
	m1->renderLayer()._setIndex(1);
	m2->renderLayer()._setIndex(2);
	m3->renderLayer()._setIndex(3);
	m4->renderLayer()._setIndex(4);
	ASSERT_STREQ("a", getValue(m1));
	ASSERT_STREQ("b", getValue(m2));
	ASSERT_STREQ("c", getValue(m3));
	ASSERT_STREQ("d", getValue(m4));

	rlmanager()->remove(1);
	ASSERT_STREQ("acd", dumpToString(rlmanager()));
	ASSERT_EQ(1, m1->renderLayer().getIndex());
	ASSERT_EQ(0, m2->renderLayer().getIndex());
	ASSERT_EQ(2, m3->renderLayer().getIndex());
	ASSERT_EQ(3, m4->renderLayer().getIndex());
	auto lx = rlmanager()->create(1);
	lx->name()._set("x");
	ASSERT_STREQ("axcd", dumpToString(rlmanager()));
	ASSERT_EQ(1, m1->renderLayer().getIndex());
	ASSERT_EQ(0, m2->renderLayer().getIndex());
	ASSERT_EQ(3, m3->renderLayer().getIndex());
	ASSERT_EQ(4, m4->renderLayer().getIndex());
	ASSERT_STREQ("a", getValue(m1));
	ASSERT_STREQ("Default", getValue(m2));
	ASSERT_STREQ("c", getValue(m3));
	ASSERT_STREQ("d", getValue(m4));

	scene()->undoStack().undo(); // undo create
	ASSERT_STREQ("acd", dumpToString(rlmanager()));
	ASSERT_EQ(1, m1->renderLayer().getIndex());
	ASSERT_EQ(0, m2->renderLayer().getIndex());
	ASSERT_EQ(2, m3->renderLayer().getIndex());
	ASSERT_EQ(3, m4->renderLayer().getIndex());
	scene()->undoStack().undo(); // undo remove
	ASSERT_STREQ("abcd", dumpToString(rlmanager()));
	ASSERT_EQ(1, m1->renderLayer().getIndex());
	ASSERT_EQ(2, m2->renderLayer().getIndex());
	ASSERT_EQ(3, m3->renderLayer().getIndex());
	ASSERT_EQ(4, m4->renderLayer().getIndex());
	ASSERT_STREQ("a", getValue(m1));
	ASSERT_STREQ("b", getValue(m2));
	ASSERT_STREQ("c", getValue(m3));
	ASSERT_STREQ("d", getValue(m4));

	rlmanager()->move(0, 3);
	ASSERT_STREQ("bcda", dumpToString(rlmanager()));
	ASSERT_EQ(4, m1->renderLayer().getIndex());
	ASSERT_EQ(1, m2->renderLayer().getIndex());
	ASSERT_EQ(2, m3->renderLayer().getIndex());
	ASSERT_EQ(3, m4->renderLayer().getIndex());
	ASSERT_STREQ("a", getValue(m1));
	ASSERT_STREQ("b", getValue(m2));
	ASSERT_STREQ("c", getValue(m3));
	ASSERT_STREQ("d", getValue(m4));
	scene()->undoStack().undo(); // undo move
	ASSERT_STREQ("abcd", dumpToString(rlmanager()));
	ASSERT_EQ(1, m1->renderLayer().getIndex());
	ASSERT_EQ(2, m2->renderLayer().getIndex());
	ASSERT_EQ(3, m3->renderLayer().getIndex());
	ASSERT_EQ(4, m4->renderLayer().getIndex());
	ASSERT_STREQ("a", getValue(m1));
	ASSERT_STREQ("b", getValue(m2));
	ASSERT_STREQ("c", getValue(m3));
	ASSERT_STREQ("d", getValue(m4));
}
