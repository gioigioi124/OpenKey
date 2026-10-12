/*----------------------------------------------------------
OpenKey - The Cross platform Open source Vietnamese Keyboard application.

Copyright (C) 2019 Mai Vu Tuyen
Contact: maivutuyen.91@gmail.com
Github: https://github.com/tuyenvm/OpenKey
Fanpage: https://www.facebook.com/OpenKeyVN

This file is belong to the OpenKey project, Win32 version
which is released under GPL license.
You can fork, modify, improve this program. If you
redistribute your new version, it MUST be open source.
-----------------------------------------------------------*/
#include "stdafx.h"
#include "AppDelegate.h"
#include <algorithm>

#pragma comment(lib, "imm32")
#define IMC_GETOPENSTATUS 0x0005

#define MASK_SHIFT				0x01
#define MASK_CONTROL			0x02
#define MASK_ALT				0x04
#define MASK_CAPITAL			0x08
#define MASK_NUMLOCK			0x10
#define MASK_WIN				0x20
#define MASK_SCROLL				0x40

#define OTHER_CONTROL_KEY (_flag & MASK_ALT) || (_flag & MASK_CONTROL)
#define DYNA_DATA(macro, pos) (macro ? pData->macroData[pos] : pData->charData[pos])
#define EMPTY_HOTKEY 0xFE0000FE

static vector<string> _chromiumBrowser = {
	"chrome.exe", "brave.exe", "msedge.exe"
};

extern int vSendKeyStepByStep;
extern int vUseGrayIcon;
extern int vShowOnStartUp;
extern int vRunWithWindows;

static HHOOK hKeyboardHook;
static HHOOK hMouseHook;
static HWINEVENTHOOK hSystemEvent;
static HWINEVENTHOOK hTitleEvent;
static HWINEVENTHOOK hFocusEvent;
static HWINEVENTHOOK hMinimizeEvent;
static KBDLLHOOKSTRUCT* keyboardData;
static MSLLHOOKSTRUCT* mouseData;
static vKeyHookState* pData;
static vector<Uint16> _syncKey;
static Uint32 _flag = 0, _lastFlag = 0, _privateFlag;
static bool _flagChanged = false, _isFlagKey;
static Uint16 _keycode;
static Uint16 _newChar, _newCharHi;

static vector<Uint16> _newCharString;
static Uint16 _newCharSize;
static bool _willSendControlKey = false;

static Uint16 _uniChar[2];
static int _i, _j, _k;
static Uint32 _tempChar;

static string macroText, macroContent;
static int _languageTemp = 0; //use for smart switch key
static vector<Byte> savedSmartSwitchKeyData; ////use for smart switch key

static bool _hasJustUsedHotKey = false;
static bool _needRuleRecheck = false; // lazy code-table re-check on next keystroke

static INPUT backspaceEvent[2];
static INPUT keyEvent[2];

