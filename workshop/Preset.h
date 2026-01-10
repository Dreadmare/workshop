#ifndef PRESET_H
#define PRESET_H

#include <string>
#include "Database_Manager.h"

class Preset {
public:
    Preset(DatabaseManager* db);

    void savePreset(const std::string& id, const std::string& name, const std::string& terrain, const std::string& climate);
    bool loadPreset(const std::string& name, std::string& terrainOut, std::string& climateOut);
    void listPresets();

private:
    DatabaseManager* db;
};

#endif
