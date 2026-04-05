#pragma once
#include "core/integers.h"

enum class Direction : u8 { Left, Right, Up, Down };

bool parallel(Direction a, Direction b);

Direction from_angle(i16 angle);
Direction clockwise(Direction dir);
Direction counter_clockwise(Direction dir);
