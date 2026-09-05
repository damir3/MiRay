#include "../Shared/Interfaces/MaterialManager.h"
#include "../Shared/Interfaces/CoreInstance.h"
#include "../Shared/Interfaces/Scene.h"
#include "../Shared/Interfaces/Material.h"
#include "../Shared/Interfaces/Camera.h"
#include "../Shared/Interfaces/Geometry.h"
#include "../Shared/Interfaces/ModelSaver.h"
#include "../Shared/Interfaces/SerializationContext.h"
#include "../Core/src/CoreInstance.h"
#include "../Core/src/Materials/MaterialImpl.h"
#include "../Core/src/Materials/MaterialGroupImpl.h"
#include "../Core/src/Materials/MaterialLayerImpl.h"

#include "Helpers.h"

class TestMaterials : public ::testing::Test
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
};

TEST_F(TestMaterials, CreateTwoMaterialsWithTheSameName)
{
	auto pMtl1 = matmanager()->create(QString("test"));
	auto pMtl2 = matmanager()->create(QString("test"));

	EXPECT_STREQ("test", pMtl1->name().get().toUtf8());
	EXPECT_STREQ("test 1", pMtl2->name().get().toUtf8());
}

TEST_F(TestMaterials, SaveThenAddFromSavedDataNameShouldBeDifferent)
{
	auto pMtl1 = matmanager()->create(QString("test"));
	auto saved = pMtl1->save(ModelSavingContext());
	auto pMtl2 = matmanager()->load(saved, ModelLoadingContext(), matmanager()->count());

	EXPECT_STREQ("test", pMtl1->name().get().toUtf8());
	EXPECT_STREQ("test 1", pMtl2->name().get().toUtf8());
}

TEST_F(TestMaterials, SerializationOfVersionedParameters)
{
	auto pMtl1 = matmanager()->create(QString("test"));

	pMtl1->group(0)->layer(0)->roughness()._set(3.f);
	pMtl1->group(0)->layer(0)->anisotropy()._set(33.f);
	pMtl1->group(0)->layer(0)->diffuseTextureLayerMask()._set(false);
	pMtl1->group(0)->layer(0)->emissiveIntensity()._set(0.3f);

	auto saved = pMtl1->save(ModelSavingContext());
	auto pMtl2 = matmanager()->load(saved, ModelLoadingContext(), matmanager()->count());

	EXPECT_FLOAT_EQ(3.f, pMtl2->group(0)->layer(0)->roughness().get());
	EXPECT_FLOAT_EQ(33.f, pMtl2->group(0)->layer(0)->anisotropy().get());
	EXPECT_FLOAT_EQ(0.3f, pMtl2->group(0)->layer(0)->emissiveIntensity().get());
	EXPECT_FALSE(pMtl2->group(0)->layer(0)->diffuseTextureLayerMask().get());
}

TEST_F(TestMaterials, ItIsOKToAddTheSameNameIfMaterialIsDeleted)
{
	auto pMtl1 = matmanager()->create(QString("test"));
	matmanager()->remove({pMtl1});
	auto pMtl2 = matmanager()->create(QString("test"));

	EXPECT_STREQ("test", pMtl2->name().get().toUtf8());
}

TEST_F(TestMaterials, CheckMaterialsNaming)
{
	auto pMtl1 = matmanager()->create(QString("test"));
	auto pMtl2 = matmanager()->create(QString("test"));
	auto pMtl3 = matmanager()->create(QString("test"));
	auto pMtl4 = matmanager()->create(QString("test 1"));
	auto pMtl5 = matmanager()->create(QString("test 1"));

	EXPECT_STREQ("test", pMtl1->name().get().toUtf8());
	EXPECT_STREQ("test 1", pMtl2->name().get().toUtf8());
	EXPECT_STREQ("test 2", pMtl3->name().get().toUtf8());
	EXPECT_STREQ("test 1 1", pMtl4->name().get().toUtf8());
	EXPECT_STREQ("test 1 2", pMtl5->name().get().toUtf8());
}

TEST_F(TestMaterials, CannotRenameMaterialToExistingName)
{
	auto pMtl1 = matmanager()->create(QString("test"));
	auto pMtl2 = matmanager()->create(QString("test 1"));

	ASSERT_STREQ("test", pMtl1->name().get().toUtf8());
	ASSERT_STREQ("test 1", pMtl2->name().get().toUtf8());

	pMtl2->name().set("test");

	ASSERT_STREQ("test", pMtl1->name().get().toUtf8());
	ASSERT_STREQ("test 1", pMtl2->name().get().toUtf8());

	pMtl1->name().set("xxx");

	ASSERT_STREQ("xxx", pMtl1->name().get().toUtf8());
	ASSERT_STREQ("test 1", pMtl2->name().get().toUtf8());

	pMtl2->name().set("test");

	ASSERT_STREQ("xxx", pMtl1->name().get().toUtf8());
	ASSERT_STREQ("test", pMtl2->name().get().toUtf8());
}

