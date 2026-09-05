#include "../Shared/Interfaces/Parameters.h"
#include "../Shared/Interfaces/MaterialManager.h"
#include "../Shared/Interfaces/CoreInstance.h"
#include "../Shared/Interfaces/SerializationContext.h"
#include "../Shared/Interfaces/Scene.h"
#include "../Shared/Interfaces/MeshNode.h"
#include "../Shared/Interfaces/Material.h"

#include "Helpers.h"

class TestSceneTwoGroupsAndLayers : public TestScene
{
	void SetUp() override
	{
		TestScene::SetUp();

		auto mtrl1 = matmanager()->create(QString("mtrl1"));
		ASSERT_NE(nullptr, mtrl1);

		auto mtrl2 = matmanager()->create(QString("mtrl2"));
		ASSERT_NE(nullptr, mtrl2);

		auto grp1 = mtrl2->addGroup(0);
		ASSERT_NE(nullptr, grp1);

		auto grp2 = mtrl2->addGroup(0);
		ASSERT_NE(nullptr, grp2);

		auto lay1 = grp2->addLayer(0, QByteArray(), ModelLoadingContext());
		ASSERT_NE(nullptr, lay1);

		auto lay2 = grp2->addLayer(0, QByteArray(), ModelLoadingContext());
		ASSERT_NE(nullptr, lay2);
	}
};

