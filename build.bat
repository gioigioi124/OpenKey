@echo off
setlocal enabledelayedexpansion

echo ==============================================
echo   Building OpenKey with UTF-8 / Vietnamese UI
echo ==============================================

cd Sources\OpenKey\win32\OpenKey\OpenKey

echo [1/3] Compiling resources with codepage 65001...
windres.exe --codepage=65001 -O coff OpenKey.rc -o OpenKey.res
if errorlevel 1 (
    echo Resource compilation failed!
    exit /b 1
)

echo [2/3] Compiling C++ sources...
clang++ -std=c++14 -O2 -D_WIN32 -DUNICODE -D_UNICODE -I. -I..\..\..\engine -c ^
  ..\..\..\engine\ConvertTool.cpp ^
  ..\..\..\engine\Engine.cpp ^
  ..\..\..\engine\Macro.cpp ^
  ..\..\..\engine\SmartSwitchKey.cpp ^
  ..\..\..\engine\Vietnamese.cpp ^
  AboutDialog.cpp ^
  AppDelegate.cpp ^
  BaseDialog.cpp ^
  ConvertToolDialog.cpp ^
  MacroDialog.cpp ^
  main.cpp ^
  MainControlDialog.cpp ^
  OpenKey.cpp ^
  OpenKeyHelper.cpp ^
  OpenKeyManager.cpp ^
  ProcessRuleHelper.cpp ^
  SystemTrayHelper.cpp ^
  stdafx.cpp
if errorlevel 1 (
    echo Source compilation failed!
    exit /b 1
)

echo [3/3] Linking OpenKey.exe...
clang++ -mwindows -municode -std=c++14 -O2 *.o OpenKey.res -o OpenKey.exe ^
  -lcomctl32 -limm32 -lversion -lurlmon -lpsapi -lshell32 -lole32 -luxtheme -luuid
if errorlevel 1 (
    echo Linking failed!
    exit /b 1
)

echo Cleaning intermediate files...
del *.o *.res

copy /Y OpenKey.exe ..\..\..\..\..\OpenKey.exe
cd ..\..\..\..\..

echo ==============================================
echo   Build Successful! OpenKey.exe is updated.
echo ==============================================
