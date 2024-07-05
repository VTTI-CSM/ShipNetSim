#ifndef CONFIGURATIONMANAGER_H
#define CONFIGURATIONMANAGER_H

#include <QSettings>
#include <QStringList>
#include <QMutex>
#include <QMutexLocker>

class ConfigurationManager {
public:
    explicit ConfigurationManager(const QString& iniFilePath);

    QString getConfigValue(const QString& section, const QString& key,
                           const QString& defaultValue = QString()) const;
    QList<QString> getConfigValues(const QString& section,
                                   const QList<QString>& keys,
                                   const QList<QString>& defaultValues = {});
    QStringList getConfigKeys(const QString &section);
    QStringList getConfigSections() const;
    void setConfigValue(const QString& section, const QString& key,
                        const QString& value);
    void setConfigValues(const QString& section, const QList<QString>& keys,
                         const QList<QString>& values);

private:
    QSettings m_settings;
    mutable QMutex m_mutex;  // used for thread safety

    QStringList getKeysInSection(const QString &section);  // helper function to reduce code duplication
};

#endif // CONFIGURATIONMANAGER_H
