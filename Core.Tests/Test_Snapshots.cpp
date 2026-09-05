#include "../Shared/Interfaces/MaterialManager.h"
#include "../Shared/Interfaces/CoreInstance.h"
#include "../Shared/Interfaces/Scene.h"
#include "../Shared/Interfaces/Material.h"
#include "../Shared/Interfaces/Camera.h"
#include "../Shared/Interfaces/Geometry.h"
#include "../Shared/Interfaces/MeshNode.h"
#include "../Shared/Interfaces/SnapshotManager.h"
#include "../Shared/Interfaces/Snapshot.h"
#include "../Shared/Interfaces/ModelSaver.h"
#include "../Shared/Interfaces/SerializationContext.h"
#include "../Core/src/CoreInstance.h"

#include "Helpers.h"

class TestSnapshots : public ::testing::Test
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

	IMaterialManager *matmanager()
	{
		return &scene()->materialManager();
	}

	ISnapshotManager *snapmanager()
	{
		return &scene()->snapshotManager();
	}

	ICamera *camera()
	{
		return &scene()->camera();
	}
};

TEST_F(TestSnapshots, FullSnapshotStoresEverything)
{
	app()->startPlugins();

	INode *pNode = scene()->root().addChildNode("mn123", SceneElement_MeshNode);
	IMeshNode *meshNode = qobject_cast<IMeshNode *>(pNode);
	ASSERT_NE(nullptr, meshNode);

	IMaterial *mat1 = matmanager()->create(QString("mat123"));
	ASSERT_NE(nullptr, mat1);

	IMaterial *mat2 = matmanager()->create(QString("mat456"));
	ASSERT_NE(nullptr, mat2);

	enum { COUNT = 99 };
	auto verts = createGeometryVertices(COUNT);
	auto uvSet0 = createGeometryUVset(COUNT, vec2(1.234f, 5.678f));
	auto uvSet1 = createGeometryUVset(COUNT, vec2(2.345f, 6.789f));
	auto uvSet2 = createGeometryUVset(COUNT, vec2(3.456f, 7.890f));
	auto uvSet3 = createGeometryUVset(COUNT, vec2(4.567f, 8.901f));
	auto indices = createGeometryIndices(COUNT);
	IGeometry *geom1 = meshNode->addGeometry(mat1);
	geom1->setVertices(verts);
	geom1->setIndices(indices);
	geom1->setUVset(0, uvSet0);
	geom1->setUVset(1, uvSet1);
	geom1->setUVset(2, uvSet2);
	geom1->setUVset(3, uvSet3);

	ICamera *pCamera = camera();
	pCamera->yaw().set(10);
	pCamera->pitch().set(20);

	ISnapshot *pSnap1 = snapmanager()->create();
	ASSERT_NE(nullptr, pSnap1);

	pNode->position().set(vec3(10, 20, 30));
	pNode->rotation().set(vec3(11, 12, 13));
	pNode->scale().set(vec3(1.f, 2.f, 4.f));
	pNode->visible().set(false);
	pCamera->yaw().set(1);
	pCamera->pitch().set(2);
	GeomSelection sel = { geom1 };
	scene()->setMaterial(sel, mat2);

	ISnapshot *pSnap2 = snapmanager()->create();
	ASSERT_NE(nullptr, pSnap2);

	pSnap1->activate();

	EXPECT_EQ(vec3(0, 0, 0), pNode->position().get());
	EXPECT_EQ(vec3(0, 0, 0), pNode->rotation().get());
	EXPECT_EQ(vec3(1, 1, 1), pNode->scale().get());
	EXPECT_TRUE(pNode->visible().get());
	EXPECT_EQ(10.f, pCamera->yaw().get());
	EXPECT_EQ(20.f, pCamera->pitch().get());
	EXPECT_EQ(mat1, geom1->material());

	pSnap2->activate();

	EXPECT_EQ(vec3(10, 20, 30), pNode->position().get());
	EXPECT_EQ(vec3(11, 12, 13), pNode->rotation().get());
	EXPECT_EQ(vec3(1, 2, 4), pNode->scale().get());
	EXPECT_FALSE(pNode->visible().get());
	EXPECT_EQ(1.f, pCamera->yaw().get());
	EXPECT_EQ(2.f, pCamera->pitch().get());
	EXPECT_EQ(mat2, geom1->material());
}

