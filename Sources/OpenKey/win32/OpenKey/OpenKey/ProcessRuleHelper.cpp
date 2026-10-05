/*----------------------------------------------------------
OpenKey - Process Rule Helper for Auto Encoding Switch
-----------------------------------------------------------*/
#include "ProcessRuleHelper.h"
#include <fstream>
#include <sstream>
#include <algorithm>

std::map<std::string, int> ProcessRuleHelper::_rules;
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
    _rules.clear();
    // Default rules according to user requirements
    _rules["s.exe"] = 1;        // TCVN3 (ABC)
    _rules["chrome.exe"] = 0;   // Unicode
    _rules["zalo.exe"] = 0;     // Unicode
}

void ProcessRuleHelper::createDefaultIniFile() {
    std::ofstream outFile(_iniFilePath.c_str());
    if (!outFile.is_open()) return;

    outFile << "# ============================================================\n";
    outFile << "# OpenKey - Cau hinh tu dong nhan dien tien trinh & bang ma\n";
    outFile << "# Dinh dang: [ten_tien_trinh] = [bang_ma]\n";
    outFile << "# Bang ma ho tro:\n";
    outFile << "#   0 hoac UNICODE          : Unicode dung san\n";
    outFile << "#   1 hoac TCVN3            : TCVN3 (ABC)\n";
    outFile << "#   2 hoac VNI              : VNI Windows\n";
    outFile << "#   3 hoac UNICODE_COMPOUND : Unicode to hop\n";
    outFile << "#   4 hoac VN_LOCALE_1258   : Vietnamese locale CP 1258\n";
    outFile << "# ============================================================\n\n";
    outFile << "s.exe = TCVN3\n";
    outFile << "chrome.exe = UNICODE\n";
    outFile << "zalo.exe = UNICODE\n";
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

    std::string line;
    while (std::getline(inFile, line)) {
        line = trimString(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string procName = trimString(line.substr(0, eqPos));
            std::string codeStr = trimString(line.substr(eqPos + 1));
            if (!procName.empty()) {
                _rules[toLower(procName)] = parseCodeTable(codeStr);
            }
        }
    }
    inFile.close();
}

int ProcessRuleHelper::getCodeTableForProcess(const std::string& exeName) {
    if (!_isInitialized) {
        init();
    }
    std::string lowerName = toLower(exeName);
    auto it = _rules.find(lowerName);
    if (it != _rules.end()) {
        return it->second;
    }
    return -1; // Not in rules
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
