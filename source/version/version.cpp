#include "version.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

std::string_view playlunky_version()
{
    return TOSTRING(PLAYLUNKY_VERSION);
}