TEST_F(TestSnapshots, CameraOnlySnapshot)
{
	app()->startPlugins();

	INode *pNode = scene()->root().addChildNode("mn123", SceneElement_MeshNode);
	IMeshNode *meshNode = qobject_cast<IMeshNode *>(pNode);
	ASSERT_NE(nullptr, meshNode);

	IMaterial *mat1 = matmanager()->create(QString("mat123"));
	ASSERT_NE(nullptr, mat1);

	IMaterial *mat2 = matmanager()->create(QString("mat456"));
	ASSERT_NE(nullptr, mat2);

	enum { COUNT = 99 };
	auto verts = createGeometryVertices(COUNT);
	auto uvSet0 = createGeometryUVset(COUNT, vec2(1.234f, 5.678f));
	auto uvSet1 = createGeometryUVset(COUNT, vec2(2.345f, 6.789f));
	auto uvSet2 = createGeometryUVset(COUNT, vec2(3.456f, 7.890f));
	auto uvSet3 = createGeometryUVset(COUNT, vec2(4.567f, 8.901f));
	auto indices = createGeometryIndices(COUNT);
	IGeometry *geom1 = meshNode->addGeometry(mat1);
	geom1->setVertices(verts);
	geom1->setIndices(indices);
	geom1->setUVset(0, uvSet0);
	geom1->setUVset(1, uvSet1);
	geom1->setUVset(2, uvSet2);
	geom1->setUVset(3, uvSet3);

	ICamera *pCamera = camera();
	pCamera->yaw().set(10);
	pCamera->pitch().set(20);

	ISnapshot *pSnap1 = snapmanager()->create();
	ASSERT_NE(nullptr, pSnap1);

	pNode->position().set(vec3(10, 20, 30));
	pNode->rotation().set(vec3(11, 12, 13));
	pNode->scale().set(vec3(1.f, 2.f, 4.f));
	pNode->visible().set(false);
	pCamera->yaw().set(1);
	pCamera->pitch().set(2);
	GeomSelection sel = { geom1 };
	scene()->setMaterial(sel, mat2);

	ISnapshot *pSnap2 = snapmanager()->create();
	ASSERT_NE(nullptr, pSnap2);
	pSnap2->hasCameraState().set(true);
	pSnap2->hasTransformations().set(false);
	pSnap2->hasVisibility().set(false);
	pSnap2->hasAssignedMaterials().set(false);

	pSnap1->activate();

	EXPECT_EQ(vec3(0, 0, 0), pNode->position().get());
	EXPECT_EQ(vec3(0, 0, 0), pNode->rotation().get());
	EXPECT_EQ(vec3(1, 1, 1), pNode->scale().get());
	EXPECT_TRUE(pNode->visible().get());
	EXPECT_EQ(10.f, pCamera->yaw().get());
	EXPECT_EQ(20.f, pCamera->pitch().get());
	EXPECT_EQ(mat1, geom1->material());

	pSnap2->activate();

	EXPECT_EQ(vec3(0, 0, 0), pNode->position().get());
	EXPECT_EQ(vec3(0, 0, 0), pNode->rotation().get());
	EXPECT_EQ(vec3(1, 1, 1), pNode->scale().get());
	EXPECT_TRUE(pNode->visible().get());
	EXPECT_EQ(1.f, pCamera->yaw().get());
	EXPECT_EQ(2.f, pCamera->pitch().get());
	EXPECT_EQ(mat1, geom1->material());
}

