#include "../Core/src/Materials/IORReader.h"
#include "../Shared/Utils/FileUtils.h"

TEST(TestIOR, ior_ev_ior)
{
	auto data = readFile(":/ior-ev.ior");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::sopra::readData(data));
	ASSERT_EQ(111, iors.size());
}

TEST(TestIOR, ior_ev_csv_ior)
{
	auto data = readFile(":/ior-ev-csv.ior");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::sopra::readData(data));
	ASSERT_EQ(101, iors.size());

	EXPECT_NEAR(1240, iors.front().x, 0.0000001);
	EXPECT_NEAR(2.2795, iors.front().y, 0.0000001);
	EXPECT_NEAR(0, iors.front().z, 0.0000001);

	EXPECT_NEAR(1240.f / 6, iors.back().x, 0.0000001);
	EXPECT_NEAR(2.24, iors.back().y, 0.0000001);
	EXPECT_NEAR(1.65, iors.back().z, 0.0000001);
}

TEST(TestIOR, ior_ev_garbage_at_the_end_ior)
{
	auto data = readFile(":/ior-ev-garbage-at-the-end.ior");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::sopra::readData(data));
	ASSERT_EQ(56, iors.size());
}

TEST(TestIOR, ior_ev_space_at_beginning_ior)
{
	auto data = readFile(":/ior-ev-space-at-beginning.ior");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::sopra::readData(data));
	ASSERT_EQ(56, iors.size());
}

TEST(TestIOR, ior_ev_tabs_before_numbers_ior)
{
	auto data = readFile(":/ior-ev-tabs-before-numbers.ior");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::sopra::readData(data));
	ASSERT_EQ(101, iors.size());
}

TEST(TestIOR, ior_nm_nk)
{
	auto data = readFile(":/ior-nm.nk");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::sopra::readData(data));
	ASSERT_EQ(69, iors.size());

	EXPECT_DOUBLE_EQ(380, iors.front().x);
	EXPECT_NEAR(1.741114482, iors.front().y, 0.0000001);
	EXPECT_NEAR(0.000055126, iors.front().z, 0.0000001);

	EXPECT_DOUBLE_EQ(720, iors.back().x);
	EXPECT_NEAR(1.741114470, iors.back().y, 0.0000001);
	EXPECT_NEAR(0.000005801, iors.back().z, 0.0000001);
}

TEST(TestIOR, ior_nm_newline_at_beginning_ior)
{
	auto data = readFile(":/ior-nm-newline-at-beginning.ior");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::sopra::readData(data));
	ASSERT_EQ(75, iors.size());
}

TEST(TestIOR, ior_um_nk)
{
	auto data = readFile(":/ior-um.nk");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::sopra::readData(data));
	ASSERT_EQ(69, iors.size());
}

TEST(TestIOR, ior_um_mixed_ior)
{
	auto data = readFile(":/ior-um-mixed.ior");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::sopra::readData(data));
	ASSERT_EQ(304, iors.size());

	EXPECT_DOUBLE_EQ(234, iors.front().x);
	EXPECT_NEAR(1.4439f, iors.front().y, 0.0000001);
	EXPECT_NEAR(3.0094f, iors.front().z, 0.0000001);

	EXPECT_DOUBLE_EQ(840, iors.back().x);
	EXPECT_NEAR(4.6263f, iors.back().y, 0.0000001);
	EXPECT_NEAR(.2373f, iors.back().z, 0.0000001);
}

TEST(TestIOR, ior_um_one_per_line_ior)
{
	auto data = readFile(":/ior-um-one-per-line.ior");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::sopra::readData(data));
	ASSERT_EQ(61, iors.size());
}

TEST(TestIOR, ior_filmetrics_txt)
{
	auto data = readFile(":/ior-filmetrics.txt");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::text::readData(data));
	ASSERT_EQ(121, iors.size());

	EXPECT_NEAR(190.77, iors.front().x, 0.0001);
	EXPECT_NEAR(0.958, iors.front().y, 0.0001);
	EXPECT_NEAR(1.37, iors.front().z, 0.0001);

	EXPECT_NEAR(2480, iors.back().x, 0.0001);
	EXPECT_NEAR(1.15, iors.back().y, 0.0001);
	EXPECT_NEAR(13.2, iors.back().z, 0.0001);
}

