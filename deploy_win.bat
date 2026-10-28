@echo off
rem Assembles a standalone polarSA folder after a Release build (called from PolarSA.pro).
rem Usage: deploy_win.bat <BuildOutDir> <TargetName> <QtBinDir> <SourceDir> <DeployDir>
setlocal

set "OUT_DIR=%~1"
set "TARGET_NAME=%~2"
set "QT_BIN_DIR=%~3"
set "SRC_DIR=%~4"
set "DEPLOY_DIR=%~5"

set "TARGET_EXE=%OUT_DIR%\%TARGET_NAME%.exe"
if not exist "%TARGET_EXE%" set "TARGET_EXE=%OUT_DIR%\release\%TARGET_NAME%.exe"
if not exist "%TARGET_EXE%" (
    echo [deploy_win] ERROR: %TARGET_NAME%.exe not found under %OUT_DIR%
    exit /b 1
)

if not exist "%DEPLOY_DIR%" mkdir "%DEPLOY_DIR%"
copy /Y "%TARGET_EXE%" "%DEPLOY_DIR%\" >nul || (
    echo [deploy_win] ERROR: failed to copy %TARGET_EXE%
    exit /b 1
)

"%QT_BIN_DIR%\windeployqt.exe" --release --compiler-runtime --no-translations "%DEPLOY_DIR%\%TARGET_NAME%.exe"
if errorlevel 1 echo [deploy_win] WARNING: windeployqt failed, the deploy folder may be incomplete

for %%D in (limits calibration) do (
    xcopy "%SRC_DIR%\%%D" "%DEPLOY_DIR%\%%D\" /E /I /Y /Q >nul
    if errorlevel 1 echo [deploy_win] WARNING: failed to copy %%D
)

echo [deploy_win] Deployment folder ready: %DEPLOY_DIR%
endlocal
exit /b 0
