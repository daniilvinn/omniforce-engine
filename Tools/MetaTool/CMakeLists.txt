# CMakeList.txt : CMake project for Omniforce Engine, include source and define
# project specific logic here.
#

include("${CMAKE_SOURCE_DIR}/utils.cmake")

# Add source to this project's executable.
set(PATH_ENV $ENV{PATH})

foreach(dir IN LISTS PATH_ENV)
    if(dir MATCHES "LLVM")
        get_filename_component(LLVM_ROOT "${dir}" DIRECTORY)
        message(STATUS "Path to LLVM: ${LLVM_ROOT}")
        break()
    endif()
endforeach()

if(NOT LLVM_ROOT)
    message(FATAL_ERROR "Cannot find LLVM")
endif()

file (GLOB_RECURSE METATOOL_FILES
    "Source/*.h"
    "Source/*.cpp"
    "Source/*.hpp"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${METATOOL_FILES})

add_executable(${METATOOL_TARGET} ${METATOOL_FILES})

target_include_directories(${METATOOL_TARGET}
    SYSTEM PRIVATE
        "Source/"
        "${LLVM_ROOT}\\include"
        "${CMAKE_SOURCE_DIR}/Omniforce/ThirdParty/nlohmann-json/single_include"
        "${CMAKE_SOURCE_DIR}/Omniforce/ThirdParty/spdlog/include"
)
target_link_directories(${METATOOL_TARGET} PRIVATE 
    "${LLVM_ROOT}\\lib"
)

get_target_property(ENGINE_INCLUDE_DIRS ${ENGINE_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(ENGINE_DEFINES ${ENGINE_TARGET} COMPILE_DEFINITIONS)

set(CLANG_PARSER_ARGS "\"-x\", \"c++\", \"-fsyntax-only\", \"-emit-ast\", \"-std=c++23\", \"-Wno-everything\", \"-DMETATOOL\"")

foreach(INCLUDE_DIR ${ENGINE_INCLUDE_DIRS})
    string(REPLACE "\\" "\\\\" FORMATTED_INCLUDE_DIR ${INCLUDE_DIR})
    list(APPEND CLANG_PARSER_ARGS "\"-I${FORMATTED_INCLUDE_DIR}\"")
endforeach()

foreach(DEFINE ${ENGINE_DEFINES})
    list(APPEND CLANG_PARSER_ARGS "\"-D${DEFINE}\"")
endforeach()

string(JOIN ", " CLANG_PARSER_ARGS_STR ${CLANG_PARSER_ARGS})

target_compile_definitions(${METATOOL_TARGET} PRIVATE 
    OMNIFORCE_STATIC
    NOMINMAX
    "METATOOL_WORKING_DIRECTORY=\"${CMAKE_BINARY_DIR}/MetaToolGen\""
    "METATOOL_OUTPUT_DIRECTORY=\"${CMAKE_SOURCE_DIR}/Omniforce/Shaders\""
    "$<$<CONFIG:Debug>:OMNIFORCE_DEBUG>"
    "$<$<CONFIG:Debug>:VMA_LEAK_LOG_FORMAT>"
    "$<$<CONFIG:RelWithDebInfo>:OMNIFORCE_RELWITHDEBINFO>"
    "$<$<CONFIG:Release>:OMNIFORCE_RELEASE>"
    WIN32_LEAN_AND_MEAN
    "CLANG_PARSER_ARGS=${CLANG_PARSER_ARGS_STR}"
)

target_link_libraries(
    ${METATOOL_TARGET}
    PRIVATE
        libclang.lib
        OpenMP::OpenMP_CXX
)

target_compile_options(
    ${METATOOL_TARGET}
    PUBLIC
        /MP
        /W1
)

set_property(TARGET ${METATOOL_TARGET} PROPERTY CXX_STANDARD 23)
set_property(TARGET ${METATOOL_TARGET} PROPERTY FOLDER "MetaToolActions")

omni_set_project_ide_folder(${METATOOL_TARGET} ${CMAKE_CURRENT_SOURCE_DIR})