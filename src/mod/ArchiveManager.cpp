#include "ArchiveManager.h"
#include <regex>
#include <fstream>
#include <iostream>
#include <algorithm>

void ArchiveManager::scan_archives(const fs::path& archive_dir, const fs::path& mod_data_dir) {
    archives.clear();
    if (!fs::exists(archive_dir)) return;

    const std::vector<std::string> extensions = {".zip", ".7z", ".rar"};
    for (const auto& entry : fs::directory_iterator(archive_dir)) {
        if (!fs::is_regular_file(entry)) continue;

        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        bool is_archive = false;
        for (const auto& supported_ext : extensions) {
            if (ext == supported_ext) {
                is_archive = true;
                break;
            }
        }
        if (!is_archive) continue;

        ArchiveInfo info;
        info.archive_path = entry.path();
        info.name = cleanModName(entry.path().filename().string());
        info.version = extractVersionString(entry.path().filename().string(), info.name);
        info.target_data_path = mod_data_dir / info.name;

        // Now, determine the status
        fs::path marker_file = info.target_data_path / "esmm_archive.txt";
        if (!fs::exists(info.target_data_path) || !fs::exists(marker_file)) {
            info.status = ArchiveInfo::NEW;
        } else {
            std::ifstream file(marker_file.string());
            std::string installed_archive_name;
            if (std::getline(file, installed_archive_name)) {
                if (installed_archive_name == info.archive_path.filename().string()) {
                    info.status = ArchiveInfo::INSTALLED;
                } else {
                    info.status = ArchiveInfo::UPDATE_AVAILABLE;
                }
            }
        }
        archives.push_back(info);
    }
    // Sort for consistent display
    std::sort(archives.begin(), archives.end(), [](const ArchiveInfo& a, const ArchiveInfo& b){ return a.name < b.name; });
}
