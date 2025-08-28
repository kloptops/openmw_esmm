#include "MloxManager.h"
#include "PluginParser.h"
#include "../utils/Logger.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
// No string_view
#include <sstream>

// ============================================================================
// REGEX AND PATTERN UTILITIES
// ============================================================================
static std::regex patternToRegex(const std::string& pattern) {
    std::string regex_str = "^"; // ANCHOR TO START
    regex_str.reserve(pattern.length() * 2 + 2);

    for (size_t i = 0; i < pattern.length(); ++i) {
        char c = pattern[i];
        switch (c) {
            case '*': regex_str += ".*"; break;
            case '?': regex_str += "."; break;
            case '.': case '+': case '(': case ')': case '{': case '}':
            case '[': case ']': case '\\': case '^': case '$': case '|':
                regex_str += '\\';
                regex_str += c;
                break;
            case '<':
                if (i + 4 < pattern.length() && pattern.substr(i, 5) == "<VER>") {
                    regex_str += "(\\d+(?:[_.-]?\\d+)*[a-z]?)";
                    i += 4;
                } else {
                    regex_str += c;
                }
                break;
            default:
                regex_str += c;
                break;
        }
    }
    regex_str += "$"; // ANCHOR TO END
    try {
        return std::regex(regex_str, std::regex::icase);
    } catch (const std::regex_error& e) {
        LOG_WARN("Failed to compile regex from pattern '", pattern, "': ", e.what());
        return std::regex("^$"); // Match only empty strings on error
    }
}

// ============================================================================
// The new optimized Pattern struct implementation
// ============================================================================

// Helper for case-insensitive string comparison
static bool iequals(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) {
        return false;
    }
    return std::equal(a.begin(), a.end(), b.begin(), b.end(),
                      [](char c1, char c2) {
                          return std::tolower(c1) == std::tolower(c2);
                      });
}

Pattern::Pattern(const std::string& pattern) : pattern_str(pattern) {
    if (pattern.find('*') != std::string::npos ||
        pattern.find('?') != std::string::npos ||
        pattern.find("<VER>") != std::string::npos) {
        is_regex = true;
        re = patternToRegex(pattern);
    } else {
        is_regex = false;
        // The simple string is stored in pattern_str
    }
}

bool Pattern::match(const std::string& text) const {
    if (is_regex) {
        return std::regex_match(text, re);
    } else {
        return iequals(text, pattern_str);
    }
}


// ============================================================================
// LEXER: Breaks the mlox file into a stream of tokens (NEWEST, FAITHFUL PORT)
// ============================================================================
namespace MloxLexer {
    enum TokenType {
        PREDICATE, MESSAGE, NEWLINE, END,
        RULE_ORDER, RULE_NEARSTART, RULE_NEAREND,
        RULE_NOTE, RULE_CONFLICT, RULE_REQUIRES, RULE_PATCH,
        RULE_ANY, RULE_ALL, RULE_NOT,
        RULE_VER, RULE_DESC, RULE_SIZE // Added data rules
    };

    struct Token {
        TokenType type;
        std::string value;
        int line_number;
    };

    static const char* TokenTypeToString(TokenType t) {
        switch (t) {
            case PREDICATE: return "PREDICATE";
            case MESSAGE: return "MESSAGE";
            case NEWLINE: return "NEWLINE";
            case END: return "END";
            case RULE_ORDER: return "ORDER";
            case RULE_NEARSTART: return "NEARSTART";
            case RULE_NEAREND: return "NEAREND";
            case RULE_NOTE: return "NOTE";
            case RULE_CONFLICT: return "CONFLICT";
            case RULE_REQUIRES: return "REQUIRES";
            case RULE_PATCH: return "PATCH";
            case RULE_ANY: return "ANY";
            case RULE_ALL: return "ALL";
            case RULE_NOT: return "NOT";
            case RULE_VER: return "VER";
            case RULE_DESC: return "DESC";
            case RULE_SIZE: return "SIZE";
            default: return "UNKNOWN";
        }
    }

    // (trim helper is unchanged)
    static void trim(std::string& s) {
        if (s.empty()) return;
        const char* whitespace = " \t\r\n";
        s.erase(0, s.find_first_not_of(whitespace));
        s.erase(s.find_last_not_of(whitespace) + 1);
    }

    // C++ port of your _eat_rule function
    static bool eat_rule(std::string& line, TokenType& out_type, std::string& out_value) {
        if (line.empty() || line[0] != '[') return false;

        std::string lower_line = line;
        std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(), ::tolower);

