#pragma once
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include "../utils/Utils.h"

namespace fs = boost::filesystem;

// Represents the state of a single mod archive
struct ArchiveInfo {
    enum Status { NEW, INSTALLED, UPDATE_AVAILABLE, UNKNOWN };

    std::string name;           // Clean name, e.g., "Better Bodies"
    std::string version;        // Version string, e.g., "3880-2-2"
    fs::path archive_path;      // Full path to the .zip, .7z, etc.
    fs::path target_data_path;  // Path it will be extracted to, e.g., mod_data/Better Bodies
    Status status = UNKNOWN;
};

class ArchiveManager {
public:
    // Main function to scan archives and determine their status
    void scan_archives(const fs::path& archive_dir, const fs::path& mod_data_dir);

    // Public state for the UI
    std::vector<ArchiveInfo> archives;
};
