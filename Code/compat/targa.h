#pragma once

struct TargaHeader
{
	unsigned short Width = 0;
	unsigned short Height = 0;
	unsigned char PixelDepth = 0;
	unsigned char ImageType = 0;
	unsigned char ColorMapType = 0;
};

struct Targa
{
	TargaHeader Header;

	void SetImage(char *)
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
