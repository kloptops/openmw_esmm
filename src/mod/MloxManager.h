#pragma once
#include "ModManager.h" // For ContentFile struct
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

// A smart pattern that uses regex only when necessary for performance.
struct Pattern {
    std::string pattern_str;
    std::regex re;
    bool is_regex;

    // Constructor analyzes the pattern and decides which matching method to use.
    Pattern(const std::string& pattern);

    // The main matching function.
    bool match(const std::string& text) const;
};

// Rules now use the optimized Pattern struct
struct OrderRule {
    Pattern plugin_pattern;
    Pattern comes_after_pattern;
};

struct MessageRule {
    Pattern plugin_pattern;
    std::vector<std::string> messages;
};

struct PriorityRule {
    Pattern plugin_pattern;
    int line_number;
};


class MloxManager {
public:
    // Loads and parses the mlox rule files using a new lexer/parser
    bool load_rules(const std::vector<fs::path>& mlox_rules_paths);

    // The core function: sorts the given list of content files
    void sort_content_files(std::vector<ContentFile>& content_files, const std::vector<fs::path>& base_data_paths, const std::vector<fs::path>& mod_data_paths);

    // Retrieves any notes or warnings for a specific plugin
    std::vector<std::string> get_messages_for_plugin(const std::string& plugin_name) const;

    // Are any rules loaded?
    bool rules_loaded() { return (!order_rules.empty() || !near_start_plugins.empty() || !near_end_plugins.empty() || !message_rules.empty()); }

private:
    std::vector<OrderRule> order_rules;
    std::vector<MessageRule> message_rules;
    std::vector<PriorityRule> near_start_plugins;
    std::vector<PriorityRule> near_end_plugins;
};
