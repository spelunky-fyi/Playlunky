# Based on https://github.com/aminya/project_options

include_guard()

# Run Conan for dependency management
macro(run_conan)
    # Download automatically, you can also just copy the conan.cmake file
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(
            DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake"
            EXPECTED_HASH SHA256=3bef79da16c2e031dc429e1dac87a08b9226418b300ce004cc125a82687baeef
            TLS_VERIFY ON)
    endif()

    set(ENV{CONAN_REVISIONS_ENABLED} 1)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

    include(${CMAKE_BINARY_DIR}/conan.cmake)

    # Add (or remove) remotes as needed
    conan_add_remote(
        NAME
        cci
        URL
        https://center.conan.io
        INDEX
        0)
    conan_add_remote(
        NAME
        bincrafters
        URL
        https://bincrafters.jfrog.io/artifactory/api/conan/public-conan)

    # For multi configuration generators, like VS and XCode
    if(NOT CMAKE_CONFIGURATION_TYPES)
        message(STATUS "Conan: Single configuration build...")
        set(LIST_OF_BUILD_TYPES ${CMAKE_BUILD_TYPE})
    else()
        message(STATUS "Conan: Multi-configuration build: '${CMAKE_CONFIGURATION_TYPES}'...")
        set(LIST_OF_BUILD_TYPES ${CMAKE_CONFIGURATION_TYPES})
    endif()

    option(PLAYLUNKY_CONAN_VERBOSE "Print verbose info from conan" OFF)

    if(${PLAYLUNKY_CONAN_VERBOSE})
        set(OUTPUT_QUIET "")
    else()
        set(OUTPUT_QUIET "OUTPUT_QUIET")
    endif()

    foreach(TYPE ${LIST_OF_BUILD_TYPES})
        message(STATUS "Conan: Running Conan for build type '${TYPE}'")

        # Detects current build settings to pass into conan
        conan_cmake_autodetect(settings BUILD_TYPE ${TYPE})
        set(CONAN_SETTINGS SETTINGS ${settings})
        set(CONAN_ENV ENV "CC=${CMAKE_C_COMPILER}" "CXX=${CMAKE_CXX_COMPILER}")

        # PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR} is used to tell conan to process
        # the external "conanfile.py" provided with the project
        # Alternatively a conanfile.txt could be used
        conan_cmake_install(
            PATH_OR_REFERENCE
            ${CMAKE_SOURCE_DIR}
            BUILD
            missing

            # Pass CMake compilers to Conan
            ${CONAN_ENV}

            # Pass autodetected settings
            ${CONAN_SETTINGS}
            ${OUTPUT_QUIET})
    endforeach()
endmacro()
