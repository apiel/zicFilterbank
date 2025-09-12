#pragma once

#ifndef clamp
#define clamp(x, _min, _max) ((x) < (_max) ? ((x) > (_min) ? (x) : (_min)) : (_max))
#endif
