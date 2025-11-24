#pragma once

#include <QString>

struct UiConfig
{
    QString daemonPath;
    QString uiName;
};

class UiConfigLoader
{
public:
    // Loads ui_config.json from:
    //   <app_dir>/resources/ui_config.json
    //
    // Returns true if successfully loaded AND parsed.
    // On failure:
    //   - fills errorMessage (if provided)
    //   - leaves UiConfig fields empty or fallback
    static bool load(UiConfig &config, QString *errorMessage = nullptr);

    // Optional helper: returns the absolute path of ui_config.json
    static QString configFilePath();
};
