CLS
@ECHO OFF
set SHADER_SRC=.\data\shaders\vulkan
set SHADER_OUT=.\data\shaders\vulkan\spirv
for /f %%i in ('dir /a-d /b %SHADER_SRC%') do (
    CALL %VULKAN_SDK%\Bin\glslc.exe %SHADER_SRC%\%%i -g -I data/shaders/vulkan/headers -o %SHADER_OUT%\%%i.spv 
    if %ERRORLEVEL% EQU 0 (echo %%i compiled successfully.) else (echo Failed to compile %%i.)
)

 

PAUSE