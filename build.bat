@echo off
setlocal ENABLEDELAYEDEXPANSION

rem ================= User Config =================
rem ��ִ���ļ����ƣ��������ɵ� exe ��һ�£�
set APP_EXE=TranFile.exe

rem Qt ��װ·����ȷ��ָ����ʹ�õ� Qt �汾��
rem ����set QTDIR=C:\Qt\6.5.3\msvc2019_64
rem ��set QTDIR=C:\Qt\6.5.3\mingw_64
set QTDIR=E:\Windowstools\QT\6.9.1\mingw_64
rem Inno Setup ������·����ISCC.exe��
rem ����set ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe
set ISCC=E:\Windowstools\Inno Setup 6\ISCC.exe

rem ��� Release ��ִ���ļ�����Ŀ¼
rem ���㹤��ʵ���޸ģ�QtCreator ͨ���� .\build\release �� .\build\TranFile\release
set APP_RELEASE_DIR=.\build\Desktop_Qt_6_9_1_MinGW_64_bit-Release\release

rem �������Ŀ¼��windeployqt �����
set DEPLOY_DIR=.\deploy

rem Inno Setup �ű�·����ǰһ���Ҹ���� TranFile.iss��
set ISS_FILE=.\TranFile.iss
rem =================================================

echo.
echo === 1. ��鹤��·�� ===
if not exist "%QTDIR%\bin\windeployqt.exe" (
  echo [ERROR] windeployqt ������: %QTDIR%\bin\windeployqt.exe
  goto :end
)
if not exist "%ISCC%" (
  echo [ERROR] Inno Setup ������ ISCC ������: %ISCC%
  goto :end
)

echo.
echo === 2. ׼�� deploy Ŀ¼ ===
if exist "%DEPLOY_DIR%" (
  echo ����ɵ� %DEPLOY_DIR% ...
  rmdir /S /Q "%DEPLOY_DIR%"
)
mkdir "%DEPLOY_DIR%"

echo.
echo === 3. ���� Release ��ִ���ļ� ===
if not exist "%APP_RELEASE_DIR%\%APP_EXE%" (
  echo [ERROR] δ�ҵ� Release ��ִ���ļ���%APP_RELEASE_DIR%\%APP_EXE%
  echo ������ Release ��������Ŀ�����޸� APP_RELEASE_DIR/APP_EXE
  goto :end
)
copy /Y "%APP_RELEASE_DIR%\%APP_EXE%" "%DEPLOY_DIR%\%APP_EXE%" >nul

echo.
echo === 4. windeployqt �ռ����� ===
"%QTDIR%\bin\windeployqt.exe" "%DEPLOY_DIR%\%APP_EXE%" --release --compiler-runtime
if %ERRORLEVEL% NEQ 0 (
  echo [ERROR] windeployqt ʧ��
  goto :end
)

echo.
echo === 5. ���򣨿�ѡ��ɾ������Ҫ��Ŀ¼/����� ===
rem ����Ŀ���������������������ʾ��������ע��/����
if exist "%DEPLOY_DIR%\translations"       rmdir /S /Q "%DEPLOY_DIR%\translations"
if exist "%DEPLOY_DIR%\qml"                rmdir /S /Q "%DEPLOY_DIR%\qml"
if exist "%DEPLOY_DIR%\bearer"             rmdir /S /Q "%DEPLOY_DIR%\bearer"
if exist "%DEPLOY_DIR%\iconengines"        rmdir /S /Q "%DEPLOY_DIR%\iconengines"
if exist "%DEPLOY_DIR%\opengl32sw.dll"     del /F /Q  "%DEPLOY_DIR%\opengl32sw.dll"

echo.
echo === 6. ���� Inno Setup ��װ�� ===
"%ISCC%" "%ISS_FILE%"
if %ERRORLEVEL% NEQ 0 (
  echo [ERROR] Inno Setup ���ʧ��
  goto :end
)

echo.
echo === ��� ===
echo ��װ�����Ŀ¼��.\dist
dir .\dist

:end
echo.
pause
endlocal