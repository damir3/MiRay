#include "../Shared/Interfaces/MaterialManager.h"
#include "../Shared/Interfaces/CoreInstance.h"
#include "../Shared/Interfaces/Scene.h"
#include "../Shared/Interfaces/Node.h"
#include "../Shared/Interfaces/MeshNode.h"
#include "../Shared/Interfaces/Geometry.h"
#include "../Shared/Interfaces/Material.h"
#include "../Shared/Interfaces/SnapshotManager.h"

typedef void* RTCScene;
#include "../Core/src/Geometry.h"

#include "Helpers.h"

class TestSceneTwoNodes : public TestScene
{
	void SetUp() override
	{
		TestScene::SetUp();

		auto mat = matmanager()->create(QString("mat1"));
		ASSERT_NE(nullptr, mat);

		n1 = scene()->root().addChildNode("mn1", SceneElement_MeshNode);
		auto meshNode1 = qobject_cast<IMeshNode *>(n1);
		ASSERT_NE(nullptr, meshNode1);

		n2 = scene()->root().addChildNode("mn2", SceneElement_MeshNode);
		auto meshNode2 = qobject_cast<IMeshNode *>(n2);
		ASSERT_NE(nullptr, meshNode1);

		m1g1 = meshNode1->addGeometry(mat);
		ASSERT_NE(nullptr, m1g1);

		m1g2 = meshNode1->addGeometry(mat);
		ASSERT_NE(nullptr, m1g2);

		m1g3 = meshNode1->addGeometry(mat);
		ASSERT_NE(nullptr, m1g3);

		m2g1 = meshNode2->addGeometry(mat);
		ASSERT_NE(nullptr, m2g1);

		m2g2 = meshNode2->addGeometry(mat);
		ASSERT_NE(nullptr, m2g2);
	}

protected:
	INode	*n1, *n2;
	IGeometry *m1g1, *m1g2, *m1g3, *m2g1, *m2g2;
};

TEST_F(TestScene, CheckSceneSaveAndImport)
{
	app()->startPlugins();

	auto meshNode = qobject_cast<IMeshNode *>(scene()->root().addChildNode("mn123", SceneElement_MeshNode));
	ASSERT_NE(nullptr, meshNode);
	ASSERT_EQ(1, scene()->root().numChildren(SceneElement_Node));

	auto mat1 = matmanager()->create(QString("mat123"));
	ASSERT_NE(nullptr, mat1);

	QJsonObject metadata;
	metadata["key1"] = 123;
	metadata["key2"] = 456.789;
	metadata["key3"] = "abc";
	scene()->setMetadata(metadata);

	enum {COUNT = 99};
	auto verts = createGeometryVertices(COUNT);
	auto uvSet0 = createGeometryUVset(COUNT, vec2(1.234f, 5.678f));
	auto uvSet1 = createGeometryUVset(COUNT, vec2(2.345f, 6.789f));
	auto uvSet2 = createGeometryUVset(COUNT, vec2(3.456f, 7.890f));
	auto uvSet3 = createGeometryUVset(COUNT, vec2(4.567f, 8.901f));
	auto indices = createGeometryIndices(COUNT);
	auto geom1 = meshNode->addGeometry(mat1);
	geom1->setVertices(verts);
	geom1->setIndices(indices);
	geom1->setUVset(0, uvSet0);
	geom1->setUVset(1, uvSet1);
	geom1->setUVset(2, uvSet2);
	geom1->setUVset(3, uvSet3);
	ASSERT_NE(nullptr, geom1);
	ASSERT_EQ(COUNT, geom1->numIndices());
	ASSERT_EQ(COUNT, geom1->numVertices());

	scene()->properties().environment().set(glm::vec3(0, 0, 0));
	scene()->properties().floorRoughness().set(3.f);

	auto tmpfile = QDir::tempPath() + "/test_model.mirayScene";
	core()->saveFile(tmpfile, SavingParameters::forSceneSaving());

	resetCore();

	ASSERT_EQ(0, scene()->root().numChildren(SceneElement_Node));
	core()->loadFile(tmpfile, LoadingParameters::forSceneImport());
	QFile::remove(tmpfile);
	ASSERT_EQ(1, scene()->root().numChildren(SceneElement_Node));

	auto metadata2 = scene()->metadata();
	ASSERT_EQ(0, metadata2.size()) << "metadata are not imported";

	ASSERT_EQ(glm::vec3(1, 1, 1), scene()->properties().environment().get()) << "environment parameters should not be imported";
	ASSERT_EQ(20.f, scene()->properties().floorRoughness().get()) << "roughness parameters should not be imported";

	auto node2 = static_cast<INode *>(scene()->root().child(0, SceneElement_Node));
	ASSERT_NE(nullptr, node2);
	EXPECT_STREQ("mn123", node2->name().get().toLocal8Bit().data());

	ASSERT_EQ(SceneElement_MeshNode, node2->type());

	auto meshNode2 = qobject_cast<IMeshNode *>(node2);
	ASSERT_NE(nullptr, meshNode2);
	ASSERT_EQ(1, meshNode2->numGeometries());

	auto geom2 = static_cast<Geometry *>(meshNode2->getGeometry(0));
	ASSERT_NE(nullptr, geom2);

	EXPECT_EQ("mat123", geom2->material()->name().get().toUtf8());

	auto & geomVerts = geom2->vertices();
	auto & geomIndices = geom2->indices();
	auto & geomUVSet0 = geom2->uvSet(0);
	auto & geomUVSet1 = geom2->uvSet(1);
	auto & geomUVSet2 = geom2->uvSet(2);
	auto & geomUVSet3 = geom2->uvSet(3);
	ASSERT_EQ(COUNT, geomVerts.size());
	ASSERT_EQ(COUNT, geomIndices.size());
	ASSERT_EQ(COUNT, geomUVSet0.size());
	ASSERT_EQ(COUNT, geomUVSet1.size());
	ASSERT_EQ(COUNT, geomUVSet2.size());
	ASSERT_EQ(COUNT, geomUVSet3.size());
	for (size_t i = 0; i < COUNT; i++)
	{
		ASSERT_EQ(verts[i].pos, geomVerts[i].pos);
		ASSERT_EQ(verts[i].normal, geomVerts[i].normal);
		ASSERT_EQ(uvSet0[i], geomUVSet0[i]);
		ASSERT_EQ(uvSet1[i], geomUVSet1[i]);
		ASSERT_EQ(uvSet2[i], geomUVSet2[i]);
		ASSERT_EQ(uvSet3[i], geomUVSet3[i]);
		ASSERT_EQ(indices[i], geomIndices[i]);
	}
}

TEST_F(TestScene, CheckSceneSaveAndLoad_FloorRoughness)
{
	app()->startPlugins();

	scene()->properties().environment().set(glm::vec3(0, 0, 0));
	scene()->properties().floorRoughness().set(3.f);

	auto tmpfile = QDir::tempPath() + "/test_model.mirayScene";
	core()->saveFile(tmpfile, SavingParameters::forSceneSaving());

	resetCore();

	ASSERT_EQ(0, scene()->root().numChildren(SceneElement_Node));
	core()->loadFile(tmpfile, LoadingParameters::forSceneLoading());
	QFile::remove(tmpfile);

	ASSERT_EQ(glm::vec3(0, 0, 0), scene()->properties().environment().get());
	ASSERT_EQ(3.f, scene()->properties().floorRoughness().get());
}

TEST_F(TestScene, CheckSetSceneMaterial)
{
	auto mat1 = matmanager()->create(QString("mat1"));
	ASSERT_NE(nullptr, mat1);

	createCore2();
	ASSERT_NE(nullptr, scene2());

	auto meshNode2 = qobject_cast<IMeshNode *>(scene2()->root().addChildNode("mn2", SceneElement_MeshNode));
	ASSERT_NE(nullptr, meshNode2);

	auto meshNode3 = qobject_cast<IMeshNode *>(scene2()->root().addChildNode("mn3", SceneElement_MeshNode));
	ASSERT_NE(nullptr, meshNode3);

	auto mat2 = matmanager2()->create(QString("mat1"));
	ASSERT_NE(nullptr, mat2);
	ASSERT_EQ(1, matmanager2()->count());

	auto geom2 = meshNode2->addGeometry(mat2);
	ASSERT_NE(nullptr, geom2);

	auto geom3 = meshNode3->addGeometry(mat2);
	ASSERT_NE(nullptr, geom3);

	scene2()->setMaterial(GeomSelection{geom2}, mat1);
	ASSERT_EQ(2, matmanager2()->count());
	EXPECT_EQ(mat2, matmanager2()->get(0));
	auto mat3 = matmanager2()->get(1);
	EXPECT_NE(mat1, mat3);

	EXPECT_EQ("mat1", mat2->name().get().toUtf8());
	EXPECT_EQ("mat1 1", mat3->name().get().toUtf8());
	EXPECT_EQ(mat3, geom2->material());
	EXPECT_EQ(mat2, geom3->material());

	scene2()->undoStack().undo();

	EXPECT_EQ(1, matmanager2()->count());
	EXPECT_EQ(mat2, matmanager2()->get(0));
	EXPECT_EQ(mat2, geom2->material());
	EXPECT_EQ(mat2, geom3->material());
}

TEST_F(TestScene, CheckReplaceSceneMaterial)
{
	auto mat1 = matmanager()->create(QString("mat1"));
	ASSERT_NE(nullptr, mat1);

	createCore2();
	ASSERT_NE(nullptr, scene2());

	auto meshNode2 = qobject_cast<IMeshNode *>(scene2()->root().addChildNode("mn2", SceneElement_MeshNode));
	ASSERT_NE(nullptr, meshNode2);

	auto meshNode3 = qobject_cast<IMeshNode *>(scene2()->root().addChildNode("mn3", SceneElement_MeshNode));
	ASSERT_NE(nullptr, meshNode3);

	auto mat2 = matmanager2()->create(QString("mat1"));
	ASSERT_NE(nullptr, mat2);
	ASSERT_EQ(1, matmanager2()->count());

	auto geom2 = meshNode2->addGeometry(mat2);
	ASSERT_NE(nullptr, geom2);

	auto geom3 = meshNode3->addGeometry(mat2);
	ASSERT_NE(nullptr, geom3);

	scene2()->replaceMaterial(mat2, mat1);
	ASSERT_EQ(2, matmanager2()->count());
	EXPECT_EQ(mat2, matmanager2()->get(0));
	auto mat3 = matmanager2()->get(1);
	EXPECT_NE(mat1, mat3);

	EXPECT_EQ("mat1", mat2->name().get().toUtf8());
	EXPECT_EQ("mat1 1", mat3->name().get().toUtf8());
	EXPECT_EQ(mat3, geom2->material());
	EXPECT_EQ(mat3, geom3->material());

	scene2()->undoStack().undo();

	EXPECT_EQ(1, matmanager2()->count());
	EXPECT_EQ(mat2, matmanager2()->get(0));
	EXPECT_EQ(mat2, geom2->material());
	EXPECT_EQ(mat2, geom3->material());
}

