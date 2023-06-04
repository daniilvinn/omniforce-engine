set(EDITOR_TARGET Editor CACHE INTERNAL "")

file(GLOB_RECURSE SANDBOX_FILES
	"src/*.h"
	"src/*.cpp"
	"src/*.hpp"
)

add_executable(${EDITOR_TARGET} ${SANDBOX_FILES})

find_package(tinyfiledialogs CONFIG REQUIRED)

target_link_libraries(${EDITOR_TARGET} PUBLIC OmniforceEngine ${OMNI_IMGUI_TARGET} tinyfiledialogs::tinyfiledialogs)
target_include_directories(${EDITOR_TARGET} PUBLIC ${IMGUI_INCLUDE_PATH})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${EDITOR_TARGET} PROPERTY CXX_STANDARD 23)
endif()

if(WIN32)
set_target_properties(${EDITOR_TARGET} PROPERTIES
	WIN32_EXECUTABLE $<$<CONFIG:Release>:1>
)
endif()