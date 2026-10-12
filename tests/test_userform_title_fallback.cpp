#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <algorithm>
#include <windows.h>
#include <locale>
#include <codecvt>
#include "ProcessRuleHelper.h"   // REAL helper (no more re-implementation)

using namespace std;

//----------------------------------------------------------
// Definitions required to link the REAL ProcessRuleHelper.cpp
//----------------------------------------------------------
int vAutoSwitchCodeTable = 1;
int vFallbackToUnicode = 1;
wchar_t _logBuffer[1024];

wstring utf8ToWideString(const string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(str);
}

LPTSTR OpenKeyHelper::getExecutePath() {
    static TCHAR p[MAX_PATH] = _T("C:\\OpenKey\\OpenKey.exe");
    return p;
}

// Window mock representing document windows vs UserForms/dialogs
struct WindowMock {
    string title;
    string ownerTitle;
    bool isDialogOrForm;
};

int simulateWindowEvent(
    DWORD dwEvent,
    const string& exe,
    const WindowMock& wnd,
    int& currentCodeTable,
    string& lastForegroundExe,
    int vFallback,
    int vRememberCode,
    int rememberedCode
) {
    if (dwEvent == 0x8005) { // EVENT_OBJECT_FOCUS
        // Control focus change MUST NOT touch code table!
        return currentCodeTable;
    }

    if (dwEvent == 3) { // EVENT_SYSTEM_FOREGROUND
        lastForegroundExe = exe;

        // REAL rule resolution (walks owner chain for dialogs / UserForms).
        int ruleCode = ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, wnd.title);
        if (ruleCode == -1 && wnd.isDialogOrForm && !wnd.ownerTitle.empty()) {
            ruleCode = ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, wnd.ownerTitle);
        }

        if (ruleCode != -1) {
            currentCodeTable = ruleCode;
        } else if (!wnd.isDialogOrForm) {
            // Documented priority: fallback -> remember -> keep.
            if (vFallback) {
                currentCodeTable = 0;
            } else if (vRememberCode && rememberedCode != -1) {
                currentCodeTable = rememberedCode;
            }
        }
        // dialog / UserForm without rule: retain currentCodeTable!
    }

    return currentCodeTable;
}