TEST_F(TestSnapshots, TransformationOnlySnapshot)
{
	app()->startPlugins();

	INode *pNode = scene()->root().addChildNode("mn123", SceneElement_MeshNode);
	IMeshNode *meshNode = qobject_cast<IMeshNode *>(pNode);
	ASSERT_NE(nullptr, meshNode);

	IMaterial *mat1 = matmanager()->create(QString("mat123"));
	ASSERT_NE(nullptr, mat1);

	IMaterial *mat2 = matmanager()->create(QString("mat456"));
	ASSERT_NE(nullptr, mat2);

	enum { COUNT = 99 };
	auto verts = createGeometryVertices(COUNT);
	auto uvSet0 = createGeometryUVset(COUNT, vec2(1.234f, 5.678f));
	auto uvSet1 = createGeometryUVset(COUNT, vec2(2.345f, 6.789f));
	auto uvSet2 = createGeometryUVset(COUNT, vec2(3.456f, 7.890f));
	auto uvSet3 = createGeometryUVset(COUNT, vec2(4.567f, 8.901f));
	auto indices = createGeometryIndices(COUNT);
	IGeometry *geom1 = meshNode->addGeometry(mat1);
	geom1->setVertices(verts);
	geom1->setIndices(indices);
	geom1->setUVset(0, uvSet0);
	geom1->setUVset(1, uvSet1);
	geom1->setUVset(2, uvSet2);
	geom1->setUVset(3, uvSet3);

	ICamera *pCamera = camera();
	pCamera->yaw().set(10);
	pCamera->pitch().set(20);

	ISnapshot *pSnap1 = snapmanager()->create();
	ASSERT_NE(nullptr, pSnap1);

	pNode->position().set(vec3(10, 20, 30));
	pNode->rotation().set(vec3(11, 12, 13));
	pNode->scale().set(vec3(1.f, 2.f, 4.f));
	pNode->visible().set(false);
	pCamera->yaw().set(1);
	pCamera->pitch().set(2);
	GeomSelection sel = { geom1 };
	scene()->setMaterial(sel, mat2);

	ISnapshot *pSnap2 = snapmanager()->create();
	ASSERT_NE(nullptr, pSnap2);
	pSnap2->hasCameraState().set(false);
	pSnap2->hasTransformations().set(true);
	pSnap2->hasVisibility().set(false);
	pSnap2->hasAssignedMaterials().set(false);

	pSnap1->activate();

	EXPECT_EQ(vec3(0, 0, 0), pNode->position().get());
	EXPECT_EQ(vec3(0, 0, 0), pNode->rotation().get());
	EXPECT_EQ(vec3(1, 1, 1), pNode->scale().get());
	EXPECT_TRUE(pNode->visible().get());
	EXPECT_EQ(10.f, pCamera->yaw().get());
	EXPECT_EQ(20.f, pCamera->pitch().get());
	EXPECT_EQ(mat1, geom1->material());

	pSnap2->activate();

	EXPECT_EQ(vec3(10, 20, 30), pNode->position().get());
	EXPECT_EQ(vec3(11, 12, 13), pNode->rotation().get());
	EXPECT_EQ(vec3(1, 2, 4), pNode->scale().get());
	EXPECT_TRUE(pNode->visible().get());
	EXPECT_EQ(10.f, pCamera->yaw().get());
	EXPECT_EQ(20.f, pCamera->pitch().get());
	EXPECT_EQ(mat1, geom1->material());
}

