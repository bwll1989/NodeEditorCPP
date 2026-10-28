@echo off
chcp 936 >nul
setlocal
title Build WebInterface
pushd "%~dp0"

echo ============================================
echo   构建网页控制台
echo ============================================
echo.

where node >nul 2>nul
if errorlevel 1 (
    echo [ERROR] 未找到 Node.js。请先安装并加入 PATH。
    goto :fail
)

echo 正在执行 npm run build ...
echo.
call npm run build
if errorlevel 1 goto :fail

set "DEST=%~dp0..\build\bin\www"
if not exist "%DEST%" mkdir "%DEST%"

echo.
echo 正在替换 %DEST%
echo.
rem /MIR 使 www 与 dist 一致，清掉旧的带哈希文件名的脚本。
robocopy "%~dp0dist" "%DEST%" /MIR /NFL /NDL /NJH /NJS /NP
if %ERRORLEVEL% GEQ 8 goto :fail

echo.
echo 网页已更新：build\bin\www
echo 重新打开网页控制台即可，不必重编 FlowViewer.exe。
echo.
pause
popd
endlocal
exit /b 0

:fail
echo.
echo [ERROR] 网页构建失败。
echo.
pause
popd
endlocal
exit /b 1
