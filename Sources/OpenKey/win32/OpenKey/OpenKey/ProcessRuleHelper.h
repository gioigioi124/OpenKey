/*----------------------------------------------------------
OpenKey - Process & Window Title Rule Helper for Auto Encoding Switch
-----------------------------------------------------------*/
#pragma once
#include "stdafx.h"
#include <vector>
#include <string>

struct AppRuleItem {
    std::string processName;   // e.g. "excel.exe" or empty (matches any process)
    std::string titlePattern;  // e.g. "a" or empty (matches any title)
    int codeTable;             // 0: Unicode, 1: TCVN3, etc.
};

class ProcessRuleHelper {
private:
    static std::vector<AppRuleItem> _ruleItems;
    static bool _isInitialized;
    static std::wstring _iniFilePath;

    static std::string toLower(const std::string& str);
    static bool isDelimiter(char c);
    static bool matchesTitle(const std::string& title, const std::string& pattern);
    static void initFilePath();
    static void loadDefaultRules();
    static void createDefaultIniFile();

public:
    static void init();
    static void reloadRules();
    static int getCodeTableForProcessAndTitle(const std::string& exeName, const std::string& windowTitle);
    static int getCodeTableForProcess(const std::string& exeName);
    static std::wstring getCodeTableName(int code);
};
