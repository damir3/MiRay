#include "../Shared/Interfaces/MaterialManager.h"
#include "../Shared/Interfaces/CoreInstance.h"
#include "../Shared/Interfaces/Scene.h"
#include "../Shared/Interfaces/Camera.h"
#include "../Shared/Interfaces/Geometry.h"
#include "../Core/src/CoreInstance.h"


class TestCamera : public ::testing::Test
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
		return core()->getScene();
	}

	ICamera *camera()
	{
		return &scene()->camera();
	}
};

TEST_F(TestCamera, DepthOfFieldIsDisabledByDefault)
{
	ASSERT_FALSE(camera()->depthOfField().get());
}

TEST_F(TestCamera, DepthOfFieldOptionsAreDisabledIfDepthOfFieldIsDisabled)
{
	camera()->depthOfField().set(false);

	EXPECT_FALSE(camera()->fStop().isEnabled());
	EXPECT_FALSE(camera()->focusDistance().isEnabled());
	EXPECT_FALSE(camera()->diaphragmBlades().isEnabled());
	EXPECT_FALSE(camera()->bokehRotation().isEnabled());
}

TEST_F(TestCamera, DepthOfFieldOptionsAreEnabledIfDepthOfFieldisEnabled)
{
	camera()->depthOfField().set(true);

	EXPECT_TRUE(camera()->fStop().isEnabled());
	EXPECT_TRUE(camera()->focusDistance().isEnabled());
	EXPECT_TRUE(camera()->diaphragmBlades().isEnabled());
	EXPECT_TRUE(camera()->bokehRotation().isEnabled());
}

TEST_F(TestCamera, CheckDiaphragmBladesRange)
{
	EXPECT_EQ(3, camera()->diaphragmBlades().min());
	EXPECT_EQ(32, camera()->diaphragmBlades().max());
}

TEST_F(TestCamera, CheckZNearToZero)
{
	EXPECT_LT(0, camera()->nearZ().min()) << "zNear must be > 0";
	camera()->nearZ().set(0); // try to set it to 0
	EXPECT_NE(0.f, camera()->nearZ().get()); // make sure it is not zero
}

TEST_F(TestCamera, CheckZFarToZero)
{
	EXPECT_LT(0, camera()->farZ().min()) << "zFar must be > 0";
	camera()->farZ().set(0); // try to set it to 0
	EXPECT_NE(0.f, camera()->farZ().get()); // make sure it is not zero
}

TEST_F(TestCamera, CheckZNearGreaterThanZFar)
{
	camera()->farZ().set(10);
	camera()->nearZ().set(20);
	EXPECT_LT(camera()->nearZ().get(), camera()->farZ().get());
}

TEST_F(TestCamera, CheckProjectors)
{
	auto MAX_ERROR = 1.2e-4f;
	vec3 coords[] = {
		{0.01f, 0.2f, 0.5f},
		{0.12f, 0.1f, 0.3f},
		{0.23f, 0.01f, 0.2f},
		{0.34f, 0.99f, 0.34f},
		{0.45f, 0.9f, 0.8f},
		{0.56f, 0.8f, 0.4f},
		{0.67f, 0.7f, 0.7f},
		{0.78f, 0.6f, 0.6f},
		{0.89f, 0.5f, 0.1f},
		{0.91f, 0.4f, 0.9f},
		{0.99f, 0.3f, 0.15f},
	};
	vec2 vp;
	vec3 origin;
	vec3 dir;

	camera()->target().set(vec3(1.234f, 2.345f, 3.456f));
	camera()->yaw().set(256.f);
	camera()->pitch().set(33.f);
	camera()->roll().set(13.f);
	camera()->distance().set(10.567f);

	camera()->projection().setIndex(CP_PERSPECTIVE);
	ASSERT_NE(nullptr, camera()->rayProjector());
	ASSERT_EQ(CP_PERSPECTIVE, camera()->rayProjector()->type());
	for (const auto & c : coords)
	{
		camera()->rayProjector()->getRay(origin, dir, reinterpret_cast<const vec2 &>(c));
		ASSERT_EQ(true, camera()->rayProjector()->getViewportCoords(vp, origin + dir * c.z, true));
		ASSERT_NEAR(c.x * 2.f - 1.f, vp.x, MAX_ERROR);
		ASSERT_NEAR(c.y * -2.f + 1.f, vp.y, MAX_ERROR);
	}

	camera()->projection().setIndex(CP_ORTHOGRAPHIC);
	ASSERT_NE(nullptr, camera()->rayProjector());
	ASSERT_EQ(CP_ORTHOGRAPHIC, camera()->rayProjector()->type());
	for (const auto & c : coords)
	{
		camera()->rayProjector()->getRay(origin, dir, reinterpret_cast<const vec2 &>(c));
		ASSERT_EQ(true, camera()->rayProjector()->getViewportCoords(vp, origin + dir * c.z, true));
		ASSERT_NEAR(c.x * 2.f - 1.f, vp.x, MAX_ERROR);
		ASSERT_NEAR(c.y * -2.f + 1.f, vp.y, MAX_ERROR);
	}

	camera()->projection().setIndex(CP_SPHERICAL);
	ASSERT_NE(nullptr, camera()->rayProjector());
	ASSERT_EQ(CP_SPHERICAL, camera()->rayProjector()->type());
	for (const auto & c : coords)
	{
		camera()->rayProjector()->getRay(origin, dir, reinterpret_cast<const vec2 &>(c));
		const vec2 cv(c.x * 2.f - 1.f, c.y * -2.f + 1.f);
		ASSERT_EQ(true, camera()->rayProjector()->getViewportCoords(vp, origin + dir * c.z, true));
		ASSERT_NEAR(cv.x, vp.x, MAX_ERROR);
		ASSERT_NEAR(cv.y, vp.y, MAX_ERROR);
	}

	camera()->projection().setIndex(CP_CYLINDRICAL);
	ASSERT_NE(nullptr, camera()->rayProjector());
	ASSERT_EQ(CP_CYLINDRICAL, camera()->rayProjector()->type());
	for (const auto & c : coords)
	{
		camera()->rayProjector()->getRay(origin, dir, reinterpret_cast<const vec2 &>(c));
		const vec2 cv(c.x * 2.f - 1.f, c.y * -2.f + 1.f);
		ASSERT_EQ(true, camera()->rayProjector()->getViewportCoords(vp, origin + dir * c.z, true));
		ASSERT_NEAR(cv.x, vp.x, MAX_ERROR);
		ASSERT_NEAR(cv.y, vp.y, MAX_ERROR);
	}

	camera()->projection().setIndex(CP_FISHEYE);
	ASSERT_NE(nullptr, camera()->rayProjector());
	ASSERT_EQ(CP_FISHEYE, camera()->rayProjector()->type());
	for (const auto & c : coords)
	{
		if (!camera()->rayProjector()->getRay(origin, dir, reinterpret_cast<const vec2 &>(c)))
			continue;
		const vec2 cv(c.x * 2.f - 1.f, c.y * -2.f + 1.f);
		ASSERT_EQ(true, camera()->rayProjector()->getViewportCoords(vp, origin + dir * c.z, true));
		ASSERT_NEAR(cv.x, vp.x, MAX_ERROR);
		ASSERT_NEAR(cv.y, vp.y, MAX_ERROR);
	}
}