TEST_F(TestScene, CheckSceneImportUndoRedo)
{
	app()->startPlugins();

	auto meshNode1 = scene()->root().addChildNode("mn1", SceneElement_MeshNode);
	ASSERT_NE(nullptr, meshNode1);
	auto meshNode2 = scene()->root().addChildNode("mn2", SceneElement_MeshNode);
	ASSERT_NE(nullptr, meshNode2);
	ASSERT_EQ(2, scene()->root().numChildren(SceneElement_Node));

	auto lightNode1 = scene()->addLight();
	ASSERT_NE(nullptr, lightNode1);
	ASSERT_EQ(3, scene()->root().numChildren(SceneElement_Node));

	auto mat1 = matmanager()->create(QString("mat1"));
	ASSERT_NE(nullptr, mat1);
	auto mat2 = matmanager()->create(QString("mat2"));
	ASSERT_NE(nullptr, mat2);
	ASSERT_EQ(2, matmanager()->count());

	auto camState1 = scene()->snapshotManager().create();
	ASSERT_NE(nullptr, camState1);
	ASSERT_EQ(1, scene()->snapshotManager().count());

	auto tmpfile1 = QDir::tempPath() + "/test_model1.mirayScene";
	core()->saveFile(tmpfile1, SavingParameters::forSceneSaving());

	auto meshNode3 = scene()->root().addChildNode("mn3", SceneElement_MeshNode);
	ASSERT_NE(nullptr, meshNode3);
	ASSERT_EQ(4, scene()->root().numChildren(SceneElement_Node));

	auto mat3 = matmanager()->create(QString("mat3"));
	ASSERT_NE(nullptr, mat3);
	ASSERT_EQ(3, matmanager()->count());

	auto lightNode2 = scene()->addLight();
	ASSERT_NE(nullptr, lightNode2);
	ASSERT_EQ(5, scene()->root().numChildren(SceneElement_Node));

	auto camState2 = scene()->snapshotManager().create();
	ASSERT_NE(nullptr, camState2);
	ASSERT_EQ(2, scene()->snapshotManager().count());

	auto tmpfile2 = QDir::tempPath() + "/test_model2.mirayScene";
	core()->saveFile(tmpfile2, SavingParameters::forSceneSaving());

	resetCore();
	EXPECT_EQ(scene()->root().numChildren(SceneElement_Node), 0);

	core()->loadFile(tmpfile1, LoadingParameters::forSceneLoading());
	ASSERT_EQ(scene()->root().numChildren(SceneElement_Node), 3);
	ASSERT_EQ(2, matmanager()->count());
	ASSERT_EQ(1, scene()->snapshotManager().count());

	core()->loadFile(tmpfile2, LoadingParameters::forSceneImport());
	ASSERT_EQ(scene()->root().numChildren(SceneElement_Node), 8);
	ASSERT_EQ(3, matmanager()->count());
	ASSERT_EQ(3, scene()->snapshotManager().count());

	scene()->undoStack().undo();
	ASSERT_EQ(scene()->root().numChildren(SceneElement_Node), 3);
	ASSERT_EQ(2, matmanager()->count());
	ASSERT_EQ(1, scene()->snapshotManager().count());

	scene()->undoStack().redo();
	ASSERT_EQ(scene()->root().numChildren(SceneElement_Node), 8);
	ASSERT_EQ(3, matmanager()->count());
	ASSERT_EQ(3, scene()->snapshotManager().count());

	QFile::remove(tmpfile1);
	QFile::remove(tmpfile2);
}

TEST_F(TestScene, LoadUnicodeSceneName)
{
	app()->startPlugins();

	const char *fileName = "\xE7\x9F\xA2.mirayScene";
	auto sceneFile = QDir::toNativeSeparators(QString("%1/../../Core.Tests/res/%2").arg(QCoreApplication::applicationDirPath()).arg(QString::fromUtf8(fileName)));

	bool res = false;
	ASSERT_NO_THROW(res = core()->loadFile(sceneFile, LoadingParameters::forSceneLoading()));
	ASSERT_TRUE(res);
	ASSERT_EQ(1, matmanager()->count());
}

TEST_F(TestScene, SaveUnicodeSceneName)
{
	app()->startPlugins();

	const char *fileName = "\xE7\x9F\xA2.mirayScene";
	auto sceneFile = QDir::toNativeSeparators(QString("%1/../../tmp/%2").arg(QCoreApplication::applicationDirPath()).arg(QString::fromUtf8(fileName)));

	auto pMaterial1 = matmanager()->create("test");

	ASSERT_NO_THROW(core()->saveFile(sceneFile, SavingParameters::forSceneSaving()));

	createCore2();

	ASSERT_TRUE(core2()->loadFile(sceneFile, LoadingParameters::forSceneLoading()));
	ASSERT_EQ(1, matmanager2()->count());
	auto pMaterial2 = matmanager2()->get(0);

	EXPECT_EQ(pMaterial1->guid(), pMaterial2->guid());
}


TEST_F(TestScene, CheckZeroScale)
{
	ASSERT_EQ(0, scene()->root().numChildren(SceneElement_Node));

	scene()->root().addChildNode("test", SceneElement_Node);

	ASSERT_EQ(1, scene()->root().numChildren(SceneElement_Node));

	auto node = static_cast<INode *>(scene()->root().child(0, SceneElement_Node));
	ASSERT_EQ(SceneElement_Node, node->type());

	auto p0 = node->position().get();
	ASSERT_EQ(0.f, p0.x);
	ASSERT_EQ(0.f, p0.y);
	ASSERT_EQ(0.f, p0.z);

	auto r0 = node->rotation().get();
	ASSERT_EQ(0.f, r0.x);
	ASSERT_EQ(0.f, r0.y);
	ASSERT_EQ(0.f, r0.z);

	auto s0 = node->scale().get();
	ASSERT_EQ(1.f, s0.x);
	ASSERT_EQ(1.f, s0.y);
	ASSERT_EQ(1.f, s0.z);

	node->scale().set(vec3(0.f, 0.f, 1.f));

	auto p1 = node->position().get();
	ASSERT_EQ(0.f, p1.x);
	ASSERT_EQ(0.f, p1.y);
	ASSERT_EQ(0.f, p1.z);

	auto r1 = node->rotation().get();
	ASSERT_EQ(0.f, r1.x);
	ASSERT_EQ(0.f, r1.y);
	ASSERT_EQ(0.f, r1.z);

	auto s1 = node->scale().get();
	ASSERT_NEAR(0.f, s1.x, 0.001f);
	ASSERT_NEAR(0.f, s1.y, 0.001f);
	ASSERT_EQ(1.f, s1.z);

	scene()->undoStack().undo();

	auto p2 = node->position().get();
	ASSERT_EQ(0.f, p2.x);
	ASSERT_EQ(0.f, p2.y);
	ASSERT_EQ(0.f, p2.z);

	auto r2 = node->rotation().get();
	ASSERT_EQ(0.f, r2.x);
	ASSERT_EQ(0.f, r2.y);
	ASSERT_EQ(0.f, r2.z);

	auto s2 = node->scale().get();
	ASSERT_EQ(1.f, s2.x);
	ASSERT_EQ(1.f, s2.y);
	ASSERT_EQ(1.f, s2.z);
}

TEST_F(TestScene, MergeWithItselfKeepsMaterials)
{
	app()->startPlugins();

	auto sceneFile = QDir::toNativeSeparators(QString("%1/../../Core.Tests/res/%2").arg(QCoreApplication::applicationDirPath()).arg("one-mtl-scene.mirayScene"));

	// load a scene and get it's only material
	ASSERT_TRUE(core()->loadFile(sceneFile, LoadingParameters::forSceneLoading()));
	ASSERT_EQ(1, matmanager()->count());
	auto pMaterial1 = matmanager()->get(0);
	auto id1 = pMaterial1->guid();

	// load it again and get the same material from the other scene
	createCore2();
	ASSERT_TRUE(core2()->loadFile(sceneFile, LoadingParameters::forSceneLoading()));
	ASSERT_EQ(1, matmanager2()->count());
	auto pMaterial2 = matmanager2()->get(0);
	auto id2 = pMaterial2->guid();

	// make sure that both materials have the same identifiers
	EXPECT_EQ(id1, id2);

	// now merge the scene to itself
	ASSERT_TRUE(core()->loadFile(sceneFile, LoadingParameters::forSceneImport()));

	// make sure it still has just one material
	EXPECT_EQ(1, matmanager()->count());
	auto pMaterial3 = matmanager()->get(0);
	auto id3 = pMaterial3->guid();

	// double check that it is the same material with the same id
	EXPECT_EQ(pMaterial1, pMaterial3);
	EXPECT_EQ(id1, id3);
}

TEST_F(TestScene, TestParamsUndoRedo)
{
	ASSERT_EQ(0, scene()->root().numChildren(SceneElement_Node));

	scene()->root().addChildNode("test", SceneElement_Node);

	ASSERT_EQ(1, scene()->root().numChildren(SceneElement_Node));

	auto node = static_cast<INode *>(scene()->root().child(0, SceneElement_Node));
	ASSERT_EQ(SceneElement_Node, node->type());

	// name param test
	node->name().set("abc123");
	ASSERT_STREQ("abc123", node->name().get().toUtf8().data());

	scene()->undoStack().undo();
	ASSERT_STREQ("test", node->name().get().toUtf8().data());

	scene()->undoStack().redo();
	ASSERT_STREQ("abc123", node->name().get().toUtf8().data());

	// visible param test
	node->visible().set(false);
	ASSERT_FALSE(node->visible().get());

	scene()->undoStack().undo();
	ASSERT_TRUE(node->visible().get());

	scene()->undoStack().redo();
	ASSERT_FALSE(node->visible().get());
}

/*
TEST_F(TestSceneTwoNodes, TestSelectionByNodeOrGeometry)
{
	ASSERT_FALSE(scene()->isSelected(n1));
	ASSERT_FALSE(scene()->isSelected(m1g1));
	ASSERT_FALSE(scene()->isSelected(m1g2));
	ASSERT_FALSE(scene()->isSelected(m1g3));
	ASSERT_FALSE(scene()->isSelected(n2));
	ASSERT_FALSE(scene()->isSelected(m2g1));
	ASSERT_FALSE(scene()->isSelected(m2g2));

	// test 1

	n1->Select(true, true);
	ASSERT_TRUE(scene()->isSelected(n1));
	ASSERT_TRUE(scene()->isSelected(m1g1));
	ASSERT_TRUE(scene()->isSelected(m1g2));
	ASSERT_TRUE(scene()->isSelected(m1g3));

	n1->Select(false, true);
	ASSERT_FALSE(scene()->isSelected(n1));
	ASSERT_FALSE(scene()->isSelected(m1g1));
	ASSERT_FALSE(scene()->isSelected(m1g2));
	ASSERT_FALSE(scene()->isSelected(m1g3));

	// test 2

	m1g2->Select(true, true);
	ASSERT_TRUE(scene()->isSelected(n1));
	ASSERT_FALSE(scene()->isSelected(m1g1));
	ASSERT_TRUE(scene()->isSelected(m1g2));
	ASSERT_FALSE(scene()->isSelected(m1g3));

	m1g1->Select(true, true);
	m1g3->Select(true, true);
	ASSERT_TRUE(scene()->isSelected(n1));
	ASSERT_TRUE(scene()->isSelected(m1g1));
	ASSERT_TRUE(scene()->isSelected(m1g2));
	ASSERT_TRUE(scene()->isSelected(m1g3));

	// test 3

	m1g1->Select(false, true);
	m1g2->Select(false, true);
	ASSERT_TRUE(scene()->isSelected(n1));
	ASSERT_FALSE(scene()->isSelected(m1g1));
	ASSERT_FALSE(scene()->isSelected(m1g2));
	ASSERT_TRUE(scene()->isSelected(m1g3));

	m1g3->Select(false, true);
	ASSERT_FALSE(scene()->isSelected(n1));
	ASSERT_FALSE(scene()->isSelected(m1g1));
	ASSERT_FALSE(scene()->isSelected(m1g2));
	ASSERT_FALSE(scene()->isSelected(m1g3));

	// test 4

	m1g1->Select(true, true);
	m2g2->Select(true, true);
	ASSERT_TRUE(scene()->isSelected(n1));
	ASSERT_TRUE(scene()->isSelected(n2));

	m1g1->Select(false, true);
	m2g2->Select(false, true);
	ASSERT_FALSE(scene()->isSelected(n1));
	ASSERT_FALSE(scene()->isSelected(n2));
}
*/

