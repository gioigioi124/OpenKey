#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <cassert>

#define _WIN32 1
#define UNICODE 1
#define _UNICODE 1

#include "DataType.h"
#include "Vietnamese.h"
#include "Macro.h"
#include "Engine.h"

using namespace std;

// Globals
int vLanguage = 1;
int vInputType = 0;
int vFreeMark = 0;
int vCodeTable = 0;
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

static int passCount = 0;
static int failCount = 0;

#define AUDIT_ASSERT(cond, desc) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] " << desc << " (" #cond ") line " << __LINE__ << std::endl; \
            failCount++; \
            return; \
        } \
    } while(0)

#define AUDIT_PASS(desc) \
    do { \
        std::cout << "  [PASS] " << desc << std::endl; \
        passCount++; \
    } while(0)

static vector<Uint32> createKey(const string& text, bool titleCase = false, bool allCaps = false) {
    vector<Uint32> key;
    for (size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        Uint32 code = 0;
        if (_characterMap.find((Uint32)c) != _characterMap.end()) {
            code = _characterMap[(Uint32)c];
        } else {
            code = (Uint32)c;
        }
        if (allCaps || (i == 0 && titleCase)) {
            if (_characterMap.find((Uint32)toupper(c)) != _characterMap.end()) {
                code = _characterMap[(Uint32)toupper(c)];
            } else {
                code |= CAPS_MASK;
            }
        }
        key.push_back(code);
    }
    return key;
}

void test_Auditor_MemoryAndStruct() {
    std::cout << "\n[AUDITOR TEST 1] Struct MacroData size and zero vector verification..." << std::endl;
    AUDIT_ASSERT(sizeof(MacroData) == sizeof(string) * 2, "MacroData must only contain 2 std::string instances");
    AUDIT_PASS("MacroData memory layout strictly 48 bytes (0 vectors in RAM)");
}

void test_Auditor_AllCodeTablesJIT() {
    std::cout << "\n[AUDITOR TEST 2] Dynamic JIT across all 5 code tables (Unicode, TCVN3, VNI, Compound, CP1258)..." << std::endl;
    addMacro("tt", "thủy thủ");
    vector<Uint32> key = createKey("tt");
    vector<Uint32> out;

    // Unicode (0)
    vCodeTable = 0;
    bool found0 = findMacro(key, out);
    AUDIT_ASSERT(found0, "Found in Unicode");
    AUDIT_ASSERT(out.size() == 8, "Length is 8 characters");
    // 'ủ' in Unicode is 0x1EE7
    AUDIT_ASSERT((out[2] & 0xFFFF) == 0x1EE7, "th[ủ]y: 'ủ' in Unicode must be 0x1EE7");

    // TCVN3 (1)
    vCodeTable = 1;
    bool found1 = findMacro(key, out);
    AUDIT_ASSERT(found1, "Found in TCVN3");
    // 'ủ' in TCVN3 KEY_U index 9 is 0xF1
    AUDIT_ASSERT((out[2] & 0xFFFF) == 0x00F1, "th[ủ]y: 'ủ' in TCVN3 must be 0xF1");

    // VNI (2)
    vCodeTable = 2;
    bool found2 = findMacro(key, out);
    AUDIT_ASSERT(found2, "Found in VNI");
    // 'ủ' in VNI KEY_U index 9 is 0xFB75
    AUDIT_ASSERT((out[2] & 0xFFFF) == 0xFB75, "th[ủ]y: 'ủ' in VNI must be 0xFB75");

    // Compound (3)
    vCodeTable = 3;
    bool found3 = findMacro(key, out);
    AUDIT_ASSERT(found3, "Found in Unicode Compound");
    AUDIT_ASSERT((out[2] & CHAR_CODE_MASK) != 0, "Compound tone mark mask present");

    // CP1258 (4)
    vCodeTable = 4;
    bool found4 = findMacro(key, out);
    AUDIT_ASSERT(found4, "Found in CP1258");
    AUDIT_ASSERT((out[2] & CHAR_CODE_MASK) != 0, "CP1258 tone mark mask present");

    AUDIT_PASS("Dynamic JIT correctly translates across all 5 code tables on demand");
}

