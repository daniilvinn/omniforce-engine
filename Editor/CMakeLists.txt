include("../utils.cmake")

set(EDITOR_TARGET Editor CACHE INTERNAL "")

file(GLOB_RECURSE EDITOR_FILES
	"Source/*.h"
	"Source/*.cpp"
	"Source/*.hpp"
	"Source/*.c"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EDITOR_FILES})

add_executable(${EDITOR_TARGET} ${EDITOR_FILES})

target_link_libraries(${EDITOR_TARGET} PUBLIC OmniforceEngine ${OMNI_IMGUI_TARGET} OpenMP::OpenMP_CXX)
target_include_directories(${EDITOR_TARGET} SYSTEM PUBLIC ${IMGUI_INCLUDE_PATH} PUBLIC "Source")

set_target_properties(${EDITOR_TARGET} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/Editor"
)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${EDITOR_TARGET} PROPERTY CXX_STANDARD 23)
endif()

if(WIN32)
set_target_properties(${EDITOR_TARGET} PROPERTIES
	WIN32_EXECUTABLE $<$<CONFIG:Release>:1>
)
endif()

omni_set_project_ide_folder(${EDITOR_TARGET} ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS Editor DESTINATION "${CMAKE_SOURCE_DIR}/Install")

install(DIRECTORY Resources DESTINATION "${CMAKE_INSTALL_PREFIX}")
install(FILES imgui.ini DESTINATION "${CMAKE_INSTALL_PREFIX}")
