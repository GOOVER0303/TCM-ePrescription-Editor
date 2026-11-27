#pragma once

#include "models.h"

#include <filesystem>
#include <string>

namespace tcm {

class Storage {
public:
    explicit Storage(std::filesystem::path dataDir);

    bool initialize(std::string& error);
    bool load(AppState& state, std::string& error);
    bool save(const AppState& state, std::string& error);
    bool resetToSeed(std::string& error);

    const std::filesystem::path& dataDir() const { return dataDir_; }
    const std::filesystem::path& seedDir() const { return seedDir_; }

private:
    bool copySeedToActive(std::string& error);
    bool writeAllFiles(const AppState& state, std::string& error);

    std::filesystem::path dataDir_;
    std::filesystem::path seedDir_;
};

}  // namespace tcm
