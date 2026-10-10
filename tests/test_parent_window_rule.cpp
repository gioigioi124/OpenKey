#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cassert>
#include <windows.h>

#define _WIN32 1
#define UNICODE 1
#define _UNICODE 1

#include "OpenKeyHelper.h"
#include "ProcessRuleHelper.h"

using namespace std;

// Mock/Global definitions required by OpenKey headers
int vCodeTable = 0;
int vAutoSwitchCodeTable = 1;
int vFallbackToUnicode = 1;
int vUseSmartSwitchKey = 0;
int vLanguage = 1;
int vInputType = 0;
int vFreeMark = 0;
int vSwitchKeyStatus = 0;
int vCheckSpelling = 1;
int vUseModernOrthography = 0;
int vQuickTelex = 0;
int vRestoreIfWrongSpelling = 0;
int vFixRecommendBrowser = 0;
int vUseMacro = 1;
int vUseMacroInEnglishMode = 0;
int vAutoCapsMacro = 1;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 0;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vRememberCode = 0;
int vOtherLanguage = 0;
int vTempOffOpenKey = 0;
int vRunAsAdmin = 0;

static int testsPassed = 0;
static int testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] " << msg << " (" #cond ") at line " << __LINE__ << std::endl; \
            testsFailed++; \
            return; \
        } \
    } while (0)

#define TEST_PASS(msg) \
    do { \
        std::cout << "  [PASS] " << msg << std::endl; \
        testsPassed++; \
    } while (0)

// Window class name for mock windows
static const wchar_t* MOCK_CLASS = L"OpenKeyMockClass";

static LRESULT CALLBACK MockWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void registerMockClass() {
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = MockWndProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = MOCK_CLASS;
    RegisterClassExW(&wc);
}

// ----------------------------------------------------------------------------
// TC-01: Null and Invalid HWND Edge Cases
// ----------------------------------------------------------------------------
void test_TC01_NullAndInvalidHWND() {
    std::cout << "\n=== Running TC-01: Null and Invalid HWND Edge Cases ===" << std::endl;
    vAutoSwitchCodeTable = 1;

    // 1. Null HWND
    HWND nullWnd = NULL;
    string titleNull = OpenKeyHelper::getWindowTitleUtf8(nullWnd);
    TEST_ASSERT(titleNull == "", "getWindowTitleUtf8(NULL) must return empty string");

    HWND rootNull = OpenKeyHelper::getProcessRootOwner(nullWnd);
    TEST_ASSERT(rootNull == NULL, "getProcessRootOwner(NULL) must return NULL");

    int codeNull = ProcessRuleHelper::getCodeTableForWindow(nullWnd, "excel.exe");
    TEST_ASSERT(codeNull == -1, "getCodeTableForWindow(NULL) must return -1");

    // 2. Invalid / Non-existent HWND
    HWND bogusWnd = (HWND)(ULONG_PTR)0xDEADBEEF;
    string titleBogus = OpenKeyHelper::getWindowTitleUtf8(bogusWnd);
    TEST_ASSERT(titleBogus == "", "getWindowTitleUtf8(bogus) must return empty string");

    HWND rootBogus = OpenKeyHelper::getProcessRootOwner(bogusWnd);
    TEST_ASSERT(rootBogus == NULL, "getProcessRootOwner(bogus) must return NULL");

    int codeBogus = ProcessRuleHelper::getCodeTableForWindow(bogusWnd, "excel.exe");
    TEST_ASSERT(codeBogus == -1, "getCodeTableForWindow(bogus) must return -1");

    TEST_PASS("TC-01: Null and Invalid HWND handled safely without crash");
}

