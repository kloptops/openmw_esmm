#include "PluginParser.h"
#include <fstream>
#include <iostream>
#include "../utils/Logger.h"

std::vector<std::string> PluginParser::get_masters(const fs::path& plugin_path) {
    std::vector<std::string> masters;
    std::ifstream file(plugin_path.string(), std::ios::binary);

    if (!file.is_open()) {
        LOG_WARN("PluginParser: Could not open plugin file: ", plugin_path.string());
        return masters;
    }

    LOG_DEBUG("PluginParser: Opened ", plugin_path.filename().string());

    // --- MAIN TES3 RECORD ---
    char record_type[5] = {0};
    uint32_t record_size;
    
    file.read(record_type, 4);
    if (file.gcount() < 4 || std::string(record_type, 4) != "TES3") {
        LOG_DEBUG("  -> Not a valid TES3 file.");
        return masters;
    }
    
    file.read(reinterpret_cast<char*>(&record_size), 4);
    file.seekg(8, std::ios::cur); // Skip flags/etc of the main TES3 record itself

    // --- SUBRECORD LOOP ---
    // The MAST records are SUB-records inside the main TES3 record's data block.
    // We must now read 'record_size' bytes to find them.
    long long end_of_tes3_record = (long long)file.tellg() + record_size;
    LOG_DEBUG("  -> TES3 record size: ", record_size, ". Reading subrecords until offset ", end_of_tes3_record);

    while ((long long)file.tellg() < end_of_tes3_record && file.good()) {
        char subrecord_type[5] = {0};
        uint32_t subrecord_size;

        file.read(subrecord_type, 4);
        if (file.gcount() < 4) break; // Clean exit if we're at the end
        
        file.read(reinterpret_cast<char*>(&subrecord_size), 4);
        LOG_DEBUG("    -> Found subrecord '", std::string(subrecord_type, 4), "' of size ", subrecord_size);

        if (std::string(subrecord_type, 4) == "MAST") {
            // This is a master file dependency. Read the name.
            std::string master_name(subrecord_size, '\0');
            file.read(&master_name[0], subrecord_size);
            master_name.erase(master_name.find('\0')); // Trim trailing nulls
            masters.push_back(master_name);
            LOG_DEBUG("      -> SUCCESS: Found master dependency: '", master_name, "'");
            
            // A MAST record is always followed by a DATA record holding the master's size.
            // We must read and skip this to stay correctly aligned for the next subrecord.
            file.seekg(4, std::ios::cur); // Skip "DATA" tag
            uint32_t data_size;
            file.read(reinterpret_cast<char*>(&data_size), 4);
            file.seekg(data_size, std::ios::cur); // Skip the data itself
            LOG_DEBUG("      -> Skipped DATA record of size ", data_size);

        } else {
            // Not a MAST, just skip its data block
            file.seekg(subrecord_size, std::ios::cur);
        }
    }
    
    return masters;
}