TEST_F(TestMaterials, CheckGroupsManagement)
{
	auto pMtl = matmanager()->create(QString("test"));
	ASSERT_EQ(pMtl->numGroups(), 1);

	ASSERT_EQ(nullptr, pMtl->addGroup(2)); // invalid paremeter
	ASSERT_EQ(pMtl->numGroups(), 1);

	auto g1 = pMtl->group(0);
	auto g2 = pMtl->addGroup(1);
	auto g0 = pMtl->addGroup(0);
	auto g3 = pMtl->addGroup(3);
	ASSERT_EQ(pMtl->numGroups(), 4);

	ASSERT_NE(g0, nullptr);
	ASSERT_NE(g1, nullptr);
	ASSERT_NE(g2, nullptr);
	ASSERT_NE(g3, nullptr);

	ASSERT_NE(g0, g1);
	ASSERT_NE(g0, g2);
	ASSERT_NE(g0, g3);
	ASSERT_NE(g1, g2);
	ASSERT_NE(g1, g3);
	ASSERT_NE(g2, g3);

	EXPECT_EQ(g0, pMtl->group(0));
	EXPECT_EQ(g1, pMtl->group(1));
	EXPECT_EQ(g2, pMtl->group(2));
	EXPECT_EQ(g3, pMtl->group(3));

	pMtl->moveGroup(1, 1);
	EXPECT_EQ(g0, pMtl->group(0));
	EXPECT_EQ(g1, pMtl->group(1));
	EXPECT_EQ(g2, pMtl->group(2));
	EXPECT_EQ(g3, pMtl->group(3));

	pMtl->moveGroup(0, 1);
	EXPECT_EQ(g1, pMtl->group(0));
	EXPECT_EQ(g0, pMtl->group(1));
	EXPECT_EQ(g2, pMtl->group(2));
	EXPECT_EQ(g3, pMtl->group(3));

	pMtl->moveGroup(1, 2);
	EXPECT_EQ(g1, pMtl->group(0));
	EXPECT_EQ(g2, pMtl->group(1));
	EXPECT_EQ(g0, pMtl->group(2));
	EXPECT_EQ(g3, pMtl->group(3));

	pMtl->moveGroup(2, 0);
	EXPECT_EQ(g0, pMtl->group(0));
	EXPECT_EQ(g1, pMtl->group(1));
	EXPECT_EQ(g2, pMtl->group(2));
	EXPECT_EQ(g3, pMtl->group(3));

	pMtl->moveGroup(0, 3);
	EXPECT_EQ(g1, pMtl->group(0));
	EXPECT_EQ(g2, pMtl->group(1));
	EXPECT_EQ(g3, pMtl->group(2));
	EXPECT_EQ(g0, pMtl->group(3));

	pMtl->moveGroup(3, 0);
	EXPECT_EQ(g0, pMtl->group(0));
	EXPECT_EQ(g1, pMtl->group(1));
	EXPECT_EQ(g2, pMtl->group(2));
	EXPECT_EQ(g3, pMtl->group(3));

	pMtl->moveGroup(0, 4); // invalid paremeter
	EXPECT_EQ(g0, pMtl->group(0));
	EXPECT_EQ(g1, pMtl->group(1));
	EXPECT_EQ(g2, pMtl->group(2));
	EXPECT_EQ(g3, pMtl->group(3));

	pMtl->removeGroup(4); // invalid paremeter
	ASSERT_EQ(pMtl->numGroups(), 4);

	pMtl->removeGroup(2);
	ASSERT_EQ(pMtl->numGroups(), 3);
	EXPECT_EQ(g0, pMtl->group(0));
	EXPECT_EQ(g1, pMtl->group(1));
	EXPECT_EQ(g3, pMtl->group(2));

	pMtl->removeGroup(2);
	ASSERT_EQ(pMtl->numGroups(), 2);
	EXPECT_EQ(g0, pMtl->group(0));
	EXPECT_EQ(g1, pMtl->group(1));

	pMtl->moveGroup(0, 2); // invalid paremeter
	EXPECT_EQ(g0, pMtl->group(0));
	EXPECT_EQ(g1, pMtl->group(1));

	pMtl->moveGroup(1, 2); // invalid paremeter
	EXPECT_EQ(g0, pMtl->group(0));
	EXPECT_EQ(g1, pMtl->group(1));
}

TEST_F(TestMaterials, CheckLayersManagement)
{
	auto pMtl = matmanager()->create(QString("test"));
	ASSERT_EQ(pMtl->numGroups(), 1);
	auto pGrp = pMtl->group(0);
	ASSERT_NE(pGrp, nullptr);

	ASSERT_EQ(nullptr, pGrp->addLayer(2, QByteArray(), ModelLoadingContext())); // invalid paremeter
	ASSERT_EQ(pGrp->numLayers(), 1);

	auto l1 = pGrp->layer(0);
	auto l2 = pGrp->addLayer(1, QByteArray(), ModelLoadingContext());
	auto l0 = pGrp->addLayer(0, QByteArray(), ModelLoadingContext());
	auto l3 = pGrp->addLayer(3, QByteArray(), ModelLoadingContext());
	ASSERT_EQ(pGrp->numLayers(), 4);

	ASSERT_NE(l0, nullptr);
	ASSERT_NE(l1, nullptr);
	ASSERT_NE(l2, nullptr);
	ASSERT_NE(l3, nullptr);

	ASSERT_NE(l0, l1);
	ASSERT_NE(l0, l2);
	ASSERT_NE(l0, l3);
	ASSERT_NE(l1, l2);
	ASSERT_NE(l1, l3);
	ASSERT_NE(l2, l3);

	EXPECT_EQ(l0, pGrp->layer(0));
	EXPECT_EQ(l1, pGrp->layer(1));
	EXPECT_EQ(l2, pGrp->layer(2));
	EXPECT_EQ(l3, pGrp->layer(3));

	pGrp->moveLayer(1, 1);
	EXPECT_EQ(l0, pGrp->layer(0));
	EXPECT_EQ(l1, pGrp->layer(1));
	EXPECT_EQ(l2, pGrp->layer(2));
	EXPECT_EQ(l3, pGrp->layer(3));

	pGrp->moveLayer(0, 1);
	EXPECT_EQ(l1, pGrp->layer(0));
	EXPECT_EQ(l0, pGrp->layer(1));
	EXPECT_EQ(l2, pGrp->layer(2));
	EXPECT_EQ(l3, pGrp->layer(3));

	pGrp->moveLayer(1, 2);
	EXPECT_EQ(l1, pGrp->layer(0));
	EXPECT_EQ(l2, pGrp->layer(1));
	EXPECT_EQ(l0, pGrp->layer(2));
	EXPECT_EQ(l3, pGrp->layer(3));

	pGrp->moveLayer(2, 0);
	EXPECT_EQ(l0, pGrp->layer(0));
	EXPECT_EQ(l1, pGrp->layer(1));
	EXPECT_EQ(l2, pGrp->layer(2));
	EXPECT_EQ(l3, pGrp->layer(3));

	pGrp->moveLayer(0, 3);
	EXPECT_EQ(l1, pGrp->layer(0));
	EXPECT_EQ(l2, pGrp->layer(1));
	EXPECT_EQ(l3, pGrp->layer(2));
	EXPECT_EQ(l0, pGrp->layer(3));

	pGrp->moveLayer(3, 0);
	EXPECT_EQ(l0, pGrp->layer(0));
	EXPECT_EQ(l1, pGrp->layer(1));
	EXPECT_EQ(l2, pGrp->layer(2));
	EXPECT_EQ(l3, pGrp->layer(3));

	pGrp->moveLayer(0, 4); // invalid paremeter
	EXPECT_EQ(l0, pGrp->layer(0));
	EXPECT_EQ(l1, pGrp->layer(1));
	EXPECT_EQ(l2, pGrp->layer(2));
	EXPECT_EQ(l3, pGrp->layer(3));

	pGrp->removeLayer(4); // invalid paremeter
	ASSERT_EQ(pGrp->numLayers(), 4);

	pGrp->removeLayer(2);
	ASSERT_EQ(pGrp->numLayers(), 3);
	EXPECT_EQ(l0, pGrp->layer(0));
	EXPECT_EQ(l1, pGrp->layer(1));
	EXPECT_EQ(l3, pGrp->layer(2));

	pGrp->removeLayer(2);
	ASSERT_EQ(pGrp->numLayers(), 2);
	EXPECT_EQ(l0, pGrp->layer(0));
	EXPECT_EQ(l1, pGrp->layer(1));

	pGrp->moveLayer(0, 2); // invalid paremeter
	EXPECT_EQ(l0, pGrp->layer(0));
	EXPECT_EQ(l1, pGrp->layer(1));

	pGrp->moveLayer(1, 2); // invalid paremeter
	EXPECT_EQ(l0, pGrp->layer(0));
	EXPECT_EQ(l1, pGrp->layer(1));
}

