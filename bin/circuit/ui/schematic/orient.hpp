#pragma once
#include "core/integers.h"

enum class Direction : u8 { Left, Right, Up, Down };

bool parallel(Direction a, Direction b);
