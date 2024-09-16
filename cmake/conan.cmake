# Based on https://github.com/aminya/project_options

include_guard()

# Run Conan for dependency management
macro(run_conan)
    # Download automatically, you can also just copy the conan.cmake file
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan_provider.cmake")
        message(STATUS "Downloading conan_provider.cmake from https://github.com/conan-io/cmake-conan")
        file(
            DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/f6464d1e13ef7a47c569f5061f9607ea63339d39/conan_provider.cmake"
            "${CMAKE_BINARY_DIR}/conan_provider.cmake"
            EXPECTED_HASH SHA256=0a5eb4afbdd94faf06dcbf82d3244331605ef2176de32c09ea9376e768cbb0fc

            # TLS_VERIFY ON # fails on some systems
        )
    endif()

    option(PLAYLUNKY_CONAN_VERBOSE "Print verbose info from conan" OFF)

    if(${PLAYLUNKY_CONAN_VERBOSE})
        set(VERBOSE_SETTING "-vverbose")
    else()
        set(VERBOSE_SETTING "-vwarning")
    endif()

    cmake_path(SET pl_conan_profile "${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan_profile")
    set(CONAN_HOST_PROFILE "${pl_conan_profile};auto-cmake" CACHE STRING "Conan host profile" FORCE)
    set(CONAN_BUILD_PROFILE "default" CACHE STRING "Conan build profile" FORCE)
    set(CONAN_INSTALL_ARGS "--build=missing;${VERBOSE_SETTING}" CACHE STRING "Command line arguments for conan install" FORCE)

    list(APPEND CMAKE_PROJECT_TOP_LEVEL_INCLUDES "${CMAKE_BINARY_DIR}/conan_provider.cmake")
endmacro()
