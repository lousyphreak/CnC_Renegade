#include "AudioSaveLoad.h"

#include "chunkio.h"
#include "SoundChunkIDs.h"

StaticAudioSaveLoadClass _StaticAudioSaveLoadSubsystem;
DynamicAudioSaveLoadClass _DynamicAudioSaveLoadSubsystem;

uint32 StaticAudioSaveLoadClass::Chunk_ID(void) const
{
	return CHUNKID_STATIC_SAVELOAD;
}

bool StaticAudioSaveLoadClass::Contains_Data(void) const
{
	return false;
}

bool StaticAudioSaveLoadClass::Save(ChunkSaveClass &)
{
	return true;
}

bool StaticAudioSaveLoadClass::Load(ChunkLoadClass &cload)
{
	while (cload.Open_Chunk()) {
		cload.Close_Chunk();
	}
	return true;
}

uint32 DynamicAudioSaveLoadClass::Chunk_ID(void) const
{
	return CHUNKID_DYNAMIC_SAVELOAD;
}

bool DynamicAudioSaveLoadClass::Contains_Data(void) const
{
	return false;
}

bool DynamicAudioSaveLoadClass::Save(ChunkSaveClass &)
{
	return true;
}

bool DynamicAudioSaveLoadClass::Load(ChunkLoadClass &cload)
{
	while (cload.Open_Chunk()) {
		cload.Close_Chunk();
	}
	return true;
}