static LRESULT CALLBACK keyboardHookProcess(int nCode, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK mouseHookProcess(int nCode, WPARAM wParam, LPARAM lParam);
VOID CALLBACK winEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

// Resolve the rule code table for a window (walking up the owner chain for
// dialogs / UserForms / owned windows). Returns -1 when no rule matches.
// Implemented later in this file, after isDialogOrOwnedWindow().
static int resolveRuleCodeForWindow(const string& exe, HWND hwnd);

// True for shell / transient windows (Start menu, taskbar, desktop, search...)
// that must NOT trigger any code-table change.
// Implemented later in this file.
static bool isNonAppContext(const string& exe, HWND hwnd);

// Re-evaluate the code table rule for the currently focused window. Used as a
// lazy safety net so a missed foreground event cannot leave the code table
// stuck. Implemented later in this file.
static void recheckRuleForCurrentWindow();


void OpenKeyFree() {
	UnhookWindowsHookEx(hMouseHook);
	UnhookWindowsHookEx(hKeyboardHook);
	UnhookWinEvent(hSystemEvent);
	if (hTitleEvent) {
		UnhookWinEvent(hTitleEvent);
		hTitleEvent = NULL;
	}
	if (hFocusEvent) {
		UnhookWinEvent(hFocusEvent);
		hFocusEvent = NULL;
	}
	if (hMinimizeEvent) {
		UnhookWinEvent(hMinimizeEvent);
		hMinimizeEvent = NULL;
	}
}

void OpenKeyInit() {
	APP_GET_DATA(vLanguage, 1);
	APP_GET_DATA(vInputType, 0);
	vFreeMark = 0;
	APP_GET_DATA(vCodeTable, 0);
	APP_GET_DATA(vCheckSpelling, 1);
	APP_GET_DATA(vUseModernOrthography, 0);
	APP_GET_DATA(vQuickTelex, 0);
	APP_GET_DATA(vSwitchKeyStatus, 0x7A000206);
	APP_GET_DATA(vRestoreIfWrongSpelling, 1);
	APP_GET_DATA(vFixRecommendBrowser, 1);
	APP_GET_DATA(vUseMacro, 1);
	APP_GET_DATA(vUseMacroInEnglishMode, 0);
	APP_GET_DATA(vAutoCapsMacro, 0);
	APP_GET_DATA(vSendKeyStepByStep, 1);
	APP_GET_DATA(vUseGrayIcon, 0);
	APP_GET_DATA(vShowOnStartUp, 1);
	APP_GET_DATA(vRunWithWindows, 1);
	OpenKeyHelper::registerRunOnStartup(vRunWithWindows);
	APP_GET_DATA(vUseSmartSwitchKey, 1);
	APP_GET_DATA(vUpperCaseFirstChar, 0);
	APP_GET_DATA(vAllowConsonantZFWJ, 0);
	APP_GET_DATA(vTempOffSpelling, 0);
	APP_GET_DATA(vQuickStartConsonant, 0);
	APP_GET_DATA(vQuickEndConsonant, 0);
	APP_GET_DATA(vSupportMetroApp, 0);
	APP_GET_DATA(vRunAsAdmin, 0);
	APP_GET_DATA(vCreateDesktopShortcut, 0);
	APP_GET_DATA(vCheckNewVersion, 0);
	APP_GET_DATA(vRememberCode, 1);
	APP_GET_DATA(vOtherLanguage, 1);
	APP_GET_DATA(vAutoSwitchCodeTable, 1);
	APP_GET_DATA(vFallbackToUnicode, 0);
	APP_GET_DATA(vTempOffOpenKey, 0);
	APP_GET_DATA(vFixChromiumBrowser, 0);

	//init convert tool
	APP_GET_DATA(convertToolDontAlertWhenCompleted, 0);
	APP_GET_DATA(convertToolToAllCaps, 0);
	APP_GET_DATA(convertToolToAllNonCaps, 0);
	APP_GET_DATA(convertToolToCapsFirstLetter, 0);
	APP_GET_DATA(convertToolToCapsEachWord, 0);
	APP_GET_DATA(convertToolRemoveMark, 0);
	APP_GET_DATA(convertToolFromCode, 0);
	APP_GET_DATA(convertToolToCode, 0);
	APP_GET_DATA(convertToolHotKey, EMPTY_HOTKEY);
	if (convertToolHotKey == 0) {
		convertToolHotKey = EMPTY_HOTKEY;
	}

	pData = (vKeyHookState*)vKeyInit();

	//pre-create back key
	backspaceEvent[0].type = INPUT_KEYBOARD;
	backspaceEvent[0].ki.dwFlags = 0;
	backspaceEvent[0].ki.wVk = VK_BACK;
	backspaceEvent[0].ki.wScan = 0;
	backspaceEvent[0].ki.time = 0;
	backspaceEvent[0].ki.dwExtraInfo = 1;

	backspaceEvent[1].type = INPUT_KEYBOARD;
	backspaceEvent[1].ki.dwFlags = KEYEVENTF_KEYUP;
	backspaceEvent[1].ki.wVk = VK_BACK;
	backspaceEvent[1].ki.wScan = 0;
	backspaceEvent[1].ki.time = 0;
	backspaceEvent[1].ki.dwExtraInfo = 1;

	//get key state
	_flag = 0;
	if (GetKeyState(VK_LSHIFT) < 0 || GetKeyState(VK_RSHIFT) < 0) _flag |= MASK_SHIFT;
	if (GetKeyState(VK_LCONTROL) < 0 || GetKeyState(VK_RCONTROL) < 0) _flag |= MASK_CONTROL;
	if (GetKeyState(VK_LMENU) < 0 || GetKeyState(VK_RMENU) < 0) _flag |= MASK_ALT;
	if (GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0) _flag |= MASK_WIN;
	if (GetKeyState(VK_NUMLOCK) < 0) _flag |= MASK_NUMLOCK;
	if (GetKeyState(VK_CAPITAL) == 1) _flag |= MASK_CAPITAL;
	if (GetKeyState(VK_SCROLL) < 0) _flag |= MASK_SCROLL;

	//init and load macro data
	DWORD macroDataSize;
	BYTE* macroData = OpenKeyHelper::getRegBinary(_T("macroData"), macroDataSize);
	initMacroMap((Byte*)macroData, (int)macroDataSize);

	//init and load smart switch key data
	DWORD smartSwitchKeySize;
	BYTE* data = OpenKeyHelper::getRegBinary(_T("smartSwitchKey"), smartSwitchKeySize);
	initSmartSwitchKey((Byte*)data, (int)smartSwitchKeySize);

	//init process rules for auto encoding switch
	ProcessRuleHelper::init();

	//init hook
	HINSTANCE hInstance = GetModuleHandle(NULL);
	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHookProcess, hInstance, 0);
	hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, mouseHookProcess, hInstance, 0);
	hSystemEvent = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, winEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
	hTitleEvent = SetWinEventHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, NULL, winEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
	hFocusEvent = SetWinEventHook(EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, NULL, winEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
	hMinimizeEvent = SetWinEventHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, NULL, winEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
}

void saveSmartSwitchKeyData() {
	getSmartSwitchKeySaveData(savedSmartSwitchKeyData);
	OpenKeyHelper::setRegBinary(_T("smartSwitchKey"), savedSmartSwitchKeyData.data(), (int)savedSmartSwitchKeyData.size());
}

static void InsertKeyLength(const Uint8& len) {
	_syncKey.push_back(len);
}

static inline void prepareKeyEvent(INPUT& input, const Uint16& keycode, const bool& isPress, const DWORD& flag=0) {
	input.type = INPUT_KEYBOARD;
	input.ki.dwFlags = isPress ? flag : flag|KEYEVENTF_KEYUP;
	input.ki.wVk = keycode;
	input.ki.wScan = 0;
	input.ki.time = 0;
	input.ki.dwExtraInfo = 1;
}

static inline void prepareUnicodeEvent(INPUT& input, const Uint16& unicode, const bool& isPress) {
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = 0;
	input.ki.wScan = unicode;
	input.ki.time = 0;
	input.ki.dwFlags = (isPress ? 0 : KEYEVENTF_KEYUP) | KEYEVENTF_UNICODE;
	input.ki.dwExtraInfo = 1;
}

static void SendCombineKey(const Uint16& key1, const Uint16& key2, const DWORD& flagKey1=0, const DWORD& flagKey2 = 0) {
	prepareKeyEvent(keyEvent[0], key1, true, flagKey1);
	SendInput(1, keyEvent, sizeof(INPUT));

	prepareKeyEvent(keyEvent[0], key2, true, flagKey2);
	prepareKeyEvent(keyEvent[1], key2, false, flagKey2);
	SendInput(2, keyEvent, sizeof(INPUT));

	prepareKeyEvent(keyEvent[0], key1, false, flagKey1);
	SendInput(1, keyEvent, sizeof(INPUT));
}

