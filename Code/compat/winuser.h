#pragma once

#include "win.h"

#ifndef SPI_GETWHEELSCROLLLINES
#define SPI_GETWHEELSCROLLLINES 0x0068
#endif

#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

inline BOOL SystemParametersInfo(UINT action, UINT, void * value, UINT)
{
	if (action == SPI_GETWHEELSCROLLLINES && value != nullptr) {
		*reinterpret_cast<UINT *>(value) = 3;
	}
	return TRUE;
}