TEST_F(TestSnapshots, VisibilityOnlySnapshot)
{
	app()->startPlugins();

	INode *pNode = scene()->root().addChildNode("mn123", SceneElement_MeshNode);
	IMeshNode *meshNode = qobject_cast<IMeshNode *>(pNode);
	ASSERT_NE(nullptr, meshNode);

	IMaterial *mat1 = matmanager()->create(QString("mat123"));
	ASSERT_NE(nullptr, mat1);

	IMaterial *mat2 = matmanager()->create(QString("mat456"));
	ASSERT_NE(nullptr, mat2);

	enum { COUNT = 99 };
	auto verts = createGeometryVertices(COUNT);
	auto uvSet0 = createGeometryUVset(COUNT, vec2(1.234f, 5.678f));
	auto uvSet1 = createGeometryUVset(COUNT, vec2(2.345f, 6.789f));
	auto uvSet2 = createGeometryUVset(COUNT, vec2(3.456f, 7.890f));
	auto uvSet3 = createGeometryUVset(COUNT, vec2(4.567f, 8.901f));
	auto indices = createGeometryIndices(COUNT);
	IGeometry *geom1 = meshNode->addGeometry(mat1);
	geom1->setVertices(verts);
	geom1->setIndices(indices);
	geom1->setUVset(0, uvSet0);
	geom1->setUVset(1, uvSet1);
	geom1->setUVset(2, uvSet2);
	geom1->setUVset(3, uvSet3);

	ICamera *pCamera = camera();
	pCamera->yaw().set(10);
	pCamera->pitch().set(20);

	ISnapshot *pSnap1 = snapmanager()->create();
	ASSERT_NE(nullptr, pSnap1);

	pNode->position().set(vec3(10, 20, 30));
	pNode->rotation().set(vec3(11, 12, 13));
	pNode->scale().set(vec3(1.f, 2.f, 4.f));
	pNode->visible().set(false);
	pCamera->yaw().set(1);
	pCamera->pitch().set(2);
	GeomSelection sel = { geom1 };
	scene()->setMaterial(sel, mat2);

	ISnapshot *pSnap2 = snapmanager()->create();
	ASSERT_NE(nullptr, pSnap2);
	pSnap2->hasCameraState().set(false);
	pSnap2->hasTransformations().set(false);
	pSnap2->hasVisibility().set(true);
	pSnap2->hasAssignedMaterials().set(false);

	pSnap1->activate();

	EXPECT_EQ(vec3(0, 0, 0), pNode->position().get());
	EXPECT_EQ(vec3(0, 0, 0), pNode->rotation().get());
	EXPECT_EQ(vec3(1, 1, 1), pNode->scale().get());
	EXPECT_TRUE(pNode->visible().get());
	EXPECT_EQ(10.f, pCamera->yaw().get());
	EXPECT_EQ(20.f, pCamera->pitch().get());
	EXPECT_EQ(mat1, geom1->material());

	pSnap2->activate();

	EXPECT_EQ(vec3(0, 0, 0), pNode->position().get());
	EXPECT_EQ(vec3(0, 0, 0), pNode->rotation().get());
	EXPECT_EQ(vec3(1, 1, 1), pNode->scale().get());
	EXPECT_FALSE(pNode->visible().get());
	EXPECT_EQ(10.f, pCamera->yaw().get());
	EXPECT_EQ(20.f, pCamera->pitch().get());
	EXPECT_EQ(mat1, geom1->material());
}

TEST_F(TestSnapshots, MaterialsOnlySnapshot)
{
	app()->startPlugins();

	INode *pNode = scene()->root().addChildNode("mn123", SceneElement_MeshNode);
	IMeshNode *meshNode = qobject_cast<IMeshNode *>(pNode);
	ASSERT_NE(nullptr, meshNode);

	IMaterial *mat1 = matmanager()->create(QString("mat123"));
	ASSERT_NE(nullptr, mat1);

	IMaterial *mat2 = matmanager()->create(QString("mat456"));
	ASSERT_NE(nullptr, mat2);

	enum { COUNT = 99 };
	auto verts = createGeometryVertices(COUNT);
	auto uvSet0 = createGeometryUVset(COUNT, vec2(1.234f, 5.678f));
	auto uvSet1 = createGeometryUVset(COUNT, vec2(2.345f, 6.789f));
	auto uvSet2 = createGeometryUVset(COUNT, vec2(3.456f, 7.890f));
	auto uvSet3 = createGeometryUVset(COUNT, vec2(4.567f, 8.901f));
	auto indices = createGeometryIndices(COUNT);
	IGeometry *geom1 = meshNode->addGeometry(mat1);
	geom1->setVertices(verts);
	geom1->setIndices(indices);
	geom1->setUVset(0, uvSet0);
	geom1->setUVset(1, uvSet1);
	geom1->setUVset(2, uvSet2);
	geom1->setUVset(3, uvSet3);

	ICamera *pCamera = camera();
	pCamera->yaw().set(10);
	pCamera->pitch().set(20);

	ISnapshot *pSnap1 = snapmanager()->create();
	ASSERT_NE(nullptr, pSnap1);

	pNode->position().set(vec3(10, 20, 30));
	pNode->rotation().set(vec3(11, 12, 13));
	pNode->scale().set(vec3(1.f, 2.f, 4.f));
	pNode->visible().set(false);
	pCamera->yaw().set(1);
	pCamera->pitch().set(2);
	GeomSelection sel = { geom1 };
	scene()->setMaterial(sel, mat2);

	ISnapshot *pSnap2 = snapmanager()->create();
	ASSERT_NE(nullptr, pSnap2);
	pSnap2->hasCameraState().set(false);
	pSnap2->hasTransformations().set(false);
	pSnap2->hasVisibility().set(false);
	pSnap2->hasAssignedMaterials().set(true);

	pSnap1->activate();

	EXPECT_EQ(vec3(0, 0, 0), pNode->position().get());
	EXPECT_EQ(vec3(0, 0, 0), pNode->rotation().get());
	EXPECT_EQ(vec3(1, 1, 1), pNode->scale().get());
	EXPECT_TRUE(pNode->visible().get());
	EXPECT_EQ(10.f, pCamera->yaw().get());
	EXPECT_EQ(20.f, pCamera->pitch().get());
	EXPECT_EQ(mat1, geom1->material());

	pSnap2->activate();

	EXPECT_EQ(vec3(0, 0, 0), pNode->position().get());
	EXPECT_EQ(vec3(0, 0, 0), pNode->rotation().get());
	EXPECT_EQ(vec3(1, 1, 1), pNode->scale().get());
	EXPECT_TRUE(pNode->visible().get());
	EXPECT_EQ(10.f, pCamera->yaw().get());
	EXPECT_EQ(20.f, pCamera->pitch().get());
	EXPECT_EQ(mat2, geom1->material());
}