        // Simple [key]
        const char* simple_keys[] = {"conflict", "requires", "note", "order", "nearstart", "nearend", "patch"};
        for(const char* key : simple_keys) {
            std::string rule = "[" + std::string(key) + "]";
            if (lower_line.rfind(rule, 0) == 0) {
                line = line.substr(rule.length());
                if (strcmp(key, "conflict") == 0) out_type = RULE_CONFLICT;
                else if (strcmp(key, "requires") == 0) out_type = RULE_REQUIRES;
                else if (strcmp(key, "note") == 0) out_type = RULE_NOTE;
                else if (strcmp(key, "order") == 0) out_type = RULE_ORDER;
                else if (strcmp(key, "nearstart") == 0) out_type = RULE_NEARSTART;
                else if (strcmp(key, "nearend") == 0) out_type = RULE_NEAREND;
                else if (strcmp(key, "patch") == 0) out_type = RULE_PATCH;
                out_value = "";
                return true;
            }
        }
        
        // Boolean [key
        const char* bool_keys[] = {"any", "all", "not"};
        for(const char* key : bool_keys) {
            std::string rule = "[" + std::string(key);
            if(lower_line.rfind(rule, 0) == 0) {
                line = line.substr(rule.length());
                if (strcmp(key, "any") == 0) out_type = RULE_ANY;
                else if (strcmp(key, "all") == 0) out_type = RULE_ALL;
                else if (strcmp(key, "not") == 0) out_type = RULE_NOT;
                out_value = "";
                return true;
            }
        }

        // Data [key data]
        const char* data_keys[] = {"ver", "desc", "size"};
        for(const char* key : data_keys) {
            std::string rule = "[" + std::string(key);
            if (lower_line.rfind(rule, 0) == 0) {
                size_t end_bracket = line.find(']');
                if(end_bracket != std::string::npos && end_bracket > rule.length()) {
                    out_value = line.substr(rule.length(), end_bracket - rule.length());
                    trim(out_value);
                    line = line.substr(end_bracket + 1);
                    if (strcmp(key, "ver") == 0) out_type = RULE_VER;
                    else if (strcmp(key, "desc") == 0) out_type = RULE_DESC;
                    else if (strcmp(key, "size") == 0) out_type = RULE_SIZE;
                    return true;
                }
            }
        }

        return false; // Not a rule we handle
    }

    std::vector<Token> tokenize(const std::string& content) {
        std::vector<Token> tokens;
        std::stringstream ss(content);
        std::string line;
        int line_number = 0;
        TokenType last_emitted_token_type = NEWLINE;
        bool want_messages = false;

        while(std::getline(ss, line)) {
            line_number++;
            size_t comment_pos = line.find(';');
            if (comment_pos != std::string::npos) line = line.substr(0, comment_pos);
            
            std::string work_line = line;
            trim(work_line);
            if (work_line.empty()) {
                if(last_emitted_token_type != NEWLINE) {
                    tokens.push_back(Token{NEWLINE, "", line_number});
                    last_emitted_token_type = NEWLINE;
                }
                want_messages = false;
                continue;
            }
            
            if (want_messages && (line[0] == ' ' || line[0] == '\t')) {
                tokens.push_back(Token{MESSAGE, work_line, line_number});
                last_emitted_token_type = MESSAGE;
                continue;
            }
            want_messages = false;

            while(!work_line.empty()) {
                trim(work_line);
                if (work_line.empty()) break;
                
                TokenType rule_type;
                std::string rule_value;
                if(eat_rule(work_line, rule_type, rule_value)) {
                    if (last_emitted_token_type != NEWLINE && (rule_type >= RULE_ORDER && rule_type <= RULE_PATCH)) {
                         tokens.push_back(Token{NEWLINE, "", line_number});
                    }
                    tokens.push_back(Token{rule_type, rule_value, line_number});
                    last_emitted_token_type = rule_type;
                    if (rule_type >= RULE_NOTE && rule_type <= RULE_PATCH) want_messages = true;
                } else {
                    // It must be a predicate
                    std::string predicate = work_line;
                    work_line = "";

                    int end_count = 0;
                    while(!predicate.empty() && predicate.back() == ']') {
                        predicate.pop_back();
                        end_count++;
                    }
                    trim(predicate);

                    if(!predicate.empty()) {
                        tokens.push_back(Token{PREDICATE, predicate, line_number});
                        last_emitted_token_type = PREDICATE;
                    }

                    for(int i = 0; i < end_count; ++i) {
                        tokens.push_back(Token{END, "", line_number});
                        last_emitted_token_type = END;
                    }
                }
            }
        }

        if(last_emitted_token_type != NEWLINE) {
            tokens.push_back(Token{NEWLINE, "", line_number + 1});
        }
        return tokens;
    }

} // namespace MloxLexer