TEST_F(TestSceneTwoNodes, TestSelectionByNodeList)
{
	ASSERT_FALSE(scene()->isSelected(n1));
	ASSERT_FALSE(scene()->isSelected(m1g1));
	ASSERT_FALSE(scene()->isSelected(m1g2));
	ASSERT_FALSE(scene()->isSelected(m1g3));
	ASSERT_FALSE(scene()->isSelected(n2));
	ASSERT_FALSE(scene()->isSelected(m2g1));
	ASSERT_FALSE(scene()->isSelected(m2g2));

	scene()->setSelection(Selection{ n1, n2 }, SelectionOperation_Set);

	EXPECT_TRUE(scene()->isSelected(n1));
	EXPECT_FALSE(scene()->isSelected(m1g1));
	EXPECT_FALSE(scene()->isSelected(m1g2));
	EXPECT_FALSE(scene()->isSelected(m1g3));
	EXPECT_TRUE(scene()->isSelected(n2));
	EXPECT_FALSE(scene()->isSelected(m2g1));
	EXPECT_FALSE(scene()->isSelected(m2g2));

	scene()->setSelection(Selection{ n2 }, SelectionOperation_Set);

	EXPECT_FALSE(scene()->isSelected(n1));
	EXPECT_FALSE(scene()->isSelected(m1g1));
	EXPECT_FALSE(scene()->isSelected(m1g2));
	EXPECT_FALSE(scene()->isSelected(m1g3));
	EXPECT_TRUE(scene()->isSelected(n2));
	EXPECT_FALSE(scene()->isSelected(m2g1));
	EXPECT_FALSE(scene()->isSelected(m2g2));

	scene()->setSelection(Selection{}, SelectionOperation_Set);

	EXPECT_FALSE(scene()->isSelected(n1));
	EXPECT_FALSE(scene()->isSelected(m1g1));
	EXPECT_FALSE(scene()->isSelected(m1g2));
	EXPECT_FALSE(scene()->isSelected(m1g3));
	EXPECT_FALSE(scene()->isSelected(n2));
	EXPECT_FALSE(scene()->isSelected(m2g1));
	EXPECT_FALSE(scene()->isSelected(m2g2));
}

TEST_F(TestSceneTwoNodes, TestSelectionByGeometryList)
{
	ASSERT_FALSE(scene()->isSelected(n1));
	ASSERT_FALSE(scene()->isSelected(m1g1));
	ASSERT_FALSE(scene()->isSelected(m1g2));
	ASSERT_FALSE(scene()->isSelected(m1g3));
	ASSERT_FALSE(scene()->isSelected(n2));
	ASSERT_FALSE(scene()->isSelected(m2g1));
	ASSERT_FALSE(scene()->isSelected(m2g2));

	Selection sel1 = { m1g1, m2g2 };
	scene()->setSelection(sel1, SelectionOperation_Set);

	EXPECT_FALSE(scene()->isSelected(n1));
	EXPECT_TRUE(scene()->isSelected(m1g1));
	EXPECT_FALSE(scene()->isSelected(m1g2));
	EXPECT_FALSE(scene()->isSelected(m1g3));
	EXPECT_FALSE(scene()->isSelected(n2));
	EXPECT_FALSE(scene()->isSelected(m2g1));
	EXPECT_TRUE(scene()->isSelected(m2g2));

	Selection sel2 = { m2g1, m2g2 };
	scene()->setSelection(sel2, SelectionOperation_Set);

	EXPECT_FALSE(scene()->isSelected(n1));
	EXPECT_FALSE(scene()->isSelected(m1g1));
	EXPECT_FALSE(scene()->isSelected(m1g2));
	EXPECT_FALSE(scene()->isSelected(m1g3));
	EXPECT_FALSE(scene()->isSelected(n2));
	EXPECT_TRUE(scene()->isSelected(m2g1));
	EXPECT_TRUE(scene()->isSelected(m2g2));

	Selection sel3;
	scene()->setSelection(sel3, SelectionOperation_Set);

	EXPECT_FALSE(scene()->isSelected(n1));
	EXPECT_FALSE(scene()->isSelected(m1g1));
	EXPECT_FALSE(scene()->isSelected(m1g2));
	EXPECT_FALSE(scene()->isSelected(m1g3));
	EXPECT_FALSE(scene()->isSelected(n2));
	EXPECT_FALSE(scene()->isSelected(m2g1));
	EXPECT_FALSE(scene()->isSelected(m2g2));
}

TEST_F(TestSceneTwoNodes, CopyPaste1)
{
	ASSERT_EQ(2, scene()->root().numChildren(SceneElement_Node));
	ASSERT_EQ(3, n1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, n2->numChildren(SceneElement_Geometry));
	ASSERT_EQ(1, matmanager()->count());

	auto mat = matmanager()->get(0);

	app()->startPlugins();
	NodeSelection nodes = { n1, n2 };
	Selection elems = { n1, n2 };
	auto data = scene()->copyNodes(nodes);

	scene()->deleteElements(elems);

	EXPECT_EQ(0, scene()->root().numChildren(SceneElement_Node));
	EXPECT_EQ(1, matmanager()->count());

	scene()->root().pasteNodes(data);

	EXPECT_EQ(2, scene()->root().numChildren(SceneElement_Node));
	EXPECT_EQ(1, matmanager()->count());
	EXPECT_EQ(mat, matmanager()->get(0));

	auto pn1 = scene()->root().child(0, SceneElement_Node);
	auto pn2 = scene()->root().child(1, SceneElement_Node);
	EXPECT_EQ(3, pn1->numChildren(SceneElement_Geometry));
	EXPECT_EQ(2, pn2->numChildren(SceneElement_Geometry));
	EXPECT_EQ(mat, static_cast<IGeometry *>(pn1->child(0, SceneElement_Geometry))->material());
	EXPECT_EQ(mat, static_cast<IGeometry *>(pn1->child(1, SceneElement_Geometry))->material());
	EXPECT_EQ(mat, static_cast<IGeometry *>(pn1->child(2, SceneElement_Geometry))->material());
	EXPECT_EQ(mat, static_cast<IGeometry *>(pn2->child(0, SceneElement_Geometry))->material());
	EXPECT_EQ(mat, static_cast<IGeometry *>(pn2->child(1, SceneElement_Geometry))->material());

	scene()->undoStack().undo();

	EXPECT_EQ(0, scene()->root().numChildren(SceneElement_Node));
	EXPECT_EQ(1, matmanager()->count());

	scene()->undoStack().redo();

	EXPECT_EQ(2, scene()->root().numChildren(SceneElement_Node));
	EXPECT_EQ(1, matmanager()->count());
}

TEST_F(TestSceneTwoNodes, CopyPaste2)
{
	ASSERT_EQ(2, scene()->root().numChildren(SceneElement_Node));
	ASSERT_EQ(3, n1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, n2->numChildren(SceneElement_Geometry));
	ASSERT_EQ(1, matmanager()->count());

	auto mat = matmanager()->get(0);

	app()->startPlugins();
	NodeSelection nodes = { n1, n2 };
	Selection elems = { n1, n2 };
	auto data = scene()->copyNodes(nodes);

	scene()->deleteElements(elems);

	EXPECT_EQ(0, scene()->root().numChildren(SceneElement_Node));
	EXPECT_EQ(1, matmanager()->count());
	mat->bump().set(0.5f);

	scene()->root().pasteNodes(data);

	EXPECT_EQ(2, scene()->root().numChildren(SceneElement_Node));
	EXPECT_EQ(1, matmanager()->count());
	EXPECT_EQ(mat, matmanager()->get(0));

	auto pn1 = scene()->root().child(0, SceneElement_Node);
	auto pn2 = scene()->root().child(1, SceneElement_Node);
	EXPECT_EQ(3, pn1->numChildren(SceneElement_Geometry));
	EXPECT_EQ(2, pn2->numChildren(SceneElement_Geometry));
	EXPECT_EQ(mat, static_cast<IGeometry *>(pn1->child(0, SceneElement_Geometry))->material());
	EXPECT_EQ(mat, static_cast<IGeometry *>(pn1->child(1, SceneElement_Geometry))->material());
	EXPECT_EQ(mat, static_cast<IGeometry *>(pn1->child(2, SceneElement_Geometry))->material());
	EXPECT_EQ(mat, static_cast<IGeometry *>(pn2->child(0, SceneElement_Geometry))->material());
	EXPECT_EQ(mat, static_cast<IGeometry *>(pn2->child(1, SceneElement_Geometry))->material());

	scene()->undoStack().undo();

	EXPECT_EQ(0, scene()->root().numChildren(SceneElement_Node));
	EXPECT_EQ(1, matmanager()->count());

	scene()->undoStack().redo();

	EXPECT_EQ(2, scene()->root().numChildren(SceneElement_Node));
	EXPECT_EQ(1, matmanager()->count());
}

TEST_F(TestSceneTwoNodes, CopyPaste3)
{
	ASSERT_EQ(2, scene()->root().numChildren(SceneElement_Node));
	ASSERT_EQ(2, n2->numChildren(SceneElement_Geometry));
	ASSERT_EQ(1, matmanager()->count());
	auto mat = matmanager()->get(0);

	app()->startPlugins();
	NodeSelection nodes = {n2};
	auto data = scene()->copyNodes(nodes);

	createCore2();
	auto n3 = scene2()->root().addChildNode("n3", SceneElement_Node);
	EXPECT_EQ(0, n3->numChildren(SceneElement_Node));
	EXPECT_EQ(0, matmanager2()->count());
	n3->pasteNodes(data);

	EXPECT_EQ(1, n3->numChildren(SceneElement_Node));
	EXPECT_EQ(1, matmanager2()->count());
	auto mat2 = matmanager2()->get(0);
	EXPECT_NE(mat, mat2);
	EXPECT_STREQ(mat->name().get().toUtf8(), mat2->name().get().toUtf8());

	auto pn2 = n3->child(0, SceneElement_Node);
	EXPECT_EQ(2, pn2->numChildren(SceneElement_Geometry));
	EXPECT_EQ(mat2, static_cast<IGeometry *>(pn2->child(0, SceneElement_Geometry))->material());
	EXPECT_EQ(mat2, static_cast<IGeometry *>(pn2->child(1, SceneElement_Geometry))->material());

	scene2()->undoStack().undo();

	EXPECT_EQ(0, n3->numChildren(SceneElement_Node));
	EXPECT_EQ(0, matmanager2()->count());

	scene2()->undoStack().redo();

	EXPECT_EQ(1, n3->numChildren(SceneElement_Node));
	EXPECT_EQ(1, matmanager2()->count());
}

TEST_F(TestSceneTwoNodes, CopyPaste4)
{
	ASSERT_EQ(2, scene()->root().numChildren(SceneElement_Node));
	ASSERT_EQ(2, n2->numChildren(SceneElement_Geometry));
	ASSERT_EQ(1, matmanager()->count());
	auto mat = matmanager()->get(0);

	app()->startPlugins();
	NodeSelection nodes = {n2};
	auto data = scene()->copyNodes(nodes);

	createCore2();
	auto n3 = scene2()->root().addChildNode("n3", SceneElement_Node);
	EXPECT_EQ(0, n3->numChildren(SceneElement_Node));
	EXPECT_EQ(0, matmanager2()->count());
	matmanager2()->create(mat->name().get());
	n3->pasteNodes(data);

	EXPECT_EQ(1, n3->numChildren(SceneElement_Node));
	EXPECT_EQ(2, matmanager2()->count());
	auto mat2 = matmanager2()->get(1);
	EXPECT_NE(mat, mat2);
	EXPECT_NE(mat->name().get().toUtf8(), mat2->name().get().toUtf8());

	auto pn2 = n3->child(0, SceneElement_Node);
	EXPECT_EQ(2, pn2->numChildren(SceneElement_Geometry));
	EXPECT_EQ(mat2, static_cast<IGeometry *>(pn2->child(0, SceneElement_Geometry))->material());
	EXPECT_EQ(mat2, static_cast<IGeometry *>(pn2->child(1, SceneElement_Geometry))->material());

	scene2()->undoStack().undo();

	EXPECT_EQ(0, n3->numChildren(SceneElement_Node));
	EXPECT_EQ(1, matmanager2()->count());

	scene2()->undoStack().redo();

	EXPECT_EQ(1, n3->numChildren(SceneElement_Node));
	EXPECT_EQ(2, matmanager2()->count());
}