TEST_F(TestMaterials, CheckGroupsName)
{
	auto pMtl = matmanager()->create(QString("test"));

	pMtl->addGroup(0);
	ASSERT_EQ(pMtl->numGroups(), 2);
	pMtl->group(0)->name().set("gn1");
	pMtl->group(1)->name().set("gn2");

	EXPECT_STREQ("gn1", pMtl->group(0)->name().get().toLocal8Bit());
	EXPECT_STREQ("gn2", pMtl->group(1)->name().get().toLocal8Bit());
}

TEST_F(TestMaterials, CheckLayersName)
{
	auto pMtl = matmanager()->create(QString("test"));
	auto pGrp = pMtl->group(0);

	pGrp->addLayer(0, QByteArray(), ModelLoadingContext());
	ASSERT_EQ(pGrp->numLayers(), 2);
	pGrp->layer(0)->name().set("ln1");
	pGrp->layer(1)->name().set("ln2");

	EXPECT_STREQ("ln1", pGrp->layer(0)->name().get().toLocal8Bit());
	EXPECT_STREQ("ln2", pGrp->layer(1)->name().get().toLocal8Bit());
}

TEST_F(TestMaterials, CheckSaveLoad)
{
	glm::vec3 c1(0.11f, 0.23f, 0.123f);
	glm::vec3 c2(0.22f, 0.34f, 0.234f);
	glm::vec3 c3(0.33f, 0.45f, 0.345f);
	glm::vec3 c4(0.44f, 0.56f, 0.456f);
	glm::vec3 c5(0.55f, 0.67f, 0.567f);
	glm::vec3 c6(0.66f, 0.78f, 0.678f);
	glm::vec3 c7(0.77f, 0.89f, 0.789f);
	glm::vec3 csss(0.88f, 0.91f, 0.981f);
	auto f1 = 2.345f;
	auto f2 = 0.3456f;
	auto f3 = 0.987f;
	auto f4 = 0.1256f;
	auto f5 = 0.5432f;

	auto pMtl1 = matmanager()->create(QString("test1"));
	pMtl1->absorptionColor().set(c1);
	pMtl1->absorptionAttenuation().set(f1);

	pMtl1->subsurfaceScattering().set(true);
	pMtl1->scatteringColor().set(csss);
	pMtl1->scatteringScale().set(4.56f);
	pMtl1->scatteringAsymmetry().set(0.543f);

	auto g1 = pMtl1->group(0);
	auto g2 = pMtl1->addGroup(1);
	ASSERT_EQ(pMtl1->numGroups(), 2);
	g1->name().set("g1");
	g2->name().set("g2");
	g1->mask().set(f2);

	auto g1l1 = g1->layer(0);
	auto g1l2 = g1->addLayer(1, QByteArray(), ModelLoadingContext());
	auto g2l1 = g2->layer(0);
	ASSERT_EQ(g1->numLayers(), 2);
	ASSERT_EQ(g2->numLayers(), 1);

	g1l1->name().set("g1l1");
	g1l2->name().set("g1l2");
	g2l1->name().set("g2l1");

	g1l1->specularLayer().set(true);
	g1l1->indexOfRefraction().type().setIndex((int)IORType::COMPLEX);
	g1l1->indexOfRefraction().fileName().set(":/test-ior1.txt");
	g1l1->indexOfRefraction().n().set(2.1f);
	g1l1->indexOfRefraction().k().set(1.2f);
	g1l1->mask().set(f5);
	g1l1->reflection().set(c6);
	g1l1->transmission().set(c7);
	g1l1->reflection90().set(c3);
	g1l1->reflection90Level().set(f3);

	g1l1->thinFilmInterference().set(true);
	g1l1->filmIndexOfRefraction().fileName().set(":/test-ior3.txt");
	g1l1->filmIndexOfRefraction().type().setIndex((int)IORType::COMPLEX);
	g1l1->filmIndexOfRefraction().n().set(2.123f);
	g1l1->filmIndexOfRefraction().k().set(1.234f);

	g1l2->specularLayer().set(true);
	g1l2->indexOfRefraction().type().setIndex((int)IORType::MEASURED);
	g1l2->indexOfRefraction().fileName().set(":/test-ior2.txt");
	g1l2->indexOfRefraction().n().set(3.2f);
	g1l2->indexOfRefraction().k().set(2.3f);
	g1l2->reflection90().set(c4);
	g1l2->reflection90Level().set(f4);

	auto data = pMtl1->save(ModelSavingContext());

	auto pMtl2 = matmanager()->load(data, ModelLoadingContext(), matmanager()->count());
	ASSERT_NE(pMtl2, nullptr);
	EXPECT_STREQ("test1 1", pMtl2->name().get().toUtf8());

	EXPECT_EQ(c1, pMtl2->absorptionColor().get());
	EXPECT_EQ(f1, pMtl2->absorptionAttenuation().get());

	EXPECT_EQ(true, pMtl2->subsurfaceScattering().get());
	EXPECT_EQ(csss, pMtl2->scatteringColor().get());
	EXPECT_EQ(4.56f, pMtl2->scatteringScale().get());
	EXPECT_EQ(0.543f, pMtl2->scatteringAsymmetry().get());

	EXPECT_STREQ("g1", pMtl2->group(0)->name().get().toLocal8Bit());
	EXPECT_STREQ("g2", pMtl2->group(1)->name().get().toLocal8Bit());

	auto g3 = pMtl2->group(0);
	EXPECT_EQ(f2, g3->mask().get());

	ASSERT_EQ(pMtl2->group(0)->numLayers(), 2);
	ASSERT_EQ(pMtl2->group(1)->numLayers(), 1);
	auto l1 = pMtl2->group(0)->layer(0);
	auto l2 = pMtl2->group(0)->layer(1);

	EXPECT_STREQ("g1l1", l1->name().get().toLocal8Bit());
	EXPECT_STREQ("g1l2", l2->name().get().toLocal8Bit());
	EXPECT_STREQ("g2l1", pMtl2->group(1)->layer(0)->name().get().toLocal8Bit());

	// group 1 layer 1
	#ifdef Q_OS_WIN
	EXPECT_STREQ(":\\test-ior1.txt", l1->indexOfRefraction().fileName().get().toLocal8Bit());
	#else
	EXPECT_STREQ(":/test-ior1.txt", l1->indexOfRefraction().fileName().get().toLocal8Bit());
	#endif

	EXPECT_EQ((int)IORType::COMPLEX, l1->indexOfRefraction().type().getIndex());
	EXPECT_EQ(2.1f, l1->indexOfRefraction().n().get());
	EXPECT_EQ(1.2f, l1->indexOfRefraction().k().get());
	EXPECT_EQ(f5, l1->mask().get());
	EXPECT_EQ(c6, l1->reflection().get());
	EXPECT_EQ(c7, l1->transmission().get());
	EXPECT_EQ(c3, l1->reflection90().get());
	EXPECT_FLOAT_EQ(f3, l1->reflection90Level().get());

	EXPECT_EQ(true, l1->thinFilmInterference().get());

	#ifdef Q_OS_WIN
	EXPECT_STREQ(":\\test-ior3.txt", l1->filmIndexOfRefraction().fileName().get().toLocal8Bit());
	#else
	EXPECT_STREQ(":/test-ior3.txt", l1->filmIndexOfRefraction().fileName().get().toLocal8Bit());
	#endif

	EXPECT_EQ((int)IORType::COMPLEX, l1->filmIndexOfRefraction().type().getIndex());
	EXPECT_EQ(2.123f, l1->filmIndexOfRefraction().n().get());
	EXPECT_EQ(1.234f, l1->filmIndexOfRefraction().k().get());

	// group 1 layer 2
	#ifdef Q_OS_WIN
	EXPECT_STREQ(":\\test-ior2.txt", l2->indexOfRefraction().fileName().get().toLocal8Bit());
	#else
	EXPECT_STREQ(":/test-ior2.txt", l2->indexOfRefraction().fileName().get().toLocal8Bit());
	#endif

	EXPECT_EQ((int)IORType::MEASURED, l2->indexOfRefraction().type().getIndex());
	EXPECT_EQ(3.2f, l2->indexOfRefraction().n().get());
	EXPECT_EQ(2.3f, l2->indexOfRefraction().k().get());
	EXPECT_EQ(c4, l2->reflection90().get());
	EXPECT_EQ(f4, l2->reflection90Level().get());
	EXPECT_EQ(false, l2->thinFilmInterference().get());
}

