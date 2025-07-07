@echo off
echo Generating compile_commands.json for clangd/Cursor...

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

cd ..\..
echo Done! 