TEST_F(TestScene, CopyPasteWithDefaultMaterial)
{
	app()->startPlugins();

	auto n1 = scene()->root().addChildNode("mn1", SceneElement_MeshNode);
	auto m1 = qobject_cast<IMeshNode *>(n1);
	ASSERT_NE(nullptr, m1);

	auto g1 = m1->addGeometry(nullptr);
	ASSERT_NE(nullptr, g1);

	auto mtl1 = g1->material();
	EXPECT_NE(nullptr, mtl1);
	EXPECT_TRUE(mtl1->guid().isNull());

	EXPECT_EQ(0, matmanager()->count());

	auto data = scene()->copyNodes(NodeSelection{n1});

	createCore2();

	EXPECT_EQ(0, scene2()->root().numChildren(SceneElement_All));
	EXPECT_EQ(0, matmanager2()->count());

	scene2()->root().pasteNodes(data);

	EXPECT_EQ(1, scene2()->root().numChildren(SceneElement_All));
	EXPECT_EQ(0, matmanager2()->count());

	auto n2 = scene2()->root().child(0, SceneElement_All);
	EXPECT_EQ(SceneElement_MeshNode, n2->type());
	EXPECT_STREQ("mn1", n2->name().toUtf8().data());

	auto m2 = qobject_cast<IMeshNode *>(n2);
	ASSERT_NE(nullptr, m2);
	EXPECT_EQ(1, m2->numGeometries());
	auto g2 = m2->getGeometry(0);
	ASSERT_NE(nullptr, g2);

	auto mtl2 = g2->material();
	EXPECT_NE(nullptr, mtl2);
	EXPECT_TRUE(mtl2->guid().isNull());
}

TEST_F(TestSceneTwoNodes, DeleteElements)
{
	ASSERT_EQ(2, scene()->root().numChildren(SceneElement_Node));

	auto nn1 = scene()->root().child(0, SceneElement_Node);
	auto nn2 = scene()->root().child(1, SceneElement_Node);

	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));

	auto g11 = static_cast<IGeometry *>(nn1->child(0, SceneElement_Geometry));
	auto g12 = static_cast<IGeometry *>(nn1->child(1, SceneElement_Geometry));
	auto g13 = static_cast<IGeometry *>(nn1->child(2, SceneElement_Geometry));
	auto g21 = static_cast<IGeometry *>(nn2->child(0, SceneElement_Geometry));
	auto g22 = static_cast<IGeometry *>(nn2->child(1, SceneElement_Geometry));

	Selection sel = { g11, g21, g22 };
	scene()->deleteElements(sel);

	ASSERT_EQ(2, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(0, nn2->numChildren(SceneElement_Geometry));

	ASSERT_EQ(g12, nn1->child(0, SceneElement_Geometry));
	ASSERT_EQ(g13, nn1->child(1, SceneElement_Geometry));

	scene()->undoStack().undo();

	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));

	ASSERT_EQ(g11, nn1->child(0, SceneElement_Geometry));
	ASSERT_EQ(g12, nn1->child(1, SceneElement_Geometry));
	ASSERT_EQ(g13, nn1->child(2, SceneElement_Geometry));
	ASSERT_EQ(g21, nn2->child(0, SceneElement_Geometry));
	ASSERT_EQ(g22, nn2->child(1, SceneElement_Geometry));

	scene()->undoStack().redo();

	ASSERT_EQ(2, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(0, nn2->numChildren(SceneElement_Geometry));

	ASSERT_EQ(g12, nn1->child(0, SceneElement_Geometry));
	ASSERT_EQ(g13, nn1->child(1, SceneElement_Geometry));
}

TEST_F(TestSceneTwoNodes, DeleteInTwoStepsAndCheckSelection)
{
	createCore2();

	auto mat = matmanager2()->create(QString("mat2"));

	auto nn1 = scene2()->root().addChildNode("mn1", SceneElement_MeshNode);
	auto meshNode1 = qobject_cast<IMeshNode *>(nn1);
	auto g11 = meshNode1->addGeometry(mat);
	auto g12 = meshNode1->addGeometry(mat);
	auto g13 = meshNode1->addGeometry(mat);
	auto g14 = meshNode1->addGeometry(mat);
	auto g15 = meshNode1->addGeometry(mat);
	auto g16 = meshNode1->addGeometry(mat);

	auto nn2 = static_cast<INode *>(nn1)->addChildNode("mn1", SceneElement_MeshNode);
	auto meshNode2 = qobject_cast<IMeshNode *>(nn2);
	ASSERT_NE(nullptr, meshNode2);

	auto g21 = meshNode2->addGeometry(mat);
	auto g22 = meshNode2->addGeometry(mat);
	auto g23 = meshNode2->addGeometry(mat);
	auto g24 = meshNode2->addGeometry(mat);
	auto g25 = meshNode2->addGeometry(mat);
	auto g26 = meshNode2->addGeometry(mat);


	Selection sel1 = { g12, g14, g16, g21, g23, g25 };
	scene2()->deleteElements(sel1);

	Selection sel2 = { g11, g13, g15, g22, g24, g26 };
	scene2()->deleteElements(sel2);

	scene2()->undoStack().undo();

	Selection selUndo2 = scene2()->selection();

	scene2()->undoStack().undo();

	Selection selUndo1 = scene2()->selection();

	ASSERT_EQ(sel2.size(), selUndo2.size());
	for (auto s : sel2)
		ASSERT_NE(selUndo2.end(), selUndo2.find(s));

	ASSERT_EQ(sel1.size(), selUndo1.size());
	for (auto s : sel1)
		ASSERT_NE(selUndo1.end(), selUndo1.find(s));

	scene2()->undoStack().redo();
	scene2()->undoStack().redo();

	scene2()->undoStack().undo();
	scene2()->undoStack().undo();
}

TEST_F(TestSceneTwoNodes, DeleteElementsBadInput)
{
	ASSERT_EQ(2, scene()->root().numChildren(SceneElement_Node));

	auto nn1 = scene()->root().child(0, SceneElement_Node);
	auto nn2 = scene()->root().child(1, SceneElement_Node);

	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));

	auto g11 = static_cast<IGeometry *>(nn1->child(0, SceneElement_Geometry));
	auto g12 = static_cast<IGeometry *>(nn1->child(1, SceneElement_Geometry));
	auto g13 = static_cast<IGeometry *>(nn1->child(2, SceneElement_Geometry));
	auto g21 = static_cast<IGeometry *>(nn2->child(0, SceneElement_Geometry));
	auto g22 = static_cast<IGeometry *>(nn2->child(1, SceneElement_Geometry));

	Selection sel = { g11, g11, g11, g11, nullptr };
	scene()->deleteElements(sel);

	ASSERT_EQ(2, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));

	ASSERT_EQ(g12, nn1->child(0, SceneElement_Geometry));
	ASSERT_EQ(g13, nn1->child(1, SceneElement_Geometry));
	ASSERT_EQ(g21, nn2->child(0, SceneElement_Geometry));
	ASSERT_EQ(g22, nn2->child(1, SceneElement_Geometry));

	scene()->undoStack().undo();

	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));

	ASSERT_EQ(g11, nn1->child(0, SceneElement_Geometry));
	ASSERT_EQ(g12, nn1->child(1, SceneElement_Geometry));
	ASSERT_EQ(g13, nn1->child(2, SceneElement_Geometry));
	ASSERT_EQ(g21, nn2->child(0, SceneElement_Geometry));
	ASSERT_EQ(g22, nn2->child(1, SceneElement_Geometry));

	scene()->undoStack().redo();

	ASSERT_EQ(2, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));

	ASSERT_EQ(g12, nn1->child(0, SceneElement_Geometry));
	ASSERT_EQ(g13, nn1->child(1, SceneElement_Geometry));
	ASSERT_EQ(g21, nn2->child(0, SceneElement_Geometry));
	ASSERT_EQ(g22, nn2->child(1, SceneElement_Geometry));
}

TEST_F(TestSceneTwoNodes, DeleteElementsFromOtherScene)
{
	ASSERT_EQ(2, scene()->root().numChildren(SceneElement_Node));

	auto nn1 = scene()->root().child(0, SceneElement_Node);
	auto nn2 = scene()->root().child(1, SceneElement_Node);

	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));

	auto g11 = static_cast<IGeometry *>(nn1->child(0, SceneElement_Geometry));
	auto g12 = static_cast<IGeometry *>(nn1->child(1, SceneElement_Geometry));
	auto g13 = static_cast<IGeometry *>(nn1->child(2, SceneElement_Geometry));
	auto g21 = static_cast<IGeometry *>(nn2->child(0, SceneElement_Geometry));
	auto g22 = static_cast<IGeometry *>(nn2->child(1, SceneElement_Geometry));

	createCore2();

	auto nn3 = scene2()->root().addChildNode("mn1", SceneElement_MeshNode);
	auto meshNode3 = qobject_cast<IMeshNode *>(nn3);
	ASSERT_NE(nullptr, meshNode3);

	auto mat = matmanager2()->create(QString("mat1"));
	ASSERT_NE(nullptr, mat);

	auto m3g1 = meshNode3->addGeometry(mat);
	ASSERT_NE(nullptr, m3g1);

	Selection sel = { m3g1 };
	scene()->deleteElements(sel);

	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));
	ASSERT_EQ(1, nn3->numChildren(SceneElement_Geometry));

	ASSERT_EQ(g11, nn1->child(0, SceneElement_Geometry));
	ASSERT_EQ(g12, nn1->child(1, SceneElement_Geometry));
	ASSERT_EQ(g13, nn1->child(2, SceneElement_Geometry));
	ASSERT_EQ(g21, nn2->child(0, SceneElement_Geometry));
	ASSERT_EQ(g22, nn2->child(1, SceneElement_Geometry));
	ASSERT_EQ(m3g1, nn3->child(0, SceneElement_Geometry));

	scene()->undoStack().undo();

	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));
	ASSERT_EQ(1, nn3->numChildren(SceneElement_Geometry));

	ASSERT_EQ(g11, nn1->child(0, SceneElement_Geometry));
	ASSERT_EQ(g12, nn1->child(1, SceneElement_Geometry));
	ASSERT_EQ(g13, nn1->child(2, SceneElement_Geometry));
	ASSERT_EQ(g21, nn2->child(0, SceneElement_Geometry));
	ASSERT_EQ(g22, nn2->child(1, SceneElement_Geometry));
	ASSERT_EQ(m3g1, nn3->child(0, SceneElement_Geometry));

	scene()->undoStack().redo();

	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));
	ASSERT_EQ(1, nn3->numChildren(SceneElement_Geometry));

	ASSERT_EQ(g11, nn1->child(0, SceneElement_Geometry));
	ASSERT_EQ(g12, nn1->child(1, SceneElement_Geometry));
	ASSERT_EQ(g13, nn1->child(2, SceneElement_Geometry));
	ASSERT_EQ(g21, nn2->child(0, SceneElement_Geometry));
	ASSERT_EQ(g22, nn2->child(1, SceneElement_Geometry));
	ASSERT_EQ(m3g1, nn3->child(0, SceneElement_Geometry));
}

