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
find_package(structopt REQUIRED)


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
target_link_libraries(playlunky64 PRIVATE
	playlunky_warnings
	playlunky_dependencies)
target_include_directories(playlunky64 PRIVATE "source/playlunky" "source/shared")


# --------------------------------------------------
# Create launcher executbale
file(GLOB_RECURSE playlunky_launcher_sources CONFIGURE_DEPENDS "source/launcher/*.cpp")
file(GLOB_RECURSE playlunky_launcher_headers CONFIGURE_DEPENDS "source/launcher/*.h" "source/launcher/*.inl")
set(playlunky_launcher_resourcess "res/playlunky_launcher.rc")
add_executable(playlunky_launcher ${playlunky_launcher_sources} ${shared_sources} ${playlunky_launcher_headers} ${shared_headers} ${playlunky_launcher_resourcess})
target_link_libraries(playlunky_launcher PRIVATE
	playlunky_warnings
	playlunky_dependencies
	structopt::structopt)
target_include_directories(playlunky_launcher PRIVATE "source/launcher" "source/shared" "res")


# --------------------------------------------------
# Find the Spel2.exe
get_filename_component(STEAM_INSTALL_DIR "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Valve\\Steam;InstallPath]" ABSOLUTE)
set(SPELUNKY_INSTALL_DIR "${STEAM_INSTALL_DIR}/SteamApps/common/Spelunky 2")
if(NOT EXISTS ${SPELUNKY_INSTALL_DIR}/Spel2.exe)
	set(STEAM_LIBRARY_FOLDERS_CONFIG "${STEAM_INSTALL_DIR}/SteamApps/libraryfolders.vdf")
	if(EXISTS ${STEAM_LIBRARY_FOLDERS_CONFIG})
		FILE(READ "${STEAM_LIBRARY_FOLDERS_CONFIG}" STEAM_LIBRARY_FOLDERS_CONFIG_CONTENT)
		STRING(REGEX MATCHALL "\"[A-Z]\:.*\"" POTENTIAL_STEAM_LIBRARY_FOLDERS ${STEAM_LIBRARY_FOLDERS_CONFIG_CONTENT})
		foreach(STEAM_LIBRARY_FOLDER ${POTENTIAL_STEAM_LIBRARY_FOLDERS})
			string(REGEX REPLACE "\^\"" "" STEAM_LIBRARY_FOLDER ${STEAM_LIBRARY_FOLDER})
			string(REGEX REPLACE "\"$" "" STEAM_LIBRARY_FOLDER ${STEAM_LIBRARY_FOLDER})
			string(REGEX REPLACE "\\\\\\\\" "/" STEAM_LIBRARY_FOLDER ${STEAM_LIBRARY_FOLDER}) # double slash, escaped for cmake string then escaped for regex, requires a total of 8 backslashes
			if(EXISTS ${STEAM_LIBRARY_FOLDER})
				set(SPELUNKY_INSTALL_DIR "${STEAM_LIBRARY_FOLDER}/SteamApps/common/Spelunky 2")
				if(EXISTS ${SPELUNKY_INSTALL_DIR}/Spel2.exe)
					break()
				endif()
			endif()
		endforeach()
	endif()
endif()
if(NOT EXISTS ${SPELUNKY_INSTALL_DIR})
	message("Could not find Spelunky 2 installation, pass it to cmake via -DSPELUNKY_INSTALL_DIR='Path/To/Install/Folder' or place the Spelunky 2 folder into the publish folder")
else()
	message("Found Spelunky 2 installation at '${SPELUNKY_INSTALL_DIR}'")
endif()


# --------------------------------------------------
# Set debugging properties
set_target_properties(playlunky_launcher PROPERTIES
	VS_DEBUGGER_COMMAND_ARGUMENTS "--exe_dir \"${SPELUNKY_INSTALL_DIR}\""
	VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT playlunky_launcher)


# --------------------------------------------------
# Install shared lib and launcher
install(TARGETS
	playlunky64 RUNTIME DESTINATION .)
install(TARGETS
	playlunky_launcher RUNTIME DESTINATION .)
install(FILES
	res/steam_appid.txt DESTINATION .)
if(EXISTS ${SPELUNKY_INSTALL_DIR})
	install(FILES
		res/steam_appid.txt DESTINATION ${SPELUNKY_INSTALL_DIR})
endif()


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
