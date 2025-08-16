# BuildConfigurations.cmake
# Custom build configuration setup for Omniforce Engine
#
# This function customizes the available build configurations:
# 1. Removes RelMinSize configuration
# 2. Renames RelWithDebInfo to Development

function(omni_setup_build_configurations)
	# Only apply customizations for multi-config generators (Visual Studio, Xcode)
	if(CMAKE_CONFIGURATION_TYPES)
		# Define our custom configurations (removes RelMinSize, renames RelWithDebInfo to Development)
		set(CMAKE_CONFIGURATION_TYPES "Debug;Development;Release" CACHE STRING "" FORCE)
		
		# Map Development configuration to RelWithDebInfo internally
		set(CMAKE_CXX_FLAGS_Development "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)
		set(CMAKE_C_FLAGS_Development "${CMAKE_C_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)
		set(CMAKE_EXE_LINKER_FLAGS_Development "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)
		set(CMAKE_MODULE_LINKER_FLAGS_Development "${CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)
		set(CMAKE_SHARED_LINKER_FLAGS_Development "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)
		
		message(STATUS "Custom build configurations: Debug, Development, Release")
		message(STATUS "  - Development maps to RelWithDebInfo")
	endif()
	
	# Handle single-config generators (Ninja, Make) when specific build types are requested
	if(CMAKE_BUILD_TYPE STREQUAL "Development")
		# Development is just RelWithDebInfo with a different name
		set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "" FORCE)
		message(STATUS "Development build type mapped to RelWithDebInfo")
	endif()
endfunction()