TEST_F(TestSceneTwoNodes, MoveMeshes)
{
	auto * root = &scene()->root();
	ASSERT_EQ(2, root->numChildren(SceneElement_Node));

	auto nn1 = root->child(0, SceneElement_Node);
	auto nn2 = root->child(1, SceneElement_Node);

	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));

	auto g11 = static_cast<IGeometry *>(nn1->child(0, SceneElement_Geometry));
	auto g12 = static_cast<IGeometry *>(nn1->child(1, SceneElement_Geometry));
	auto g13 = static_cast<IGeometry *>(nn1->child(2, SceneElement_Geometry));
	auto g21 = static_cast<IGeometry *>(nn2->child(0, SceneElement_Geometry));
	auto g22 = static_cast<IGeometry *>(nn2->child(1, SceneElement_Geometry));

	auto target = scene()->moveMeshes(GeomSelection{ g12, g22 }, false, nullptr);
	EXPECT_STREQ("Extracted Meshes", target->name().get().toUtf8().data());

	ASSERT_EQ(3, root->numChildren(SceneElement_Node));
	ASSERT_EQ(nn1, root->child(0, SceneElement_Node));
	ASSERT_EQ(nn2, root->child(1, SceneElement_Node));
	ASSERT_EQ(target, root->child(2, SceneElement_Node));
	ASSERT_EQ(root, target->parent());

	ASSERT_EQ(2, target->numChildren(SceneElement_Geometry));
	EXPECT_EQ(g12, static_cast<IGeometry *>(target->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g22, static_cast<IGeometry *>(target->child(1, SceneElement_Geometry)));

	ASSERT_EQ(2, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(1, nn2->numChildren(SceneElement_Geometry));
	EXPECT_EQ(g11, static_cast<IGeometry *>(nn1->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g13, static_cast<IGeometry *>(nn1->child(1, SceneElement_Geometry)));
	EXPECT_EQ(g21, static_cast<IGeometry *>(nn2->child(0, SceneElement_Geometry)));

	scene()->undoStack().undo();

	ASSERT_EQ(2, root->numChildren(SceneElement_Node));
	ASSERT_EQ(nn1, root->child(0, SceneElement_Node));
	ASSERT_EQ(nn2, root->child(1, SceneElement_Node));

	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));
	ASSERT_EQ(0, target->numChildren(SceneElement_Geometry));
	EXPECT_EQ(g11, static_cast<IGeometry *>(nn1->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g12, static_cast<IGeometry *>(nn1->child(1, SceneElement_Geometry)));
	EXPECT_EQ(g13, static_cast<IGeometry *>(nn1->child(2, SceneElement_Geometry)));
	EXPECT_EQ(g21, static_cast<IGeometry *>(nn2->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g22, static_cast<IGeometry *>(nn2->child(1, SceneElement_Geometry)));

	auto target2 = scene()->moveMeshes(GeomSelection{ g11, g13, g21, g22 }, false, static_cast<INode *>(nn1));
	ASSERT_EQ(target2, nn1);

	ASSERT_EQ(2, root->numChildren(SceneElement_Node));
	ASSERT_EQ(nn1, root->child(0, SceneElement_Node));
	ASSERT_EQ(nn2, root->child(1, SceneElement_Node));

	ASSERT_EQ(5, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(0, nn2->numChildren(SceneElement_Geometry));
	EXPECT_EQ(g11, static_cast<IGeometry *>(nn1->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g12, static_cast<IGeometry *>(nn1->child(1, SceneElement_Geometry)));
	EXPECT_EQ(g13, static_cast<IGeometry *>(nn1->child(2, SceneElement_Geometry)));
	EXPECT_EQ(g21, static_cast<IGeometry *>(nn1->child(3, SceneElement_Geometry)));
	EXPECT_EQ(g22, static_cast<IGeometry *>(nn1->child(4, SceneElement_Geometry)));

	scene()->undoStack().undo();

	ASSERT_EQ(2, root->numChildren(SceneElement_Node));
	ASSERT_EQ(nn1, root->child(0, SceneElement_Node));
	ASSERT_EQ(nn2, root->child(1, SceneElement_Node));

	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));
	EXPECT_EQ(g11, static_cast<IGeometry *>(nn1->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g12, static_cast<IGeometry *>(nn1->child(1, SceneElement_Geometry)));
	EXPECT_EQ(g13, static_cast<IGeometry *>(nn1->child(2, SceneElement_Geometry)));
	EXPECT_EQ(g21, static_cast<IGeometry *>(nn2->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g22, static_cast<IGeometry *>(nn2->child(1, SceneElement_Geometry)));
}

TEST_F(TestSceneTwoNodes, MoveMeshes2)
{
	auto * root = &scene()->root();
	ASSERT_EQ(2, root->numChildren(SceneElement_Node));

	auto nn1 = root->child(0, SceneElement_Node);
	auto nn2 = root->child(1, SceneElement_Node);
	auto nn3 = scene()->root().addChildNode("mn3", SceneElement_MeshNode);
	auto meshNode3 = qobject_cast<IMeshNode *>(nn3);
	ASSERT_NE(nullptr, meshNode3);

	ASSERT_EQ(3, root->numChildren(SceneElement_Node));
	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));

	auto g11 = static_cast<IGeometry *>(nn1->child(0, SceneElement_Geometry));
	auto g12 = static_cast<IGeometry *>(nn1->child(1, SceneElement_Geometry));
	auto g13 = static_cast<IGeometry *>(nn1->child(2, SceneElement_Geometry));
	auto g21 = static_cast<IGeometry *>(nn2->child(0, SceneElement_Geometry));
	auto g22 = static_cast<IGeometry *>(nn2->child(1, SceneElement_Geometry));
	auto g31 = meshNode3->addGeometry(g11->material());
	ASSERT_NE(nullptr, g31);

	ASSERT_EQ(1, nn3->numChildren(SceneElement_Geometry));

	auto target = scene()->moveMeshes(GeomSelection{ g12, g31 }, true, static_cast<INode *>(nn2));
	ASSERT_EQ(nn2, target);

	ASSERT_EQ(2, root->numChildren(SceneElement_Node));
	ASSERT_EQ(nn1, root->child(0, SceneElement_Node));
	ASSERT_EQ(nn2, root->child(1, SceneElement_Node));
	ASSERT_EQ(2, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(4, nn2->numChildren(SceneElement_Geometry));
	EXPECT_EQ(g11, static_cast<IGeometry *>(nn1->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g13, static_cast<IGeometry *>(nn1->child(1, SceneElement_Geometry)));
	EXPECT_EQ(g21, static_cast<IGeometry *>(nn2->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g22, static_cast<IGeometry *>(nn2->child(1, SceneElement_Geometry)));
	EXPECT_EQ(g12, static_cast<IGeometry *>(nn2->child(2, SceneElement_Geometry)));
	EXPECT_EQ(g31, static_cast<IGeometry *>(nn2->child(3, SceneElement_Geometry)));

	scene()->undoStack().undo();

	ASSERT_EQ(3, root->numChildren(SceneElement_Node));
	ASSERT_EQ(nn1, root->child(0, SceneElement_Node));
	ASSERT_EQ(nn2, root->child(1, SceneElement_Node));
	ASSERT_EQ(nn3, root->child(2, SceneElement_Node));

	ASSERT_EQ(3, nn1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nn2->numChildren(SceneElement_Geometry));
	ASSERT_EQ(1, nn3->numChildren(SceneElement_Geometry));
	EXPECT_EQ(g11, static_cast<IGeometry *>(nn1->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g12, static_cast<IGeometry *>(nn1->child(1, SceneElement_Geometry)));
	EXPECT_EQ(g13, static_cast<IGeometry *>(nn1->child(2, SceneElement_Geometry)));
	EXPECT_EQ(g21, static_cast<IGeometry *>(nn2->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g22, static_cast<IGeometry *>(nn2->child(1, SceneElement_Geometry)));
	EXPECT_EQ(g31, static_cast<IGeometry *>(nn3->child(0, SceneElement_Geometry)));
}

TEST_F(TestScene, CorrectUndoRedoOrder_Issue962_2nodes)
{
	IScene *s = scene();
	auto &r = s->root();
	ASSERT_EQ(0, r.numChildren(SceneElement_Node));

	auto n1 = r.addChildNode("node 1", SceneElement_Node);
	auto n2 = r.addChildNode("node 2", SceneElement_Node);

	ASSERT_EQ(2, r.numChildren(SceneElement_Node));

	SceneEventsHelper helper(s);

	Selection sel = { n1, n2 };
	s->deleteElements(sel);

	ASSERT_EQ(0, r.numChildren(SceneElement_Node));

	ASSERT_EQ(4, helper.getEventCount());

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(0).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(0).mod.type);
	EXPECT_EQ(n2, helper.getEvent(0).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(0).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(0).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(1).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(1).mod.type);
	EXPECT_EQ(n2, helper.getEvent(1).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(1).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(1).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(2).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(2).mod.type);
	EXPECT_EQ(n1, helper.getEvent(2).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(2).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(2).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(3).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(3).mod.type);
	EXPECT_EQ(n1, helper.getEvent(3).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(3).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(3).mod.targetNode);

	s->undoStack().undo();

	ASSERT_EQ(2, r.numChildren(SceneElement_Node));

	ASSERT_EQ(8, helper.getEventCount());

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(4).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(4).mod.type);
	EXPECT_EQ(n1, helper.getEvent(4).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(4).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(4).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(5).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(5).mod.type);
	EXPECT_EQ(n1, helper.getEvent(5).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(5).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(5).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(6).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(6).mod.type);
	EXPECT_EQ(n2, helper.getEvent(6).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(6).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(6).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(7).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(7).mod.type);
	EXPECT_EQ(n2, helper.getEvent(7).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(7).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(7).mod.targetNode);
}


TEST_F(TestScene, CorrectUndoRedoOrder_Issue962_3nodes)
{
	IScene *s = scene();
	auto &r = s->root();
	ASSERT_EQ(0, r.numChildren(SceneElement_Node));

	auto n1 = r.addChildNode("node 1", SceneElement_Node);
	auto n2 = r.addChildNode("node 2", SceneElement_Node);
	auto n3 = r.addChildNode("node 3", SceneElement_Node);

	ASSERT_EQ(3, r.numChildren(SceneElement_Node));

	SceneEventsHelper helper(s);

	Selection sel = { n1, n2 };
	s->deleteElements(sel);

	ASSERT_EQ(1, r.numChildren(SceneElement_Node));

	ASSERT_EQ(4, helper.getEventCount());

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(0).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(0).mod.type);
	EXPECT_EQ(n2, helper.getEvent(0).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(0).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(0).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(1).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(1).mod.type);
	EXPECT_EQ(n2, helper.getEvent(1).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(1).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(1).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(2).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(2).mod.type);
	EXPECT_EQ(n1, helper.getEvent(2).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(2).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(2).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(3).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(3).mod.type);
	EXPECT_EQ(n1, helper.getEvent(3).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(3).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(3).mod.targetNode);

	s->undoStack().undo();

	ASSERT_EQ(3, r.numChildren(SceneElement_Node));

	ASSERT_EQ(8, helper.getEventCount());

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(4).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(4).mod.type);
	EXPECT_EQ(n1, helper.getEvent(4).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(4).mod.insertPosition) << "n1 " << n1 << ", n2 " << n2 << ", n3 " << n3;
	EXPECT_EQ(&r, helper.getEvent(4).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(5).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(5).mod.type);
	EXPECT_EQ(n1, helper.getEvent(5).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(5).mod.insertPosition) << "n1 " << n1 << ", n2 " << n2 << ", n3 " << n3;;
	EXPECT_EQ(&r, helper.getEvent(5).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(6).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(6).mod.type);
	EXPECT_EQ(n2, helper.getEvent(6).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(6).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(6).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(7).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(7).mod.type);
	EXPECT_EQ(n2, helper.getEvent(7).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(7).mod.insertPosition);
	EXPECT_EQ(&r, helper.getEvent(7).mod.targetNode);
}


TEST_F(TestScene, CorrectUndoRedoOrder_Issue962_2meshes)
{
	IScene *s = scene();
	auto &r = s->root();
	ASSERT_EQ(0, r.numChildren(SceneElement_Node));

	auto mat = matmanager()->create("mat1");

	auto n1 = r.addChildNode("node 1", SceneElement_MeshNode);
	auto mn1 = qobject_cast<IMeshNode *>(n1);
	ASSERT_NE(nullptr, mn1);
	auto m1 = mn1->addGeometry(mat);
	auto m2 = mn1->addGeometry(mat);

	ASSERT_EQ(1, r.numChildren(SceneElement_Node));
	ASSERT_EQ(0, n1->numChildren(SceneElement_AnyNode));
	ASSERT_EQ(2, n1->numChildren(SceneElement_Geometry));

	// as we use set here, the order of deletion depends on the addresses of the meshes
	// we need the second mesh to have smaller address, so we need to swap them in some cases

	if (m2 > m1)
	{
		GeomSelection sel = { m1 };
		// the next two lines should make m1 the second mesh in n1
		s->moveMeshes(sel, false, nullptr);
		s->moveMeshes(sel, true, n1);
		std::swap(m1, m2);
	}

	ASSERT_EQ(m1, mn1->getGeometry(0));
	ASSERT_EQ(m2, mn1->getGeometry(1));

	SceneEventsHelper helper(s);

	ASSERT_LT(m2, m1) << "the order will be incorrect in the set";
	Selection sel = { m2, m1 };
	s->deleteElements(sel);

	ASSERT_EQ(0, n1->numChildren(SceneElement_Geometry));

	ASSERT_EQ(4, helper.getEventCount());

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(0).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(0).mod.type);
	EXPECT_EQ(m2, helper.getEvent(0).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(0).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(0).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(1).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(1).mod.type);
	EXPECT_EQ(m2, helper.getEvent(1).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(1).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(1).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(2).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(2).mod.type);
	EXPECT_EQ(m1, helper.getEvent(2).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(2).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(2).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(3).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(3).mod.type);
	EXPECT_EQ(m1, helper.getEvent(3).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(3).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(3).mod.targetNode);

	s->undoStack().undo();

	ASSERT_EQ(2, n1->numChildren(SceneElement_Geometry));

	ASSERT_EQ(8, helper.getEventCount());

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(4).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(4).mod.type);
	EXPECT_EQ(m1, helper.getEvent(4).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(4).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(4).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(5).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(5).mod.type);
	EXPECT_EQ(m1, helper.getEvent(5).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(5).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(5).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(6).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(6).mod.type);
	EXPECT_EQ(m2, helper.getEvent(6).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(6).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(6).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(7).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(7).mod.type);
	EXPECT_EQ(m2, helper.getEvent(7).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(7).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(7).mod.targetNode);
}


TEST_F(TestScene, CorrectUndoRedoOrder_Issue962_3meshes)
{
	IScene *s = scene();
	auto &r = s->root();
	ASSERT_EQ(0, r.numChildren(SceneElement_Node));

	auto mat = matmanager()->create("mat1");

	auto n1 = r.addChildNode("node 1", SceneElement_MeshNode);
	auto mn1 = qobject_cast<IMeshNode *>(n1);
	ASSERT_NE(nullptr, mn1);
	auto m1 = mn1->addGeometry(mat);
	auto m2 = mn1->addGeometry(mat);
	auto m3 = mn1->addGeometry(mat);

	ASSERT_EQ(1, r.numChildren(SceneElement_Node));
	ASSERT_EQ(0, n1->numChildren(SceneElement_AnyNode));
	ASSERT_EQ(3, n1->numChildren(SceneElement_Geometry));

	// as we use set here, the order of deletion depends on the addresses of the meshes
	// we need the second mesh to have smaller address, so we need to swap them in some cases

	if (m2 > m1)
	{
		GeomSelection sel1 = { m1 };
		GeomSelection sel2 = { m3 };
		GeomSelection sel3 = { m1, m3 };
		// the next three lines should make m1 the second mesh in n1
		s->moveMeshes(sel3, false, nullptr); // [m1,m2,m3]->[m2] + [m1,m3]
		s->moveMeshes(sel1, true, n1); // [m2] + [m1,m3] -> [m2,m1] + [m3]
		s->moveMeshes(sel2, true, n1); // [m2,m1] + [m3] -> [m2,m1,m3]
		std::swap(m1, m2);
	}

	ASSERT_EQ(m1, mn1->getGeometry(0));
	ASSERT_EQ(m2, mn1->getGeometry(1));
	ASSERT_EQ(m3, mn1->getGeometry(2));

	SceneEventsHelper helper(s);

	ASSERT_LT(m2, m1) << "the order will be incorrect in the set";
	Selection sel = { m2, m1 };
	s->deleteElements(sel);

	ASSERT_EQ(1, n1->numChildren(SceneElement_Geometry));

	ASSERT_EQ(4, helper.getEventCount());

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(0).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(0).mod.type);
	EXPECT_EQ(m2, helper.getEvent(0).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(0).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(0).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(1).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(1).mod.type);
	EXPECT_EQ(m2, helper.getEvent(1).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(1).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(1).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(2).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(2).mod.type);
	EXPECT_EQ(m1, helper.getEvent(2).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(2).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(2).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(3).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(3).mod.type);
	EXPECT_EQ(m1, helper.getEvent(3).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(3).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(3).mod.targetNode);

	s->undoStack().undo();

	ASSERT_EQ(3, n1->numChildren(SceneElement_Geometry));

	ASSERT_EQ(8, helper.getEventCount());

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(4).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(4).mod.type);
	EXPECT_EQ(m1, helper.getEvent(4).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(4).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(4).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(5).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(5).mod.type);
	EXPECT_EQ(m1, helper.getEvent(5).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(5).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(5).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(6).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(6).mod.type);
	EXPECT_EQ(m2, helper.getEvent(6).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(6).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(6).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(7).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(7).mod.type);
	EXPECT_EQ(m2, helper.getEvent(7).mod.sourceElement);
	EXPECT_EQ(1, helper.getEvent(7).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(7).mod.targetNode);
}