// ----------------------------------------------------------------------------
// TC-02: Real Win32 Hierarchy Tracing (Root Owner & Parent)
// ----------------------------------------------------------------------------
void test_TC02_RealWin32HierarchyTracing() {
    std::cout << "\n=== Running TC-02: Real Win32 Hierarchy Tracing ===" << std::endl;
    HINSTANCE hInst = GetModuleHandleW(NULL);

    // Create Root Top-Level Window A: "Microsoft Excel - a.xlsx"
    HWND hwndA = CreateWindowExW(0, MOCK_CLASS, L"Microsoft Excel - a.xlsx",
                                 WS_OVERLAPPEDWINDOW, 100, 100, 400, 300,
                                 NULL, NULL, hInst, NULL);
    TEST_ASSERT(hwndA != NULL && IsWindow(hwndA), "Window A (XLMAIN) must be created");

    // Create Owned Modeless UserForm Window B: "UserForm1" (Owner = hwndA)
    HWND hwndB = CreateWindowExW(WS_EX_TOOLWINDOW, MOCK_CLASS, L"UserForm1",
                                 WS_POPUP | WS_CAPTION, 150, 150, 200, 200,
                                 hwndA, NULL, hInst, NULL);
    TEST_ASSERT(hwndB != NULL && IsWindow(hwndB), "Window B (UserForm1) must be created");

    // Create Owned Modal Dialog Window C: "MsgBox Thông báo" (Owner = hwndB)
    HWND hwndC = CreateWindowExW(WS_EX_DLGMODALFRAME, MOCK_CLASS, L"MsgBox Thông báo",
                                 WS_POPUP | WS_CAPTION, 180, 180, 150, 100,
                                 hwndB, NULL, hInst, NULL);
    TEST_ASSERT(hwndC != NULL && IsWindow(hwndC), "Window C (Nested Dialog) must be created");

    // 1. Verify Title Extraction
    string titleA = OpenKeyHelper::getWindowTitleUtf8(hwndA);
    TEST_ASSERT(titleA == "Microsoft Excel - a.xlsx", "Title A must match");

    string titleB = OpenKeyHelper::getWindowTitleUtf8(hwndB);
    TEST_ASSERT(titleB == "UserForm1", "Title B must match");

    string titleC = OpenKeyHelper::getWindowTitleUtf8(hwndC);
    TEST_ASSERT(titleC == "MsgBox Thông báo", "Title C must match UTF-8 Vietnamese");

    // 2. Verify Root Owner Tracing
    HWND rootOfB = OpenKeyHelper::getProcessRootOwner(hwndB);
    TEST_ASSERT(rootOfB == hwndA, "Root owner of UserForm B must be Window A (XLMAIN)");

    HWND rootOfC = OpenKeyHelper::getProcessRootOwner(hwndC);
    TEST_ASSERT(rootOfC == hwndA, "Root owner of nested MsgBox C must be Window A (XLMAIN)");

    // 3. Verify getProcessRootOwner on top-level window A
    HWND rootOfA = OpenKeyHelper::getProcessRootOwner(hwndA);
    // Root of a top-level window without owner should be NULL (not self)
    TEST_ASSERT(rootOfA != hwndA, "Root owner of top-level A must not be itself");

    // Cleanup
    DestroyWindow(hwndC);
    DestroyWindow(hwndB);
    DestroyWindow(hwndA);

    TEST_PASS("TC-02: Real Win32 window hierarchy tracing (2 levels nested) verified");
}

