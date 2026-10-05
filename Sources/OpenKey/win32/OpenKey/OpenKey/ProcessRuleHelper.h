/*----------------------------------------------------------
OpenKey - Process Rule Helper for Auto Encoding Switch
-----------------------------------------------------------*/
#pragma once
#include "stdafx.h"
#include <map>
#include <string>

class ProcessRuleHelper {
private:
    static std::map<std::string, int> _rules;
    static bool _isInitialized;
    static std::wstring _iniFilePath;

    static std::string toLower(const std::string& str);
    static void initFilePath();
    static void loadDefaultRules();
    static void createDefaultIniFile();

public:
    static void init();
    static void reloadRules();
    static int getCodeTableForProcess(const std::string& exeName);
    static std::wstring getCodeTableName(int code);
};