TEST_F(TestScene, CorrectUndoRedoOrder_Issue962_3meshes_2)
{
	IScene *s = scene();
	auto &r = s->root();
	ASSERT_EQ(0, r.numChildren(SceneElement_Node));

	auto mat = matmanager()->create("mat1");

	auto n1 = r.addChildNode("node 1", SceneElement_MeshNode);
	auto mn1 = qobject_cast<IMeshNode *>(n1);
	ASSERT_NE(nullptr, mn1);
	auto m1 = mn1->addGeometry(mat);
	auto m2 = mn1->addGeometry(mat);
	auto m3 = mn1->addGeometry(mat);

	ASSERT_EQ(1, r.numChildren(SceneElement_Node));
	ASSERT_EQ(0, n1->numChildren(SceneElement_AnyNode));
	ASSERT_EQ(3, n1->numChildren(SceneElement_Geometry));

	// as we use set here, the order of deletion depends on the addresses of the meshes
	// we need the first mesh to have smaller address, so we need to swap them in some cases

	if (m1 > m2)
	{
		GeomSelection sel1 = { m1 };
		GeomSelection sel2 = { m3 };
		GeomSelection sel3 = { m1, m3 };
		// the next two lines should make m2 the first mesh in n1
		s->moveMeshes(sel3, false, nullptr); // [m1,m2,m3]->[m2] + [m1,m3]
		s->moveMeshes(sel1, true, n1); // [m2] + [m1,m3] -> [m2,m1] + [m3]
		s->moveMeshes(sel2, true, n1); // [m2,m1] + [m3] -> [m2,m1,m3]
		std::swap(m1, m2);
	}

	ASSERT_EQ(m1, mn1->getGeometry(0));
	ASSERT_EQ(m2, mn1->getGeometry(1));
	ASSERT_EQ(m3, mn1->getGeometry(2));

	SceneEventsHelper helper(s);

	ASSERT_LT(m1, m2) << "the order will be incorrect in the set";
	Selection sel = { m1, m2 };
	s->deleteElements(sel);

	ASSERT_EQ(1, n1->numChildren(SceneElement_Geometry));

	ASSERT_EQ(4, helper.getEventCount());

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(0).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(0).mod.type);
	EXPECT_EQ(m1, helper.getEvent(0).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(0).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(0).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(1).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(1).mod.type);
	EXPECT_EQ(m1, helper.getEvent(1).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(1).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(1).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(2).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(2).mod.type);
	EXPECT_EQ(m2, helper.getEvent(2).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(2).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(2).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(3).type);
	EXPECT_EQ(SceneModification_Delete, helper.getEvent(3).mod.type);
	EXPECT_EQ(m2, helper.getEvent(3).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(3).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(3).mod.targetNode);

	s->undoStack().undo();

	ASSERT_EQ(3, n1->numChildren(SceneElement_Geometry));

	ASSERT_EQ(8, helper.getEventCount());

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(4).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(4).mod.type);
	EXPECT_EQ(m2, helper.getEvent(4).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(4).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(4).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(5).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(5).mod.type);
	EXPECT_EQ(m2, helper.getEvent(5).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(5).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(5).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_BEFORE_SCENE_UPDATE, helper.getEvent(6).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(6).mod.type);
	EXPECT_EQ(m1, helper.getEvent(6).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(6).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(6).mod.targetNode);

	EXPECT_EQ(SceneEventsHelper::SCENE_EVENT_AFTER_SCENE_UPDATE, helper.getEvent(7).type);
	EXPECT_EQ(SceneModification_Add, helper.getEvent(7).mod.type);
	EXPECT_EQ(m1, helper.getEvent(7).mod.sourceElement);
	EXPECT_EQ(0, helper.getEvent(7).mod.insertPosition);
	EXPECT_EQ(n1, helper.getEvent(7).mod.targetNode);
}

TEST_F(TestScene, CutAndPasteKeepMatrix)
{
	app()->startPlugins();

	ASSERT_EQ(0, scene()->root().numChildren(SceneElement_Node));

	auto node1 = scene()->root().addChildNode("test 1", SceneElement_MeshNode);
	auto node2 = scene()->root().addChildNode("test 2", SceneElement_MeshNode);
	ASSERT_EQ(2, scene()->root().numChildren(SceneElement_Node));

	enum { COUNT = 99 };
	auto geom1 = qobject_cast<IMeshNode *>(node2)->addGeometry(nullptr);
	geom1->setVertices(createGeometryVertices(COUNT));
	geom1->setIndices(createGeometryIndices(COUNT));
	geom1->setUVset(0, createGeometryUVset(COUNT, vec2(1.234f, 5.678f)));

	EXPECT_TRUE(vec3(0, 0, 0) == node1->position().get());
	EXPECT_TRUE(vec3(0, 0, 0) == node1->rotation().get());
	EXPECT_TRUE(vec3(1, 1, 1) == node1->scale().get());

	EXPECT_TRUE(vec3(0, 0, 0) == node2->position().get());
	EXPECT_TRUE(vec3(0, 0, 0) == node2->rotation().get());
	EXPECT_TRUE(vec3(1, 1, 1) == node2->scale().get());

	NodeSelection nsel = { node2 };
	Selection sel = { node2 };

	auto data = scene()->copyNodes(nsel);
	scene()->deleteElements(sel);

	ASSERT_EQ(1, scene()->root().numChildren(SceneElement_Node));

	ASSERT_EQ(0, node1->numChildren(SceneElement_Node));
	node1->pasteNodes(data);
	ASSERT_EQ(1, node1->numChildren(SceneElement_Node));

	auto pasted = static_cast<INode *>(node1->child(0, SceneElement_Node));
	ASSERT_NE(nullptr, pasted);

	EXPECT_GT(100, glm::length(pasted->position().get())); // it is not zero, so let's check that it at least is not too big
	EXPECT_TRUE(vec3(0, 0, 0) == pasted->rotation().get());
	EXPECT_TRUE(vec3(1, 1, 1) == pasted->scale().get());
}

QString dumpToString(INode *n)
{
	auto c = n->numChildren(SceneElement_Node);
	if (c == 0)
		return n->name().get();

	QString str = n->name().get() + "(";
	for (size_t i = 0; i < c; i++)
		str += dumpToString(static_cast<INode *>(n->child(i, SceneElement_Node)));
	return str + ")";
}

