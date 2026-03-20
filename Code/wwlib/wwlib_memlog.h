#pragma once

#ifndef WWLIB_MEMLOG_H
#define WWLIB_MEMLOG_H

enum
{
	MEM_UNKNOWN = 0,
	MEM_GEOMETRY,
	MEM_ANIMATION,
	MEM_TEXTURE,
	MEM_PATHFIND,
	MEM_VIS,
	MEM_SOUND,
	MEM_CULLINGDATA,
	MEM_STRINGS,
	MEM_GAMEDATA,
	MEM_PHYSICSDATA,
	MEM_W3DDATA,
	MEM_STATICALLOCATION,
	MEM_GAMEINIT,
	MEM_RENDERER,
	MEM_NETWORK,
	MEM_BINK,
	MEM_COUNT
};

class WWLibMemorySampleClass
{
public:
	explicit WWLibMemorySampleClass(int)
	{
	}
};

#ifdef WWDEBUG
#define WWLIB_MEMLOG(category)	WWLibMemorySampleClass _wwlib_memsample(category)
#else
#define WWLIB_MEMLOG(category)	((void)0)
#endif

#endif