TEST_F(TestSnapshots, _create)
{
	app()->startPlugins();

	ASSERT_EQ(0, snapmanager()->count());

	SnapshotParams params;
	params.name = "Snapshot 1";
	params.isCameraValid = true;
	params.camera.center = vec3(0, 1, 2);
	params.camera.yaw = 10.f;
	params.camera.pitch = 20.f;
	params.camera.roll = 30.f;
	params.camera.distance = 40.f;
	params.camera.aspect = 1.5f;
	params.camera.fov = 50.f;
	params.camera.nearZ = 1.f;
	params.camera.farZ = 100.f;
	params.camera.depthOfField = true;
	params.camera.fStop = 5.f;
	params.camera.focusDistance = 120.f;
	ISnapshot *pSnap1 = snapmanager()->_create(params);
	ASSERT_EQ(1, snapmanager()->count());
	ASSERT_TRUE(pSnap1->hasCameraState().get());

	params.name = "Snapshot 2";
	params.isCameraValid = true;
	params.camera.center = vec3(1, 2, 3);
	params.camera.yaw = 20.f;
	params.camera.pitch = 30.f;
	params.camera.roll = 40.f;
	params.camera.distance = 50.f;
	params.camera.aspect = 2.f;
	params.camera.fov = 60.f;
	params.camera.nearZ = 2.f;
	params.camera.farZ = 80.f;
	params.camera.depthOfField = false;
	params.camera.fStop = 10.f;
	params.camera.focusDistance = 220.f;
	ISnapshot *pSnap2 = snapmanager()->_create(params);
	ASSERT_EQ(2, snapmanager()->count());
	ASSERT_TRUE(pSnap2->hasCameraState().get());

	ICamera *pCamera = camera();

	pSnap1->activate();
	EXPECT_EQ(10.f, pCamera->yaw().get());
	EXPECT_EQ(20.f, pCamera->pitch().get());
	EXPECT_EQ(30.f, pCamera->roll().get());
	EXPECT_EQ(0.f, pCamera->target().get().x);
	EXPECT_EQ(1.f, pCamera->target().get().y);
	EXPECT_EQ(2.f, pCamera->target().get().z);
	EXPECT_EQ(40.f, pCamera->distance().get());
	EXPECT_EQ(50.f, pCamera->fov().get());
	EXPECT_EQ(1.5f, pCamera->aspect().get());
	EXPECT_EQ(1.f, pCamera->nearZ().get());
	EXPECT_EQ(100.f, pCamera->farZ().get());
	EXPECT_TRUE(pCamera->depthOfField().get());
	EXPECT_EQ(5.f, pCamera->fStop().get());
	EXPECT_EQ(120.f, pCamera->focusDistance().get());

	pSnap2->activate();
	EXPECT_EQ(20.f, pCamera->yaw().get());
	EXPECT_EQ(30.f, pCamera->pitch().get());
	EXPECT_EQ(40.f, pCamera->roll().get());
	EXPECT_EQ(1.f, pCamera->target().get().x);
	EXPECT_EQ(2.f, pCamera->target().get().y);
	EXPECT_EQ(3.f, pCamera->target().get().z);
	EXPECT_EQ(50.f, pCamera->distance().get());
	EXPECT_EQ(60.f, pCamera->fov().get());
	EXPECT_EQ(2.f, pCamera->aspect().get());
	EXPECT_EQ(2.f, pCamera->nearZ().get());
	EXPECT_EQ(80.f, pCamera->farZ().get());
	EXPECT_FALSE(pCamera->depthOfField().get());
}

