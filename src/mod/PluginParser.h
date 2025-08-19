#pragma once
#include <string>
#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

class PluginParser {
public:
    // Returns a list of master files (.esm) that a given plugin depends on.
    std::vector<std::string> get_masters(const fs::path& plugin_path);
};