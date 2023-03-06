set(SANDBOX_TARGET Sandbox CACHE INTERNAL "")

file(GLOB_RECURSE SANDBOX_FILES
	"src/*.h"
	"src/*.cpp"
	"src/*.hpp"
)

add_executable(${SANDBOX_TARGET} ${SANDBOX_FILES})

target_link_libraries(${SANDBOX_TARGET} PRIVATE CursedEngine)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${SANDBOX_TARGET} PROPERTY CXX_STANDARD 20)
endif()

if(WIN32)
set_target_properties(${SANDBOX_TARGET} PROPERTIES
	WIN32_EXECUTABLE $<$<CONFIG:Release>:1>
)
endif()