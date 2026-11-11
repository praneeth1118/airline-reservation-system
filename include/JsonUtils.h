#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>

namespace JsonUtils {

inline std::string escapeJson(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < ' ') {
                    // Control character
                    ss << "\\u00" << (c < 16 ? "0" : "") << std::hex << (int)c;
                } else {
                    ss << c;
                }
        }
    }
    return ss.str();
}

inline std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

// Simple key-value extractor for JSON payloads
inline std::string extractField(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::string::npos) return "";

    size_t colonPos = json.find(':', keyPos + searchKey.length());
    if (colonPos == std::string::npos) return "";

    size_t valStart = json.find_first_not_of(" \t\r\n", colonPos + 1);
    if (valStart == std::string::npos) return "";

    if (json[valStart] == '"') {
        // String value
        std::string result;
        bool escaping = false;
        for (size_t i = valStart + 1; i < json.length(); ++i) {
            char c = json[i];
            if (escaping) {
                result += c;
                escaping = false;
            } else if (c == '\\') {
                escaping = true;
            } else if (c == '"') {
                return result;
            } else {
                result += c;
            }
        }
        return result;
    } else {
        // Number, boolean, or literal
        size_t valEnd = json.find_first_of(",}\n\r", valStart);
        if (valEnd == std::string::npos) valEnd = json.length();
        return trim(json.substr(valStart, valEnd - valStart));
    }
}

inline int getInt(const std::string& json, const std::string& key, int defaultVal = 0) {
    std::string val = extractField(json, key);
    if (val.empty()) return defaultVal;
    try {
        return std::stoi(val);
    } catch (...) {
        return defaultVal;
    }
}

inline double getDouble(const std::string& json, const std::string& key, double defaultVal = 0.0) {
    std::string val = extractField(json, key);
    if (val.empty()) return defaultVal;
    try {
        return std::stod(val);
    } catch (...) {
        return defaultVal;
    }
}

inline std::string getString(const std::string& json, const std::string& key, const std::string& defaultVal = "") {
    std::string val = extractField(json, key);
    return val.empty() ? defaultVal : val;
}

} // namespace JsonUtils

#endif // JSON_UTILS_H

