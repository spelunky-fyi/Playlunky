macro(run_conan)
    # Download automatically, you can also just copy the conan.cmake file
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        message(STATUS "Downloading conan.cmake from github.com/conan-io/cmake-conan")

        if(GNU)
            # A bug in newer versions prevents building with gcc
            set(CONAN_SOURCE_URL "https://github.com/conan-io/cmake-conan/raw/v0.13/conan.cmake")
            message("Building with g++: Downloading old version of conan.cmake (v0.13) to avoid a bug...")
        else()
            set(CONAN_SOURCE_URL "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake")
        endif()

        file(DOWNLOAD "${CONAN_SOURCE_URL}" "${CMAKE_BINARY_DIR}/conan.cmake")
    endif()

    include(${CMAKE_BINARY_DIR}/conan.cmake)

    conan_add_remote(NAME bincrafters URL
                     https://api.bintray.com/conan/bincrafters/public-conan)

    conan_cmake_run(
        CONANFILE
        "conanfile.txt"
        OPTIONS
        ${CONAN_EXTRA_OPTIONS}
        BASIC_SETUP
        CMAKE_TARGETS # individual targets to link to
        BUILD
        missing)
endmacro()