static void SendKeyCode(Uint32 data) {
	_newChar = (Uint16)data;
	if (!(data & CHAR_CODE_MASK)) {
		if (IS_DOUBLE_CODE(vCodeTable)) //VNI
			InsertKeyLength(1);

		_newChar = keyCodeToCharacter(data);
		if (_newChar == 0) {
			_newChar = (Uint16)data;
			prepareKeyEvent(keyEvent[0], _newChar, true);
			prepareKeyEvent(keyEvent[1], _newChar, false);
			SendInput(2, keyEvent, sizeof(INPUT));
		} else {
			prepareUnicodeEvent(keyEvent[0], _newChar, true);
			prepareUnicodeEvent(keyEvent[1], _newChar, false);
			SendInput(2, keyEvent, sizeof(INPUT));
		}
	} else {
		if (vCodeTable == 0) { //unicode 2 bytes code
			prepareUnicodeEvent(keyEvent[0], _newChar, true);
			prepareUnicodeEvent(keyEvent[1], _newChar, false);
			SendInput(2, keyEvent, sizeof(INPUT));
		} else if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4) { //others such as VNI Windows, TCVN3: 1 byte code
			_newCharHi = HIBYTE(_newChar);
			_newChar = LOBYTE(_newChar);

			prepareUnicodeEvent(keyEvent[0], _newChar, true);
			prepareUnicodeEvent(keyEvent[1], _newChar, false);
			SendInput(2, keyEvent, sizeof(INPUT));

			if (_newCharHi > 32) {
				if (vCodeTable == 2) //VNI
					InsertKeyLength(2);
				prepareUnicodeEvent(keyEvent[0], _newCharHi, true);
				prepareUnicodeEvent(keyEvent[1], _newCharHi, false);
				SendInput(2, keyEvent, sizeof(INPUT));
			} else {
				if (vCodeTable == 2) //VNI
					InsertKeyLength(1);
			}
		} else if (vCodeTable == 3) { //Unicode Compound
			_newCharHi = (_newChar >> 13);
			_newChar &= 0x1FFF;
			_uniChar[0] = _newChar;
			_uniChar[1] = _newCharHi > 0 ? (_unicodeCompoundMark[_newCharHi - 1]) : 0;
			InsertKeyLength(_newCharHi > 0 ? 2 : 1);
			prepareUnicodeEvent(keyEvent[0], _uniChar[0], true);
			prepareUnicodeEvent(keyEvent[1], _uniChar[0], false);
			SendInput(2, keyEvent, sizeof(INPUT));
			if (_newCharHi > 0) {
				prepareUnicodeEvent(keyEvent[0], _uniChar[1], true);
				prepareUnicodeEvent(keyEvent[1], _uniChar[1], false);
				SendInput(2, keyEvent, sizeof(INPUT));
			}
		}
	}
}

static void SendBackspace() {
	SendInput(2, backspaceEvent, sizeof(INPUT));
	if (vSupportMetroApp && OpenKeyHelper::getLastAppExecuteName().compare("ApplicationFrameHost.exe") == 0) {//Metro App
		SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
		SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
	}
	if (IS_DOUBLE_CODE(vCodeTable)) { //VNI or Unicode Compound
		if (_syncKey.back() > 1) {
			/*if (!(vCodeTable == 3 && containUnicodeCompoundApp(FRONT_APP))) {
				SendInput(2, backspaceEvent, sizeof(INPUT));
			}*/
			SendInput(2, backspaceEvent, sizeof(INPUT));
			if (vSupportMetroApp && OpenKeyHelper::getLastAppExecuteName().compare("ApplicationFrameHost.exe") == 0) {//Metro App
				SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
				SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
			}
		}
		_syncKey.pop_back();
	}
}

static void SendEmptyCharacter() {
	if (IS_DOUBLE_CODE(vCodeTable)) //VNI or Unicode Compound
		InsertKeyLength(1);

	_newChar = 0x202F; //empty char

	prepareUnicodeEvent(keyEvent[0], _newChar, true);
	prepareUnicodeEvent(keyEvent[1], _newChar, false);
	SendInput(2, keyEvent, sizeof(INPUT));
}

