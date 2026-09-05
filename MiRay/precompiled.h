#define NOMINMAX

#include <Qt>
#include <QObject>

#include <QSet>
#include <QMap>
#include <QDir>
#include <QImage>
#include <QApplication>
#include <QQuickWidget>
#include <QQmlContext>
#include <QBuffer>
#include <QJsonObject>
#include <QStandardItemModel>

#include <set>
#include <stdexcept>

#define GLM_USE_SHORT_TYPES
#include <glm-math.h>

#include "../Shared/Interfaces/CoreInstance.h"
#include "../Shared/Interfaces/Node.h"
#include "../Shared/Interfaces/Scene.h"

#include "../Core/Application.h"
