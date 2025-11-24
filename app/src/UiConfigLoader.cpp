#include "UiConfigLoader.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

QString UiConfigLoader::configFilePath()
{
    // ui_config.json is stored under: <app_dir>/resources/ui_config.json
    return QCoreApplication::applicationDirPath() + "/ui_config.json";
}

bool UiConfigLoader::load(UiConfig &config, QString *errorMessage)
{
    const QString path = configFilePath();
    QFile file(path);

    if (!file.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("ui_config.json not found at: %1").arg(path);
        }
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Failed to open ui_config.json at: %1").arg(path);
        }
        return false;
    }

    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("ui_config.json parse error: %1 at offset %2")
                                .arg(parseError.errorString())
                                .arg(parseError.offset);
        }
        return false;
    }

    QJsonObject obj = doc.object();

    config.daemonPath = obj.value("daemon_path").toString();
    config.uiName     = obj.value("ui_name").toString();

    if (config.daemonPath.isEmpty() || config.uiName.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("ui_config.json missing required fields: daemon_path or ui_name");
        }
        return false;
    }

    return true;
}