TEST_F(TestSnapshots, SceneParamsSnapshot)
{
	app()->startPlugins();

	ISceneProperties& sp = scene()->properties();
	auto *et = sp.environment().texture();
	auto *bt = sp.background().texture();

	const auto envC1 = sp.environment().get();
	const auto envI1 = sp.environmentIntensity().get();
	const auto envS1 = sp.environmentSize().get();
	const auto envVO1 = sp.environmentVerticalOffset().get();
	const auto envHR1 = sp.environmentHorizontalRotation().get();
	const auto envVR1 = sp.environmentVerticalRotation().get();
	const auto etF1 = et->fileName().get();
	const auto etE1 = et->enabled().get();
	const auto etI1 = et->invert().get();
	const auto etNM1 = et->normalMap().get();
	const auto etB1 = et->brightness().get();
	const auto etC1 = et->contrast().get();
	const auto etG1 = et->gamma().get();
	const auto etRt1 = et->repeat().get();
	const auto etO1 = et->offset().get();
	const auto etRn1 = et->rotation().get();
	const auto etCL1 = et->cropLeft().get();
	const auto etCT1 = et->cropTop().get();
	const auto etCR1 = et->cropRight().get();
	const auto etCB1 = et->cropBottom().get();
	const auto etWX1 = et->wrapX().getIndex();
	const auto etWY1 = et->wrapY().getIndex();
	const auto etM1 = et->mapping().getIndex();
	const auto bgC1 = sp.background().get();
	const auto bgM1 = sp.backgroundMode().getIndex();
	const auto btF1 = bt->fileName().get();
	const auto btE1 = bt->enabled().get();
	const auto btI1 = bt->invert().get();
	const auto btNM1 = bt->normalMap().get();
	const auto btB1 = bt->brightness().get();
	const auto btC1 = bt->contrast().get();
	const auto btG1 = bt->gamma().get();
	const auto btRt1 = bt->repeat().get();
	const auto btO1 = bt->offset().get();
	const auto btRn1 = bt->rotation().get();
	const auto btCL1 = bt->cropLeft().get();
	const auto btCT1 = bt->cropTop().get();
	const auto btCR1 = bt->cropRight().get();
	const auto btCB1 = bt->cropBottom().get();
	const auto btWX1 = bt->wrapX().getIndex();
	const auto btWY1 = bt->wrapY().getIndex();
	const auto btM1 = bt->mapping().getIndex();
	const auto envC2 = glm::vec3(0.111f, 0.222f, 0.333f);
	const auto envI2 = 123.f;
	const auto envS2 = 234.f;
	const auto envVO2 = 0.45f;
	const auto envHR2 = 56.f;
	const auto envVR2 = 67.f;
	const auto etF2 = "env2.jpg";
	const auto etE2 = !etE1;
	const auto etI2 = !etI1;
	const auto etNM2 = !etNM1;
	const auto etB2 = 0.98f;
	const auto etC2 = 0.87f;
	const auto etG2 = 3.21f;
	const auto etRt2 = vec2(4.32f, 5.43f);
	const auto etO2 = vec2(6.54f, 7.65f);
	const auto etRn2 = 8.76f;
	const auto etCL2 = 0.123f;
	const auto etCT2 = 0.234f;
	const auto etCR2 = 0.987f;
	const auto etCB2 = 0.876f;
	const auto etWX2 = ITexture::WRAP_CLAMP;
	const auto etWY2 = ITexture::WRAP_MIRROR;
	const auto etM2 = ITexture::MAPPING_UV2;
	const auto bgM2 = BackgroundMode_PlaneImage;
	const auto bgC2 = glm::vec3(0.444f, 0.555f, 0.666f);
	const auto btF2 = "bg2.jpg";
	const auto btE2 = !etE1;
	const auto btI2 = !etI1;
	const auto btNM2 = !etNM1;
	const auto btB2 = 0.89f;
	const auto btC2 = 0.78f;
	const auto btG2 = 1.23f;
	const auto btRt2 = vec2(2.34f, 3.45f);
	const auto btO2 = vec2(4.56f, 5.67f);
	const auto btRn2 = 6.78f;
	const auto btCL2 = 0.132f;
	const auto btCT2 = 0.243f;
	const auto btCR2 = 0.978f;
	const auto btCB2 = 0.867f;
	const auto btWX2 = ITexture::WRAP_MIRROR;
	const auto btWY2 = ITexture::WRAP_CLAMP;
	const auto btM2 = ITexture::MAPPING_UV3;

	auto snapshot1 = snapmanager()->create();

	sp.environment()._set(envC2);
	sp.environmentIntensity()._set(envI2);
	sp.environmentSize()._set(envS2);
	sp.environmentVerticalOffset()._set(envVO2);
	sp.environmentHorizontalRotation()._set(envHR2);
	sp.environmentVerticalRotation()._set(envVR2);
	et->_set(etE2, etF2, RectF(etCL2, etCT2, etCR2, etCB2), etWX2, etWY2, etM2, etRt2, etO2, etRn2, etI2, etB2, etC2, etG2, etNM2);
	sp.background()._set(bgC2);
	sp.backgroundMode()._setIndex(bgM2);
	bt->_set(btE2, btF2, RectF(btCL2, btCT2, btCR2, btCB2), btWX2, btWY2, btM2, btRt2, btO2, btRn2, btI2, btB2, btC2, btG2, btNM2);

	auto snapshot2 = snapmanager()->create();

	snapshot1->activate();

	EXPECT_EQ(sp.environment().get().r, envC1.r);
	EXPECT_EQ(sp.environment().get().g, envC1.g);
	EXPECT_EQ(sp.environment().get().b, envC1.b);
	EXPECT_EQ(sp.environmentIntensity().get(), envI1);
	EXPECT_EQ(sp.environmentSize().get(), envS1);
	EXPECT_EQ(sp.environmentVerticalOffset().get(), envVO1);
	EXPECT_EQ(sp.environmentHorizontalRotation().get(), envHR1);
	EXPECT_EQ(sp.environmentVerticalRotation().get(), envVR1);

	EXPECT_EQ(et->fileName().get(), etF1);
	EXPECT_EQ(et->enabled().get(), etE1);
	EXPECT_EQ(et->invert().get(), etI1);
	EXPECT_EQ(et->normalMap().get(), etNM1);
	EXPECT_EQ(et->brightness().get(), etB1);
	EXPECT_EQ(et->contrast().get(), etC1);
	EXPECT_EQ(et->gamma().get(), etG1);
	EXPECT_EQ(et->repeat().get().x, etRt1.x);
	EXPECT_EQ(et->repeat().get().y, etRt1.y);
	EXPECT_EQ(et->offset().get().x, etO1.x);
	EXPECT_EQ(et->offset().get().y, etO1.y);
	EXPECT_EQ(et->rotation().get(), etRn1);
	EXPECT_EQ(et->cropLeft().get(), etCL1);
	EXPECT_EQ(et->cropTop().get(), etCT1);
	EXPECT_EQ(et->cropRight().get(), etCR1);
	EXPECT_EQ(et->cropBottom().get(), etCB1);
	EXPECT_EQ(et->wrapX().getIndex(), etWX1);
	EXPECT_EQ(et->wrapY().getIndex(), etWY1);
	EXPECT_EQ(et->mapping().getIndex(), etM1);

	EXPECT_EQ(sp.background().get().r, bgC1.r);
	EXPECT_EQ(sp.background().get().g, bgC1.g);
	EXPECT_EQ(sp.background().get().b, bgC1.b);
	EXPECT_EQ(sp.backgroundMode().getIndex(), bgM1);

	EXPECT_EQ(bt->fileName().get(), btF1);
	EXPECT_EQ(bt->enabled().get(), btE1);
	EXPECT_EQ(bt->invert().get(), btI1);
	EXPECT_EQ(bt->normalMap().get(), btNM1);
	EXPECT_EQ(bt->brightness().get(), btB1);
	EXPECT_EQ(bt->contrast().get(), btC1);
	EXPECT_EQ(bt->gamma().get(), btG1);
	EXPECT_EQ(bt->repeat().get().x, btRt1.x);
	EXPECT_EQ(bt->repeat().get().y, btRt1.y);
	EXPECT_EQ(bt->offset().get().x, btO1.x);
	EXPECT_EQ(bt->offset().get().y, btO1.y);
	EXPECT_EQ(bt->rotation().get(), btRn1);
	EXPECT_EQ(bt->cropLeft().get(), btCL1);
	EXPECT_EQ(bt->cropTop().get(), btCT1);
	EXPECT_EQ(bt->cropRight().get(), btCR1);
	EXPECT_EQ(bt->cropBottom().get(), btCB1);
	EXPECT_EQ(bt->wrapX().getIndex(), btWX1);
	EXPECT_EQ(bt->wrapY().getIndex(), btWY1);
	EXPECT_EQ(bt->mapping().getIndex(), btM1);

	snapshot2->activate();

	EXPECT_EQ(sp.environment().get().r, envC2.r);
	EXPECT_EQ(sp.environment().get().g, envC2.g);
	EXPECT_EQ(sp.environment().get().b, envC2.b);
	EXPECT_EQ(sp.environmentIntensity().get(), envI2);
	EXPECT_EQ(sp.environmentSize().get(), envS2);
	EXPECT_EQ(sp.environmentVerticalOffset().get(), envVO2);
	EXPECT_EQ(sp.environmentHorizontalRotation().get(), envHR2);
	EXPECT_EQ(sp.environmentVerticalRotation().get(), envVR2);

	EXPECT_EQ(et->fileName().get(), etF2);
	EXPECT_EQ(et->enabled().get(), etE2);
	EXPECT_EQ(et->invert().get(), etI2);
	EXPECT_EQ(et->normalMap().get(), etNM2);
	EXPECT_EQ(et->brightness().get(), etB2);
	EXPECT_EQ(et->contrast().get(), etC2);
	EXPECT_EQ(et->gamma().get(), etG2);
	EXPECT_EQ(et->repeat().get().x, etRt2.x);
	EXPECT_EQ(et->repeat().get().y, etRt2.y);
	EXPECT_EQ(et->offset().get().x, etO2.x);
	EXPECT_EQ(et->offset().get().y, etO2.y);
	EXPECT_EQ(et->rotation().get(), etRn2);
	EXPECT_EQ(et->cropLeft().get(), etCL2);
	EXPECT_EQ(et->cropTop().get(), etCT2);
	EXPECT_EQ(et->cropRight().get(), etCR2);
	EXPECT_EQ(et->cropBottom().get(), etCB2);
	EXPECT_EQ(et->wrapX().getIndex(), etWX2);
	EXPECT_EQ(et->wrapY().getIndex(), etWY2);
	EXPECT_EQ(et->mapping().getIndex(), etM2);

	EXPECT_EQ(sp.background().get().r, bgC2.r);
	EXPECT_EQ(sp.background().get().g, bgC2.g);
	EXPECT_EQ(sp.background().get().b, bgC2.b);
	EXPECT_EQ(sp.backgroundMode().getIndex(), bgM2);

	EXPECT_EQ(bt->fileName().get(), btF2);
	EXPECT_EQ(bt->enabled().get(), btE2);
	EXPECT_EQ(bt->invert().get(), btI2);
	EXPECT_EQ(bt->normalMap().get(), btNM2);
	EXPECT_EQ(bt->brightness().get(), btB2);
	EXPECT_EQ(bt->contrast().get(), btC2);
	EXPECT_EQ(bt->gamma().get(), btG2);
	EXPECT_EQ(bt->repeat().get().x, btRt2.x);
	EXPECT_EQ(bt->repeat().get().y, btRt2.y);
	EXPECT_EQ(bt->offset().get().x, btO2.x);
	EXPECT_EQ(bt->offset().get().y, btO2.y);
	EXPECT_EQ(bt->rotation().get(), btRn2);
	EXPECT_EQ(bt->cropLeft().get(), btCL2);
	EXPECT_EQ(bt->cropTop().get(), btCT2);
	EXPECT_EQ(bt->cropRight().get(), btCR2);
	EXPECT_EQ(bt->cropBottom().get(), btCB2);
	EXPECT_EQ(bt->wrapX().getIndex(), btWX2);
	EXPECT_EQ(bt->wrapY().getIndex(), btWY2);
	EXPECT_EQ(bt->mapping().getIndex(), btM2);
}