TEST_F(TestScene, MoveNodes_CantMoveToAnotherScene)
{
	createCore2();

	INode *root1 = &scene()->root();
	INode *root2 = &scene2()->root();
	auto node1 = root1->addChildNode("a", SceneElement_MeshNode);
	root1->addChildNode("b", SceneElement_MeshNode);

	ASSERT_STREQ("Root(ab)", dumpToString(root1).toUtf8());
	ASSERT_STREQ("Root", dumpToString(root2).toUtf8());

	ASSERT_NO_THROW(scene()->moveNodes(NodeSelection{ node1 }, &scene2()->root(), 0, QString()));

	ASSERT_STREQ("Root(ab)", dumpToString(root1).toUtf8());
	ASSERT_STREQ("Root", dumpToString(root2).toUtf8());
}

TEST_F(TestScene, MoveNodes_MoveToNullTarget)
{
	INode *root = &scene()->root();
	auto node1 = root->addChildNode("a", SceneElement_MeshNode);
	root->addChildNode("b", SceneElement_MeshNode);

	ASSERT_STREQ("Root(ab)", dumpToString(root).toUtf8());

	ASSERT_NO_THROW(scene()->moveNodes(NodeSelection{ node1 }, nullptr, 4, QString()));

	ASSERT_STREQ("Root(ba)", dumpToString(root).toUtf8());
}

TEST_F(TestScene, MoveNodes_CantMoveTwice)
{
	INode *root = &scene()->root();
	auto node1 = root->addChildNode("a", SceneElement_MeshNode);
	root->addChildNode("b", SceneElement_MeshNode);

	ASSERT_STREQ("Root(ab)", dumpToString(root).toUtf8());

	ASSERT_NO_THROW(scene()->moveNodes(NodeSelection{ node1, node1, node1 }, root, (size_t)-1, "g"));

	ASSERT_STREQ("Root(bg(a))", dumpToString(root).toUtf8());
}

TEST_F(TestScene, MoveNodes_ToTheSamePlace)
{
	INode *root = &scene()->root();
	root->addChildNode("a", SceneElement_MeshNode);
	auto node2 = root->addChildNode("b", SceneElement_MeshNode);
	root->addChildNode("c", SceneElement_MeshNode);

	ASSERT_STREQ("Root(abc)", dumpToString(root).toUtf8());

	ASSERT_NO_THROW(scene()->moveNodes(NodeSelection{ node2 }, root, 1, QString()));

	ASSERT_STREQ("Root(abc)", dumpToString(root).toUtf8());
}


TEST_F(TestScene, MoveNodes_NotThatObviousOutOfRange)
{
	INode *root = &scene()->root();
	auto node1 = root->addChildNode("a", SceneElement_MeshNode);
	auto node2 = root->addChildNode("b", SceneElement_MeshNode);
	auto node3 = root->addChildNode("c", SceneElement_MeshNode);
	root->addChildNode("d", SceneElement_MeshNode);
	ASSERT_STREQ("Root(abcd)", dumpToString(root).toUtf8());

	ASSERT_NO_THROW(scene()->moveNodes(NodeSelection{ node1, node2, node3 }, root, 3, QString()));

	ASSERT_STREQ("Root(abcd)", dumpToString(root).toUtf8());
}

TEST_F(TestScene, MoveNodes_SwapUsingOrder)
{
	INode *root = &scene()->root();
	root->addChildNode("a", SceneElement_MeshNode);
	auto node2 = root->addChildNode("b", SceneElement_MeshNode);
	auto node3 = root->addChildNode("c", SceneElement_MeshNode);
	root->addChildNode("d", SceneElement_MeshNode);
	ASSERT_STREQ("Root(abcd)", dumpToString(root).toUtf8());

	ASSERT_NO_THROW(scene()->moveNodes(NodeSelection{ node3, node2 }, root, 1, QString()));

	ASSERT_STREQ("Root(acbd)", dumpToString(root).toUtf8());
}

TEST_F(TestScene, MoveNodes_CantMoveToChild)
{
	INode *root = &scene()->root();
	auto a = root->addChildNode("a", SceneElement_MeshNode);
	auto b = a->addChildNode("b", SceneElement_MeshNode);
	auto c = b->addChildNode("c", SceneElement_MeshNode);
	ASSERT_STREQ("Root(a(b(c)))", dumpToString(root).toUtf8());

	ASSERT_NO_THROW(scene()->moveNodes(NodeSelection{ a }, c, 0, QString()));

	ASSERT_STREQ("Root(a(b(c)))", dumpToString(root).toUtf8());
}

TEST_F(TestScene, MoveNodes_CantMoveToItself)
{
	INode *root = &scene()->root();
	auto a = root->addChildNode("a", SceneElement_MeshNode);
	auto b = a->addChildNode("b", SceneElement_MeshNode);
	b->addChildNode("c", SceneElement_MeshNode);
	ASSERT_STREQ("Root(a(b(c)))", dumpToString(root).toUtf8());

	ASSERT_NO_THROW(scene()->moveNodes(NodeSelection{ b }, b, 0, QString()));

	ASSERT_STREQ("Root(a(b(c)))", dumpToString(root).toUtf8());
}

TEST_F(TestScene, MoveNodes_UndoRedo)
{
	INode *root = &scene()->root();
	auto a = root->addChildNode("a", SceneElement_MeshNode);
	auto b = a->addChildNode("b", SceneElement_MeshNode);
	a->addChildNode("c", SceneElement_MeshNode);
	auto d = b->addChildNode("d", SceneElement_MeshNode);
	auto e = d->addChildNode("e", SceneElement_MeshNode);
	ASSERT_STREQ("Root(a(b(d(e))c))", dumpToString(root).toUtf8());

	ASSERT_NO_THROW(scene()->moveNodes(NodeSelection{ b, e }, root, 0, QString()));

	ASSERT_STREQ("Root(b(d)ea(c))", dumpToString(root).toUtf8());

	scene()->undoStack().undo();

	ASSERT_STREQ("Root(a(b(d(e))c))", dumpToString(root).toUtf8());

	scene()->undoStack().redo();

	ASSERT_STREQ("Root(b(d)ea(c))", dumpToString(root).toUtf8());
}