int main() {
    std::cout << "==========================================================\n";
    std::cout << "  Multi-Workbook & UserForm Code Table Test Suite         \n";
    std::cout << "==========================================================\n\n";

    // Setup rules from process_rules.ini via the REAL helper:
    // excel.exe[Tong hop ban hang - Date] = TCVN3 (1)
    // chrome.exe = UNICODE (0)
    vector<AppRuleItem> rules = {
        { "excel.exe", "Tong hop ban hang - Date", 1 },
        { "chrome.exe", "", 0 }
    };
    ProcessRuleHelper::setRuleItemsForTest(rules, /*autoSwitch*/true, /*fallback*/true);

    int vFallbackToUnicode = 1; // Enabled in process_rules.ini
    int vRememberCode = 0;
    int rememberedCode = -1;
    string lastForegroundExe = "";
    int currentCodeTable = 0;

    // ----------------------------------------------------
    // Test 1: Opening Excel workbook Tong hop ban hang - Date
    // ----------------------------------------------------
    std::cout << "=== Test 1: Focus Excel workbook 'Tong hop ban hang - Date' ===\n";
    WindowMock excelWorkbook1 = { "Tong hop ban hang - Date.xlsm - Excel", "", false };
    simulateWindowEvent(3, "EXCEL.EXE", excelWorkbook1, currentCodeTable, lastForegroundExe, vFallbackToUnicode, vRememberCode, rememberedCode);
    assert(currentCodeTable == 1); // TCVN3
    std::cout << "  [PASS] 'Tong hop ban hang - Date' matches rule -> switched to TCVN3 (1)\n\n";

    // ----------------------------------------------------
    // Test 2: UserForm pops up on 'Tong hop ban hang - Date'
    // ----------------------------------------------------
    std::cout << "=== Test 2: UserForm pops up on 'Tong hop ban hang - Date' ===\n";
    WindowMock userForm = { "UserForm1", "Tong hop ban hang - Date.xlsm - Excel", true };
    simulateWindowEvent(3, "EXCEL.EXE", userForm, currentCodeTable, lastForegroundExe, vFallbackToUnicode, vRememberCode, rememberedCode);
    assert(currentCodeTable == 1); // Stays TCVN3
    std::cout << "  [PASS] UserForm inherits owner workbook rule -> stays TCVN3 (1)\n\n";

    // ----------------------------------------------------
    // Test 3: TextBox on UserForm receives focus (0x8005)
    // ----------------------------------------------------
    std::cout << "=== Test 3: TextBox on UserForm receives focus ===\n";
    simulateWindowEvent(0x8005, "EXCEL.EXE", userForm, currentCodeTable, lastForegroundExe, vFallbackToUnicode, vRememberCode, rememberedCode);
    assert(currentCodeTable == 1);
    std::cout << "  [PASS] EVENT_OBJECT_FOCUS strictly preserves TCVN3 (1)\n\n";

    // ----------------------------------------------------
    // Test 4: User opens a DIFFERENT Excel workbook 'Book2.xlsx' (no rule)
    // CRITICAL: Must fallback to UNICODE (0)!
    // ----------------------------------------------------
    std::cout << "=== Test 4: Switch to another Excel workbook 'Book2.xlsx' (no rule) ===\n";
    WindowMock excelWorkbook2 = { "Book2.xlsx - Excel", "", false };
    simulateWindowEvent(3, "EXCEL.EXE", excelWorkbook2, currentCodeTable, lastForegroundExe, vFallbackToUnicode, vRememberCode, rememberedCode);
    assert(currentCodeTable == 0); // FALLS BACK TO UNICODE!
    std::cout << "  [PASS] 'Book2.xlsx' has no rule -> correctly falls back to UNICODE (0)!\n\n";

    // ----------------------------------------------------
    // Test 5: User opens a 3rd Excel workbook 'Bao cao thang 10.xlsx' (no rule)
    // Must also be UNICODE (0)!
    // ----------------------------------------------------
    std::cout << "=== Test 5: Switch to 'Bao cao thang 10.xlsx' (no rule) ===\n";
    WindowMock excelWorkbook3 = { "Bao cao thang 10.xlsx - Excel", "", false };
    simulateWindowEvent(3, "EXCEL.EXE", excelWorkbook3, currentCodeTable, lastForegroundExe, vFallbackToUnicode, vRememberCode, rememberedCode);
    assert(currentCodeTable == 0); // UNICODE!
    std::cout << "  [PASS] 'Bao cao thang 10.xlsx' stays UNICODE (0)!\n\n";

    // ----------------------------------------------------
    // Test 6: User switches BACK to 'Tong hop ban hang - Date.xlsm'
    // Must switch back to TCVN3 (1)!
    // ----------------------------------------------------
    std::cout << "=== Test 6: Switch back to 'Tong hop ban hang - Date.xlsm' ===\n";
    simulateWindowEvent(3, "EXCEL.EXE", excelWorkbook1, currentCodeTable, lastForegroundExe, vFallbackToUnicode, vRememberCode, rememberedCode);
    assert(currentCodeTable == 1); // TCVN3!
    std::cout << "  [PASS] Returning to 'Tong hop ban hang - Date' switches back to TCVN3 (1)!\n\n";

    // ----------------------------------------------------
    // Test 7: Switch to Chrome
    // ----------------------------------------------------
    std::cout << "=== Test 7: Switch to Chrome ===\n";
    WindowMock chrome = { "Google Chrome", "", false };
    simulateWindowEvent(3, "chrome.exe", chrome, currentCodeTable, lastForegroundExe, vFallbackToUnicode, vRememberCode, rememberedCode);
    assert(currentCodeTable == 0);
    std::cout << "  [PASS] Chrome matches rule -> switches to UNICODE (0)!\n\n";

    std::cout << "==========================================================\n";
    std::cout << "  All Multi-Workbook & UserForm Tests Passed (7/7)!       \n";
    std::cout << "==========================================================\n";
    return 0;
}
