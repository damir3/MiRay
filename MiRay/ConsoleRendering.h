#pragma once

#include <QVariantMap>
#include <QStringList>

#ifdef Q_OS_WIN
void redirectIOToConsole();
#endif

bool isConsoleMode(const QVariantMap& args);
int processInConsoleMode(const QVariantMap& args, const QStringList& scenes);