TEST_F(TestMaterials, CheckLayerSave)
{
	auto pMtl1 = matmanager()->create(QString("test1"));
	auto group = pMtl1->group(0);
	group->layer(0)->name().set("test layer");

	auto data = group->layer(0)->save(ModelSavingContext());

	group->removeLayer(0);
	ASSERT_EQ(group->numLayers(), 0);

	group->addLayer(0, data, ModelLoadingContext());
	ASSERT_EQ(group->numLayers(), 1);

	EXPECT_STREQ("test layer", group->layer(0)->name().get().toLocal8Bit());
}

TEST_F(TestMaterials, CheckMoveLayer)
{
	auto pMtl = matmanager()->create(QString("test1"));
	auto g1 = pMtl->group(0);
	auto g2 = pMtl->addGroup(1);
	ASSERT_EQ(pMtl->numGroups(), 2);
	ASSERT_NE(g1, g2);

	auto g1l1 = g1->layer(0);
	auto g1l2 = g1->addLayer(1, QByteArray(), ModelLoadingContext());
	auto g2l1 = g2->layer(0);
	auto g2l2 = g2->addLayer(1, QByteArray(), ModelLoadingContext());

	ASSERT_EQ(g1->numLayers(), 2);
	ASSERT_EQ(g2->numLayers(), 2);
	ASSERT_EQ(g1l1, g1->layer(0));
	ASSERT_EQ(g1l2, g1->layer(1));
	ASSERT_EQ(g2l1, g2->layer(0));
	ASSERT_EQ(g2l2, g2->layer(1));

	pMtl->moveLayer(0, 0, 1, 1);

	ASSERT_EQ(g1->numLayers(), 1);
	ASSERT_EQ(g2->numLayers(), 3);
	ASSERT_EQ(g1l2, g1->layer(0));
	ASSERT_EQ(g2l1, g2->layer(0));
	ASSERT_EQ(g1l1, g2->layer(1));
	ASSERT_EQ(g2l2, g2->layer(2));

	scene()->undoStack().undo();

	ASSERT_EQ(g1->numLayers(), 2);
	ASSERT_EQ(g2->numLayers(), 2);
	ASSERT_EQ(g1l1, g1->layer(0));
	ASSERT_EQ(g1l2, g1->layer(1));
	ASSERT_EQ(g2l1, g2->layer(0));
	ASSERT_EQ(g2l2, g2->layer(1));

	pMtl->moveLayer(0, 0, 0, 1);

	ASSERT_EQ(g1->numLayers(), 2);
	ASSERT_EQ(g1l2, g1->layer(0));
	ASSERT_EQ(g1l1, g1->layer(1));

	scene()->undoStack().undo();

	ASSERT_EQ(g1->numLayers(), 2);
	ASSERT_EQ(g1l1, g1->layer(0));
	ASSERT_EQ(g1l2, g1->layer(1));

	pMtl->moveLayer(0, 0, 1, 2);

	ASSERT_EQ(g1->numLayers(), 1);
	ASSERT_EQ(g2->numLayers(), 3);
	ASSERT_EQ(g1l2, g1->layer(0));
	ASSERT_EQ(g2l1, g2->layer(0));
	ASSERT_EQ(g2l2, g2->layer(1));
	ASSERT_EQ(g1l1, g2->layer(2));

	scene()->undoStack().undo();

	ASSERT_EQ(g1->numLayers(), 2);
	ASSERT_EQ(g2->numLayers(), 2);
	ASSERT_EQ(g1l1, g1->layer(0));
	ASSERT_EQ(g1l2, g1->layer(1));
	ASSERT_EQ(g2l1, g2->layer(0));
	ASSERT_EQ(g2l2, g2->layer(1));

	pMtl->moveLayer(0, 0, 0, 0); // nothing changed

	ASSERT_EQ(g1->numLayers(), 2);
	ASSERT_EQ(g2->numLayers(), 2);
	ASSERT_EQ(g1l1, g1->layer(0));
	ASSERT_EQ(g1l2, g1->layer(1));
	ASSERT_EQ(g2l1, g2->layer(0));
	ASSERT_EQ(g2l2, g2->layer(1));

	pMtl->moveLayer(2, 0, 0, 0); // invalid group from

	ASSERT_EQ(g1->numLayers(), 2);
	ASSERT_EQ(g2->numLayers(), 2);
	ASSERT_EQ(g1l1, g1->layer(0));
	ASSERT_EQ(g1l2, g1->layer(1));
	ASSERT_EQ(g2l1, g2->layer(0));
	ASSERT_EQ(g2l2, g2->layer(1));

	pMtl->moveLayer(0, 0, 2, 0); // invalid group to

	ASSERT_EQ(g1->numLayers(), 2);
	ASSERT_EQ(g2->numLayers(), 2);
	ASSERT_EQ(g1l1, g1->layer(0));
	ASSERT_EQ(g1l2, g1->layer(1));
	ASSERT_EQ(g2l1, g2->layer(0));
	ASSERT_EQ(g2l2, g2->layer(1));

	pMtl->moveLayer(0, 2, 1, 0); // invalid layer from

	ASSERT_EQ(g1->numLayers(), 2);
	ASSERT_EQ(g2->numLayers(), 2);
	ASSERT_EQ(g1l1, g1->layer(0));
	ASSERT_EQ(g1l2, g1->layer(1));
	ASSERT_EQ(g2l1, g2->layer(0));
	ASSERT_EQ(g2l2, g2->layer(1));

	pMtl->moveLayer(0, 0, 1, 3); // invalid layer to

	ASSERT_EQ(g1->numLayers(), 2);
	ASSERT_EQ(g2->numLayers(), 2);
	ASSERT_EQ(g1l1, g1->layer(0));
	ASSERT_EQ(g1l2, g1->layer(1));
	ASSERT_EQ(g2l1, g2->layer(0));
	ASSERT_EQ(g2l2, g2->layer(1));
}

