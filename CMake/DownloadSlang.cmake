# CMake script to download and setup Slang shader language from GitHub releases
# This script downloads the latest Slang release for Windows x64 and sets up
# the necessary variables for the build system

include(FetchContent)

# Function to download and setup Slang from GitHub releases
function(download_slang)
    # Define the target directory for Slang
    get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
    set(SLANG_TARGET_DIR "${PROJECT_ROOT}/Omniforce/ThirdParty/slang")
    
    # Check if Slang is already installed
    if(EXISTS "${SLANG_TARGET_DIR}/bin/slang.dll" AND EXISTS "${SLANG_TARGET_DIR}/lib/slang.lib")
        message(STATUS "Slang already exists in ${SLANG_TARGET_DIR}")
        setup_slang_variables("${SLANG_TARGET_DIR}")
        return()
    endif()
    
    # GitHub API endpoint for latest release
    set(SLANG_RELEASES_API "https://api.github.com/repos/shader-slang/slang/releases/latest")
    
    # Create a temporary directory for downloading
    set(TEMP_DIR "${CMAKE_BINARY_DIR}/slang_download")
    file(MAKE_DIRECTORY ${TEMP_DIR})
    
    # Download latest release info
    message(STATUS "Fetching latest Slang release information...")
    file(DOWNLOAD 
        "${SLANG_RELEASES_API}" 
        "${TEMP_DIR}/release_info.json"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    
    # Check if download was successful
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(NOT STATUS_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download Slang release information")
    endif()
    
    # Read the JSON file
    file(READ "${TEMP_DIR}/release_info.json" RELEASE_JSON)
    
    # Extract tag name (version) from JSON
    string(REGEX MATCH "\"tag_name\"[ \t\n\r]*:[ \t\n\r]*\"([^\"]+)\"" TAG_MATCH "${RELEASE_JSON}")
    if(TAG_MATCH)
        set(SLANG_VERSION "${CMAKE_MATCH_1}")
        message(STATUS "Latest Slang version: ${SLANG_VERSION}")
    else()
        message(FATAL_ERROR "Could not extract version from release info")
    endif()
    
    # Strip "v" prefix from version for the filename
    string(REGEX REPLACE "^v" "" SLANG_VERSION_NUMBER "${SLANG_VERSION}")
    
    # Construct download URL for Windows x64 release
    set(SLANG_DOWNLOAD_URL "https://github.com/shader-slang/slang/releases/download/${SLANG_VERSION}/slang-${SLANG_VERSION_NUMBER}-windows-x86_64.zip")
    set(SLANG_ARCHIVE_NAME "slang-${SLANG_VERSION_NUMBER}-windows-x86_64.zip")
    
    message(STATUS "Download URL: ${SLANG_DOWNLOAD_URL}")
    
    # Download the Slang archive
    message(STATUS "Downloading Slang ${SLANG_VERSION} for Windows x64...")
    file(DOWNLOAD 
        "${SLANG_DOWNLOAD_URL}" 
        "${TEMP_DIR}/${SLANG_ARCHIVE_NAME}"
        SHOW_PROGRESS
        STATUS DOWNLOAD_STATUS
    )
    
    # Check if download was successful
    list(GET DOWNLOAD_STATUS 0 STATUS_CODE)
    if(NOT STATUS_CODE EQUAL 0)
        message(FATAL_ERROR "Failed to download Slang archive from ${SLANG_DOWNLOAD_URL}")
    endif()
    
    # Create the target directory if it doesn't exist
    file(MAKE_DIRECTORY ${SLANG_TARGET_DIR})
    
    # Extract the archive
    message(STATUS "Extracting Slang to ${SLANG_TARGET_DIR}...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf "${TEMP_DIR}/${SLANG_ARCHIVE_NAME}"
        WORKING_DIRECTORY ${SLANG_TARGET_DIR}
        RESULT_VARIABLE EXTRACT_RESULT
    )
    
    if(NOT EXTRACT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to extract Slang archive")
    endif()
    
    # The extracted content might be in a subdirectory, so we need to move it up
    file(GLOB EXTRACTED_DIRS "${SLANG_TARGET_DIR}/*")
    list(LENGTH EXTRACTED_DIRS NUM_DIRS)
    
    if(NUM_DIRS EQUAL 1)
        list(GET EXTRACTED_DIRS 0 EXTRACTED_DIR)
        if(IS_DIRECTORY "${EXTRACTED_DIR}")
            # Move contents up one level
            file(GLOB EXTRACTED_CONTENTS "${EXTRACTED_DIR}/*")
            foreach(ITEM ${EXTRACTED_CONTENTS})
                get_filename_component(ITEM_NAME ${ITEM} NAME)
                file(RENAME ${ITEM} "${SLANG_TARGET_DIR}/${ITEM_NAME}")
            endforeach()
            # Remove the now empty directory
            file(REMOVE_RECURSE ${EXTRACTED_DIR})
        endif()
    endif()
    
    # Clean up temporary directory
    file(REMOVE_RECURSE ${TEMP_DIR})
    
    # Verify installation
    if(EXISTS "${SLANG_TARGET_DIR}/bin/slang.dll" AND EXISTS "${SLANG_TARGET_DIR}/lib/slang.lib")
        message(STATUS "Slang ${SLANG_VERSION} successfully installed to ${SLANG_TARGET_DIR}")
    else()
        message(FATAL_ERROR "Slang installation verification failed - expected files not found")
    endif()
    
    # Set up variables
    setup_slang_variables("${SLANG_TARGET_DIR}")
endfunction()

# Function to setup Slang variables
function(setup_slang_variables SLANG_ROOT_DIR)
    # Use the passed SLANG_ROOT_DIR parameter instead of calculating it
    
    # Define the Slang directories
    set(SLANG_INCLUDE_DIR "${SLANG_ROOT_DIR}/include" CACHE PATH "Slang include directory")
    set(SLANG_LIBRARY_DIR "${SLANG_ROOT_DIR}/lib" CACHE PATH "Slang library directory")
    set(SLANG_BINARY_DIR "${SLANG_ROOT_DIR}/bin" CACHE PATH "Slang binary directory")
    
    # Make variables available to parent scope
    set(SLANG_INCLUDE_DIR ${SLANG_INCLUDE_DIR} PARENT_SCOPE)
    set(SLANG_LIBRARY_DIR ${SLANG_LIBRARY_DIR} PARENT_SCOPE)  
    set(SLANG_BINARY_DIR ${SLANG_BINARY_DIR} PARENT_SCOPE)
    
    # Verify the directories exist
    if(NOT EXISTS ${SLANG_INCLUDE_DIR})
        message(FATAL_ERROR "Slang include directory not found: ${SLANG_INCLUDE_DIR}")
    endif()
    
    if(NOT EXISTS ${SLANG_LIBRARY_DIR})
        message(FATAL_ERROR "Slang library directory not found: ${SLANG_LIBRARY_DIR}")
    endif()
    
    if(NOT EXISTS ${SLANG_BINARY_DIR})
        message(FATAL_ERROR "Slang binary directory not found: ${SLANG_BINARY_DIR}")
    endif()
    
    message(STATUS "Slang variables set:")
    message(STATUS "  SLANG_INCLUDE_DIR: ${SLANG_INCLUDE_DIR}")
    message(STATUS "  SLANG_LIBRARY_DIR: ${SLANG_LIBRARY_DIR}")
    message(STATUS "  SLANG_BINARY_DIR: ${SLANG_BINARY_DIR}")
endfunction()

# Call the download function if this file is included
if(CMAKE_CURRENT_LIST_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
    download_slang()
endif() 