#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <windows.h>
#include "DataType.h"
#include "Vietnamese.h"
#include "Engine.h"
#include "Macro.h"

using namespace std;

// Engine global configurations required when linking Engine.cpp
int vLanguage = 1;
int vInputType = 0;
int vFreeMark = 0;
int vCodeTable = 0; // 0: Unicode, 1: TCVN3, 2: VNI, 3: Unicode Compound
int vSwitchKeyStatus = 0;
int vCheckSpelling = 1;
int vUseModernOrthography = 0;
int vQuickTelex = 0;
int vRestoreIfWrongSpelling = 0;
int vFixRecommendBrowser = 0;
int vUseMacro = 1;
int vUseMacroInEnglishMode = 0;
int vAutoCapsMacro = 1;
int vUseSmartSwitchKey = 0;
int vUpperCaseFirstChar = 0;
int vTempOffSpelling = 0;
int vAllowConsonantZFWJ = 0;
int vQuickStartConsonant = 0;
int vQuickEndConsonant = 0;
int vRememberCode = 0;
int vOtherLanguage = 0;
int vTempOffOpenKey = 0;

extern vKeyHookState HookState;
extern map<vector<Uint32>, MacroData> macroMap;
extern bool isWordBreak(const vKeyEvent& event, const vKeyEventState& state, const Uint16& data);

int main() {
    std::cout << "==========================================================\n";
    std::cout << "  OpenKey Focus Change & Macro Clean State Test Suite     \n";
    std::cout << "==========================================================\n\n";

    // Initialize engine
    vKeyInit();
    vCodeTable = 0; // Unicode
    addMacro("ms", "Microsoft");
    addMacro("dc", "được");

    // ----------------------------------------------------
    // Test 1: startNewSession() clears hMacroKey
    // ----------------------------------------------------
    std::cout << "=== Test 1: startNewSession() clears macroKey ===\n";
    HookState.macroKey.push_back('x');
    HookState.macroKey.push_back('y');
    assert(HookState.macroKey.size() == 2);
    startNewSession();
    assert(HookState.macroKey.empty());
    std::cout << "  [PASS] HookState.macroKey is successfully cleared by startNewSession()\n\n";

    // ----------------------------------------------------
    // Test 2: resetInputState() clears all buffers
    // ----------------------------------------------------
    std::cout << "=== Test 2: resetInputState() clears all buffers ===\n";
    HookState.macroKey.push_back('a');
    HookState.macroKey.push_back('b');
    resetInputState();
    assert(HookState.macroKey.empty());
    assert(HookState.backspaceCount == 0);
    assert(HookState.newCharCount == 0);
    std::cout << "  [PASS] resetInputState() fully resets session and macro buffers\n\n";

    // ----------------------------------------------------
    // Test 3: Function keys (VK_F1 - VK_F24) are recognized as word breaks
    // ----------------------------------------------------
    std::cout << "=== Test 3: Function keys (VK_F1 - VK_F24) recognized as isWordBreak ===\n";
    for (Uint16 vk = VK_F1; vk <= VK_F24; ++vk) {
        bool breakResult = isWordBreak(vKeyEvent::Keyboard, vKeyEventState::KeyDown, vk);
        assert(breakResult == true);
    }
    std::cout << "  [PASS] VK_F1 through VK_F24 all correctly return isWordBreak == true\n\n";

    // ----------------------------------------------------
    // Test 4: Simulation of UserForm focus:
    // Before fix: dirty buffer -> macro fails.
    // After fix: resetInputState() -> macro expands immediately!
    // ----------------------------------------------------
    std::cout << "=== Test 4: Excel UserForm macro expansion simulation ===\n";
    // Simulate user previously typed in Excel or pressed F5
    HookState.macroKey.push_back(VK_F5); // F5 to show UserForm
    HookState.macroKey.push_back(KEY_D);
    HookState.macroKey.push_back(KEY_C);

    vector<Uint32> macroOutput;
    // With dirty buffer, looking for [VK_F5, KEY_D, KEY_C] fails:
    bool foundDirty = findMacro(HookState.macroKey, macroOutput);
    assert(!foundDirty);
    std::cout << "  [PASS] Dirty macroKey correctly failed to match macro 'dc'\n";

    // Now simulate focus event arriving at TextBox on UserForm:
    resetInputState();
    assert(HookState.macroKey.empty());

    // User types 'd' then 'c' in the clean TextBox:
    HookState.macroKey.push_back(KEY_D);
    HookState.macroKey.push_back(KEY_C);

    bool foundClean = findMacro(HookState.macroKey, macroOutput);
    assert(foundClean == true);
    std::cout << "  [PASS] Clean macroKey immediately matches and expands macro 'dc'\n\n";

    std::cout << "==========================================================\n";
    std::cout << "  All Focus & Macro Tests Passed Successfully!            \n";
    std::cout << "==========================================================\n";
    return 0;
}