// ============================================================================
// PARSER and SORTER IMPLEMENTATION
// ============================================================================
bool MloxManager::load_rules(const std::vector<fs::path>& mlox_rules_paths) {
    order_rules.clear(); message_rules.clear(); near_start_plugins.clear(); near_end_plugins.clear();
    std::string full_content;
    bool file_opened_successfully = false;
    for (const auto& path : mlox_rules_paths) {
        std::ifstream file(path.string());
        if (!file.is_open()) continue;
        file_opened_successfully = true;
        LOG_INFO("Loading mlox rules from ", path.string());
        full_content += std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        full_content += "\n";
    }

    if (!file_opened_successfully)
        return false;

    // Normalize all line endings (CRLF, CR) to LF before tokenizing.
    full_content.erase(std::remove(full_content.begin(), full_content.end(), '\r'), full_content.end());

    // Ensure the content ends with a newline for the lexer.
    if (full_content.empty() || full_content.back() != '\n') {
        full_content += '\n';
    }


    auto tokens = MloxLexer::tokenize(full_content);

    // ---- DEBUG TOKEN DUMP ----
    if (WANT_DEBUG) {
        for(const auto& token : tokens) {
            std::cout << "(" << token.line_number << ", " << MloxLexer::TokenTypeToString(token.type) << ", '" << token.value << "')" << std::endl;
            if (token.type == MloxLexer::NEWLINE) std::cout << std::endl;
        }
    }
    // ---- END DEBUG ----

    MloxLexer::TokenType current_rule_type = MloxLexer::NEWLINE;
    std::string last_predicate_for_order;
    std::vector<std::string> current_messages;

    for(const auto& token : tokens) {
        switch(token.type) {
            case MloxLexer::RULE_ORDER:
            case MloxLexer::RULE_NEARSTART:
            case MloxLexer::RULE_NEAREND:
            case MloxLexer::RULE_NOTE:
            case MloxLexer::RULE_CONFLICT:
            case MloxLexer::RULE_REQUIRES:
            case MloxLexer::RULE_PATCH:
                current_rule_type = token.type;
                break;
            case MloxLexer::MESSAGE:
                current_messages.push_back(token.value);
                break;
            case MloxLexer::PREDICATE: {
                const std::string& predicate_str = token.value;
                if (predicate_str.empty()) continue;
                switch(current_rule_type) {
                    case MloxLexer::RULE_ORDER:
                        if (!last_predicate_for_order.empty() && last_predicate_for_order != predicate_str) {
                            // CONSTRUCT THE PATTERN OBJECTS HERE
                            order_rules.push_back(OrderRule{Pattern(predicate_str), Pattern(last_predicate_for_order)});
                        }
                        last_predicate_for_order = predicate_str;
                        break;
                    case MloxLexer::RULE_NEARSTART:
                        near_start_plugins.push_back(PriorityRule{Pattern(predicate_str), token.line_number});
                        break;
                    case MloxLexer::RULE_NEAREND:
                        near_end_plugins.push_back(PriorityRule{Pattern(predicate_str), token.line_number});
                        break;
                    case MloxLexer::RULE_NOTE:
                    case MloxLexer::RULE_CONFLICT:
                    case MloxLexer::RULE_REQUIRES:
                    case MloxLexer::RULE_PATCH:
                        if (!current_messages.empty()) {
                            message_rules.push_back(MessageRule{Pattern(predicate_str), current_messages});
                        }
                        current_messages.clear();
                        break;
                    default: break;
                }
                break;
            }
            case MloxLexer::NEWLINE:
                last_predicate_for_order.clear();
                current_messages.clear();
                current_rule_type = MloxLexer::NEWLINE;
                break;
            default:
                break;
        }
    }

    LOG_INFO("Successfully loaded ", order_rules.size(), " mlox ordering rules and ", message_rules.size(), " message rules.");
    return true;
}

