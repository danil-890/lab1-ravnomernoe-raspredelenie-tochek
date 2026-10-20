@echo off
setlocal
if not defined BDS (
  if exist "C:\Program Files (x86)\Embarcadero\Studio\37.0\bin\rsvars.bat" (
    call "C:\Program Files (x86)\Embarcadero\Studio\37.0\bin\rsvars.bat"
  ) else (
    echo Open this script from the RAD Studio Command Prompt, or set BDS.
    exit /b 1
  )
)
if not defined GLSCENE_ROOT (
  if exist "%~dp0..\GLXEngine" (
    set "GLSCENE_ROOT=%~dp0..\GLXEngine"
  ) else (
    set "GLSCENE_ROOT=%~dp0..\..\..\GLXEngine"
  )
)
if not defined WINSDK_ROOT (
  if exist "%~dp0..\WindowsSDK\37.0\windows.h" (
    set "WINSDK_ROOT=%~dp0..\WindowsSDK\37.0"
  ) else (
    if exist "%~dp0..\..\..\WindowsSDK\37.0\windows.h" (
      set "WINSDK_ROOT=%~dp0..\..\..\WindowsSDK\37.0"
    ) else (
      set "WINSDK_ROOT=%BDS%\include\windows\sdk"
    )
  )
)
msbuild "%~dp0RandomStars_cpp.cbproj" /t:Build /p:Config=Release /p:Platform=Win32 "/p:GLSCENE_ROOT=%GLSCENE_ROOT%" "/p:WINSDK_ROOT=%WINSDK_ROOT%" /v:minimal /nologo
exit /b %errorlevel%
