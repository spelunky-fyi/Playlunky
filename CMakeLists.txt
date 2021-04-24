cmake_minimum_required(VERSION 3.9)
project(Playlunky LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/publish")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


# --------------------------------------------------
# Add submodules
add_subdirectory(submodules)


# --------------------------------------------------
# Get version tag
find_package(Git)
if (NOT GIT_FOUND)
	SET(GIT_EXECUTABLE git)
endif()

execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty=-modified
    OUTPUT_VARIABLE PLAYLUNKY_GIT_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE)


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
find_package(structopt REQUIRED)
find_package(lodepng REQUIRED)
find_package(libzip REQUIRED)
find_package(ctre REQUIRED)
find_package(zstd REQUIRED)
find_package(opencv REQUIRED)
find_package(nlohmann_json REQUIRED)


# --------------------------------------------------
# Create interface libs
add_library(playlunky_warnings INTERFACE)
if(MSVC)
	target_compile_options(playlunky_warnings INTERFACE /W4 /WX /permissive-)
else()
	target_compile_options(playlunky_warnings INTERFACE -Wall -Wextra -pedantic -Werror)
endif()

add_library(playlunky_definitions INTERFACE)
target_compile_definitions(playlunky_definitions INTERFACE
	_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
	_WIN32_MEAN_AND_LEAN
	NOMINMAX
	"PLAYLUNKY_VERSION=\"${PLAYLUNKY_GIT_VERSION}\"")

add_library(playlunky_dependencies INTERFACE)
target_link_libraries(playlunky_dependencies INTERFACE
	fmt
	libzip::libzip)

add_library(playlunky_inject_dependencies INTERFACE)
target_link_libraries(playlunky_inject_dependencies INTERFACE
	lib_detours)

add_library(playlunky_lib_dependencies INTERFACE)
target_link_libraries(playlunky_lib_dependencies INTERFACE
	lodepng::lodepng
	ctre::ctre
	zstd::zstd
	opencv::opencv
	nlohmann_json::nlohmann_json
	imgui
	libnyquist
	inih
	spel2)


# --------------------------------------------------
# Glob for shared sources
file(GLOB_RECURSE shared_sources CONFIGURE_DEPENDS "source/shared/*.cpp")
file(GLOB_RECURSE shared_headers CONFIGURE_DEPENDS "source/shared/*.h" "source/shared/*.inl")


# --------------------------------------------------
# Create shared lib
file(GLOB_RECURSE playlunky64_sources CONFIGURE_DEPENDS "source/playlunky/*.cpp")
file(GLOB_RECURSE playlunky64_headers CONFIGURE_DEPENDS "source/playlunky/*.h" "source/playlunky/*.inl")
set(playlunky64_resources "res/playlunky64.rc" "res/resource_playlunky64.h")
add_library(playlunky64 SHARED ${playlunky64_sources} ${shared_sources} ${playlunky64_headers} ${shared_headers} ${playlunky64_resources})
target_link_libraries(playlunky64 PRIVATE
	playlunky_warnings
	playlunky_definitions
	playlunky_dependencies
	playlunky_inject_dependencies
	playlunky_lib_dependencies)
target_include_directories(playlunky64 PRIVATE "source/playlunky" "source/shared")


# --------------------------------------------------
# Create launcher executbale
file(GLOB_RECURSE playlunky_launcher_sources CONFIGURE_DEPENDS "source/launcher/*.cpp")
file(GLOB_RECURSE playlunky_launcher_headers CONFIGURE_DEPENDS "source/launcher/*.h" "source/launcher/*.inl")
set(playlunky_launcher_resources "res/playlunky_launcher.rc")
add_executable(playlunky_launcher WIN32 ${playlunky_launcher_sources} ${shared_sources} ${playlunky_launcher_headers} ${shared_headers} ${playlunky_launcher_resources})
target_link_libraries(playlunky_launcher PRIVATE
	playlunky_warnings
	playlunky_definitions
	playlunky_dependencies
	playlunky_inject_dependencies
	structopt::structopt)
