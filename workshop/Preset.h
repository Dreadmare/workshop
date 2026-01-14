#ifndef PRESET_H
#define PRESET_H

#include <string>
#include "Database_Manager.h"

class Preset {
public:
    Preset(DatabaseManager* db);

    void savePreset(const std::string& name, double gradient, double roughness, double temperature);

    bool loadPreset(const std::string& name, double& gradOut, double& roughOut, double& tempOut);

    bool deletePreset(const std::string& name);

    void listPresets();

private:
    DatabaseManager* db;
};

#endif