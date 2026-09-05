#pragma once

struct ModelSavingContext {
	QString targetFileName;
	QString targetFolder;
	QMap<QString, QString> mapFileNamesOverride;
};

struct ModelLoadingContext {
	QString sourceFileName;
	QString sourceFolder;
	bool loadNewScene = true; // false means import
	bool putOnTheFloor = true;
	uint16_t fileVersion = 0xFFFF;
	float unitScale = 0.0f; // 0 means use format default
};
