#ifndef SETTINGSUNTIL_H
#define SETTINGSUNTIL_H

#include <QSettings>
#include <QString>
#include <QVariant>

void saveParameter(const QString &key, const QVariant &value,
                   const QString &group);
QVariant loadParameter(const QString &key,  const QString &group,
                       const QVariant &defaultValue);

#endif // SETTINGSUNTIL_H