void test_Auditor_AutoCapsAdvanced() {
    std::cout << "\n[AUDITOR TEST 3] Advanced AutoCaps Title Case & All-Caps in TCVN3..." << std::endl;
    vAutoCapsMacro = 1;
    addMacro("hoc", "học sinh");

    // Title case in TCVN3
    vCodeTable = 1; // TCVN3
    vector<Uint32> keyTitle = createKey("hoc", true, false); // "Hoc"
    vector<Uint32> out;
    bool foundTitle = findMacro(keyTitle, out);
    AUDIT_ASSERT(foundTitle, "Title case matched");
    AUDIT_ASSERT(keyCodeToCharacter(out[0]) == 'H', "First char 'H' uppercase");
    // 'ọ' in TCVN3 KEY_O index 13 is 0xE4 (lowercase)
    AUDIT_ASSERT((out[1] & 0xFFFF) == 0x00E4, "'ọ' must remain lowercase 0xE4 in Title Case");

    // All-Caps in TCVN3
    vector<Uint32> keyAll = createKey("hoc", false, true); // "HOC"
    bool foundAll = findMacro(keyAll, out);
    AUDIT_ASSERT(foundAll, "All-Caps matched");
    AUDIT_ASSERT(keyCodeToCharacter(out[0]) == 'H', "First char 'H' uppercase");
    // In TCVN3 KEY_O index 12/13 is 0xE4
    AUDIT_ASSERT((out[1] & 0xFFFF) == 0x00E4, "'ọ' converted in All-Caps");

    AUDIT_PASS("AutoCaps casing correctly maps uppercase/lowercase per code table");
}

void test_Auditor_LargeContentJIT() {
    std::cout << "\n[AUDITOR TEST 4] Large macro content (5000 chars) stress test..." << std::endl;
    string largeText = "";
    for (int i = 0; i < 500; i++) {
        largeText += "Việt Nam ";
    }
    addMacro("big", largeText);

    vCodeTable = 0;
    vector<Uint32> key = createKey("big");
    vector<Uint32> out;

    auto t0 = std::chrono::high_resolution_clock::now();
    bool found = findMacro(key, out);
    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsedUs = std::chrono::duration<double, std::micro>(t1 - t0).count();

    AUDIT_ASSERT(found, "Large macro found");
    AUDIT_ASSERT(out.size() == 500 * 9, "All 4500 characters translated");
    std::cout << "  Translated 4,500 characters in " << elapsedUs << " microseconds" << std::endl;
    AUDIT_ASSERT(elapsedUs < 10000.0, "Must translate within 10 ms");

    deleteMacro("big");
    AUDIT_PASS("Large macro content JIT translated without error");
}

void test_Auditor_SerializationRoundtrip() {
    std::cout << "\n[AUDITOR TEST 5] Binary serialization and deserialization (zero eager pre-translation)..." << std::endl;
    addMacro("k1", "giá trị một");
    addMacro("k2", "giá trị hai");

    vector<Byte> savedData;
    getMacroSaveData(savedData);
    AUDIT_ASSERT(savedData.size() > 4, "Saved data not empty");

    // Clear and restore
    initMacroMap(savedData.data(), (int)savedData.size());

    // Verify lookup still JIT-converts properly
    vCodeTable = 0;
    vector<Uint32> k1 = createKey("k1");
    vector<Uint32> out;
    bool f1 = findMacro(k1, out);
    AUDIT_ASSERT(f1, "Restored macro k1 found");
    // "giá": out[0] = 'g', out[1] = 'i', out[2] = 'á' (0x00E1)
    AUDIT_ASSERT((out[2] & 0xFFFF) == 0x00E1, "Restored content has correct Unicode 'á'");

    vCodeTable = 1; // Switch to TCVN3
    bool f1_tcvn = findMacro(k1, out);
    AUDIT_ASSERT(f1_tcvn, "Restored macro k1 found in TCVN3");
    AUDIT_ASSERT((out[2] & 0xFFFF) == 0x00B8, "Restored content adapts JIT to TCVN3 'á' (0xB8)");

    AUDIT_PASS("Binary serialization roundtrips cleanly with zero eager pre-translation");
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << "  Independent Auditor Deep Verification Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    vKeyInit();

    test_Auditor_MemoryAndStruct();
    test_Auditor_AllCodeTablesJIT();
    test_Auditor_AutoCapsAdvanced();
    test_Auditor_LargeContentJIT();
    test_Auditor_SerializationRoundtrip();

    std::cout << "\n==========================================================" << std::endl;
    std::cout << "  Auditor Results: " << passCount << " PASSED, " << failCount << " FAILED" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return (failCount == 0) ? 0 : 1;
}
