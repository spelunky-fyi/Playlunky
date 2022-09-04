# --------------------------------------------------
# Add subdirectories directly into a folder in the IDE
# Inspired by https://stackoverflow.com/questions/45092198/cmake-how-do-i-change-properties-on-subdirectory-project-targets#
function(get_all_targets _result _dir)
    get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)

    foreach(_subdir IN LISTS _subdirs)
        get_all_targets(${_result} "${_subdir}")
    endforeach()

    get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
    set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
endfunction()

function(add_subdirectory_with_folder _folder_name _folder)
    add_subdirectory(${_folder} ${ARGN} EXCLUDE_FROM_ALL)
    get_all_targets(_targets "${_folder}")

    foreach(_target IN LISTS _targets)
        get_target_property(_target_type ${_target} TYPE)

        if(NOT _target_type STREQUAL "INTERFACE_LIBRARY")
            get_property(_current_folder_name TARGET ${_target} PROPERTY FOLDER)
            set_target_properties(${_target} PROPERTIES
                FOLDER "${_folder_name}/${_current_folder_name}")

            if(NOT _target_type STREQUAL "UTILITY")
                target_compile_options(${_target} PRIVATE
                    "/W0")
            endif()
        endif()
    endforeach()
endfunction()

# --------------------------------------------------
# Detours
add_library(lib_detours STATIC
    detours/src/creatwth.cpp
    detours/src/detours.cpp
    detours/src/detours.h
    detours/src/detver.h
    detours/src/disasm.cpp
    detours/src/disolarm.cpp
    detours/src/disolarm64.cpp
    detours/src/disolia64.cpp
    detours/src/disolx64.cpp
    detours/src/disolx86.cpp
    detours/src/image.cpp
    detours/src/modules.cpp
    detours/src/uimports.cpp)

set_target_properties(lib_detours PROPERTIES
    FOLDER "3rd_party")

# This file is included and not compiled on its own
set_property(
    SOURCE detours/src/uimports.cpp
    APPEND PROPERTY HEADER_FILE_ONLY true)

target_compile_options(lib_detours PRIVATE /W4 /WX /Zi /MT /Gy /Gm- /Zl /Od)
target_include_directories(lib_detours PUBLIC detours/src)

# --------------------------------------------------
# inih
add_library(inih STATIC
    inih/ini.c
    inih/ini.h
    inih/cpp/INIReader.cpp
    inih/cpp/INIReader.h)

set_target_properties(inih PROPERTIES
    FOLDER "3rd_party")

target_compile_options(inih PRIVATE /w /Zi /Gy /Gm- /Zl /Od)
target_include_directories(inih PUBLIC inih/cpp)

# --------------------------------------------------
# nyquist
option(BUILD_EXAMPLE "Build example application" OFF)
add_subdirectory_with_folder("3rd_party" libnyquist)

# Remove the vectorization instructions since those cause a crash for some users
function(remove_arch_avx target)
    get_target_property(target_compile_options ${target} COMPILE_OPTIONS)
    list(REMOVE_ITEM target_compile_options /arch:AVX)
    set_target_properties(${target} PROPERTIES COMPILE_OPTIONS "${target_compile_options}")
endfunction()

remove_arch_avx(libwavpack)
remove_arch_avx(libnyquist)

# --------------------------------------------------
# zip-adaptor
add_subdirectory_with_folder("3rd_party" "zip-adaptor")

# --------------------------------------------------
# overlunky -- later to be spelunky-api
option(BUILD_OVERLUNKY CACHE OFF)
option(BUILD_INFO_DUMP CACHE OFF)
option(BUILD_SPEL2_DLL CACHE ON)
add_subdirectory_with_folder("3rd_party" overlunky)

# --------------------------------------------------
# force imgui to use a big wchar (for emojis)
target_compile_definitions(imgui PUBLIC IMGUI_USE_WCHAR32 IMGUI_ENABLE_FREETYPE)
target_sources(imgui PRIVATE
    overlunky/src/imgui/misc/freetype/imgui_freetype.h
    overlunky/src/imgui/misc/freetype/imgui_freetype.cpp)
target_link_libraries(imgui PRIVATE Freetype::Freetype)