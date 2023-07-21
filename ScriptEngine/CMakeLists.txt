set(SCRIPT_ENGINE_TARGET ScriptEngine CACHE INTERNAL "")

file(GLOB_RECURSE FILES_TO_COMPILE
	"src/*.cs"
)

project(ScriptEngine VERSION 0.1.0 LANGUAGES CSharp)

add_library(ScriptEngine SHARED ${FILES_TO_COMPILE})
set_property(TARGET ScriptEngine PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2")

set_target_properties(ScriptEngine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Resources/scripting/bin"
)