#include "../Shared/Interfaces/Parameters.h"
#include "../Core/src/ParametersImpl.h"
#include "../Shared/Interfaces/CoreInstance.h"
#include "../Core/src/CoreInstance.h"
#include "../Shared/Interfaces/SerializationContext.h"
#include "Helpers.h"

class TestParameters : public ::testing::Test
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
	CoreInstance *core()
	{
		return static_cast<CoreInstance *>(m_spCore.data());
	}
};

class MockParameterOwner : public IParameterOwner
{
	CoreInstance & m_core;
public:
	MockParameterOwner(CoreInstance & core) : m_core(core) {}

	CoreInstance & core() const override { return m_core; }
	void pushCommand(QUndoCommand *cmd) override { delete cmd; }
	void fireChanged(eParamId paramId) override {}
};

TEST_F(TestParameters, BasicParametersJsonSaveAndLoad)
{
	MockParameterOwner owner(*core());

	// Test ColorParameterImpl
	{
		ColorParameterImpl param1(vec3(0.1f, 0.2f, 0.3f), PID_SCENE_BACKGROUND, owner);
		QJsonObject obj;
		param1.save(obj, "color");

		ColorParameterImpl param2(vec3(0.f), PID_SCENE_BACKGROUND, owner);
		EXPECT_TRUE(param2.load(obj, "color"));
		EXPECT_FLOAT_EQ(0.1f, param2.get().x);
		EXPECT_FLOAT_EQ(0.2f, param2.get().y);
		EXPECT_FLOAT_EQ(0.3f, param2.get().z);
	}

	// Test ScalarParameterImpl
	{
		ScalarParameterImpl param1(5.5f, 0.f, 10.f, 2, PID_SCENE_DIFFUSE_INTENSITY, owner, 2.0f);
		QJsonObject obj;
		param1.save(obj, "scalar");

		ScalarParameterImpl param2(0.f, 0.f, 10.f, 2, PID_SCENE_DIFFUSE_INTENSITY, owner, 2.0f);
		EXPECT_TRUE(param2.load(obj, "scalar"));
		EXPECT_FLOAT_EQ(5.5f, param2.get());
	}

	// Test BooleanParameterImpl
	{
		BooleanParameterImpl param1(true, PID_SCENE_FLOOR_ENABLED, owner);
		QJsonObject obj;
		param1.save(obj, "boolean");

		BooleanParameterImpl param2(false, PID_SCENE_FLOOR_ENABLED, owner);
		EXPECT_TRUE(param2.load(obj, "boolean"));
		EXPECT_TRUE(param2.get());
	}

	// Test IntegerParameterImpl
	{
		IntegerParameterImpl param1(42, 0, 100, PID_CAMERA_DIAPHRAGM_BLADES, owner);
		QJsonObject obj;
		param1.save(obj, "integer");

		IntegerParameterImpl param2(0, 0, 100, PID_CAMERA_DIAPHRAGM_BLADES, owner);
		EXPECT_TRUE(param2.load(obj, "integer"));
		EXPECT_EQ(42, param2.get());
	}

	// Test Vec2ParameterImpl
	{
		Vec2ParameterImpl param1(vec2(1.5f, -2.5f), 2, PID_TEXTURE_REPEAT, owner);
		QJsonObject obj;
		param1.save(obj, "vec2");

		Vec2ParameterImpl param2(vec2(0.f), 2, PID_TEXTURE_REPEAT, owner);
		EXPECT_TRUE(param2.load(obj, "vec2"));
		EXPECT_FLOAT_EQ(1.5f, param2.get().x);
		EXPECT_FLOAT_EQ(-2.5f, param2.get().y);
	}

	// Test Vec3ParameterImpl
	{
		Vec3ParameterImpl param1(vec3(1.f, 2.f, 3.f), 2, PID_NODE_POSITION, owner);
		QJsonObject obj;
		param1.save(obj, "vec3");

		Vec3ParameterImpl param2(vec3(0.f), 2, PID_NODE_POSITION, owner);
		EXPECT_TRUE(param2.load(obj, "vec3"));
		EXPECT_FLOAT_EQ(1.f, param2.get().x);
		EXPECT_FLOAT_EQ(2.f, param2.get().y);
		EXPECT_FLOAT_EQ(3.f, param2.get().z);
	}

	// Test EnumParameterImpl
	{
		static const char * const items[] = { "A", "B", "C", nullptr };
		EnumParameterImpl param1(1, items, PID_IOR_TYPE, owner);
		QJsonObject obj;
		param1.save(obj, "enum");

		EnumParameterImpl param2(0, items, PID_IOR_TYPE, owner);
		EXPECT_TRUE(param2.load(obj, "enum"));
		EXPECT_EQ(1, param2.getIndex());
	}

	// Test StringParameterImpl
	{
		StringParameterImpl param1(QString("Hello"), PID_NODE_NAME, owner, nullptr);
		QJsonObject obj;
		param1.save(obj, "string");

		StringParameterImpl param2(QString(""), PID_NODE_NAME, owner, nullptr);
		EXPECT_TRUE(param2.load(obj, "string"));
		EXPECT_EQ(QString("Hello"), param2.get());
	}

	// Test FileNameParameterImpl
	{
		FileNameParameterImpl param1(FileFormatsList(), PID_TEXTURE_FILENAME, owner);
		param1._set(QString("test_file.png"));
		QJsonObject obj;
		param1.save(obj, "file", ModelSavingContext());

		FileNameParameterImpl param2(FileFormatsList(), PID_TEXTURE_FILENAME, owner);
		EXPECT_TRUE(param2.load(obj, "file", ModelLoadingContext()));
		EXPECT_EQ(QString("test_file.png"), param2.get());

		FileNameParameterImpl param3(FileFormatsList(), PID_TEXTURE_FILENAME, owner);
		param3._set(QString("owlet://test_file.png"));
		EXPECT_EQ(QString("miray://test_file.png"), param3.get());

		QJsonObject obj3;
		param3.save(obj3, "file", ModelSavingContext());
		EXPECT_EQ(QString("miray://test_file.png"), obj3["file"].toString());

		FileNameParameterImpl param3Loaded(FileFormatsList(), PID_TEXTURE_FILENAME, owner);
		EXPECT_TRUE(param3Loaded.load(obj3, "file", ModelLoadingContext()));
		EXPECT_EQ(QString("miray://test_file.png"), param3Loaded.get());

		FileNameParameterImpl param4(FileFormatsList(), PID_TEXTURE_FILENAME, owner);
		param4._set(QString("folder/owlet://test_file.png"));
		EXPECT_EQ(QDir::toNativeSeparators("folder/owlet://test_file.png"), param4.get());
	}
}
