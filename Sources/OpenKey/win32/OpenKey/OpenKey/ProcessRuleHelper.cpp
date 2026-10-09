/*----------------------------------------------------------
OpenKey - Process & Window Title Rule Helper for Auto Encoding Switch
-----------------------------------------------------------*/
#include "ProcessRuleHelper.h"
#include <fstream>
#include <sstream>
#include <algorithm>

std::vector<AppRuleItem> ProcessRuleHelper::_ruleItems;
bool ProcessRuleHelper::_isInitialized = false;
std::wstring ProcessRuleHelper::_iniFilePath = L"";

std::string ProcessRuleHelper::toLower(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        if (c >= 'A' && c <= 'Z') {
            c += ('a' - 'A');
        }
    }
    return result;
}

bool ProcessRuleHelper::isDelimiter(char c) {
    // Delimiters include space, dot, hyphen, underscore, brackets, quotes, etc.
    return !((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'));
}

bool ProcessRuleHelper::matchesTitle(const std::string& title, const std::string& pattern) {
    if (pattern.empty()) return true;
    if (title.empty()) return false;

    std::string lowerTitle = toLower(title);
    std::string lowerPattern = toLower(pattern);

    size_t pos = 0;
    while ((pos = lowerTitle.find(lowerPattern, pos)) != std::string::npos) {
        // Check character before match
        bool beforeOk = (pos == 0) || isDelimiter(lowerTitle[pos - 1]);

        // Check character after match
        size_t afterPos = pos + lowerPattern.length();
        bool afterOk = (afterPos == lowerTitle.length()) || isDelimiter(lowerTitle[afterPos]);

        if (beforeOk && afterOk) {
            return true;
        }

        pos += 1;
    }

    return false;
}

void ProcessRuleHelper::initFilePath() {
    LPTSTR execPath = OpenKeyHelper::getExecutePath();
    std::wstring pathStr(execPath);
    size_t lastSlash = pathStr.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        _iniFilePath = pathStr.substr(0, lastSlash + 1) + L"process_rules.ini";
    } else {
        _iniFilePath = L"process_rules.ini";
    }
}

void ProcessRuleHelper::loadDefaultRules() {
    _ruleItems.clear();
    // Default rules according to user requirements:
    _ruleItems.push_back({ "s.exe", "", 1 });         // s.exe -> TCVN3
    _ruleItems.push_back({ "excel.exe", "a", 1 });     // excel with file 'a' -> TCVN3
    _ruleItems.push_back({ "excel.exe", "b", 0 });     // excel with file 'b' -> Unicode
}

void ProcessRuleHelper::createDefaultIniFile() {
    std::ofstream outFile(_iniFilePath.c_str());
    if (!outFile.is_open()) return;

    outFile << "# ============================================================\n";
    outFile << "# OpenKey - Cau hinh tu dong nhan dien tien trinh & bang ma\n";
    outFile << "#\n";
    outFile << "# Cai dat khoa / bat tinh nang:\n";
    outFile << "#   enabled = 1               (1: Bat tu dong chuyen, 0: Khoa / Tat)\n";
    outFile << "#   fallback_to_unicode = 0   (1: Bat fallback ve Unicode khi roi app, 0: Giu nguyen bang ma)\n";
    outFile << "#\n";
    outFile << "# Dinh dang quy tac ho tro:\n";
    outFile << "# 1. Theo tien trinh:\n";
    outFile << "#      [ten_tien_trinh] = [bang_ma]\n";
    outFile << "#      Vi du: s.exe = TCVN3\n";
    outFile << "#\n";
    outFile << "# 2. Theo tien trinh + ten file / tieu de cua so:\n";
    outFile << "#      [ten_tien_trinh][ten_file] = [bang_ma]\n";
    outFile << "#      Vi du: excel.exe[a] = TCVN3\n";
    outFile << "#             excel.exe[b] = UNICODE\n";
    outFile << "#\n";
    outFile << "# 3. Theo tieu de / ten file chung:\n";
    outFile << "#      title:[ten_file] = [bang_ma]\n";
    outFile << "#      Vi du: title:a = TCVN3\n";
    outFile << "#             title:b = UNICODE\n";
    outFile << "#\n";
    outFile << "# Bang ma ho tro:\n";
    outFile << "#   0 hoac UNICODE          : Unicode dung san\n";
    outFile << "#   1 hoac TCVN3            : TCVN3 (ABC)\n";
    outFile << "#   2 hoac VNI              : VNI Windows\n";
    outFile << "#   3 hoac UNICODE_COMPOUND : Unicode to hop\n";
    outFile << "#   4 hoac VN_LOCALE_1258   : Vietnamese locale CP 1258\n";
    outFile << "#\n";
    outFile << "# Luu y: Neu fallback_to_unicode = 0, cac phan mem / file\n";
    outFile << "# KHONG co trong danh sach se GIU NGUYEN bang ma hien tai.\n";
    outFile << "# ============================================================\n\n";
    outFile << "enabled = 1\n";
    outFile << "fallback_to_unicode = 0\n\n";
    outFile << "s.exe = TCVN3\n";
    outFile << "excel.exe[a] = TCVN3\n";
    outFile << "excel.exe[b] = UNICODE\n";
    outFile.close();
}

static inline std::string trimString(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static int parseCodeTable(const std::string& valueStr) {
    std::string valUpper = valueStr;
    for (char& c : valUpper) {
        if (c >= 'a' && c <= 'z') c -= ('a' - 'A');
    }

    if (valUpper == "UNICODE" || valUpper == "0") return 0;
    if (valUpper == "TCVN3" || valUpper == "ABC" || valUpper == "1") return 1;
    if (valUpper == "VNI" || valUpper == "VNI_WINDOWS" || valUpper == "2") return 2;
    if (valUpper == "UNICODE_COMPOUND" || valUpper == "COMPOUND" || valUpper == "TO_HOP" || valUpper == "3") return 3;
    if (valUpper == "VN_LOCALE_1258" || valUpper == "1258" || valUpper == "4") return 4;

    return 0; // Default to Unicode if unknown
}

void ProcessRuleHelper::init() {
    if (_isInitialized) return;
    initFilePath();
    reloadRules();
    _isInitialized = true;
}

void ProcessRuleHelper::reloadRules() {
    loadDefaultRules();

    if (_iniFilePath.empty()) {
        initFilePath();
    }

    std::ifstream inFile(_iniFilePath.c_str());
    if (!inFile.is_open()) {
        createDefaultIniFile();
        return;
    }

    _ruleItems.clear();

    std::string line;
    while (std::getline(inFile, line)) {
        line = trimString(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string rawKey = trimString(line.substr(0, eqPos));
            std::string rawVal = trimString(line.substr(eqPos + 1));
            std::string lowerKey = toLower(rawKey);

            if (lowerKey == "enabled" || lowerKey == "enable_auto_switch") {
                std::string lowerVal = toLower(rawVal);
                if (lowerVal == "0" || lowerVal == "false" || lowerVal == "off" || lowerVal == "no") {
                    vAutoSwitchCodeTable = 0;
                } else {
                    vAutoSwitchCodeTable = 1;
                }
                continue;
            }

            if (lowerKey == "fallback_to_unicode" || lowerKey == "fallback_unicode" || lowerKey == "fallback") {
                std::string lowerVal = toLower(rawVal);
                if (lowerVal == "1" || lowerVal == "true" || lowerVal == "on" || lowerVal == "yes") {
                    vFallbackToUnicode = 1;
                } else {
                    vFallbackToUnicode = 0;
                }
                continue;
            }

            int code = parseCodeTable(rawVal);
            AppRuleItem item;
            item.codeTable = code;

            // Check if key is formatted as process[title]
            size_t bracketOpen = lowerKey.find('[');
            size_t bracketClose = lowerKey.find(']');
            if (bracketOpen != std::string::npos && bracketClose != std::string::npos && bracketClose > bracketOpen) {
                std::string prefix = trimString(lowerKey.substr(0, bracketOpen));
                std::string pattern = trimString(lowerKey.substr(bracketOpen + 1, bracketClose - bracketOpen - 1));
                item.processName = (prefix == "title") ? "" : prefix;
                item.titlePattern = pattern;
                _ruleItems.push_back(item);
            } else {
                size_t colonPos = lowerKey.find(':');
                if (colonPos != std::string::npos) {
                    std::string prefix = trimString(lowerKey.substr(0, colonPos));
                    std::string pattern = trimString(lowerKey.substr(colonPos + 1));
                    item.processName = (prefix == "title") ? "" : prefix;
                    item.titlePattern = pattern;
                    _ruleItems.push_back(item);
                } else if (lowerKey.find(".xls") != std::string::npos || lowerKey.find(".doc") != std::string::npos || lowerKey.find(".txt") != std::string::npos) {
                    item.processName = "";
                    item.titlePattern = lowerKey;
                    _ruleItems.push_back(item);
                } else {
                    item.processName = lowerKey;
                    item.titlePattern = "";
                    _ruleItems.push_back(item);
                }
            }
        }
    }
    inFile.close();
}

int ProcessRuleHelper::getCodeTableForProcessAndTitle(const std::string& exeName, const std::string& windowTitle) {
    if (!vAutoSwitchCodeTable) {
        return -1;
    }
    if (!_isInitialized) {
        init();
    }
    std::string lowerExe = toLower(exeName);
    std::string lowerTitle = toLower(windowTitle);

    // Pass 1: Rules matching BOTH specific processName AND titlePattern
    for (const auto& item : _ruleItems) {
        if (!item.processName.empty() && !item.titlePattern.empty()) {
            if (lowerExe == item.processName && matchesTitle(lowerTitle, item.titlePattern)) {
                return item.codeTable;
            }
        }
    }

    // Pass 2: Rules matching titlePattern only (processName is empty)
    for (const auto& item : _ruleItems) {
        if (item.processName.empty() && !item.titlePattern.empty()) {
            if (matchesTitle(lowerTitle, item.titlePattern)) {
                return item.codeTable;
            }
        }
    }

    // Pass 3: Rules matching processName only (titlePattern is empty)
    for (const auto& item : _ruleItems) {
        if (!item.processName.empty() && item.titlePattern.empty()) {
            if (lowerExe == item.processName) {
                return item.codeTable;
            }
        }
    }

    return -1; // No match -> Keep current code table!
}

int ProcessRuleHelper::getCodeTableForProcess(const std::string& exeName) {
    return getCodeTableForProcessAndTitle(exeName, "");
}

std::wstring ProcessRuleHelper::getCodeTableName(int code) {
    switch (code) {
    case 0: return L"Unicode";
    case 1: return L"TCVN3 (ABC)";
    case 2: return L"VNI Windows";
    case 3: return L"Unicode tổ hợp";
    case 4: return L"Vietnamese locale CP 1258";
    default: return L"Unicode";
    }
}
