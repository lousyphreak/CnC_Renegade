#pragma once

#include <cstdint>
#include "bittype.h"

using D3DCOLOR = uint32;
using HRESULT = long;
using FLOAT = float;

struct D3DADAPTER_IDENTIFIER8 {
	char Driver[512] = {0};
	char Description[512] = {0};
	char DeviceName[32] = {0};
	std::uint64_t DriverVersion = 0;
	std::uint32_t VendorId = 0;
	std::uint32_t DeviceId = 0;
	std::uint32_t SubSysId = 0;
	std::uint32_t Revision = 0;
	struct {
		std::uint32_t Data1 = 0;
		std::uint16_t Data2 = 0;
		std::uint16_t Data3 = 0;
		unsigned char Data4[8] = {0};
	} DeviceIdentifier;
	unsigned long WHQLLevel = 0;
};
