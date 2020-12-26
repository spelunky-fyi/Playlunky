# --------------------------------------------------
# Detours
set(DETOURS_SOURCE detours/src)

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

# This file is included and not compiled on its own
set_property (
    SOURCE detours/src/uimports.cpp
    APPEND PROPERTY HEADER_FILE_ONLY true)

target_compile_options(lib_detours PRIVATE /W4 /WX /Zi /MT /Gy /Gm- /Zl /Od)
target_include_directories(lib_detours PUBLIC detours/src)