target_include_directories(playlunky_launcher PRIVATE "source/launcher" "source/shared" "res")


# --------------------------------------------------
# Create installer executbale
file(GLOB_RECURSE playlunky_installer_sources CONFIGURE_DEPENDS "source/installer/*.cpp")
file(GLOB_RECURSE playlunky_installer_headers CONFIGURE_DEPENDS "source/installer/*.h" "source/installer/*.inl")
set(playlunky_installer_resources "res/playlunky_installer.rc")
add_executable(playlunky_installer ${playlunky_installer_sources} ${shared_sources} ${playlunky_installer_headers} ${shared_headers} ${playlunky_installer_resources})
target_link_libraries(playlunky_installer PRIVATE
	playlunky_warnings
	playlunky_definitions
	playlunky_dependencies)
target_include_directories(playlunky_installer PRIVATE "source/installer" "source/shared" "res")


# --------------------------------------------------
# Merge files from source and include in the IDE
function(group_files sources)
	foreach(FILE ${sources})
		# Get the directory of the source file
		get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

		# Remove common directory prefix to make the group
		string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")

		# Make sure we are using windows slashes
		string(REPLACE "/" "\\" GROUP "${GROUP}")

		# Strip the root parts for each possible component
		if("${FILE}" MATCHES "source/launcher/.*")
			string(SUBSTRING ${GROUP} 16 -1 GROUP)
		elseif("${FILE}" MATCHES "source/playlunky/.*")
			string(SUBSTRING ${GROUP} 17 -1 GROUP)
		elseif("${FILE}" MATCHES "source/shared/.*")
			string(SUBSTRING ${GROUP} 14 -1 GROUP)
		endif()

		# Do the grouping
		source_group("${GROUP}" FILES "${FILE}")
	endforeach()
endfunction()
group_files("${playlunky64_sources};${playlunky_launcher_sources};${shared_sources};${playlunky64_headers};${playlunky_launcher_headers};${shared_headers};${playlunky_launcher_resources}")


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
	message(STATUS "Could not find Spelunky 2 installation, pass it to cmake via -DSPELUNKY_INSTALL_DIR='Path/To/Install/Folder' or place the Spelunky 2 folder into the publish folder")
else()
	message(STATUS "Found Spelunky 2 installation at '${SPELUNKY_INSTALL_DIR}'")
endif()


# --------------------------------------------------
# Set debugging properties
if(EXISTS ${SPELUNKY_INSTALL_DIR})
	set_target_properties(playlunky_launcher PROPERTIES
		VS_DEBUGGER_COMMAND_ARGUMENTS "--console --exe_dir \"${SPELUNKY_INSTALL_DIR}\""
		VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:playlunky_launcher>)
	set_target_properties(playlunky_installer PROPERTIES
		VS_DEBUGGER_WORKING_DIRECTORY "${SPELUNKY_INSTALL_DIR}")

	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT playlunky_launcher)
endif()


# --------------------------------------------------
# Install shared lib and launcher
install(TARGETS
	playlunky64
	playlunky_launcher
	playlunky_installer
	spel2
	RUNTIME
	DESTINATION .)
install(FILES
	res/readme.txt
	DESTINATION .)
if(EXISTS ${SPELUNKY_INSTALL_DIR})
	install(FILES
		DESTINATION ${SPELUNKY_INSTALL_DIR})
endif()

	
# --------------------------------------------------
# Map Release config to MinSizeRel and RelWithDebInfo because conant is borked
set_target_properties(playlunky64 PROPERTIES
	MAP_IMPORTED_CONFIG_MINSIZEREL Release
	MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
set_target_properties(playlunky_launcher PROPERTIES
	MAP_IMPORTED_CONFIG_MINSIZEREL Release
	MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