TEST_F(TestSceneTwoGroupsAndLayers, UpdateFileNames)
{
	app()->startPlugins();

	auto mtrl1 = matmanager()->create(QString("mtrl1"));
	ASSERT_NE(nullptr, mtrl1);

	auto mtrl2 = matmanager()->create(QString("mtrl2"));
	ASSERT_NE(nullptr, mtrl2);

	auto grp1 = mtrl2->addGroup(0);
	ASSERT_NE(nullptr, grp1);

	auto grp2 = mtrl2->addGroup(0);
	ASSERT_NE(nullptr, grp2);

	auto lay1 = grp2->addLayer(0, QByteArray(), ModelLoadingContext());
	ASSERT_NE(nullptr, lay1);

	auto lay2 = grp2->addLayer(0, QByteArray(), ModelLoadingContext());
	ASSERT_NE(nullptr, lay2);

	auto& sceneProp = scene()->properties();
	sceneProp.backgroundMode().setIndex(2);
	sceneProp.background().texture()->enabled().set(true);
	sceneProp.background().texture()->fileName().set(nativePath(":/Scene_Background_File_1.png"));

	sceneProp.environment().texture()->enabled().set(true);
	sceneProp.environment().texture()->fileName().set(nativePath(":/Scene_Environment_File_1.png"));

	mtrl2->bump().texture()->enabled().set(true);
	mtrl2->bump().texture()->fileName().set(nativePath(":/Mtrl_Bump_File_1.png"));

	mtrl2->medium().setIndex(2);
	mtrl2->indexOfRefraction().fileName().set(nativePath(":/Mtrl_IOF_File_1.png"));

	grp2->mask().texture()->enabled().set(true);
	grp2->mask().texture()->fileName().set(nativePath(":/Grp_Mask_File_1.png"));

	lay2->mask().texture()->enabled().set(true);
	lay2->mask().texture()->fileName().set(nativePath(":/Lay_Mask_File_1.png"));

	lay2->bump().texture()->enabled().set(true);
	lay2->bump().texture()->fileName().set(nativePath(":/Lay_Bump_File_1.png"));

	lay2->diffuseLayer().set(true);
	lay2->diffuseColor().texture()->enabled().set(true);
	lay2->diffuseColor().texture()->fileName().set(nativePath(":/Lay_DiffuseColor_File_1.png"));

	lay2->diffuseOpacity().texture()->enabled().set(true);
	lay2->diffuseOpacity().texture()->fileName().set(nativePath(":/Lay_DiffuseOpacity_File_1.png"));

	lay2->diffuseTransmission().texture()->enabled().set(true);
	lay2->diffuseTransmission().texture()->fileName().set(nativePath(":/Lay_DiffuseTransmission_File_1.png"));

	lay2->emissiveLayer().set(true);
	lay2->emissiveColor().texture()->enabled().set(true);
	lay2->emissiveColor().texture()->fileName().set(nativePath(":/Lay_EmissiveColor_File_1.png"));

	lay2->specularLayer().set(true);
	lay2->indexOfRefraction().type().setIndex(2);
	lay2->indexOfRefraction().fileName().set(nativePath(":/Lay_IndexOfRefraction_File_1.png"));

	lay2->reflection().texture()->enabled().set(true);
	lay2->reflection().texture()->fileName().set(nativePath(":/Lay_Reflection_File_1.png"));

	lay2->transmission().texture()->enabled().set(true);
	lay2->transmission().texture()->fileName().set(nativePath(":/Lay_Transmission_File_1.png"));

	lay2->reflection90().texture()->enabled().set(true);
	lay2->reflection90().texture()->fileName().set(nativePath(":/Lay_Reflection90_File_1.png"));

	lay2->roughness().texture()->enabled().set(true);
	lay2->roughness().texture()->fileName().set(nativePath(":/Lay_RoughnessFile_1.png"));

	lay2->anisotropy().texture()->enabled().set(true);
	lay2->anisotropy().texture()->fileName().set(nativePath(":/Lay_Anisotropy_File_1.png"));

	lay2->anisotropyAngle().texture()->enabled().set(true);
	lay2->anisotropyAngle().texture()->fileName().set(nativePath(":/Lay_AnisotropyAngle_File_1.png"));

	lay2->thinFilmInterference().set(true);
	lay2->thickness().texture()->enabled().set(true);
	lay2->thickness().texture()->fileName().set(nativePath(":/Lay_Thickness_File_1.png"));

	lay2->filmIndexOfRefraction().type().setIndex(2);
	lay2->filmIndexOfRefraction().fileName().set(nativePath(":/Lay_FilmIndexOfRefraction_File_1.png"));

	////////////////////////
	QSet<QString> res;
	scene()->collectFileNames(res);

	EXPECT_TRUE(res.contains(nativePath(":/Scene_Background_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Scene_Environment_File_1.png")));

	EXPECT_TRUE(res.contains(nativePath(":/Mtrl_Bump_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Mtrl_IOF_File_1.png")));

	EXPECT_TRUE(res.contains(nativePath(":/Grp_Mask_File_1.png")));

	EXPECT_TRUE(res.contains(nativePath(":/Lay_Mask_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_Bump_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_DiffuseColor_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_DiffuseOpacity_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_DiffuseTransmission_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_EmissiveColor_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_IndexOfRefraction_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_Reflection_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_Transmission_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_Reflection90_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_RoughnessFile_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_Anisotropy_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_AnisotropyAngle_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_Thickness_File_1.png")));
	EXPECT_TRUE(res.contains(nativePath(":/Lay_FilmIndexOfRefraction_File_1.png")));

	QMap<QString, QString> map;
	map[":/Scene_Background_File_1.png"]	= nativePath(":/Scene_Background_File_2.png");
	map[":/Scene_Environment_File_1.png"]	= nativePath(":/Scene_Environment_File_2.png");
	map[":/Mtrl_Bump_File_1.png"]			= nativePath(":/Mtrl_Bump_File_2.png");
	map[":/Mtrl_IOF_File_1.png"]			= nativePath(":/Mtrl_IOF_File_2.png");
	map[":/Grp_Mask_File_1.png"]			= nativePath(":/Grp_Mask_File_2.png");

	map[":/Lay_Mask_File_1.png"]					= nativePath(":/Lay_Mask_File_2.png");
	map[":/Lay_Bump_File_1.png"]					= nativePath(":/Lay_Bump_File_2.png");
	map[":/Lay_DiffuseColor_File_1.png"]			= nativePath(":/Lay_DiffuseColor_File_2.png");
	map[":/Lay_DiffuseOpacity_File_1.png"]			= nativePath(":/Lay_DiffuseOpacity_File_2.png");
	map[":/Lay_DiffuseTransmission_File_1.png"]		= nativePath(":/Lay_DiffuseTransmission_File_2.png");
	map[":/Lay_EmissiveColor_File_1.png"]			= nativePath(":/Lay_EmissiveColor_File_2.png");
	map[":/Lay_IndexOfRefraction_File_1.png"]		= nativePath(":/Lay_IndexOfRefraction_File_2.png");
	map[":/Lay_Reflection_File_1.png"]				= nativePath(":/Lay_Reflection_File_2.png");
	map[":/Lay_Transmission_File_1.png"]			= nativePath(":/Lay_Transmission_File_2.png");
	map[":/Lay_Reflection90_File_1.png"]			= nativePath(":/Lay_Reflection90_File_2.png");
	map[":/Lay_RoughnessFile_1.png"]				= nativePath(":/Lay_RoughnessFile_2.png");
	map[":/Lay_Anisotropy_File_1.png"]				= nativePath(":/Lay_Anisotropy_File_2.png");
	map[":/Lay_AnisotropyAngle_File_1.png"]			= nativePath(":/Lay_AnisotropyAngle_File_2.png");
	map[":/Lay_Thickness_File_1.png"]				= nativePath(":/Lay_Thickness_File_2.png");
	map[":/Lay_FilmIndexOfRefraction_File_1.png"]	= nativePath(":/Lay_FilmIndexOfRefraction_File_2.png");

	scene()->updateFileNames(map);

	EXPECT_EQ(sceneProp.background().texture()->fileName().get(), nativePath(":/Scene_Background_File_2.png"));
	EXPECT_EQ(sceneProp.environment().texture()->fileName().get(), nativePath(":/Scene_Environment_File_2.png"));
	EXPECT_EQ(mtrl2->bump().texture()->fileName().get(), nativePath(":/Mtrl_Bump_File_2.png"));
	EXPECT_EQ(mtrl2->indexOfRefraction().fileName().get(), nativePath(":/Mtrl_IOF_File_2.png"));
	EXPECT_EQ(grp2->mask().texture()->fileName().get(), nativePath(":/Grp_Mask_File_2.png"));

	EXPECT_EQ(lay2->mask().texture()->fileName().get(), nativePath(":/Lay_Mask_File_2.png"));
	EXPECT_EQ(lay2->bump().texture()->fileName().get(), nativePath(":/Lay_Bump_File_2.png"));
	EXPECT_EQ(lay2->diffuseColor().texture()->fileName().get(), nativePath(":/Lay_DiffuseColor_File_2.png"));
	EXPECT_EQ(lay2->diffuseOpacity().texture()->fileName().get(), nativePath(":/Lay_DiffuseOpacity_File_2.png"));
	EXPECT_EQ(lay2->diffuseTransmission().texture()->fileName().get(), nativePath(":/Lay_DiffuseTransmission_File_2.png"));
	EXPECT_EQ(lay2->emissiveColor().texture()->fileName().get(), nativePath(":/Lay_EmissiveColor_File_2.png"));
	EXPECT_EQ(lay2->indexOfRefraction().fileName().get(), nativePath(":/Lay_IndexOfRefraction_File_2.png"));
	EXPECT_EQ(lay2->reflection().texture()->fileName().get(), nativePath(":/Lay_Reflection_File_2.png"));
	EXPECT_EQ(lay2->transmission().texture()->fileName().get(), nativePath(":/Lay_Transmission_File_2.png"));
	EXPECT_EQ(lay2->reflection90().texture()->fileName().get(), nativePath(":/Lay_Reflection90_File_2.png"));
	EXPECT_EQ(lay2->roughness().texture()->fileName().get(), nativePath(":/Lay_RoughnessFile_2.png"));
	EXPECT_EQ(lay2->anisotropy().texture()->fileName().get(), nativePath(":/Lay_Anisotropy_File_2.png"));
	EXPECT_EQ(lay2->anisotropyAngle().texture()->fileName().get(), nativePath(":/Lay_AnisotropyAngle_File_2.png"));
	EXPECT_EQ(lay2->thickness().texture()->fileName().get(), nativePath(":/Lay_Thickness_File_2.png"));
	EXPECT_EQ(lay2->filmIndexOfRefraction().fileName().get(), nativePath(":/Lay_FilmIndexOfRefraction_File_2.png"));

	scene()->undoStack().undo();

	EXPECT_EQ(sceneProp.background().texture()->fileName().get(), nativePath(":/Scene_Background_File_1.png"));
	EXPECT_EQ(sceneProp.environment().texture()->fileName().get(), nativePath(":/Scene_Environment_File_1.png"));
	EXPECT_EQ(mtrl2->bump().texture()->fileName().get(), nativePath(":/Mtrl_Bump_File_1.png"));
	EXPECT_EQ(mtrl2->indexOfRefraction().fileName().get(), nativePath(":/Mtrl_IOF_File_1.png"));
	EXPECT_EQ(grp2->mask().texture()->fileName().get(), nativePath(":/Grp_Mask_File_1.png"));

	EXPECT_EQ(lay2->mask().texture()->fileName().get(), nativePath(":/Lay_Mask_File_1.png"));
	EXPECT_EQ(lay2->bump().texture()->fileName().get(), nativePath(":/Lay_Bump_File_1.png"));
	EXPECT_EQ(lay2->diffuseColor().texture()->fileName().get(), nativePath(":/Lay_DiffuseColor_File_1.png"));
	EXPECT_EQ(lay2->diffuseOpacity().texture()->fileName().get(), nativePath(":/Lay_DiffuseOpacity_File_1.png"));
	EXPECT_EQ(lay2->diffuseTransmission().texture()->fileName().get(), nativePath(":/Lay_DiffuseTransmission_File_1.png"));
	EXPECT_EQ(lay2->emissiveColor().texture()->fileName().get(), nativePath(":/Lay_EmissiveColor_File_1.png"));
	EXPECT_EQ(lay2->indexOfRefraction().fileName().get(), nativePath(":/Lay_IndexOfRefraction_File_1.png"));
	EXPECT_EQ(lay2->reflection().texture()->fileName().get(), nativePath(":/Lay_Reflection_File_1.png"));
	EXPECT_EQ(lay2->transmission().texture()->fileName().get(), nativePath(":/Lay_Transmission_File_1.png"));
	EXPECT_EQ(lay2->reflection90().texture()->fileName().get(), nativePath(":/Lay_Reflection90_File_1.png"));
	EXPECT_EQ(lay2->roughness().texture()->fileName().get(), nativePath(":/Lay_RoughnessFile_1.png"));
	EXPECT_EQ(lay2->anisotropy().texture()->fileName().get(), nativePath(":/Lay_Anisotropy_File_1.png"));
	EXPECT_EQ(lay2->anisotropyAngle().texture()->fileName().get(), nativePath(":/Lay_AnisotropyAngle_File_1.png"));
	EXPECT_EQ(lay2->thickness().texture()->fileName().get(), nativePath(":/Lay_Thickness_File_1.png"));
	EXPECT_EQ(lay2->filmIndexOfRefraction().fileName().get(), nativePath(":/Lay_FilmIndexOfRefraction_File_1.png"));

	scene()->undoStack().redo();

	EXPECT_EQ(sceneProp.background().texture()->fileName().get(), nativePath(":/Scene_Background_File_2.png"));
	EXPECT_EQ(sceneProp.environment().texture()->fileName().get(), nativePath(":/Scene_Environment_File_2.png"));
	EXPECT_EQ(mtrl2->bump().texture()->fileName().get(), nativePath(":/Mtrl_Bump_File_2.png"));
	EXPECT_EQ(mtrl2->indexOfRefraction().fileName().get(), nativePath(":/Mtrl_IOF_File_2.png"));
	EXPECT_EQ(grp2->mask().texture()->fileName().get(), nativePath(":/Grp_Mask_File_2.png"));

	EXPECT_EQ(lay2->mask().texture()->fileName().get(), nativePath(":/Lay_Mask_File_2.png"));
	EXPECT_EQ(lay2->bump().texture()->fileName().get(), nativePath(":/Lay_Bump_File_2.png"));
	EXPECT_EQ(lay2->diffuseColor().texture()->fileName().get(), nativePath(":/Lay_DiffuseColor_File_2.png"));
	EXPECT_EQ(lay2->diffuseOpacity().texture()->fileName().get(), nativePath(":/Lay_DiffuseOpacity_File_2.png"));
	EXPECT_EQ(lay2->diffuseTransmission().texture()->fileName().get(), nativePath(":/Lay_DiffuseTransmission_File_2.png"));
	EXPECT_EQ(lay2->emissiveColor().texture()->fileName().get(), nativePath(":/Lay_EmissiveColor_File_2.png"));
	EXPECT_EQ(lay2->indexOfRefraction().fileName().get(), nativePath(":/Lay_IndexOfRefraction_File_2.png"));
	EXPECT_EQ(lay2->reflection().texture()->fileName().get(), nativePath(":/Lay_Reflection_File_2.png"));
	EXPECT_EQ(lay2->transmission().texture()->fileName().get(), nativePath(":/Lay_Transmission_File_2.png"));
	EXPECT_EQ(lay2->reflection90().texture()->fileName().get(), nativePath(":/Lay_Reflection90_File_2.png"));
	EXPECT_EQ(lay2->roughness().texture()->fileName().get(), nativePath(":/Lay_RoughnessFile_2.png"));
	EXPECT_EQ(lay2->anisotropy().texture()->fileName().get(), nativePath(":/Lay_Anisotropy_File_2.png"));
	EXPECT_EQ(lay2->anisotropyAngle().texture()->fileName().get(), nativePath(":/Lay_AnisotropyAngle_File_2.png"));
	EXPECT_EQ(lay2->thickness().texture()->fileName().get(), nativePath(":/Lay_Thickness_File_2.png"));
	EXPECT_EQ(lay2->filmIndexOfRefraction().fileName().get(), nativePath(":/Lay_FilmIndexOfRefraction_File_2.png"));
}