static void SendNewCharString(const bool& dataFromMacro = false) {
	_j = 0;
	_newCharSize = dataFromMacro ? (Uint16)pData->macroData.size() : pData->newCharCount;
	if (_newCharString.size() < _newCharSize) {
		_newCharString.resize(_newCharSize);
	}
	_willSendControlKey = false;
	
	if (_newCharSize > 0) {
		for (_k = dataFromMacro ? 0 : pData->newCharCount - 1;
			dataFromMacro ? _k < pData->macroData.size() : _k >= 0;
			dataFromMacro ? _k++ : _k--) {

			_tempChar = DYNA_DATA(dataFromMacro, _k);
			if (_tempChar & PURE_CHARACTER_MASK) {
				_newCharString[_j++] = _tempChar;
				if (IS_DOUBLE_CODE(vCodeTable)) {
					InsertKeyLength(1);
				}
			} else if (!(_tempChar & CHAR_CODE_MASK)) {
				if (IS_DOUBLE_CODE(vCodeTable)) //VNI
					InsertKeyLength(1);
				_newCharString[_j++] = keyCodeToCharacter(_tempChar);
			} else {
				_newChar = _tempChar;
				if (vCodeTable == 0) {  //unicode 2 bytes code
					_newCharString[_j++] = _newChar;
				} else if (vCodeTable == 1 || vCodeTable == 2 || vCodeTable == 4) { //others such as VNI Windows, TCVN3: 1 byte code
					_newCharHi = HIBYTE(_newChar);
					_newChar = LOBYTE(_newChar);
					_newCharString[_j++] = _newChar;

					if (_newCharHi > 32) {
						if (vCodeTable == 2) //VNI
							InsertKeyLength(2);
						_newCharString[_j++] = _newCharHi;
						_newCharSize++;
					}
					else {
						if (vCodeTable == 2) //VNI
							InsertKeyLength(1);
					}
				} else if (vCodeTable == 3) { //Unicode Compound
					_newCharHi = (_newChar >> 13);
					_newChar &= 0x1FFF;

					InsertKeyLength(_newCharHi > 0 ? 2 : 1);
					_newCharString[_j++] = _newChar;
					if (_newCharHi > 0) {
						_newCharSize++;
						_newCharString[_j++] = _unicodeCompoundMark[_newCharHi - 1];
					}

				}
			}
		}//end for
	}

	if (pData->code == vRestore || pData->code == vRestoreAndStartNewSession) { //if is restore
		if (keyCodeToCharacter(_keycode) != 0) {
			_newCharSize++;
			_newCharString[_j++] = keyCodeToCharacter(_keycode | ((_flag & MASK_SHIFT) || (_flag & MASK_CAPITAL) ? CAPS_MASK : 0));
		} else {
			_willSendControlKey = true;
		}
	}
	if (pData->code == vRestoreAndStartNewSession) {
		startNewSession();
	}

	OpenKeyHelper::setClipboardText((LPCTSTR)_newCharString.data(), _newCharSize + 1, CF_UNICODETEXT);

	//Send shift + insert
	SendCombineKey(KEY_LEFT_SHIFT, VK_INSERT, 0, KEYEVENTF_EXTENDEDKEY);
	
	//the case when hCode is vRestore or vRestoreAndStartNewSession,
	//the word is invalid and last key is control key such as TAB, LEFT ARROW, RIGHT ARROW,...
	if (_willSendControlKey) {
		SendKeyCode(_keycode);
	}
}

bool checkHotKey(int hotKeyData, bool checkKeyCode = true) {
	if ((hotKeyData & (~0x8000)) == EMPTY_HOTKEY)
		return false;
	if (HAS_CONTROL(hotKeyData) ^ GET_BOOL(_lastFlag & MASK_CONTROL))
		return false;
	if (HAS_OPTION(hotKeyData) ^ GET_BOOL(_lastFlag & MASK_ALT))
		return false;
	if (HAS_COMMAND(hotKeyData) ^ GET_BOOL(_lastFlag & MASK_WIN))
		return false;
	if (HAS_SHIFT(hotKeyData) ^ GET_BOOL(_lastFlag & MASK_SHIFT))
		return false;
	if (checkKeyCode) {
		if (GET_SWITCH_KEY(hotKeyData) != _keycode)
			return false;
	}
	return true;
}

void switchLanguage() {
	if (vLanguage == 0)
		vLanguage = 1;
	else
		vLanguage = 0;
	if (HAS_BEEP(vSwitchKeyStatus))
		MessageBeep(MB_OK);
	AppDelegate::getInstance()->onInputMethodChangedFromHotKey();
	if (vUseSmartSwitchKey) {
		setAppInputMethodStatus(OpenKeyHelper::getFrontMostAppExecuteName(), vLanguage | (vCodeTable << 1));
		saveSmartSwitchKeyData();
	}
	startNewSession();
}

static void SendPureCharacter(const Uint16& ch) {
	if (ch < 128)
		SendKeyCode(ch);
	else {
		prepareUnicodeEvent(keyEvent[0], ch, true);
		prepareUnicodeEvent(keyEvent[1], ch, false);
		SendInput(2, keyEvent, sizeof(INPUT));
		if (IS_DOUBLE_CODE(vCodeTable)) {
			InsertKeyLength(1);
		}
	}
}

static void handleMacro() {
	//fix autocomplete
	if (vFixRecommendBrowser) {
		SendEmptyCharacter();
		pData->backspaceCount++;
	}

	//send backspace
	if (pData->backspaceCount > 0) {
		for (int i = 0; i < pData->backspaceCount; i++) {
			SendBackspace();
		}
	}
	//send real data
	if (!vSendKeyStepByStep) {
		SendNewCharString(true);
	} else {
		for (int i = 0; i < pData->macroData.size(); i++) {
			if (pData->macroData[i] & PURE_CHARACTER_MASK) {
				SendPureCharacter(pData->macroData[i]);
			} else {
				SendKeyCode(pData->macroData[i]);
			}
		}
	}
	SendKeyCode(_keycode | (_flag & MASK_SHIFT ? CAPS_MASK : 0));
}

static bool SetModifierMask(const Uint16& vkCode) {
	// For caps lock case, toggling the flag isn't enough. We need to check the actual state, which should be done before each key press.
	// Example: the caps lock state can be changed without the key being pressed, or the key toggle is made with admin privilege, making the app not able to detect the change.
	if (GetKeyState(VK_CAPITAL) == 1) _flag |= MASK_CAPITAL;
	else _flag &= ~MASK_CAPITAL;

	if (vkCode == VK_LSHIFT || vkCode == VK_RSHIFT) _flag |= MASK_SHIFT;
	else if (vkCode == VK_LCONTROL || vkCode == VK_RCONTROL) _flag |= MASK_CONTROL;
	else if (vkCode == VK_LMENU || vkCode == VK_RMENU) _flag |= MASK_ALT;
	else if (vkCode == VK_LWIN || vkCode == VK_RWIN) _flag |= MASK_WIN;
	else if (vkCode == VK_NUMLOCK) _flag |= MASK_NUMLOCK;
	else if (vkCode == VK_SCROLL) _flag |= MASK_SCROLL;
	else { 
		_isFlagKey = false;
		return false; 
	}
	_isFlagKey = true;
	return true;
}

