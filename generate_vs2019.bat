CLS

CALL premake\premake5.exe vs2019
@ECHO OFF
echo:
CHOICE /C:YN /N /M "Launch solution ? [Y/N]"
IF %ERRORLEVEL% EQU 1 GOTO LAUNCH
IF %ERRORLEVEL% EQU 2 GOTO END
ECHO %ERRORLEVEL%
:LAUNCH
    start Cardinalis.sln
:END
    PAUSE