// ... the rest of the file (sorter and message getter) is unchanged from the last good version ...
void MloxManager::sort_content_files(std::vector<ContentFile>& content_files, const std::vector<fs::path>& base_data_paths, const std::vector<fs::path>& mod_data_paths) {
    if (order_rules.empty() && near_start_plugins.empty() && near_end_plugins.empty()) {
        LOG_WARN("No mlox rules loaded, skipping sort.");
        return;
    }
    LOG_INFO("Starting mlox auto-sort process...");

    // --- Hard Dependencies (from headers) ---
    std::map<std::string, std::vector<std::string>> header_dependencies;
    PluginParser parser;
    std::vector<fs::path> all_search_paths = base_data_paths;
    all_search_paths.insert(all_search_paths.end(), mod_data_paths.begin(), mod_data_paths.end());

    std::map<std::string, fs::path> plugin_locations;
    for (const auto& cf : content_files) {
         for (auto it = all_search_paths.rbegin(); it != all_search_paths.rend(); ++it) {
            fs::path potential_path = *it / cf.name;
            if (fs::exists(potential_path)) {
                plugin_locations[cf.name] = potential_path;
                break;
            }
        }
    }

    for (const auto& cf : content_files) {
        if (plugin_locations.count(cf.name)) {
            header_dependencies[cf.name] = parser.get_masters(plugin_locations[cf.name]);
        }
    }

    // --- Iterative Sorter ---
    for (int i = 0; i < 200; ++i) {
        bool swapped_this_pass = false;
        // We compare every plugin (k) with every plugin that comes after it (j)
        for (size_t k = 0; k < content_files.size(); ++k) {
            for (size_t j = k + 1; j < content_files.size(); ++j) {
                const auto& plugin_k = content_files[k]; // The plugin earlier in the list
                const auto& plugin_j = content_files[j]; // The plugin later in the list

                bool should_swap = false;

                // --- LOGIC CORRECTION 1: Header Dependencies ---
                // The current order is (k, j). It's WRONG if j is a master for k.
                // We must swap them to put the master (j) first.
                const auto& masters_of_k = header_dependencies[plugin_k.name];
                if (std::find(masters_of_k.begin(), masters_of_k.end(), plugin_j.name) != masters_of_k.end()) {
                    should_swap = true;
                }

                // --- LOGIC CORRECTION 2: Mlox Rules ---
                // The rule is "plugin_pattern" must come AFTER "comes_after_pattern".
                // The current order is (k, j). It's WRONG if a rule says "k must come after j".
                if (!should_swap) {
                    for (const auto& rule : order_rules) {
                        // Condition for violation: k matches the "after" part, and j matches the "before" part.
                        if (rule.plugin_pattern.match(plugin_k.name) && rule.comes_after_pattern.match(plugin_j.name)) {
                           should_swap = true;
                           break;
                        }
                    }
                }

                if (should_swap) {
                    std::swap(content_files[j], content_files[k]);
                    swapped_this_pass = true;
                }
            }
        }
        
        if (!swapped_this_pass) {
            LOG_DEBUG("  -> List is stable after ", i + 1, " passes.");
            break;
        }
        if (i == 199) {
            LOG_WARN("mlox sort did not stabilize after 200 passes. There may be a dependency cycle.");
        }
    }

    // --- NearEnd and NearStart application ---
    for (const auto& rule : near_end_plugins) {
        std::stable_sort(content_files.begin(), content_files.end(), [&](const ContentFile& a, const ContentFile& b){
            bool a_matches = rule.plugin_pattern.match(a.name);
            bool b_matches = rule.plugin_pattern.match(b.name);
            return b_matches && !a_matches;
        });
    }
    for (const auto& rule : near_start_plugins) {
        std::stable_sort(content_files.begin(), content_files.end(), [&](const ContentFile& a, const ContentFile& b){
            bool a_matches = rule.plugin_pattern.match(a.name);
            bool b_matches = rule.plugin_pattern.match(b.name);
            return a_matches && !b_matches;
        });
    }

    // --- ESM Heuristic Pass ---
    std::stable_sort(content_files.begin(), content_files.end(), [](const ContentFile& a, const ContentFile& b){
        bool a_is_esm = a.name.size() > 4 && (a.name.substr(a.name.size() - 4) == ".esm" || a.name.substr(a.name.size() - 4) == ".ESM");
        bool b_is_esm = b.name.size() > 4 && (b.name.substr(b.name.size() - 4) == ".esm" || b.name.substr(b.name.size() - 4) == ".ESM");
        return a_is_esm && !b_is_esm;
    });

    // --- Final guarantee for official masters ---
    auto force_to_top = [&](const std::string& name) {
        auto it = std::find_if(content_files.begin(), content_files.end(), [&](const ContentFile& cf){
            return cf.name == name;
        });
        if (it != content_files.end()) {
            ContentFile master = *it;
            content_files.erase(it);
            content_files.insert(content_files.begin(), master);
        }
    };
    force_to_top("Bloodmoon.esm");
    force_to_top("Tribunal.esm");
    force_to_top("Morrowind.esm");

    LOG_INFO("Mlox auto-sort complete.");
}


std::vector<std::string> MloxManager::get_messages_for_plugin(const std::string& plugin_name) const {
    for(const auto& rule : message_rules) {
        if(rule.plugin_pattern.match(plugin_name)) {
            return rule.messages;
        }
    }
    return {};
}
