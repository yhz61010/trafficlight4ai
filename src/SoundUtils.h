#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

inline constexpr char kDefaultYellowSound[] = ":/effects/effects/yellow.ogg";
inline constexpr char kDefaultGreenSound[] = ":/effects/effects/green.ogg";

QUrl soundUrlForPath(const QString &filePath);
void playSound(const QString &filePath, QObject *errorContext = nullptr);
void initBundledGStreamerPlugins();