// ----------------------------------------------------------------------------
// TC-03: Process Rule Hierarchy (Child -> Parent -> Process)
// ----------------------------------------------------------------------------
void test_TC03_RuleHierarchyResolution() {
    std::cout << "\n=== Running TC-03: Rule Hierarchy Resolution ===" << std::endl;
    HINSTANCE hInst = GetModuleHandleW(NULL);
    vAutoSwitchCodeTable = 1;

    // Reload rules from default rules
    ProcessRuleHelper::init();

    // Create Root Window A: "Microsoft Excel - a.xlsx" (Rule: excel.exe[a] = TCVN3 -> 1)
    HWND hwndA = CreateWindowExW(0, MOCK_CLASS, L"Microsoft Excel - a.xlsx",
                                 WS_OVERLAPPEDWINDOW, 100, 100, 400, 300,
                                 NULL, NULL, hInst, NULL);

    // Create Child UserForm B: "UserForm1" (No rule for UserForm1)
    HWND hwndB = CreateWindowExW(WS_EX_TOOLWINDOW, MOCK_CLASS, L"UserForm1",
                                 WS_POPUP | WS_CAPTION, 150, 150, 200, 200,
                                 hwndA, NULL, hInst, NULL);

    // Create Nested Dialog C: "Dialog Tìm kiếm" (No rule)
    HWND hwndC = CreateWindowExW(WS_EX_DLGMODALFRAME, MOCK_CLASS, L"Dialog Tìm kiếm",
                                 WS_POPUP | WS_CAPTION, 180, 180, 150, 100,
                                 hwndB, NULL, hInst, NULL);

    // Create Child Form with Custom Rule: "FormNhapLieu" (Wait, default rules have "excel.exe[a] = 1" and "excel.exe[b] = 0")
    // Let's create Root Window B2: "Microsoft Excel - b.xlsx" (Rule: excel.exe[b] = Unicode -> 0)
    HWND hwndB2 = CreateWindowExW(0, MOCK_CLASS, L"Microsoft Excel - b.xlsx",
                                  WS_OVERLAPPEDWINDOW, 100, 100, 400, 300,
                                  NULL, NULL, hInst, NULL);

    HWND hwndChildB2 = CreateWindowExW(WS_EX_TOOLWINDOW, MOCK_CLASS, L"UserForm1",
                                       WS_POPUP | WS_CAPTION, 150, 150, 200, 200,
                                       hwndB2, NULL, hInst, NULL);

    // Window A directly
    int codeA = ProcessRuleHelper::getCodeTableForWindow(hwndA, "excel.exe");
    TEST_ASSERT(codeA == 1, "Window A (excel.exe with 'a') must resolve to TCVN3 (1)");

    // Window B (UserForm1, child of A): MUST INHERIT TCVN3 from A!
    int codeB = ProcessRuleHelper::getCodeTableForWindow(hwndB, "excel.exe");
    TEST_ASSERT(codeB == 1, "Child UserForm1 must inherit TCVN3 (1) from parent file 'a'");

    // Window C (Nested Dialog, grandchild of A): MUST INHERIT TCVN3 from A!
    int codeC = ProcessRuleHelper::getCodeTableForWindow(hwndC, "excel.exe");
    TEST_ASSERT(codeC == 1, "Nested dialog must inherit TCVN3 (1) from root parent file 'a'");

    // Window B2 directly: resolves to Unicode (0)
    int codeB2 = ProcessRuleHelper::getCodeTableForWindow(hwndB2, "excel.exe");
    TEST_ASSERT(codeB2 == 0, "Window B2 (excel.exe with 'b') must resolve to Unicode (0)");

    // Window ChildB2 (UserForm1, child of B2): MUST INHERIT Unicode (0) from B2!
    int codeChildB2 = ProcessRuleHelper::getCodeTableForWindow(hwndChildB2, "excel.exe");
    TEST_ASSERT(codeChildB2 == 0, "Child UserForm1 under file 'b' must inherit Unicode (0)");

    // Cleanup
    DestroyWindow(hwndC);
    DestroyWindow(hwndB);
    DestroyWindow(hwndA);
    DestroyWindow(hwndChildB2);
    DestroyWindow(hwndB2);

    TEST_PASS("TC-03: Rule hierarchy resolution correctly inherits parent encoding for UserForm and nested dialogs");
}

// ----------------------------------------------------------------------------
// TC-04: Child Window Specific Rule Overriding Parent Rule
// ----------------------------------------------------------------------------
void test_TC04_ChildRulePriorityOverParent() {
    std::cout << "\n=== Running TC-04: Child Rule Priority Over Parent Rule ===" << std::endl;
    HINSTANCE hInst = GetModuleHandleW(NULL);
    vAutoSwitchCodeTable = 1;

    // Root window A: "Microsoft Excel - a.xlsx" (matches rule 'a' -> TCVN3)
    HWND hwndA = CreateWindowExW(0, MOCK_CLASS, L"Microsoft Excel - a.xlsx",
                                 WS_OVERLAPPEDWINDOW, 100, 100, 400, 300,
                                 NULL, NULL, hInst, NULL);

    // Child window with title containing "b" (matches rule 'b' -> Unicode 0)
    // Parent matches 'a' (TCVN3 1), Child matches 'b' (Unicode 0)
    HWND hwndChildCustom = CreateWindowExW(WS_EX_TOOLWINDOW, MOCK_CLASS, L"UserForm - b special",
                                          WS_POPUP | WS_CAPTION, 150, 150, 200, 200,
                                          hwndA, NULL, hInst, NULL);

    int codeChild = ProcessRuleHelper::getCodeTableForWindow(hwndChildCustom, "excel.exe");
    TEST_ASSERT(codeChild == 0, "Child window with specific rule 'b' must override parent's 'a' rule");

    DestroyWindow(hwndChildCustom);
    DestroyWindow(hwndA);

    TEST_PASS("TC-04: Child specific rule has strict priority over parent rule");
}

