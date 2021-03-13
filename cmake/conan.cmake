macro(run_conan)
    # Download automatically, you can also just copy the conan.cmake file
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        message(STATUS "Downloading conan.cmake from github.com/conan-io/cmake-conan")

        set(CONAN_SOURCE_URL "https://github.com/conan-io/cmake-conan/raw/v0.16.1/conan.cmake")

        file(DOWNLOAD "${CONAN_SOURCE_URL}" "${CMAKE_BINARY_DIR}/conan.cmake")
    endif()

    include(${CMAKE_BINARY_DIR}/conan.cmake)

    conan_add_remote(NAME bincrafters URL
                     https://api.bintray.com/conan/bincrafters/public-conan)

    conan_cmake_run(
        CONANFILE
        "conanfile.txt"
        OPTIONS ${CONAN_EXTRA_OPTIONS}
        BASIC_SETUP CMAKE_TARGETS # individual targets to link to
        BUILD missing)
endmacro()