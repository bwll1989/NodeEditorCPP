@echo off
chcp 936 >nul
setlocal
title Build FlowViewer
pushd "%~dp0"

echo ============================================
echo   构建桌面控制台 exe
echo ============================================
echo.

where node >nul 2>nul
if errorlevel 1 (
    echo [ERROR] 未找到 Node.js。请先安装并加入 PATH。
    goto :fail
)

set "PATH=%USERPROFILE%\.cargo\bin;%PATH%"
where cargo >nul 2>nul
if errorlevel 1 (
    echo [ERROR] 未找到 cargo。请先安装 rustup 后再运行本脚本。
    goto :fail
)

echo 正在执行 npm run desktop:build ...
echo.
call npm run desktop:build
if errorlevel 1 goto :fail

set "SRC=%~dp0src-tauri\target\release\FlowViewer.exe"
set "BIN=%~dp0..\build\bin"
if not exist "%BIN%" mkdir "%BIN%"

copy /Y "%SRC%" "%BIN%\FlowViewer.exe" >nul
if errorlevel 1 goto :fail

echo.
echo 已生成：
echo   %SRC%
echo   %BIN%\FlowViewer.exe
echo.
echo 只改了控制台页面时不必运行本脚本，用 build-web.bat 替换 www 即可。
echo.
pause
popd
endlocal
exit /b 0

:fail
echo.
echo [ERROR] exe 构建失败。
echo.
pause
popd
endlocal
exit /b 1