TEST_F(TestMaterials, CheckGuid)
{
	auto mtl = matmanager()->create(QString("test1"));
	auto guid1 = mtl->guid();
	EXPECT_FALSE(guid1.isEmpty());
	mtl->group(0)->mask().set(0.5f);
	auto guid2 = mtl->guid();
	EXPECT_EQ(guid1, guid2);
	mtl->group(0)->layer(0)->reflection().set(glm::vec3(1.f, 0.5f, 0.f));
	auto guid3 = mtl->guid();
	EXPECT_EQ(guid1, guid3);

	scene()->undoStack().undo();
	EXPECT_EQ(mtl->guid(), guid1);
	scene()->undoStack().undo();
	EXPECT_EQ(mtl->guid(), guid1);
	scene()->undoStack().redo();
	EXPECT_EQ(mtl->guid(), guid1);
	scene()->undoStack().redo();
	EXPECT_EQ(mtl->guid(), guid1);

	auto data = mtl->save(ModelSavingContext());

	auto mtl2 = matmanager()->load(data, ModelLoadingContext(), matmanager()->count());
	ASSERT_NE(mtl2, nullptr);
	EXPECT_NE(mtl2->guid(), guid1);
	EXPECT_FALSE(mtl2->guid().isEmpty());
}

TEST_F(TestMaterials, CheckTextureSaveLoad)
{
	app()->startPlugins();
	auto material = matmanager()->create(QString(":/"));
	auto layer = material->group(0)->layer(0);
	ASSERT_NE(layer, nullptr);

	auto tex1 = layer->reflection().texture();
	auto tex2 = layer->transmission().texture();

	tex1->enabled().set(false);
#ifdef Q_OS_WIN
	tex1->fileName().set(":\\4x4.hdr");
#else
	tex1->fileName().set(":/4x4.hdr");
#endif
	tex1->cropLeft().set(0.123f);
	tex1->cropTop().set(0.234f);
	tex1->cropRight().set(0.678f);
	tex1->cropBottom().set(0.789f);
	tex1->wrapX().setIndex(1);
	tex1->wrapY().setIndex(2);
	tex1->repeat().set(vec2(1.234f, 4.567f));
	tex1->offset().set(vec2(0.345f, 0.456f));
	tex1->rotation().set(2.345f);
	tex1->invert().set(true);
	auto data = tex1->save(ModelSavingContext());

	ASSERT_NE(tex1->enabled().get(), tex2->enabled().get());
	ASSERT_NE(tex1->fileName().get(), tex2->fileName().get());
	ASSERT_NE(tex1->cropLeft().get(), tex2->cropLeft().get());
	ASSERT_NE(tex1->cropTop().get(), tex2->cropTop().get());
	ASSERT_NE(tex1->cropRight().get(), tex2->cropRight().get());
	ASSERT_NE(tex1->cropBottom().get(), tex2->cropBottom().get());
	ASSERT_NE(tex1->wrapX().getIndex(), tex2->wrapX().getIndex());
	ASSERT_NE(tex1->wrapY().getIndex(), tex2->wrapY().getIndex());
	ASSERT_NE(tex1->repeat().get(), tex2->repeat().get());
	ASSERT_NE(tex1->offset().get(), tex2->offset().get());
	ASSERT_NE(tex1->rotation().get(), tex2->rotation().get());
	ASSERT_NE(tex1->invert().get(), tex2->invert().get());

	tex2->load(data, ModelLoadingContext());

	ASSERT_EQ(tex1->enabled().get(), tex2->enabled().get());
	ASSERT_EQ(tex1->fileName().get(), tex2->fileName().get());
	ASSERT_EQ(tex1->cropLeft().get(), tex2->cropLeft().get());
	ASSERT_EQ(tex1->cropTop().get(), tex2->cropTop().get());
	ASSERT_EQ(tex1->cropRight().get(), tex2->cropRight().get());
	ASSERT_EQ(tex1->cropBottom().get(), tex2->cropBottom().get());
	ASSERT_EQ(tex1->wrapX().getIndex(), tex2->wrapX().getIndex());
	ASSERT_EQ(tex1->wrapY().getIndex(), tex2->wrapY().getIndex());
	ASSERT_EQ(tex1->repeat().get(), tex2->repeat().get());
	ASSERT_EQ(tex1->offset().get(), tex2->offset().get());
	ASSERT_EQ(tex1->rotation().get(), tex2->rotation().get());
	ASSERT_EQ(tex1->invert().get(), tex2->invert().get());
}