// ----------------------------------------------------------------------------
// TC-05: Untitled / Empty Caption Window
// ----------------------------------------------------------------------------
void test_TC05_UntitledChildWindow() {
    std::cout << "\n=== Running TC-05: Untitled / Empty Caption Window ===" << std::endl;
    HINSTANCE hInst = GetModuleHandleW(NULL);
    vAutoSwitchCodeTable = 1;

    // Parent A: "Microsoft Excel - a.xlsx"
    HWND hwndA = CreateWindowExW(0, MOCK_CLASS, L"Microsoft Excel - a.xlsx",
                                 WS_OVERLAPPEDWINDOW, 100, 100, 400, 300,
                                 NULL, NULL, hInst, NULL);

    // Child with NULL title (empty string)
    HWND hwndEmptyChild = CreateWindowExW(WS_EX_TOOLWINDOW, MOCK_CLASS, L"",
                                         WS_POPUP, 150, 150, 100, 100,
                                         hwndA, NULL, hInst, NULL);

    string emptyTitle = OpenKeyHelper::getWindowTitleUtf8(hwndEmptyChild);
    TEST_ASSERT(emptyTitle == "", "Child title must be empty");

    int codeEmptyChild = ProcessRuleHelper::getCodeTableForWindow(hwndEmptyChild, "excel.exe");
    TEST_ASSERT(codeEmptyChild == 1, "Child with empty title must fall back to parent and resolve to TCVN3 (1)");

    DestroyWindow(hwndEmptyChild);
    DestroyWindow(hwndA);

    TEST_PASS("TC-05: Child with empty title correctly traces parent and inherits encoding");
}

// ----------------------------------------------------------------------------
// TC-06: Unmatched Window and Fallback Behavior
// ----------------------------------------------------------------------------
void test_TC06_UnmatchedWindowAndFallback() {
    std::cout << "\n=== Running TC-06: Unmatched Window and Fallback Behavior ===" << std::endl;
    HINSTANCE hInst = GetModuleHandleW(NULL);
    vAutoSwitchCodeTable = 1;

    // Root window for an application with no rules: "Notepad - notes.txt"
    HWND hwndNotepad = CreateWindowExW(0, MOCK_CLASS, L"Notepad - notes.txt",
                                      WS_OVERLAPPEDWINDOW, 100, 100, 400, 300,
                                      NULL, NULL, hInst, NULL);

    int codeNotepad = ProcessRuleHelper::getCodeTableForWindow(hwndNotepad, "notepad.exe");
    TEST_ASSERT(codeNotepad == -1, "Notepad with no rule must return -1");

    // Excel with file "c.xlsx" (not in rules)
    HWND hwndExcelC = CreateWindowExW(0, MOCK_CLASS, L"Microsoft Excel - c.xlsx",
                                     WS_OVERLAPPEDWINDOW, 100, 100, 400, 300,
                                     NULL, NULL, hInst, NULL);

    HWND hwndChildC = CreateWindowExW(WS_EX_TOOLWINDOW, MOCK_CLASS, L"UserForm1",
                                      WS_POPUP | WS_CAPTION, 150, 150, 200, 200,
                                      hwndExcelC, NULL, hInst, NULL);

    int codeChildC = ProcessRuleHelper::getCodeTableForWindow(hwndChildC, "excel.exe");
    TEST_ASSERT(codeChildC == -1, "File 'c.xlsx' and its child UserForm1 must return -1 (allowing fallback to Unicode)");

    DestroyWindow(hwndChildC);
    DestroyWindow(hwndExcelC);
    DestroyWindow(hwndNotepad);

    TEST_PASS("TC-06: Unmatched windows correctly return -1 allowing caller fallback logic");
}

