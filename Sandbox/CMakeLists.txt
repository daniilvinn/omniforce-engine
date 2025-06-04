include("../utils.cmake")

set(SANDBOX_TARGET Sandbox CACHE INTERNAL "")

file(GLOB_RECURSE SANDBOX_FILES
	"Source/*.h"
	"Source/*.cpp"
	"Source/*.hpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SANDBOX_FILES})

add_executable(${SANDBOX_TARGET} ${SANDBOX_FILES})

target_link_libraries(${SANDBOX_TARGET} PUBLIC OmniforceEngine ${OMNI_IMGUI_TARGET})
target_include_directories(${SANDBOX_TARGET} PUBLIC ${IMGUI_INCLUDE_PATH})

set_target_properties(${SANDBOX_TARGET} PROPERTIES
    CXX_STANDARD 23
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Runtime"
)

if(WIN32)
set_target_properties(${SANDBOX_TARGET} PROPERTIES
	WIN32_EXECUTABLE $<$<CONFIG:Release>:1>
)
endif()

# Setup target
SetupTarget(${SANDBOX_TARGET})

omni_set_project_ide_folder(${SANDBOX_TARGET} ${CMAKE_CURRENT_SOURCE_DIR})