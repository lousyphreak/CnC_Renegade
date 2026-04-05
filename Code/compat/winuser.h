#pragma once

#include <cstdint>

#include "win.h"

#ifndef SPI_GETWHEELSCROLLLINES
#define SPI_GETWHEELSCROLLLINES 0x0068
#endif

#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

inline int32_t SystemParametersInfo(uint32_t action, uint32_t, void * value, uint32_t)
{
	if (action == SPI_GETWHEELSCROLLLINES && value != nullptr) {
		*reinterpret_cast<uint32_t *>(value) = 3;
	}
	return TRUE;
}