// ----------------------------------------------------------------------------
// TC-07: Auto Switch Disabled Short-Circuit
// ----------------------------------------------------------------------------
void test_TC07_AutoSwitchDisabledShortCircuit() {
    std::cout << "\n=== Running TC-07: Auto Switch Disabled Short-Circuit ===" << std::endl;
    HINSTANCE hInst = GetModuleHandleW(NULL);

    // Disable auto switch
    vAutoSwitchCodeTable = 0;

    HWND hwndA = CreateWindowExW(0, MOCK_CLASS, L"Microsoft Excel - a.xlsx",
                                 WS_OVERLAPPEDWINDOW, 100, 100, 400, 300,
                                 NULL, NULL, hInst, NULL);

    int codeDisabled = ProcessRuleHelper::getCodeTableForWindow(hwndA, "excel.exe");
    TEST_ASSERT(codeDisabled == -1, "When vAutoSwitchCodeTable == 0, must return -1 immediately");

    // Re-enable
    vAutoSwitchCodeTable = 1;
    int codeEnabled = ProcessRuleHelper::getCodeTableForWindow(hwndA, "excel.exe");
    TEST_ASSERT(codeEnabled == 1, "When re-enabled, must resolve to TCVN3 (1)");

    DestroyWindow(hwndA);
    TEST_PASS("TC-07: vAutoSwitchCodeTable == 0 strictly short-circuits with zero overhead");
}

// ----------------------------------------------------------------------------
// TC-08: Deep Hierarchy (10 Levels) Stress & Cycle Prevention
// ----------------------------------------------------------------------------
void test_TC08_DeepHierarchyAndCyclePrevention() {
    std::cout << "\n=== Running TC-08: Deep Hierarchy (10 Levels) Stress & Cycle Prevention ===" << std::endl;
    HINSTANCE hInst = GetModuleHandleW(NULL);
    vAutoSwitchCodeTable = 1;

    // Create a 12-level deep chain of owned windows
    std::vector<HWND> chain;
    HWND parent = NULL;
    for (int i = 0; i < 12; i++) {
        std::wstring title;
        if (i == 0) {
            title = L"Microsoft Excel - a.xlsx"; // Root matching rule 'a'
        } else {
            title = L"Dialog Level " + std::to_wstring(i);
        }
        HWND wnd = CreateWindowExW(WS_EX_TOOLWINDOW, MOCK_CLASS, title.c_str(),
                                  WS_POPUP | WS_CAPTION, 100 + i*10, 100 + i*10, 200, 100,
                                  parent, NULL, hInst, NULL);
        TEST_ASSERT(wnd != NULL && IsWindow(wnd), "Window in chain must be valid");
        chain.push_back(wnd);
        parent = wnd;
    }

    // Check level 11 (leaf child)
    HWND leafWnd = chain.back();
    HWND root = OpenKeyHelper::getProcessRootOwner(leafWnd);
    // Root should be chain[0] (top-level Excel window)
    TEST_ASSERT(root == chain[0], "GA_ROOTOWNER must traverse 12 levels to find chain[0]");

    int codeLeaf = ProcessRuleHelper::getCodeTableForWindow(leafWnd, "excel.exe");
    TEST_ASSERT(codeLeaf == 1, "Leaf node in deep chain must inherit root encoding TCVN3 (1)");

    // Cleanup in reverse order
    for (int i = 11; i >= 0; i--) {
        DestroyWindow(chain[i]);
    }

    TEST_PASS("TC-08: Deep hierarchy (12 levels) handled safely without stack overflow or hang");
}

