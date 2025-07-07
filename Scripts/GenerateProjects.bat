@echo off
echo Omniforce Engine Project Generator
echo ===================================
echo.
echo 1. Generate MSBuild projects (for C# and normal development)
echo 2. Generate compile_commands.json (for Cursor/clangd)
echo 3. Generate both
echo.
set /p choice="Choose option (1-3): "

if "%choice%"=="1" goto :msbuild
if "%choice%"=="2" goto :compile_commands
if "%choice%"=="3" goto :both
echo Invalid choice. Please run the script again.
pause
exit /b 1

:msbuild
echo.
echo Generating MSBuild projects...
cd ..
mkdir Build
cd Build
cmake ..
echo MSBuild projects generated successfully!
goto :end

:compile_commands
echo.
echo Generating compile_commands.json for Cursor/clangd...
cd ..
mkdir Build
cd Build
if not exist "BuildClangd" mkdir BuildClangd
cd BuildClangd
cmake -G "Ninja" ..\..
if exist "compile_commands.json" (
    copy "compile_commands.json" "..\..\compile_commands.json"
    echo compile_commands.json generated and copied to project root
) else (
    echo Failed to generate compile_commands.json
)
goto :end

:both
echo.
echo Generating both MSBuild projects and compile_commands.json...
cd ..
mkdir Build
cd Build
cmake ..
if not exist "BuildClangd" mkdir BuildClangd
cd BuildClangd
cmake -G "Ninja" ..\..
if exist "compile_commands.json" (
    copy "compile_commands.json" "..\..\compile_commands.json"
    echo compile_commands.json generated and copied to project root
) else (
    echo Failed to generate compile_commands.json
)
cd ..
echo Both configurations generated successfully!

:end
cd ..
echo.
echo Done!
pause