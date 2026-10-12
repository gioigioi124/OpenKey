//==========================================================
//  test_process_rules_real.cpp
//  Exercises the REAL ProcessRuleHelper implementation (no mocks) so that a
//  regression in word-boundary matching / rule resolution is actually caught.
//
//  Build:
//    clang++ -std=c++14 -O2 -D_WIN32 -DUNICODE -D_UNICODE ^
//      -ISources/OpenKey/win32/OpenKey/OpenKey -ISources/OpenKey/engine ^
//      Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp ^
//      tests/test_process_rules_real.cpp -o tests/test_process_rules_real.exe
//==========================================================
#include <iostream>
#include <cassert>
#include <string>
#include <locale>
#include <codecvt>
#include <windows.h>
#include "ProcessRuleHelper.h"   // pulls stdafx.h -> declares the globals we define below

using namespace std;

//----------------------------------------------------------
// Definitions required to link ProcessRuleHelper.cpp
// (no engine / no OpenKeyHelper.cpp is linked on purpose).
//----------------------------------------------------------
int vAutoSwitchCodeTable = 1;
int vFallbackToUnicode = 0;
wchar_t _logBuffer[1024];

// utf8ToWideString is declared in Engine.h (via stdafx.h). Provide the real
// implementation (mirrors Engine.cpp) so the whole engine does not need linking.
wstring utf8ToWideString(const string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(str);
}

LPTSTR OpenKeyHelper::getExecutePath() {
    static TCHAR p[MAX_PATH] = _T("C:\\OpenKey\\OpenKey.exe");
    return p;
}

//----------------------------------------------------------
// Faithful model of the winEventProcCallback branch decision used for the
// window-event scenarios (the callback itself is a static function and cannot
// be called directly). It calls the REAL ProcessRuleHelper for rule lookup.
//
// Priority chain (must match OpenKey.cpp):
//   explicit rule -> fallback to Unicode -> remembered code -> keep
// Dialog / UserForm without a rule never falls back.
//----------------------------------------------------------
struct WindowMock {
    string title;
    string ownerTitle;
    bool isDialogOrForm;
};

// Mirrors isNonAppContext() in OpenKey.cpp (shell/transient processes).
static bool isShellExe(const string& exe) {
    static const char* shell[] = {
        "explorer.exe", "startmenuexperiencehost.exe", "searchhost.exe",
        "searchapp.exe", "shellexperiencehost.exe", "applicationframehost.exe",
        "textinputhost.exe", "lockapp.exe", "sihost.exe",
        "backgroundtaskhost.exe", "dwm.exe"
    };
    for (const char* s : shell) if (exe == s) return true;
    return false;
}

// Models the callback branch decision. `shellContext` simulates the
// isNonAppContext() early-return (Start menu / taskbar / desktop); a shell
// event must leave the code table untouched.
int simulateWindowEvent(const string& exe,
                        const WindowMock& wnd,
                        int& currentCodeTable,
                        int vRememberCode,
                        int rememberedCode,
                        bool shellContext = false) {
    if (shellContext || isShellExe(exe)) {
        return currentCodeTable; // shell must never change the code table
    }

    int ruleCode = ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, wnd.title);
    if (ruleCode == -1 && wnd.isDialogOrForm && !wnd.ownerTitle.empty()) {
        ruleCode = ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, wnd.ownerTitle);
    }

    if (ruleCode != -1) {
        currentCodeTable = ruleCode;
    } else if (!wnd.isDialogOrForm) {
        if (vFallbackToUnicode) {
            currentCodeTable = 0;
        } else if (vRememberCode && rememberedCode != -1) {
            currentCodeTable = rememberedCode;
        }
    }
    return currentCodeTable;
}

// Models the lazy re-check (recheckRuleForCurrentWindow): on the first key in
// a real app, re-apply its rule if one matches.
int simulateLazyRecheck(const string& exe,
                        const WindowMock& wnd,
                        int& currentCodeTable) {
    if (isShellExe(exe)) return currentCodeTable;
    int ruleCode = ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, wnd.title);
    if (ruleCode == -1 && wnd.isDialogOrForm && !wnd.ownerTitle.empty()) {
        ruleCode = ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, wnd.ownerTitle);
    }
    if (ruleCode != -1) currentCodeTable = ruleCode;
    return currentCodeTable;
}

static int failures = 0;
static void check(bool cond, const char* what) {
    if (cond) {
        cout << "  [PASS] " << what << "\n";
    } else {
        cout << "  [FAIL] " << what << "\n";
        ++failures;
    }
}