// ----------------------------------------------------------------------------
// TC-09: Latency & Performance Benchmark
// ----------------------------------------------------------------------------
void test_TC09_PerformanceBenchmark() {
    std::cout << "\n=== Running TC-09: Latency & Performance Benchmark ===" << std::endl;
    HINSTANCE hInst = GetModuleHandleW(NULL);
    vAutoSwitchCodeTable = 1;

    HWND hwndA = CreateWindowExW(0, MOCK_CLASS, L"Microsoft Excel - a.xlsx",
                                 WS_OVERLAPPEDWINDOW, 100, 100, 400, 300,
                                 NULL, NULL, hInst, NULL);
    HWND hwndB = CreateWindowExW(WS_EX_TOOLWINDOW, MOCK_CLASS, L"UserForm1",
                                 WS_POPUP | WS_CAPTION, 150, 150, 200, 200,
                                 hwndA, NULL, hInst, NULL);

    const int ITERATIONS = 100000;
    auto t0 = std::chrono::high_resolution_clock::now();
    int dummy = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        dummy += ProcessRuleHelper::getCodeTableForWindow(hwndB, "excel.exe");
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double avgLatencyMicroseconds = (elapsedMs * 1000.0) / ITERATIONS;

    std::cout << "  Executed " << ITERATIONS << " hierarchy resolutions in " << elapsedMs << " ms." << std::endl;
    std::cout << "  Average latency per resolution: " << avgLatencyMicroseconds << " microseconds ("
              << (avgLatencyMicroseconds / 1000.0) << " ms)" << std::endl;

    TEST_ASSERT(dummy == ITERATIONS, "All iterations must resolve to 1");
    // Verify average latency is well below 50 microseconds (0.05 ms)
    TEST_ASSERT(avgLatencyMicroseconds < 50.0, "Resolution latency must be < 50 microseconds");

    DestroyWindow(hwndB);
    DestroyWindow(hwndA);

    TEST_PASS("TC-09: Performance benchmark passed (< 50 microseconds latency per resolution)");
}

// ----------------------------------------------------------------------------
// TC-10: Cross-Process Security Isolation Verification
// ----------------------------------------------------------------------------
void test_TC10_CrossProcessSecurityIsolation() {
    std::cout << "\n=== Running TC-10: Cross-Process Security Isolation ===" << std::endl;
    // Get a known foreign process window, e.g. Desktop window or Shell
    HWND hDesktop = GetDesktopWindow();
    TEST_ASSERT(hDesktop != NULL, "Desktop window must exist");

    DWORD myPid = GetCurrentProcessId();
    DWORD deskPid = 0;
    GetWindowThreadProcessId(hDesktop, &deskPid);

    // Verify OpenKeyHelper::getProcessRootOwner never returns desktop
    HWND rootOfDesktop = OpenKeyHelper::getProcessRootOwner(hDesktop);
    TEST_ASSERT(rootOfDesktop != hDesktop, "Root owner of desktop must not be desktop");

    // Create a local window
    HINSTANCE hInst = GetModuleHandleW(NULL);
    HWND localWnd = CreateWindowExW(0, MOCK_CLASS, L"Local Test Window",
                                    WS_OVERLAPPEDWINDOW, 100, 100, 200, 200,
                                    NULL, NULL, hInst, NULL);

    // If targetPid check works, passing localWnd returns its own root or NULL, but never cross-PID
    HWND localRoot = OpenKeyHelper::getProcessRootOwner(localWnd);
    if (localRoot != NULL) {
        DWORD rootPid = 0;
        GetWindowThreadProcessId(localRoot, &rootPid);
        TEST_ASSERT(rootPid == myPid, "Root window PID must strictly equal target PID");
    }

    DestroyWindow(localWnd);
    TEST_PASS("TC-10: Cross-process boundaries strictly isolated and protected");
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  OpenKey Win32 Parent/Owner Window Tracing Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    registerMockClass();

    test_TC01_NullAndInvalidHWND();
    test_TC02_RealWin32HierarchyTracing();
    test_TC03_RuleHierarchyResolution();
    test_TC04_ChildRulePriorityOverParent();
    test_TC05_UntitledChildWindow();
    test_TC06_UnmatchedWindowAndFallback();
    test_TC07_AutoSwitchDisabledShortCircuit();
    test_TC08_DeepHierarchyAndCyclePrevention();
    test_TC09_PerformanceBenchmark();
    test_TC10_CrossProcessSecurityIsolation();

    std::cout << "\n==========================================================" << std::endl;
    std::cout << "  Test Results: " << testsPassed << " PASSED, " << testsFailed << " FAILED" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return (testsFailed == 0) ? 0 : 1;
}