TEST_F(TestMaterials, CheckTexParamSaveLoad)
{
	auto material = matmanager()->create(QString(":/"));
	auto layer1 = material->group(0)->layer(0);
	ASSERT_NE(layer1, nullptr);

	auto & param1 = layer1->mask();
	EXPECT_EQ(true, param1.texture()->enabled().get());
	param1.texture()->enabled().set(false);
	EXPECT_EQ(false, param1.texture()->enabled().get());

	auto data = layer1->save(ModelSavingContext());

	auto layer2 = material->group(0)->addLayer(1, data, ModelLoadingContext());
	auto layer3 = material->group(0)->addLayer(2, QByteArray(), ModelLoadingContext());
	ASSERT_NE(nullptr, layer2);
	ASSERT_NE(nullptr, layer3);
	ASSERT_NE(layer1, layer2);
	ASSERT_NE(layer1, layer3);
	ASSERT_NE(layer2, layer3);

	EXPECT_EQ(false, layer2->mask().texture()->enabled().get());
	EXPECT_EQ(true, layer3->mask().texture()->enabled().get());
}

TEST_F(TestMaterials, CheckTexturePathSaveLoad)
{
	app()->startPlugins();
	auto material = matmanager()->create(QString(":/"));
	auto layer = material->group(0)->layer(0);
	ASSERT_NE(layer, nullptr);

	auto tex = layer->diffuseColor().texture();

#ifdef Q_OS_WIN
	const char *folder = "c:\\test\\path";
	const char *image1 = "c:\\test\\path\\image1.png";
	const char *image2 = "c:\\test\\path\\local\\image2.png";
	const char *image3 = "c:\\test\\image3.png";
	const char *res1 = "\"file\": \"image1.png\"";
	const char *res2 = "\"file\": \"local\\\\image2.png\"";
	const char *res3 = "\"file\": \"C:\\\\test\\\\image3.png\"";
#else
	const char *folder = "/test/path";
	const char *image1 = "/test/path/image1.png";
	const char *image2 = "/test/path/local/image2.png";
	const char *image3 = "/test/image3.png";
	const char *res1 = "\"file\": \"image1.png\"";
	const char *res2 = "\"file\": \"local/image2.png\"";
	const char *res3 = "\"file\": \"/test/image3.png\"";
#endif

	ModelSavingContext ctx;
	ctx.targetFolder = folder;

	tex->fileName().set(image1);
	auto data1 = QString::fromLocal8Bit(material->save(ctx));

	EXPECT_NE(-1, data1.indexOf(res1));

	tex->fileName().set(image2);
	auto data2 = QString::fromLocal8Bit(material->save(ctx));
	EXPECT_NE(-1, data2.indexOf(res2));

	tex->fileName().set(image3);
	auto data3 = QString::fromLocal8Bit(material->save(ctx));
	EXPECT_NE(-1, data3.indexOf(res3));
}

TEST_F(TestMaterials, ReplaceMaterialWithSavedOne)
{
	auto pMtl1 = matmanager()->create(QString("new one"));
	auto layer = pMtl1->group(0)->layer(0);
	layer->name().set("my cool name");
	layer->mask().set(25.f);

	ModelSavingContext ctx_s;
	auto data = pMtl1->save(ctx_s);

	matmanager()->remove({ pMtl1 });

	ASSERT_EQ(0, matmanager()->count());

	auto pMtl2 = matmanager()->create(QString("to be replaced"));
	auto layer2 = pMtl2->group(0)->layer(0);
	layer2->name().set("tmp name");
	layer2->mask().set(34.f);

	ModelLoadingContext ctx_l;
	ctx_l.loadNewScene = true;
	matmanager()->replaceWithSerializedOne(pMtl2, data, ctx_l);

	ASSERT_EQ(1, matmanager()->count());

	EXPECT_EQ("to be replaced", pMtl2->name().get());

	auto layer3 = pMtl2->group(0)->layer(0);
	EXPECT_EQ("my cool name", layer3->name().get());
	EXPECT_FLOAT_EQ(25.f, layer3->mask().get());

	scene()->undoStack().undo();

	EXPECT_EQ("to be replaced", pMtl2->name().get());

	auto layer4 = pMtl2->group(0)->layer(0);
	EXPECT_EQ("tmp name", layer4->name().get());
	EXPECT_FLOAT_EQ(34.f, layer4->mask().get());
}

