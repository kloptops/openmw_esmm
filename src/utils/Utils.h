#pragma once

#include <string>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

// Forward declaration to avoid circular dependencies
struct AppContext;

// Utility function declarations
std::string getEnvVar(const std::string& name, const std::string& default_val);
std::string trim(const std::string& s);

// Utility functions that will be defined in the .cpp file
std::string cleanModName(const std::string& filename);
std::string extractVersionString(const std::string& filename, const std::string& clean_name);