TEST_F(TestSceneTwoNodes, MegreMeshes)
{
	auto * root = &scene()->root();
	ASSERT_EQ(2, root->numChildren(SceneElement_Node));

	auto nm1 = root->child(0, SceneElement_Node);
	auto nm2 = root->child(1, SceneElement_Node);

	ASSERT_EQ(3, nm1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nm2->numChildren(SceneElement_Geometry));
	auto g11 = static_cast<IGeometry *>(nm1->child(0, SceneElement_Geometry));
	auto g12 = static_cast<IGeometry *>(nm1->child(1, SceneElement_Geometry));
	auto g13 = static_cast<IGeometry *>(nm1->child(2, SceneElement_Geometry));
	auto g21 = static_cast<IGeometry *>(nm2->child(0, SceneElement_Geometry));
	auto g22 = static_cast<IGeometry *>(nm2->child(1, SceneElement_Geometry));

	auto g14 = scene()->mergeMeshes(GeomSelection{ g12, g21 });

	ASSERT_EQ(3, nm1->numChildren(SceneElement_Geometry));
	EXPECT_EQ(g11, static_cast<IGeometry *>(nm1->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g13, static_cast<IGeometry *>(nm1->child(1, SceneElement_Geometry)));
	EXPECT_EQ(g14, static_cast<IGeometry *>(nm1->child(2, SceneElement_Geometry)));

	ASSERT_EQ(1, nm2->numChildren(SceneElement_Geometry));
	EXPECT_EQ(g22, static_cast<IGeometry *>(nm2->child(0, SceneElement_Geometry)));

	scene()->undoStack().undo();

	ASSERT_EQ(3, nm1->numChildren(SceneElement_Geometry));
	ASSERT_EQ(2, nm2->numChildren(SceneElement_Geometry));
	EXPECT_EQ(g11, static_cast<IGeometry *>(nm1->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g12, static_cast<IGeometry *>(nm1->child(1, SceneElement_Geometry)));
	EXPECT_EQ(g13, static_cast<IGeometry *>(nm1->child(2, SceneElement_Geometry)));
	EXPECT_EQ(g21, static_cast<IGeometry *>(nm2->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g22, static_cast<IGeometry *>(nm2->child(1, SceneElement_Geometry)));
}

TEST_F(TestSceneTwoNodes, MegreMeshes2)
{
	auto * root = &scene()->root();
	ASSERT_EQ(2, root->numChildren(SceneElement_Node));

	ASSERT_EQ(1, matmanager()->count());
	auto *pMat = matmanager()->get(0);

	auto nm1 = root->child(0, SceneElement_Node);

	ASSERT_EQ(3, nm1->numChildren(SceneElement_Geometry));
	auto g11 = static_cast<IGeometry *>(nm1->child(0, SceneElement_Geometry));
	auto g12 = static_cast<IGeometry *>(nm1->child(1, SceneElement_Geometry));
	auto g13 = static_cast<IGeometry *>(nm1->child(2, SceneElement_Geometry));

	scene()->setSelection(Selection(), SelectionOperation_Set);

	auto g14 = scene()->mergeMeshes(GeomSelection{ g12, g13 });

	EXPECT_TRUE(scene()->geomSelection().size() == 1);
	ASSERT_EQ(g14, scene()->geomSelection().front());

	ASSERT_EQ(2, nm1->numChildren(SceneElement_Geometry));
	EXPECT_EQ(g11, static_cast<IGeometry *>(nm1->child(0, SceneElement_Geometry)));
	EXPECT_EQ(g14, static_cast<IGeometry *>(nm1->child(1, SceneElement_Geometry)));

	ASSERT_EQ(1, matmanager()->count());
	EXPECT_EQ(pMat, matmanager()->get(0));

	EXPECT_EQ(pMat, g14->material());
}

TEST_F(TestScene, DropToSurfaceSingleNode)
{
	auto mat = matmanager()->create(QString("mat1"));
	auto * node = scene()->root().addChildNode("node1", SceneElement_MeshNode);
	auto * meshNode = qobject_cast<IMeshNode *>(node);

	auto * geom = meshNode->addGeometry(mat);
	std::vector<Vertex> verts = {
		{ vec3(-1, -1, 0), vec3(0, 0, 1) },
		{ vec3( 1, -1, 0), vec3(0, 0, 1) },
		{ vec3( 1,  1, 0), vec3(0, 0, 1) },
		{ vec3(-1,  1, 0), vec3(0, 0, 1) },
		{ vec3(-1, -1, 2), vec3(0, 0, 1) },
		{ vec3( 1, -1, 2), vec3(0, 0, 1) },
		{ vec3( 1,  1, 2), vec3(0, 0, 1) },
		{ vec3(-1,  1, 2), vec3(0, 0, 1) },
	};
	std::vector<uint32_t> indices = {
		0, 2, 1,  0, 3, 2,
		4, 5, 6,  4, 6, 7,
		0, 1, 5,  0, 5, 4,
		2, 3, 7,  2, 7, 6,
		3, 0, 4,  3, 4, 7,
		1, 2, 6,  1, 6, 5
	};
	geom->setVertices(verts);
	geom->setIndices(indices);

	node->setTransformation(glm::translate(glm::mat4(1.f), vec3(0.f, 0.f, 5.f)));

	EXPECT_FLOAT_EQ(5.f, node->transformation()[3][2]);

	scene()->beginDropToSurface({ node });

	EXPECT_NEAR(0.f, node->transformation()[3][2], 1e-4f);

	// Test Undo
	scene()->undoStack().undo();
	EXPECT_NEAR(5.f, node->transformation()[3][2], 1e-4f);

	// Test Redo
	scene()->undoStack().redo();
	EXPECT_NEAR(0.f, node->transformation()[3][2], 1e-4f);
}

TEST_F(TestScene, DropToSurfaceStackingTwoNodes)
{
	auto mat = matmanager()->create(QString("mat1"));

	std::vector<Vertex> verts = {
		{ vec3(-1, -1, 0), vec3(0, 0, 1) },
		{ vec3( 1, -1, 0), vec3(0, 0, 1) },
		{ vec3( 1,  1, 0), vec3(0, 0, 1) },
		{ vec3(-1,  1, 0), vec3(0, 0, 1) },
		{ vec3(-1, -1, 2), vec3(0, 0, 1) },
		{ vec3( 1, -1, 2), vec3(0, 0, 1) },
		{ vec3( 1,  1, 2), vec3(0, 0, 1) },
		{ vec3(-1,  1, 2), vec3(0, 0, 1) },
	};
	std::vector<uint32_t> indices = {
		0, 2, 1,  0, 3, 2,
		4, 5, 6,  4, 6, 7,
		0, 1, 5,  0, 5, 4,
		2, 3, 7,  2, 7, 6,
		3, 0, 4,  3, 4, 7,
		1, 2, 6,  1, 6, 5
	};

	// Lower box: height 2 (local z: [0, 2]), initial world z: 5 -> [5, 7]
	auto * node1 = scene()->root().addChildNode("node1", SceneElement_MeshNode);
	auto * meshNode1 = qobject_cast<IMeshNode *>(node1);
	auto * geom1 = meshNode1->addGeometry(mat);
	geom1->setVertices(verts);
	geom1->setIndices(indices);
	node1->setTransformation(glm::translate(glm::mat4(1.f), vec3(0.f, 0.f, 5.f)));

	// Upper box: height 2 (local z: [0, 2]), initial world z: 12 -> [12, 14]
	auto * node2 = scene()->root().addChildNode("node2", SceneElement_MeshNode);
	auto * meshNode2 = qobject_cast<IMeshNode *>(node2);
	auto * geom2 = meshNode2->addGeometry(mat);
	geom2->setVertices(verts);
	geom2->setIndices(indices);
	node2->setTransformation(glm::translate(glm::mat4(1.f), vec3(0.f, 0.f, 12.f)));

	scene()->buildCollisionScene();

	// Both selected in any order (e.g. node2, node1)
	scene()->beginDropToSurface({ node2, node1 });

	// node1 should be on the floor: z = 0 (top of node1 is at z = 2)
	EXPECT_NEAR(0.f, node1->transformation()[3][2], 1e-3f);

	// node2 should be on top of node1: bottom of node2 at z = 2
	EXPECT_NEAR(2.f, node2->transformation()[3][2], 1e-3f);

	// Test Undo: both should return to original positions
	scene()->undoStack().undo();
	EXPECT_NEAR(5.f, node1->transformation()[3][2], 1e-3f);
	EXPECT_NEAR(12.f, node2->transformation()[3][2], 1e-3f);

	// Test Redo: both should return to stacked positions
	scene()->undoStack().redo();
	EXPECT_NEAR(0.f, node1->transformation()[3][2], 1e-3f);
	EXPECT_NEAR(2.f, node2->transformation()[3][2], 1e-3f);
}

TEST_F(TestScene, DropToSurfaceStackingThreeNodesAndSideNode)
{
	auto mat = matmanager()->create(QString("mat1"));

	std::vector<Vertex> verts = {
		{ vec3(-1, -1, 0), vec3(0, 0, 1) },
		{ vec3( 1, -1, 0), vec3(0, 0, 1) },
		{ vec3( 1,  1, 0), vec3(0, 0, 1) },
		{ vec3(-1,  1, 0), vec3(0, 0, 1) },
		{ vec3(-1, -1, 2), vec3(0, 0, 1) },
		{ vec3( 1, -1, 2), vec3(0, 0, 1) },
		{ vec3( 1,  1, 2), vec3(0, 0, 1) },
		{ vec3(-1,  1, 2), vec3(0, 0, 1) },
	};
	std::vector<uint32_t> indices = {
		0, 2, 1,  0, 3, 2,
		4, 5, 6,  4, 6, 7,
		0, 1, 5,  0, 5, 4,
		2, 3, 7,  2, 7, 6,
		3, 0, 4,  3, 4, 7,
		1, 2, 6,  1, 6, 5
	};

	// Box 1: [0, 2], initial z = 5 -> [5, 7]
	auto * node1 = scene()->root().addChildNode("node1", SceneElement_MeshNode);
	qobject_cast<IMeshNode *>(node1)->addGeometry(mat)->setVertices(verts);
	qobject_cast<IMeshNode *>(node1)->getGeometry(0)->setIndices(indices);
	node1->setTransformation(glm::translate(glm::mat4(1.f), vec3(0.f, 0.f, 5.f)));

	// Box 2: [0, 2], initial z = 12 -> [12, 14]
	auto * node2 = scene()->root().addChildNode("node2", SceneElement_MeshNode);
	qobject_cast<IMeshNode *>(node2)->addGeometry(mat)->setVertices(verts);
	qobject_cast<IMeshNode *>(node2)->getGeometry(0)->setIndices(indices);
	node2->setTransformation(glm::translate(glm::mat4(1.f), vec3(0.f, 0.f, 12.f)));

	// Box 3: [0, 2], initial z = 20 -> [20, 22]
	auto * node3 = scene()->root().addChildNode("node3", SceneElement_MeshNode);
	qobject_cast<IMeshNode *>(node3)->addGeometry(mat)->setVertices(verts);
	qobject_cast<IMeshNode *>(node3)->getGeometry(0)->setIndices(indices);
	node3->setTransformation(glm::translate(glm::mat4(1.f), vec3(0.f, 0.f, 20.f)));

	// Box 4: off to the side at x = 10, initial z = 15 -> [15, 17]
	auto * node4 = scene()->root().addChildNode("node4", SceneElement_MeshNode);
	qobject_cast<IMeshNode *>(node4)->addGeometry(mat)->setVertices(verts);
	qobject_cast<IMeshNode *>(node4)->getGeometry(0)->setIndices(indices);
	node4->setTransformation(glm::translate(glm::mat4(1.f), vec3(10.f, 0.f, 15.f)));

	scene()->buildCollisionScene();

	scene()->beginDropToSurface({ node3, node1, node4, node2 });

	// node1 should be on the floor: z = 0
	EXPECT_NEAR(0.f, node1->transformation()[3][2], 1e-3f);

	// node2 should be on node1: z = 2
	EXPECT_NEAR(2.f, node2->transformation()[3][2], 1e-3f);

	// node3 should be on node2: z = 4
	EXPECT_NEAR(4.f, node3->transformation()[3][2], 1e-3f);

	// node4 should be on the floor (misses boxes 1-3 because x = 10): z = 0
	EXPECT_NEAR(0.f, node4->transformation()[3][2], 1e-3f);
	EXPECT_NEAR(10.f, node4->transformation()[3][0], 1e-3f);
}

TEST_F(TestScene, NodeGetAABBAndBottomPoint)
{
	// 1. Empty Node
	auto * emptyNode = scene()->root().addChildNode("empty", SceneElement_Node);
	emptyNode->setTransformation(glm::translate(glm::mat4(1.f), vec3(5.f, 6.f, 7.f)));
	BBox emptyBBox = emptyNode->getAABB();
	EXPECT_FALSE(emptyBBox.isNull());
	EXPECT_FLOAT_EQ(5.f, emptyBBox.min.x);
	EXPECT_FLOAT_EQ(6.f, emptyBBox.min.y);
	EXPECT_FLOAT_EQ(7.f, emptyBBox.min.z);
	EXPECT_FLOAT_EQ(5.f, emptyBBox.max.x);
	EXPECT_FLOAT_EQ(6.f, emptyBBox.max.y);
	EXPECT_FLOAT_EQ(7.f, emptyBBox.max.z);

	vec3 emptyBottom = emptyNode->getObjectBottomPoint();
	EXPECT_FLOAT_EQ(5.f, emptyBottom.x);
	EXPECT_FLOAT_EQ(6.f, emptyBottom.y);
	EXPECT_FLOAT_EQ(7.f, emptyBottom.z);

	// 2. MeshNode with geometry
	auto mat = matmanager()->create(QString("matAABB"));
	auto * meshNode = scene()->root().addChildNode("meshNode", SceneElement_MeshNode);
	auto * imesh = qobject_cast<IMeshNode *>(meshNode);
	auto * geom = imesh->addGeometry(mat);
	std::vector<Vertex> verts = {
		{ vec3(-1, -1, 0), vec3(0, 0, 1) },
		{ vec3( 1, -1, 0), vec3(0, 0, 1) },
		{ vec3( 1,  1, 0), vec3(0, 0, 1) },
		{ vec3(-1,  1, 0), vec3(0, 0, 1) },
		{ vec3(-1, -1, 2), vec3(0, 0, 1) },
		{ vec3( 1, -1, 2), vec3(0, 0, 1) },
		{ vec3( 1,  1, 2), vec3(0, 0, 1) },
		{ vec3(-1,  1, 2), vec3(0, 0, 1) },
	};
	std::vector<uint32_t> indices = { 0, 1, 2, 0, 2, 3 };
	geom->setVertices(verts);
	geom->setIndices(indices);

	meshNode->setTransformation(glm::translate(glm::mat4(1.f), vec3(2.f, 3.f, 4.f)));

	BBox meshBBox = meshNode->getAABB();
	EXPECT_FLOAT_EQ(1.f, meshBBox.min.x); // 2 - 1
	EXPECT_FLOAT_EQ(2.f, meshBBox.min.y); // 3 - 1
	EXPECT_FLOAT_EQ(4.f, meshBBox.min.z); // 4 + 0
	EXPECT_FLOAT_EQ(3.f, meshBBox.max.x); // 2 + 1
	EXPECT_FLOAT_EQ(4.f, meshBBox.max.y); // 3 + 1
	EXPECT_FLOAT_EQ(6.f, meshBBox.max.z); // 4 + 2

	vec3 meshBottom = meshNode->getObjectBottomPoint();
	EXPECT_FLOAT_EQ(2.f, meshBottom.x);
	EXPECT_FLOAT_EQ(3.f, meshBottom.y);
	EXPECT_FLOAT_EQ(4.f, meshBottom.z);

	// 3. Hierarchy: parent Node contains child MeshNode
	auto * parentGroup = scene()->root().addChildNode("group", SceneElement_Node);
	parentGroup->setTransformation(glm::translate(glm::mat4(1.f), vec3(10.f, 0.f, 0.f)));
	auto * childMesh = parentGroup->addChildNode("childMesh", SceneElement_MeshNode);
	auto * childImesh = qobject_cast<IMeshNode *>(childMesh);
	auto * childGeom = childImesh->addGeometry(mat);
	childGeom->setVertices(verts);
	childGeom->setIndices(indices);
	childMesh->setTransformation(glm::translate(glm::mat4(1.f), vec3(0.f, 5.f, 1.f)));

	BBox parentBBox = parentGroup->getAABB();
	// childMesh global transform: x = 10, y = 5, z = 1
	EXPECT_FLOAT_EQ(9.f, parentBBox.min.x);  // 10 - 1
	EXPECT_FLOAT_EQ(4.f, parentBBox.min.y);  // 5 - 1
	EXPECT_FLOAT_EQ(1.f, parentBBox.min.z);  // 1 + 0
	EXPECT_FLOAT_EQ(11.f, parentBBox.max.x); // 10 + 1
	EXPECT_FLOAT_EQ(6.f, parentBBox.max.y);  // 5 + 1
	EXPECT_FLOAT_EQ(3.f, parentBBox.max.z);  // 1 + 2

	vec3 parentBottom = parentGroup->getObjectBottomPoint();
	EXPECT_FLOAT_EQ(10.f, parentBottom.x);
	EXPECT_FLOAT_EQ(5.f, parentBottom.y);
	EXPECT_FLOAT_EQ(1.f, parentBottom.z);
}