TEST_F(TestMaterials, CheckSignalsSentOnMaterialChanges)
{
	auto pMtl = matmanager()->create(QString("xxx"));

	MaterialEventsHelper eh;
	QObject::connect(matmanager(), SIGNAL(materialChanged(const IMaterial*, bool)), &eh, SLOT(onAnyMaterialChanged(const IMaterial*)));
	QObject::connect(pMtl, SIGNAL(changed()), &eh, SLOT(onSpecificMaterialChanged()));

	IMaterialGroup *pGroup = pMtl->group(0);
	IMaterialLayer *pLayer = pGroup->layer(0);

	// test 1: changing of general layer property makes notification from material manager, but not the material itself
	eh.m_events.clear();
	pLayer->mask().set(10);
	ASSERT_EQ(1, eh.m_events.size());
	ASSERT_EQ(true, eh.m_events[0].any);
	ASSERT_EQ(pMtl, eh.m_events[0].material);

	// test 2: changing of layer name property makes notification from both material itself and material manager
	eh.m_events.clear();
	pLayer->name().set("test");
	ASSERT_EQ(2, eh.m_events.size());
	ASSERT_EQ(false, eh.m_events[0].any);
	ASSERT_EQ(pMtl, eh.m_events[0].material);
	ASSERT_EQ(true, eh.m_events[1].any);
	ASSERT_EQ(pMtl, eh.m_events[1].material);

	// test 3: changing of layer visibility makes notification from both material itself and material manager
	eh.m_events.clear();
	pLayer->enabled().set(false);
	ASSERT_EQ(2, eh.m_events.size());
	ASSERT_EQ(false, eh.m_events[0].any);
	ASSERT_EQ(pMtl, eh.m_events[0].material);
	ASSERT_EQ(true, eh.m_events[1].any);
	ASSERT_EQ(pMtl, eh.m_events[1].material);

	// test 4: adding layer makes notification from both material itself and material manager
	eh.m_events.clear();
	pGroup->addLayer(1, QByteArray(), ModelLoadingContext());
	ASSERT_EQ(2, eh.m_events.size());
	ASSERT_EQ(false, eh.m_events[0].any);
	ASSERT_EQ(pMtl, eh.m_events[0].material);
	ASSERT_EQ(true, eh.m_events[1].any);
	ASSERT_EQ(pMtl, eh.m_events[1].material);

	// test 5: deleting layer makes notification from both material itself and material manager
	eh.m_events.clear();
	pGroup->removeLayer(1);
	ASSERT_EQ(2, eh.m_events.size());
	ASSERT_EQ(false, eh.m_events[0].any);
	ASSERT_EQ(pMtl, eh.m_events[0].material);
	ASSERT_EQ(true, eh.m_events[1].any);
	ASSERT_EQ(pMtl, eh.m_events[1].material);

	// test 6: changing of general group property makes notification from material manager, but not the material itself
	eh.m_events.clear();
	pGroup->mask().set(10);
	ASSERT_EQ(1, eh.m_events.size());
	ASSERT_EQ(true, eh.m_events[0].any);
	ASSERT_EQ(pMtl, eh.m_events[0].material);

	// test 7: changing of group name property makes notification from both material itself and material manager
	eh.m_events.clear();
	pGroup->name().set("test");
	ASSERT_EQ(2, eh.m_events.size());
	ASSERT_EQ(false, eh.m_events[0].any);
	ASSERT_EQ(pMtl, eh.m_events[0].material);
	ASSERT_EQ(true, eh.m_events[1].any);
	ASSERT_EQ(pMtl, eh.m_events[1].material);

	// test 8: changing of group visibility makes notification from both material itself and material manager
	eh.m_events.clear();
	pGroup->enabled().set(false);
	ASSERT_EQ(2, eh.m_events.size());
	ASSERT_EQ(false, eh.m_events[0].any);
	ASSERT_EQ(pMtl, eh.m_events[0].material);
	ASSERT_EQ(true, eh.m_events[1].any);
	ASSERT_EQ(pMtl, eh.m_events[1].material);

	// test 9: adding group makes notification from both material itself and material manager
	eh.m_events.clear();
	pMtl->addGroup(1);
	ASSERT_EQ(2, eh.m_events.size());
	ASSERT_EQ(false, eh.m_events[0].any);
	ASSERT_EQ(pMtl, eh.m_events[0].material);
	ASSERT_EQ(true, eh.m_events[1].any);
	ASSERT_EQ(pMtl, eh.m_events[1].material);

	// test 10: deleting group makes notification from both material itself and material manager
	eh.m_events.clear();
	pMtl->removeGroup(1);
	ASSERT_EQ(2, eh.m_events.size());
	ASSERT_EQ(false, eh.m_events[0].any);
	ASSERT_EQ(pMtl, eh.m_events[0].material);
	ASSERT_EQ(true, eh.m_events[1].any);
	ASSERT_EQ(pMtl, eh.m_events[1].material);

	// test 11: changing of material name property makes notification from both material itself and material manager
	eh.m_events.clear();
	pMtl->name().set("test");
	ASSERT_EQ(2, eh.m_events.size());
	ASSERT_EQ(false, eh.m_events[0].any);
	ASSERT_EQ(pMtl, eh.m_events[0].material);
	ASSERT_EQ(true, eh.m_events[1].any);
	ASSERT_EQ(pMtl, eh.m_events[1].material);

	// test 12: changing of general material property makes notification from material manager, but not the material itself
	eh.m_events.clear();
	pMtl->bump().set(10);
	ASSERT_EQ(1, eh.m_events.size());
	ASSERT_EQ(true, eh.m_events[0].any);
	ASSERT_EQ(pMtl, eh.m_events[0].material);

}

TEST_F(TestMaterials, CheckInvalidGroupAndLayerIndices)
{
	auto pMtl = matmanager()->create(QString("test"));

	ASSERT_EQ(1, pMtl->numGroups());
	auto pGrp = pMtl->group(0);
	ASSERT_EQ(1, pGrp->numLayers());

	EXPECT_NE(nullptr, pGrp);
	EXPECT_NO_THROW(pMtl->group(1));
	EXPECT_EQ(nullptr, pMtl->group(2));

	EXPECT_NE(nullptr, pGrp->layer(0));
	EXPECT_NO_THROW(pGrp->layer(1));
	EXPECT_EQ(nullptr, pGrp->layer(1));
}