TEST(TestIOR, ior_refractiveindex_nk)
{
	auto data = readFile(":/ior-refractiveindex-nk.txt");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::text::readData(data));
	ASSERT_EQ(200, iors.size());

	EXPECT_NEAR(206.6, iors.front().x, 0.0001);
	EXPECT_NEAR(1.30297032889, iors.front().y, 0.0001);
	EXPECT_NEAR(1.68242565429, iors.front().z, 0.0001);

	EXPECT_NEAR(12400, iors.back().x, 0.0001);
	EXPECT_NEAR(12.4092166102, iors.back().y, 0.0001);
	EXPECT_NEAR(78.5888502881, iors.back().z, 0.0001);
}

TEST(TestIOR, ior_refractiveindex_n)
{
	auto data = readFile(":/ior-refractiveindex-n.txt");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::text::readData(data));
	ASSERT_EQ(101, iors.size());

	EXPECT_NEAR(200, iors.front().x, 0.0001);
	EXPECT_NEAR(1.7899887573267, iors.front().y, 0.0001);
	EXPECT_NEAR(0, iors.front().z, 0.0001);

	EXPECT_NEAR(30000, iors.back().x, 0.0001);
	EXPECT_NEAR(1.0909719812108, iors.back().y, 0.0001);
	EXPECT_NEAR(0, iors.back().z, 0.0001);
}

TEST(TestIOR, ior_refractiveindex_csv)
{
	auto data = readFile(":/ior-refractiveindex-csv.txt");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::text::readData(data));
	ASSERT_EQ(52, iors.size());
}

TEST(TestIOR, ior_different_wavelengths_nk)
{
	auto data = readFile(":/ior-different-wavelengths-nk.txt");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::text::readData(data));
	ASSERT_EQ(22, iors.size());

	ASSERT_FLOAT_EQ(310, iors[0].x);
	ASSERT_FLOAT_EQ(1.5539233467171f, iors[0].y);
	ASSERT_FLOAT_EQ(4.996E-5f, iors[0].z);

	ASSERT_FLOAT_EQ(320, iors[1].x);
	ASSERT_GT(1.5539233467171f, iors[1].y);
	ASSERT_LT(1.5444188160462f, iors[1].y);
	ASSERT_FLOAT_EQ(1.375E-5f, iors[1].z);
}

TEST(TestIOR, ior_different_wavelengths_nk_2)
{
	auto data = readFile(":/ior-different-wavelengths-nk-2.txt");
	std::vector<vec3> iors;
	ASSERT_NO_THROW(iors = ior::text::readData(data));
	ASSERT_EQ(6, iors.size());

	ASSERT_FLOAT_EQ(100, iors[0].x);
	ASSERT_FLOAT_EQ(200, iors[1].x);
	ASSERT_FLOAT_EQ(300, iors[2].x);
	ASSERT_FLOAT_EQ(400, iors[3].x);
	ASSERT_FLOAT_EQ(500, iors[4].x);
	ASSERT_FLOAT_EQ(700, iors[5].x);

	ASSERT_FLOAT_EQ(1, iors[0].y);
	ASSERT_FLOAT_EQ(2, iors[1].y);
	ASSERT_FLOAT_EQ(3, iors[2].y);
	ASSERT_FLOAT_EQ(4, iors[3].y);
	ASSERT_FLOAT_EQ(5, iors[4].y);
	ASSERT_FLOAT_EQ(7, iors[5].y);

	ASSERT_FLOAT_EQ(2, iors[0].z);
	ASSERT_FLOAT_EQ(2, iors[1].z);
	ASSERT_FLOAT_EQ(3, iors[2].z);
	ASSERT_FLOAT_EQ(4, iors[3].z);
	ASSERT_FLOAT_EQ(4, iors[4].z);
	ASSERT_FLOAT_EQ(4, iors[5].z);
}
