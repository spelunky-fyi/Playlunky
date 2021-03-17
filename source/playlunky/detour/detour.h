#pragma once

#include "sigfun.h"
#include "sigscan.h"

#include <type_traits>

class PlaylunkySettings;
void Attach(const PlaylunkySettings& settings);
void Detach(const PlaylunkySettings& settings);
