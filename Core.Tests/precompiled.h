#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QDebug>
#include <QObject>
#include <QByteArray>
#include <QtPlugin>
#include <QMetaType>
#include <QImage>
#include <QBuffer>
#include <QIODevice>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QImageWriter>
#include <QDir>
#include <QKeySequence>
#include <QSharedPointer>
#include <QUndoStack>
#include <QDomDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QColor>
#include <QUuid>
#include <QMutex>
#include <QTimer>
#include <QQueue>

#include <QFuture>

#include <QApplication>

#include <cassert>
#include <cfloat>
#include <complex>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>

#define GLM_USE_SHORT_TYPES
#include <glm-math.h>

#include "../Shared/Interfaces/Image.h"
#include "../Shared/Utils/FileUtils.h"

#include "../Core/Application.h"