static bool UnsetModifierMask(const Uint16& vkCode) {
	if (vkCode == VK_LSHIFT || vkCode == VK_RSHIFT) _flag &= ~MASK_SHIFT;
	else if (vkCode == VK_LCONTROL || vkCode == VK_RCONTROL) _flag &= ~MASK_CONTROL;
	else if (vkCode == VK_LMENU || vkCode == VK_RMENU) _flag &= ~MASK_ALT;
	else if (vkCode == VK_LWIN || vkCode == VK_RWIN) _flag &= ~MASK_WIN;
	else if (vkCode == VK_NUMLOCK) _flag &= ~MASK_NUMLOCK;
	else if (vkCode == VK_SCROLL) _flag &= ~MASK_SCROLL;
	else { 
		_isFlagKey = false;
		return false; 
	}
	_isFlagKey = true;
	return true;
}

LRESULT CALLBACK keyboardHookProcess(int nCode, WPARAM wParam, LPARAM lParam) {
	keyboardData = (KBDLLHOOKSTRUCT *)lParam;
	//ignore my event
	if (keyboardData->dwExtraInfo != 0) {
		return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
	}
	
	//ignore if IME pad is open when typing Japanese/Chinese...
	HWND hWnd = GetForegroundWindow();
	HWND hIME = ImmGetDefaultIMEWnd(hWnd);
	LRESULT isImeON = SendMessage(hIME, WM_IME_CONTROL, IMC_GETOPENSTATUS, 0);
	if (isImeON) {
		return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
	}
	
	//check modifier key
	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
		//LOG(L"Key down: %d\n", keyboardData->vkCode);
		SetModifierMask((Uint16)keyboardData->vkCode);
	} else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
		//LOG(L"Key up: %d\n", keyboardData->vkCode);
		UnsetModifierMask((Uint16)keyboardData->vkCode);
	}
	if (!_isFlagKey && wParam != WM_KEYUP && wParam != WM_SYSKEYUP)
		_keycode = (Uint16)keyboardData->vkCode;

	// Lazy safety net: if the foreground changed but the code table was not
	// (re)evaluated (missed/raced foreground event), re-check it on the next
	// real keystroke. Restoring an app from the taskbar therefore recovers its
	// code table even if no foreground event reached us.
	if (_needRuleRecheck && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
		_needRuleRecheck = false;
		recheckRuleForCurrentWindow();
	}

	//switch language shortcut; convert hotkey
	if ((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && !_isFlagKey && _keycode != 0) {
		// Hotkeys: Ctrl + Shift + F1 (Unicode), Ctrl + Shift + F2 (TCVN3), Ctrl + Shift + F12 (lock)
		// Guard: skip if the key is already bound to the user-configured switch key or
		// convert-tool hotkey, so the user's own binding always takes precedence.
		bool _conflictsWithUserHotkey =
			(GET_SWITCH_KEY(vSwitchKeyStatus) == _keycode) ||
			(GET_SWITCH_KEY(convertToolHotKey) == _keycode);
		if (!_conflictsWithUserHotkey &&
			(_flag & MASK_CONTROL) && (_flag & MASK_SHIFT) && !(_flag & MASK_ALT) && !(_flag & MASK_WIN)) {
			if (_keycode == VK_F1) {
				AppDelegate::getInstance()->onTableCode(0);
				wstring _codeName = wstring(_T("Bảng mã: ")) + ProcessRuleHelper::getCodeTableName(0);
				SystemTrayHelper::showNotification(_T("OpenKey"), _codeName.c_str());
				if (HAS_BEEP(vSwitchKeyStatus)) {
					MessageBeep(MB_OK);
				}
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			} else if (_keycode == VK_F2) {
				AppDelegate::getInstance()->onTableCode(1);
				wstring _codeName = wstring(_T("Bảng mã: ")) + ProcessRuleHelper::getCodeTableName(1);
				SystemTrayHelper::showNotification(_T("OpenKey"), _codeName.c_str());
				if (HAS_BEEP(vSwitchKeyStatus)) {
					MessageBeep(MB_OK);
				}
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			} else if (_keycode == VK_F12) {
				AppDelegate::getInstance()->onToggleAutoSwitchCodeTable();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
		}

		if (GET_SWITCH_KEY(vSwitchKeyStatus) != _keycode && GET_SWITCH_KEY(convertToolHotKey) != _keycode) {
			_lastFlag = 0;
		} else {
			if (GET_SWITCH_KEY(vSwitchKeyStatus) == _keycode && checkHotKey(vSwitchKeyStatus, GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)) {
				switchLanguage();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
			if (GET_SWITCH_KEY(convertToolHotKey) == _keycode && checkHotKey(convertToolHotKey, GET_SWITCH_KEY(convertToolHotKey) != 0xFE)) {
				AppDelegate::getInstance()->onQuickConvert();
				_hasJustUsedHotKey = true;
				_keycode = 0;
				return -1;
			}
		}
		_hasJustUsedHotKey = _lastFlag != 0;
	} else if (_isFlagKey) {
		if (_lastFlag == 0 || _lastFlag < _flag)
			_lastFlag = _flag;
		else if (_lastFlag > _flag) {
			//check switch
			if (checkHotKey(vSwitchKeyStatus, GET_SWITCH_KEY(vSwitchKeyStatus) != 0xFE)) {
				switchLanguage();
				_hasJustUsedHotKey = true;
			}
			if (checkHotKey(convertToolHotKey, GET_SWITCH_KEY(convertToolHotKey) != 0xFE)) {
				AppDelegate::getInstance()->onQuickConvert();
				_hasJustUsedHotKey = true;
			}
			//check temporarily turn off spell checking
			if (vTempOffSpelling && !_hasJustUsedHotKey && _lastFlag & MASK_CONTROL) {
				vTempOffSpellChecking();
			}
			if (vTempOffOpenKey && !_hasJustUsedHotKey && _lastFlag & MASK_ALT) {
				vTempOffEngine();
			}
			_lastFlag = _flag;
			_hasJustUsedHotKey = false;
		}
		_keycode = 0;
		return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
	}

	//if is in english mode
	if (vLanguage == 0) {
		if (vUseMacro && vUseMacroInEnglishMode && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
			vEnglishMode(((wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) ? vKeyEventState::KeyDown : vKeyEventState::MouseDown),
				_keycode,
				(_flag & MASK_SHIFT) || (_flag & MASK_CAPITAL),
				OTHER_CONTROL_KEY);

			if (pData->code == vReplaceMaro) { //handle macro in english mode
				handleMacro();
				return NULL;
			}
		}
		return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
	}

	//handle keyboard
	if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
		//send event signal to Engine
		vKeyHandleEvent(vKeyEvent::Keyboard,
						vKeyEventState::KeyDown,
						_keycode,
						(_flag & MASK_SHIFT && _flag & MASK_CAPITAL) ? 0 : (_flag & MASK_SHIFT ? 1 : (_flag & MASK_CAPITAL ? 2 : 0)),
						OTHER_CONTROL_KEY);
		if (pData->code == vDoNothing) { //do nothing
			if (IS_DOUBLE_CODE(vCodeTable)) { //VNI
				if (pData->extCode == 1) { //break key
					_syncKey.clear();
				} else if (pData->extCode == 2) { //delete key
					if (_syncKey.size() > 0) {
						if (_syncKey.back() > 1 && (vCodeTable == 2 || vCodeTable == 3)) {
							//send one more backspace
							SendInput(2, backspaceEvent, sizeof(INPUT));
						}
						_syncKey.pop_back();
					}
				} else if (pData->extCode == 3) { //normal key
					InsertKeyLength(1);
				}
			}
			return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
		} else if (pData->code == vWillProcess || pData->code == vRestore || pData->code == vRestoreAndStartNewSession) { //handle result signal
			//fix autocomplete
			if (vFixRecommendBrowser && pData->extCode != 4) {
				if (vFixChromiumBrowser && 
					std::find(_chromiumBrowser.begin(), _chromiumBrowser.end(), OpenKeyHelper::getLastAppExecuteName()) != _chromiumBrowser.end()) {
					SendCombineKey(KEY_LEFT_SHIFT, KEY_LEFT, 0, KEYEVENTF_EXTENDEDKEY);
					if (pData->backspaceCount == 1)
						pData->backspaceCount--;
				} else {
					SendEmptyCharacter();
					pData->backspaceCount++;
				}
			}
			
			//send backspace
			if (pData->backspaceCount > 0 && pData->backspaceCount < MAX_BUFF) {
				for (_i = 0; _i < pData->backspaceCount; _i++) {
					SendBackspace();
				}
			}

			//send new character
			if (!vSendKeyStepByStep) {
				SendNewCharString();
			} else {
				if (pData->newCharCount > 0 && pData->newCharCount <= MAX_BUFF) {
					for (int i = pData->newCharCount - 1; i >= 0; i--) {
						SendKeyCode(pData->charData[i]);
					}
				}
				if (pData->code == vRestore || pData->code == vRestoreAndStartNewSession) {
					SendKeyCode(_keycode | ((_flag & MASK_CAPITAL) || (_flag & MASK_SHIFT) ? CAPS_MASK : 0));
				}
				if (pData->code == vRestoreAndStartNewSession) {
					startNewSession();
				}
			}
		} else if (pData->code == vReplaceMaro) { //MACRO
			handleMacro();
		}
		return -1; //consume event
	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK mouseHookProcess(int nCode, WPARAM wParam, LPARAM lParam) {
	mouseData = (MSLLHOOKSTRUCT *)lParam;
	switch (wParam) {
	case WM_LBUTTONDOWN:
	
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_NCXBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
	case WM_NCXBUTTONUP:
		//send event signal to Engine
		vKeyHandleEvent(vKeyEvent::Mouse, vKeyEventState::MouseDown, 0);
		if (IS_DOUBLE_CODE(vCodeTable)) { //VNI
			_syncKey.clear();
		}
		break;
	}
	return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

static bool isDialogOrOwnedWindow(HWND hwnd) {
	if (!hwnd) return false;
	HWND hOwner = GetWindow(hwnd, GW_OWNER);
	if (hOwner != NULL && hOwner != hwnd)
		return true;

	WCHAR className[128] = { 0 };
	if (GetClassNameW(hwnd, className, 128) > 0) {
		if (_wcsicmp(className, L"ThunderDFrame") == 0) return true;
		if (_wcsicmp(className, L"#32770") == 0) return true;
		if (_wcsicmp(className, L"bosa_sdm_XL") == 0) return true;
		if (_wcsicmp(className, L"bosa_sdm_Mso96") == 0) return true;
	}

	// Root owner differs from self -> it is a child/owned top-level window.
	HWND hRootOwner = GetAncestor(hwnd, GA_ROOTOWNER);
	if (hRootOwner != NULL && hRootOwner != hwnd)
		return true;

	// Heuristic for owner-less popup windows (dialogs shown without an owner by
	// some toolkits, messaging forms, plugins...). A real document window is
	// resizable and has both a caption and a system menu; an owner-less popup
	// dialog is not resizable and lacks the caption+system-menu pair.
	LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
	bool isResizable = (style & WS_THICKFRAME) != 0;
	bool hasCaption = (style & WS_CAPTION) == WS_CAPTION;
	bool hasSysMenu = (style & WS_SYSMENU) != 0;
	if ((style & WS_POPUP) && !isResizable && !(hasCaption && hasSysMenu)) {
		return true;
	}
	return false;
}

// Walk up the owner/ancestor chain so that a UserForm / dialog / owned popup
// inherits the code table rule of the workbook/document window that owns it.
static int resolveRuleCodeForWindow(const string& exe, HWND hwnd) {
	if (!hwnd) return -1;

	// Direct rule on the window itself (title may be e.g. "Tong hop ban hang - Date.xlsm - Excel").
	string title = OpenKeyHelper::getWindowTitleUtf8(hwnd);
	int ruleCode = -1;
	if (!title.empty()) {
		ruleCode = ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, title);
		if (ruleCode != -1) return ruleCode;
	}

	// Not a dialog / owned window -> nothing else to inherit from.
	if (!isDialogOrOwnedWindow(hwnd)) return -1;

	// Walk the owner / root-owner chain (bounded) looking for a matching rule.
	DWORD selfPid = 0;
	GetWindowThreadProcessId(hwnd, &selfPid);
	HWND cur = hwnd;
	for (int depth = 0; depth < 8; ++depth) {
		HWND next = GetWindow(cur, GW_OWNER);
		if (!next) {
			HWND root = GetAncestor(cur, GA_ROOTOWNER);
			if (root && root != cur) next = root;
		}
		if (!next || next == cur) break;
		cur = next;

		DWORD pid = 0;
		GetWindowThreadProcessId(cur, &pid);
		if (pid != selfPid) break; // do not inherit across processes

		string ownerTitle = OpenKeyHelper::getWindowTitleUtf8(cur);
		if (ownerTitle.empty()) continue;
		int code = ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, ownerTitle);
		if (code != -1) return code;
	}
	return -1;
}

// Shell / system / transient windows must never change the code table.
// Windows ships the Start menu, taskbar, desktop, search, action center, IME
// host... as separate executables/windows, so the single explorer.exe guard is
// not enough: touching the Start menu (e.g. to minimize an app) would otherwise
// be treated as "switched to another app without a rule" and force a Unicode
// fallback, and clicking the app's taskbar tab would not switch back.
static bool isNonAppContext(const string& exeName, HWND hwnd) {
	// The exe name preserves the on-disk casing, so lower-case a copy first.
	string exe = exeName;
	for (char& c : exe) {
		if (c >= 'A' && c <= 'Z') c += ('a' - 'A');
	}

	// Process-name based filter (exeName is already ASCII-lowercased).
	static const char* shellExes[] = {
		"explorer.exe",
		"startmenuexperiencehost.exe",
		"searchhost.exe",
		"searchapp.exe",
		"shellexperiencehost.exe",
		"textinputhost.exe",
		"lockapp.exe",
		"sihost.exe",
		"backgroundtaskhost.exe",
		"dwm.exe",
	};
	for (const char* name : shellExes) {
		if (exe == name) return true;
	}

	// Window-class based filter (Start, taskbar, desktop background...).
	if (hwnd) {
		WCHAR cls[128] = { 0 };
		if (GetClassNameW(hwnd, cls, 128) > 0) {
			static const wchar_t* shellClasses[] = {
				L"Shell_TrayWnd",
				L"Shell_SecondaryTrayWnd",
				L"Progman",
				L"WorkerW",
				L"Windows.UI.Core.CoreWindow",
				L"ForegroundStaging",
				L"MultitaskingViewFrame",
				L"XamlExplorerHostIslandWindow",
			};
			for (const wchar_t* c : shellClasses) {
				if (_wcsicmp(cls, c) == 0) return true;
			}
		}
	}
	return false;
}

// Lazy safety net: re-evaluate the rule of the focused window. Called on the
// first keystroke after a foreground change so a missed/raced foreground event
// cannot leave the code table stuck.
static void recheckRuleForCurrentWindow() {
	if (!vAutoSwitchCodeTable) return;

	HWND hForeground = GetForegroundWindow();
	if (!hForeground) return;

	string& currentExe = OpenKeyHelper::getFrontMostAppExecuteName();
	if (isNonAppContext(currentExe, hForeground)) return;

	int ruleCode = resolveRuleCodeForWindow(currentExe, hForeground);
	if (ruleCode != -1 && vCodeTable != ruleCode) {
		AppDelegate::getInstance()->onTableCode(ruleCode);
		SystemTrayHelper::updateData();
	}
}


VOID CALLBACK winEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	// Throttle EVENT_OBJECT_NAMECHANGE: window titles change very frequently
	// (browsers, editors, clocks...). Only re-evaluate when the title actually
	// changed and at most once every 250ms to avoid CPU churn and mid-typing
	// code table flips.
	static DWORD _lastTitleEventTime = 0;
	static string _lastProcessedTitle = "";
	static HWND _lastTitleHwnd = NULL;

	static HWND _lastFocusHwnd = NULL;
	static LONG _lastFocusObj = 0;
	static LONG _lastFocusChild = 0;

	if (dwEvent == EVENT_OBJECT_NAMECHANGE) {
		if (idObject != OBJID_WINDOW || hwnd != GetForegroundWindow())
			return;

		DWORD now = GetTickCount();
		if (now - _lastTitleEventTime < 250)
			return;
		_lastTitleEventTime = now;

		string currentTitle = OpenKeyHelper::getFrontMostWindowTitleUtf8();
		if (hwnd == _lastTitleHwnd && currentTitle == _lastProcessedTitle)
			return;
		_lastTitleHwnd = hwnd;
		_lastProcessedTitle = currentTitle;
	} else if (dwEvent == EVENT_OBJECT_FOCUS) {
		// Ignore caret/cursor events
		if (idObject == OBJID_CARET || idObject == OBJID_CURSOR)
			return;

		// Deduplicate: ignore if the exact same control/object already has focus
		if (hwnd == _lastFocusHwnd && idObject == _lastFocusObj && idChild == _lastFocusChild)
			return;

		_lastFocusHwnd = hwnd;
		_lastFocusObj = idObject;
		_lastFocusChild = idChild;

		// Focus changes between controls/textboxes:
		// Reset input engine and macro buffers so typing in the new control starts fresh.
		// DO NOT re-evaluate code table or language switches on control focus!
		resetInputState();
		_syncKey.clear();
		_lastFlag = 0;
		_hasJustUsedHotKey = false;
		_keycode = 0;
		return;
	} else if (dwEvent == EVENT_SYSTEM_FOREGROUND) {
		_lastFocusHwnd = hwnd;
		_lastFocusObj = idObject;
		_lastFocusChild = idChild;

		// A new foreground window owns the title now: invalidate the title-dedup
		// cache so the first EVENT_OBJECT_NAMECHANGE for this window is not
		// wrongly skipped (would otherwise delay the code-table evaluation).
		_lastTitleHwnd = NULL;
		_lastProcessedTitle.clear();
	} else if (dwEvent == EVENT_SYSTEM_MINIMIZESTART || dwEvent == EVENT_SYSTEM_MINIMIZEEND) {
		// Minimizing/restoring must never change the code table (the window is
		// still the same document). Only reset the typing session.
		resetInputState();
		_syncKey.clear();
		_lastFlag = 0;
		_hasJustUsedHotKey = false;
		_keycode = 0;
		return;
	}

	string& exe = OpenKeyHelper::getFrontMostAppExecuteName();

	// Arm the lazy re-check for any context change (even shell ones): the next
	// keystroke in a real app then recovers its code table deterministically.
	if (dwEvent == EVENT_SYSTEM_FOREGROUND || dwEvent == EVENT_OBJECT_NAMECHANGE) {
		_needRuleRecheck = true;
	}

	// Shell / Start menu / taskbar / desktop / search ... must not change the
	// code table. Keep whatever the active application is using.
	if (isNonAppContext(exe, hwnd))
		return;

	// Read the remembered input-method status (language + code table packed) for
	// this app once, used by both the language smart-switch and the
	// "remember table code" feature. -1 means this app has no remembered status.
	if (dwEvent == EVENT_SYSTEM_FOREGROUND && vUseSmartSwitchKey) {
		_languageTemp = getAppInputMethodStatus(exe, vLanguage | (vCodeTable << 1));
	} else {
		_languageTemp = -1;
	}

	// 1. Process & Title recognition rule (only active when not locked)
	if (vAutoSwitchCodeTable) {
		HWND hForeground = GetForegroundWindow();
		bool isDialogOrForm = isDialogOrOwnedWindow(hForeground);

		// Resolve the rule for the foreground window, walking the owner chain so
		// that dialogs / UserForms / owned popups inherit the rule of the
		// workbook/document window that owns them.
		int ruleCode = resolveRuleCodeForWindow(exe, hForeground);

		if (ruleCode != -1) {
			// Explicit rule matched (directly or inherited from the owner).
			if (vCodeTable != ruleCode) {
				AppDelegate::getInstance()->onTableCode(ruleCode);
				SystemTrayHelper::updateData();
			}
		} else if (!isDialogOrForm) {
			// No rule matched for a *document* window.
			// Documented priority chain:
			//   Explicit rule -> Fallback to Unicode -> Remembered code -> Keep.
			// A dialog / UserForm without its own rule keeps the current code
			// table of the active document (it must never force a fallback).
			int rememberedCode = (_languageTemp >> 1);
			if (vFallbackToUnicode) {
				if (vCodeTable != 0) {
					AppDelegate::getInstance()->onTableCode(0);
					SystemTrayHelper::updateData();
				}
			} else if (vRememberCode && _languageTemp != -1) {
				// Restore the code table the user last used inside this app
				// (applies both across apps and between documents of one app,
				// e.g. two Excel workbooks).
				if (vCodeTable != rememberedCode) {
					AppDelegate::getInstance()->onTableCode(rememberedCode);
					SystemTrayHelper::updateData();
				}
			}
		}
		// A dialog / UserForm with no rule: keep the current code table.
	}

	// 2. Smart switch key (for Vietnamese/English language toggle only)
	if (dwEvent == EVENT_SYSTEM_FOREGROUND && vUseSmartSwitchKey) {
		vTempOffEngine(false);
		if ((_languageTemp & 0x01) != vLanguage) {
			if (_languageTemp != -1) {
				// _languageTemp packs (language | (codeTable << 1)); extract ONLY
				// the language bit, otherwise vLanguage gets poisoned with the
				// code table value and the English/Vietnamese mode breaks.
				vLanguage = _languageTemp & 0x01;
				AppDelegate::getInstance()->onInputMethodChangedFromHotKey();
			} else {
				saveSmartSwitchKeyData();
			}
		}
	}

	if (dwEvent == EVENT_SYSTEM_FOREGROUND) {
		resetInputState();
		_syncKey.clear();
		_lastFlag = 0;
		_hasJustUsedHotKey = false;
		_keycode = 0;

		// Allow the lazy re-check to recover the rule for this window on the
		// next keystroke in case anything above did not settle the code table.
		_needRuleRecheck = true;

		if (vSupportMetroApp && exe.compare("ApplicationFrameHost.exe") == 0) {//Metro App
			SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
			SendMessage(HWND_BROADCAST, WM_CHAR, VK_BACK, 0L);
		}
	}
}