TEST_F(TestMaterials, JsonSaveAndLoad)
{
	auto pMtl1 = matmanager()->create(QString("Material1"));
	auto pMtlImpl1 = static_cast<MaterialImpl *>(pMtl1);

	// Configure some properties on pMtl1
	pMtlImpl1->name().set(QString("CustomMaterialName"));
	pMtlImpl1->doubleSided().set(false);
	pMtlImpl1->priority().set(42);
	pMtlImpl1->bump().set(12.5f);
	pMtlImpl1->bump().texture()->_set(true, QString("bump_texture.png"));

	auto group1 = pMtlImpl1->group(0);
	group1->name().set(QString("CustomGroup"));
	group1->mask().set(75.f);

	auto layer1 = group1->layer(0);
	layer1->name().set(QString("CustomLayer"));
	layer1->diffuseLayer().set(true);
	layer1->diffuseColor().set(vec3(0.1f, 0.2f, 0.3f));
	layer1->diffuseColor().texture()->_set(true, QString("diffuse_texture.png"));
	layer1->diffuseColor().texture()->brightness().set(0.5f);

	layer1->specularLayer().set(true);
	layer1->indexOfRefraction().n().set(1.8f);
	layer1->roughness().set(15.f);
	layer1->thinFilmInterference().set(true);
	layer1->thickness().set(200.f);

	layer1->emissiveLayer().set(true);
	layer1->emissiveColor().set(vec3(0.9f, 0.8f, 0.7f));
	layer1->emissiveIntensity().set(150.f);

	// Save to JSON
	QJsonObject obj;
	pMtlImpl1->_save(obj, ModelSavingContext());

	// Remove pMtl1 so that the name "CustomMaterialName" is available and doesn't get auto-renamed to "CustomMaterialName 1"
	matmanager()->remove({pMtl1});

	// Create pMtl2 and load from JSON
	auto pMtl2 = matmanager()->create(QString("Material2"));
	auto pMtlImpl2 = static_cast<MaterialImpl *>(pMtl2);
	bool res = pMtlImpl2->_load(obj, ModelLoadingContext());

	ASSERT_TRUE(res);

	// Verify loaded properties on pMtl2
	EXPECT_EQ(QString("CustomMaterialName"), pMtlImpl2->name().get());
	EXPECT_FALSE(pMtlImpl2->doubleSided().get());
	EXPECT_EQ(42, pMtlImpl2->priority().get());
	EXPECT_FLOAT_EQ(12.5f, pMtlImpl2->bump().get());
	EXPECT_TRUE(pMtlImpl2->bump().texture()->enabled().get());
	EXPECT_EQ(QString("bump_texture.png"), pMtlImpl2->bump().texture()->fileName().get());

	ASSERT_EQ(1, pMtlImpl2->numGroups());
	auto group2 = pMtlImpl2->group(0);
	EXPECT_EQ(QString("CustomGroup"), group2->name().get());
	EXPECT_FLOAT_EQ(75.f, group2->mask().get());

	ASSERT_EQ(1, group2->numLayers());
	auto layer2 = group2->layer(0);
	EXPECT_EQ(QString("CustomLayer"), layer2->name().get());
	
	EXPECT_TRUE(layer2->diffuseLayer().get());
	EXPECT_TRUE(layer2->diffuseColor().texture()->enabled().get());
	EXPECT_EQ(QString("diffuse_texture.png"), layer2->diffuseColor().texture()->fileName().get());
	EXPECT_FLOAT_EQ(0.5f, layer2->diffuseColor().texture()->brightness().get());
	EXPECT_FLOAT_EQ(0.1f, layer2->diffuseColor().get().x);
	EXPECT_FLOAT_EQ(0.2f, layer2->diffuseColor().get().y);
	EXPECT_FLOAT_EQ(0.3f, layer2->diffuseColor().get().z);

	EXPECT_TRUE(layer2->specularLayer().get());
	EXPECT_FLOAT_EQ(1.8f, layer2->indexOfRefraction().n().get());
	EXPECT_FLOAT_EQ(15.f, layer2->roughness().get());
	EXPECT_TRUE(layer2->thinFilmInterference().get());
	EXPECT_FLOAT_EQ(200.f, layer2->thickness().get());

	EXPECT_TRUE(layer2->emissiveLayer().get());
	EXPECT_FLOAT_EQ(0.9f, layer2->emissiveColor().get().x);
	EXPECT_FLOAT_EQ(0.8f, layer2->emissiveColor().get().y);
	EXPECT_FLOAT_EQ(0.7f, layer2->emissiveColor().get().z);
	EXPECT_FLOAT_EQ(150.f, layer2->emissiveIntensity().get());
}

TEST_F(TestMaterials, CalculateReflectionFromMeasuredIOR)
{
	auto pMtl = matmanager()->create(QString("test_zinc"));
	auto layer = static_cast<MaterialLayerImpl *>(pMtl->group(0)->layer(0));
	ASSERT_NE(nullptr, layer);

	layer->indexOfRefraction().type().setIndex((int)IORType::MEASURED);
	layer->indexOfRefraction().fileName().set(QDir::current().absoluteFilePath("Resources/Library/Materials/Measured/Metal/Zink.txt"));

	vec3 refl, refl90;
	float refl90Level = 0.f;
	EXPECT_TRUE(layer->calculateReflectionFromMeasuredIOR(refl, refl90, refl90Level, nullptr));

	EXPECT_GT(refl.x, 0.5f);
	EXPECT_GT(refl.y, 0.5f);
	EXPECT_GT(refl.z, 0.5f);

	EXPECT_GT(refl90.x, 0.5f);
	EXPECT_GT(refl90.y, 0.5f);
	EXPECT_GT(refl90.z, 0.5f);

	EXPECT_GT(refl90Level, 0.f);
	EXPECT_LE(refl90Level, 1.f);
}

TEST_F(TestMaterials, BumpTextureAutoDetectNormalMap)
{
	auto pMtl = matmanager()->create(QString("TestBumpNM"));
	auto pMtlImpl = static_cast<MaterialImpl *>(pMtl);
	auto bumpTex = pMtlImpl->bump().texture();
	ASSERT_NE(nullptr, bumpTex);

	EXPECT_FALSE(bumpTex->normalMap().get());

	// 1. Assign normal map file -> normalMap should auto-detect as true
	const QString normalMapPath = QDir::current().absoluteFilePath("Resources/Library/Textures/Miscellaneous/rough-plastic-normalmap.jpg");
	bumpTex->fileName().set(normalMapPath);
	EXPECT_TRUE(bumpTex->normalMap().get());

	// 2. Assign non-normal texture -> normalMap should auto-detect as false
	const QString noisePath = QDir::current().absoluteFilePath("Resources/Library/Textures/Miscellaneous/noise.jpg");
	bumpTex->fileName().set(noisePath);
	EXPECT_FALSE(bumpTex->normalMap().get());

	// 3. Assign normal map again -> auto-detects as true
	bumpTex->fileName().set(normalMapPath);
	EXPECT_TRUE(bumpTex->normalMap().get());

	// 4. Manually uncheck normal map
	bumpTex->normalMap().set(false);
	EXPECT_FALSE(bumpTex->normalMap().get());

	// 5. Changing other properties should NOT overwrite manual uncheck
	bumpTex->brightness().set(0.5f);
	EXPECT_FALSE(bumpTex->normalMap().get());
}


