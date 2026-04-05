#pragma once

#include <cstdint>

struct TargaHeader
{
	uint16_t Width = 0;
	uint16_t Height = 0;
	uint8_t PixelDepth = 0;
	uint8_t ImageType = 0;
	uint8_t ColorMapType = 0;
};

struct Targa
{
	TargaHeader Header;

	void SetImage(uint8_t *)
	{
	}

	void YFlip()
	{
	}

	void Save(const char *, int, bool)
	{
	}
};

#ifndef TGA_TRUECOLOR
#define TGA_TRUECOLOR 2
#endif

#ifndef TGA_MONO
#define TGA_MONO 3
#endif

#ifndef TGAF_IMAGE
#define TGAF_IMAGE 0
#endif
