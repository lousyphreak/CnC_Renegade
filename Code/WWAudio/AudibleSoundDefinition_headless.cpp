#include "AudibleSound.h"

#include "SoundChunkIDs.h"
#include "chunkio.h"
#include "persistfactory.h"
#include "simpledefinitionfactory.h"

DECLARE_DEFINITION_FACTORY(AudibleSoundDefinitionClass, CLASSID_SOUND_DEF, "Sound") _HeadlessSoundDefFactory;
SimplePersistFactoryClass<AudibleSoundDefinitionClass, CHUNKID_SOUND_DEF> _HeadlessAudibleSoundDefPersistFactory;

AudibleSoundDefinitionClass::AudibleSoundDefinitionClass(void)
	: m_Priority(0.5f),
	  m_Volume(1.0f),
	  m_VolumeRandomizer(0.0f),
	  m_Pan(0.5f),
	  m_LoopCount(1),
	  m_VirtualChannel(0),
	  m_DropOffRadius(0.0f),
	  m_MaxVolRadius(0.0f),
	  m_Is3D(false),
	  m_Filename(),
	  m_Type(AudibleSoundClass::TYPE_SOUND_EFFECT),
	  m_DisplayText(),
	  m_StartOffset(0.0f),
	  m_PitchFactor(1.0f),
	  m_PitchFactorRandomizer(0.0f),
	  m_LogicalTypeMask(0),
	  m_LogicalNotifyDelay(0.0f),
	  m_LogicalDropOffRadius(0.0f),
	  m_CreateLogical(false),
	  m_AttenuationSphereColor(1.0f, 1.0f, 1.0f)
{
}

uint32 AudibleSoundDefinitionClass::Get_Class_ID(void) const
{
	return CLASSID_SOUND_DEF;
}

const PersistFactoryClass &AudibleSoundDefinitionClass::Get_Factory(void) const
{
	return _HeadlessAudibleSoundDefPersistFactory;
}

bool AudibleSoundDefinitionClass::Save(ChunkSaveClass &)
{
	return true;
}

bool AudibleSoundDefinitionClass::Load(ChunkLoadClass &cload)
{
	while (cload.Open_Chunk()) {
		cload.Close_Chunk();
	}
	return true;
}

PersistClass *AudibleSoundDefinitionClass::Create(void) const
{
	return new AudibleSoundDefinitionClass();
}

AudibleSoundClass *AudibleSoundDefinitionClass::Create_Sound(int) const
{
	return nullptr;
}

void AudibleSoundDefinitionClass::Initialize_From_Sound(AudibleSoundClass *)
{
}

LogicalSoundClass *AudibleSoundDefinitionClass::Create_Logical(void)
{
	return nullptr;
}

bool AudibleSoundDefinitionClass::Save_Variables(ChunkSaveClass &)
{
	return true;
}

bool AudibleSoundDefinitionClass::Load_Variables(ChunkLoadClass &)
{
	return true;
}