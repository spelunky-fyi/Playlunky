cmake_minimum_required(VERSION 3.9)
project(Playlunky LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/publish")


# --------------------------------------------------
# Run conan
set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR} ${CMAKE_MODULE_PATH})
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR} ${CMAKE_PREFIX_PATH})
get_property(IS_MULTI_GENERATOR GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT IS_MULTI_GENERATOR)
	# The default option is only useful to write package functions for binary libraries
	# Unsetting the default here does not make any difference for multi-generators, yet causes CI to fail
	SET(CONAN_EXTRA_OPTIONS NO_OUTPUT_DIRS)
endif()

include(cmake/conan.cmake)
run_conan()


# --------------------------------------------------
# Find packages
find_package(fmt REQUIRED)


# --------------------------------------------------
# Create interface libs
add_library(playlunky_warnings INTERFACE)
if(MSVC)
	target_compile_options(playlunky_warnings INTERFACE /W4 /WX /permissive-)
else()
	target_compile_options(playlunky_warnings INTERFACE -Wall -Wextra -pedantic -Werror)
endif()

add_library(playlunky_dependencies INTERFACE)
target_include_directories(playlunky_dependencies INTERFACE
	lib/include)
target_link_directories(playlunky_dependencies INTERFACE
	lib/detours)
target_link_libraries(playlunky_dependencies INTERFACE
	fmt::fmt)
target_link_libraries(playlunky_dependencies INTERFACE
	debug detours_d)
target_link_libraries(playlunky_dependencies INTERFACE
	optimized detours)


# --------------------------------------------------
# Glob for shared sources
file(GLOB_RECURSE shared_sources CONFIGURE_DEPENDS "source/shared/*.cpp")
file(GLOB_RECURSE shared_headers CONFIGURE_DEPENDS "source/shared/*.h" "source/shared/*.inl")


# --------------------------------------------------
# Create shared lib
file(GLOB_RECURSE playlunky64_sources CONFIGURE_DEPENDS "source/playlunky/*.cpp")
file(GLOB_RECURSE playlunky64_headers CONFIGURE_DEPENDS "source/playlunky/*.h" "source/playlunky/*.inl")
add_library(playlunky64 SHARED ${playlunky64_sources} ${shared_sources} ${playlunky64_headers} ${shared_headers})
target_link_libraries(playlunky64 PRIVATE playlunky_warnings playlunky_dependencies)
target_include_directories(playlunky64 PRIVATE "source/playlunky" "source/shared")


# --------------------------------------------------
# Create launcher executbale
file(GLOB_RECURSE playlunky_launcher_sources CONFIGURE_DEPENDS "source/launcher/*.cpp")
file(GLOB_RECURSE playlunky_launcher_headers CONFIGURE_DEPENDS "source/launcher/*.h" "source/launcher/*.inl")
add_executable(playlunky_launcher ${playlunky_launcher_sources} ${shared_sources} ${playlunky_launcher_headers} ${shared_headers})
target_link_libraries(playlunky_launcher PRIVATE playlunky_warnings playlunky_dependencies)
target_include_directories(playlunky_launcher PRIVATE "source/launcher" "source/shared")


# --------------------------------------------------
# Set debugging properties
set(PLAYLUNKY_SPELUNKY_2_DIR "game" CACHE FILEPATH "Path to Spel2.exe")
if(NOT IS_ABSOLUTE ${PLAYLUNKY_SPELUNKY_2_DIR})
    set(PLAYLUNKY_SPELUNKY_2_DIR "${CMAKE_SOURCE_DIR}/${PLAYLUNKY_SPELUNKY_2_DIR}")
endif()
set_target_properties(playlunky_launcher PROPERTIES
	VS_DEBUGGER_COMMAND_ARGUMENTS ${PLAYLUNKY_SPELUNKY_2_DIR}
	VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT playlunky_launcher)


# --------------------------------------------------
# Install shared lib and launcher
install(TARGETS
	playlunky64 RUNTIME DESTINATION .)
install(TARGETS
	playlunky_launcher RUNTIME DESTINATION .)


# --------------------------------------------------
# Install shared lib after build
add_custom_command(
	TARGET playlunky64 POST_BUILD
	COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --config $<CONFIG>)

	
# --------------------------------------------------
# Map Release config to MinSizeRel and RelWithDebInfo because conant is borked
set_target_properties(playlunky64 PROPERTIES
	MAP_IMPORTED_CONFIG_MINSIZEREL Release
	MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
set_target_properties(playlunky_launcher PROPERTIES
	MAP_IMPORTED_CONFIG_MINSIZEREL Release
	MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)