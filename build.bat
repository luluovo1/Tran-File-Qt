@echo off
setlocal ENABLEDELAYEDEXPANSION

rem ================= User Config =================
rem 可执行文件名称（与你生成的 exe 名一致）
set APP_EXE=TranFile.exe

rem Qt 安装路径（确保指向你使用的 Qt 版本）
rem 例：set QTDIR=C:\Qt\6.5.3\msvc2019_64
rem 或：set QTDIR=C:\Qt\6.5.3\mingw_64
set QTDIR=E:\Windowstools\QT\6.9.1\mingw_64
rem Inno Setup 编译器路径（ISCC.exe）
rem 例：set ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe
set ISCC=E:\Windowstools\Inno Setup 6\ISCC.exe

rem 你的 Release 可执行文件所在目录
rem 按你工程实际修改：QtCreator 通常是 .\build\release 或 .\build\TranFile\release
set APP_RELEASE_DIR=.\build\Desktop_Qt_6_9_1_MinGW_64_bit-Release\release

rem 部署输出目录（windeployqt 输出）
set DEPLOY_DIR=.\deploy

rem Inno Setup 脚本路径（前一步我给你的 TranFile.iss）
set ISS_FILE=.\TranFile.iss
rem =================================================

echo.
echo === 1. 检查工具路径 ===
if not exist "%QTDIR%\bin\windeployqt.exe" (
  echo [ERROR] windeployqt 不存在: %QTDIR%\bin\windeployqt.exe
  goto :end
)
if not exist "%ISCC%" (
  echo [ERROR] Inno Setup 编译器 ISCC 不存在: %ISCC%
  goto :end
)

echo.
echo === 2. 准备 deploy 目录 ===
if exist "%DEPLOY_DIR%" (
  echo 清理旧的 %DEPLOY_DIR% ...
  rmdir /S /Q "%DEPLOY_DIR%"
)
mkdir "%DEPLOY_DIR%"

echo.
echo === 3. 拷贝 Release 可执行文件 ===
if not exist "%APP_RELEASE_DIR%\%APP_EXE%" (
  echo [ERROR] 未找到 Release 可执行文件：%APP_RELEASE_DIR%\%APP_EXE%
  echo 请先以 Release 构建本项目，或修改 APP_RELEASE_DIR/APP_EXE
  goto :end
)
copy /Y "%APP_RELEASE_DIR%\%APP_EXE%" "%DEPLOY_DIR%\%APP_EXE%" >nul

echo.
echo === 4. windeployqt 收集依赖 ===
"%QTDIR%\bin\windeployqt.exe" "%DEPLOY_DIR%\%APP_EXE%" --release --compiler-runtime
if %ERRORLEVEL% NEQ 0 (
  echo [ERROR] windeployqt 失败
  goto :end
)

echo.
echo === 5. 精简（可选：删掉不需要的目录/插件） ===
rem 视项目需求保留所需插件；下面仅作示例，按需注释/保留
if exist "%DEPLOY_DIR%\translations"       rmdir /S /Q "%DEPLOY_DIR%\translations"
if exist "%DEPLOY_DIR%\qml"                rmdir /S /Q "%DEPLOY_DIR%\qml"
if exist "%DEPLOY_DIR%\bearer"             rmdir /S /Q "%DEPLOY_DIR%\bearer"
if exist "%DEPLOY_DIR%\iconengines"        rmdir /S /Q "%DEPLOY_DIR%\iconengines"
if exist "%DEPLOY_DIR%\opengl32sw.dll"     del /F /Q  "%DEPLOY_DIR%\opengl32sw.dll"

echo.
echo === 6. 编译 Inno Setup 安装包 ===
"%ISCC%" "%ISS_FILE%"
if %ERRORLEVEL% NEQ 0 (
  echo [ERROR] Inno Setup 打包失败
  goto :end
)

echo.
echo === 完成 ===
echo 安装包输出目录：.\dist
dir .\dist

:end
echo.
pause
endlocal