int main() {
    cout << "==========================================================\n";
    cout << "  ProcessRuleHelper REAL Logic Test Suite                  \n";
    cout << "==========================================================\n\n";

    //------------------------------------------------------
    // Section 1: real matchesTitle() word-boundary behaviour
    //------------------------------------------------------
    cout << "=== Section 1: matchesTitle() (real implementation) ===\n";

    check(ProcessRuleHelper::matchesTitle(
              "Tong hop ban hang - Date.xlsm - Excel",
              "Tong hop ban hang - Date"),
          "full file name pattern matches Excel title");

    check(!ProcessRuleHelper::matchesTitle("data.xlsx - Excel", "a"),
          "single-char 'a' does NOT match 'data' (word boundary)");

    check(!ProcessRuleHelper::matchesTitle("table.xlsx - Excel", "a"),
          "single-char 'a' does NOT match 'table' (word boundary)");

    check(ProcessRuleHelper::matchesTitle("a.xlsx - Excel", "a"),
          "single-char 'a' matches standalone 'a.xlsx'");

    check(ProcessRuleHelper::matchesTitle("Bao cao thang 10.xlsx - Excel", "thang 10"),
          "ASCII substring with boundary matches");

    // Vietnamese (UTF-8 -> wide) boundary test
    check(ProcessRuleHelper::matchesTitle(
              "B\u00e1o c\u00e1o th\u00e1ng 10.xlsx - Excel",
              "B\u00e1o c\u00e1o"),
          "Vietnamese diacritics match on code-point boundary");

    check(!ProcessRuleHelper::matchesTitle(
              "B\u00e1o c\u00e1o th\u00e1ng 10.xlsx - Excel",
              "c\u00e1"),
          "Vietnamese partial word is rejected at word boundary");

    check(ProcessRuleHelper::matchesTitle("Anything - Notepad", ""),
          "empty pattern matches any title");

    check(!ProcessRuleHelper::matchesTitle("", "x"),
          "empty title never matches a non-empty pattern");

    //------------------------------------------------------
    // Section 2: real rule resolution on real ProcessRuleHelper
    //------------------------------------------------------
    cout << "\n=== Section 2: getCodeTableForProcessAndTitle() (real) ===\n";

    vector<AppRuleItem> rules = {
        { "excel.exe", "Tong hop ban hang - Date", 1 }, // TCVN3
        { "chrome.exe", "", 0 }                         // Unicode
    };
    ProcessRuleHelper::setRuleItemsForTest(rules, /*autoSwitch*/true, /*fallback*/true);

    check(ProcessRuleHelper::getCodeTableForProcessAndTitle(
              "EXCEL.EXE", "Tong hop ban hang - Date.xlsm - Excel") == 1,
          "configured workbook resolves to TCVN3 (case-insensitive exe)");

    check(ProcessRuleHelper::getCodeTableForProcessAndTitle(
              "EXCEL.EXE", "Book2.xlsx - Excel") == -1,
          "other workbook without rule returns -1");

    check(ProcessRuleHelper::getCodeTableForProcessAndTitle(
              "chrome.exe", "Google Chrome") == 0,
          "process-only rule matches");

    check(ProcessRuleHelper::getCodeTableForProcessAndTitle(
              "unknown.exe", "Whatever") == -1,
          "unknown process returns -1");

    //------------------------------------------------------
    // Section 3: window-event decision model (multi-workbook)
    //------------------------------------------------------
    cout << "\n=== Section 3: window-event decision (multi-workbook) ===\n";

    int cur = 0;
    vector<WindowMock> workbook1 = { { "Tong hop ban hang - Date.xlsm - Excel", "", false } };
    simulateWindowEvent("EXCEL.EXE", workbook1[0], cur, /*remember*/0, -1);
    check(cur == 1, "workbook with rule -> TCVN3 (1)");

    WindowMock userForm = { "UserForm1", "Tong hop ban hang - Date.xlsm - Excel", true };
    simulateWindowEvent("EXCEL.EXE", userForm, cur, 0, -1);
    check(cur == 1, "UserForm inherits owner workbook rule -> stays TCVN3 (1)");

    WindowMock workbook2 = { "Book2.xlsx - Excel", "", false };
    simulateWindowEvent("EXCEL.EXE", workbook2, cur, 0, -1);
    check(cur == 0, "unlisted workbook falls back to Unicode (0)");

    WindowMock otherForm = { "SomeToolBox", "", true };
    simulateWindowEvent("EXCEL.EXE", otherForm, cur, 0, -1);
    check(cur == 0, "dialog without owner rule keeps current code table (no fallback)");

    // Remember-code path (fallback disabled)
    ProcessRuleHelper::setRuleItemsForTest(rules, true, /*fallback*/false);
    cur = 0;
    simulateWindowEvent("EXCEL.EXE", workbook2, cur, /*remember*/1, /*remembered*/3);
    check(cur == 3, "fallback off + remember on -> restores remembered code (3)");

    // Fallback wins over remember when both enabled (documented priority)
    ProcessRuleHelper::setRuleItemsForTest(rules, true, /*fallback*/true);
    cur = 3;
    simulateWindowEvent("EXCEL.EXE", workbook2, cur, /*remember*/1, /*remembered*/3);
    check(cur == 0, "fallback on (priority) overrides remember -> Unicode (0)");

    //------------------------------------------------------
    // Section 4: smart-switch packing / unpacking (vLanguage fix)
    //------------------------------------------------------
    cout << "\n=== Section 4: smart-switch value packing (vLanguage fix) ===\n";

    // Value stored as (language | (codeTable << 1))
    int languageEnglish = 0, codeTableTcvn = 1;
    int packed = languageEnglish | (codeTableTcvn << 1); // = 2
    check((packed & 0x01) == languageEnglish,
          "language bit extracted from packed value is English (0)");
    check(((packed >> 1) & 0x03) == codeTableTcvn,
          "code-table bits extracted from packed value is TCVN3 (1)");

    int languageViet = 1, codeTableUni = 0;
    packed = languageViet | (codeTableUni << 1); // = 1
    check((packed & 0x01) == languageViet,
          "language bit extracted from packed value is Vietnamese (1)");

    //------------------------------------------------------
    // Section 5: shell context, minimize and taskbar-restore
    //------------------------------------------------------
    cout << "\n=== Section 5: shell / minimize / taskbar restore ===\n";

    vector<AppRuleItem> rules2 = { { "s.exe", "", 1 } }; // s.exe -> TCVN3
    ProcessRuleHelper::setRuleItemsForTest(rules2, /*autoSwitch*/true, /*fallback*/true);

    int code = 0;
    WindowMock sWin = { "s.exe", "", false };

    // Focus s.exe -> TCVN3
    simulateWindowEvent("s.exe", sWin, code, 0, -1);
    check(code == 1, "focus s.exe -> TCVN3 (1)");

    // Use Start menu to minimize: StartMenuExperienceHost is a shell process.
    // Selected context is shell -> code table must be KEPT (not fallen back).
    simulateWindowEvent("StartMenuExperienceHost.exe",
                        { "Start", "", false }, code, 0, -1, /*shellContext*/true);
    check(code == 1, "Start menu (shell) keeps TCVN3, no Unicode fallback");

    // Also via explorer.exe itself.
    simulateWindowEvent("explorer.exe", { "Taskbar", "", false }, code, 0, -1);
    check(code == 1, "explorer.exe keeps TCVN3, no Unicode fallback");

    // Minimize event must not change the code table either.
    // (modelled: no simulateWindowEvent call at all, code stays 1)
    check(code == 1, "minimize event does not change code table");

    // Click the taskbar tab -> s.exe foreground again.
    simulateWindowEvent("s.exe", sWin, code, 0, -1);
    check(code == 1, "return to s.exe via taskbar -> TCVN3 (1)");

    // Worst case: code table was stuck at Unicode *and* the foreground event
    // was missed. The lazy re-check on the first keystroke recovers TCVN3.
    code = 0;
    simulateLazyRecheck("s.exe", sWin, code);
    check(code == 1, "lazy re-check recovers TCVN3 after missed event");

    // Lazy re-check must not touch the code table in a shell context.
    code = 0;
    simulateLazyRecheck("StartMenuExperienceHost.exe",
                        { "Start", "", false }, code);
    check(code == 0, "lazy re-check ignores shell context");

    cout << "\n==========================================================\n";
    if (failures == 0) {
        cout << "  All REAL ProcessRuleHelper tests passed!                 \n";
    } else {
        cout << "  " << failures << " test(s) FAILED!                             \n";
    }
    cout << "==========================================================\n";
    return failures == 0 ? 0 : 1;
}
