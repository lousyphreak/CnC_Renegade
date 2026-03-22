#include "decalsys.h"

#include "rendobj.h"

uint32 DecalSystemClass::DecalIDGenerator = 1;

DecalSystemClass::DecalSystemClass(void) {}
DecalSystemClass::~DecalSystemClass(void) {}
DecalGeneratorClass * DecalSystemClass::Lock_Decal_Generator(void) { return new DecalGeneratorClass(Generate_Decal_Id(), this); }
void DecalSystemClass::Unlock_Decal_Generator(DecalGeneratorClass * generator) { delete generator; }
uint32 DecalSystemClass::Generate_Unique_Global_Decal_Id(void) { return DecalIDGenerator++; }

DecalGeneratorClass::DecalGeneratorClass(uint32 id,DecalSystemClass * system)
	: System(system), DecalID(id), BackfaceVal(-1.0f), ApplyToTranslucentMeshes(false), Material(NEW_REF(MaterialPassClass, ())), MeshList()
{
}

DecalGeneratorClass::~DecalGeneratorClass(void) { REF_PTR_RELEASE(Material); }
void DecalGeneratorClass::Add_Mesh(RenderObjClass * mesh) { if (mesh != NULL) MeshList.Add(mesh); }
NonRefRenderObjListClass & DecalGeneratorClass::Get_Mesh_List(void) { return MeshList; }
void DecalGeneratorClass::Set_Mesh_Transform(const Matrix3D & tm) { Set_Transform(tm); }

MultiFixedPoolDecalSystemClass::LogicalDecalClass::LogicalDecalClass(void) {}
MultiFixedPoolDecalSystemClass::LogicalDecalClass::~LogicalDecalClass(void) { MeshList.Reset_List(); }
void MultiFixedPoolDecalSystemClass::LogicalDecalClass::Set(DecalGeneratorClass * generator) { MeshList.Reset_List(); if (generator != NULL) { NonRefRenderObjListClass & list = generator->Get_Mesh_List(); for (RenderObjClass * obj = list.Remove_Head(); obj != NULL; obj = list.Remove_Head()) { MeshList.Add(obj); } } }
void MultiFixedPoolDecalSystemClass::LogicalDecalClass::Clear(uint32) { MeshList.Reset_List(); }
MultiFixedPoolDecalSystemClass::LogicalDecalPoolClass::LogicalDecalPoolClass(void) : Array(NULL), Size(0) {}
MultiFixedPoolDecalSystemClass::LogicalDecalPoolClass::~LogicalDecalPoolClass(void) { delete [] Array; }
void MultiFixedPoolDecalSystemClass::LogicalDecalPoolClass::Initialize(uint32 size) { delete [] Array; Size = size; Array = size > 0 ? new LogicalDecalClass[size] : NULL; }

MultiFixedPoolDecalSystemClass::MultiFixedPoolDecalSystemClass(uint32 num_pools, const uint32 * pool_sizes)
	: Pools(num_pools > 0 ? new LogicalDecalPoolClass[num_pools] : NULL), PoolCount(num_pools), Generator_PoolID(0), Generator_SlotID(0)
{
	for (uint32 index = 0; index < PoolCount; ++index) {
		Pools[index].Initialize(pool_sizes != NULL ? pool_sizes[index] : 0);
	}
}
MultiFixedPoolDecalSystemClass::MultiFixedPoolDecalSystemClass(const MultiFixedPoolDecalSystemClass & that)
	: Pools(that.PoolCount > 0 ? new LogicalDecalPoolClass[that.PoolCount] : NULL), PoolCount(that.PoolCount), Generator_PoolID(0), Generator_SlotID(0)
{
	for (uint32 index = 0; index < PoolCount; ++index) {
		Pools[index].Initialize(that.Pools[index].Size);
	}
}
MultiFixedPoolDecalSystemClass::~MultiFixedPoolDecalSystemClass(void) { delete [] Pools; }
DecalGeneratorClass * MultiFixedPoolDecalSystemClass::Lock_Decal_Generator(void) { return DecalSystemClass::Lock_Decal_Generator(); }
void MultiFixedPoolDecalSystemClass::Unlock_Decal_Generator(DecalGeneratorClass * generator) { DecalSystemClass::Unlock_Decal_Generator(generator); }
void MultiFixedPoolDecalSystemClass::Decal_Mesh_Destroyed(uint32, DecalMeshClass *) {}
void MultiFixedPoolDecalSystemClass::Clear_Decal_Slot(uint32 pool_id, uint32 slot_id) { if (pool_id < PoolCount && slot_id < Pools[pool_id].Size) Pools[pool_id].Array[slot_id].Clear(encode_decal_id(pool_id, slot_id)); }
void MultiFixedPoolDecalSystemClass::Clear_Pool(uint32 pool_id) { if (pool_id < PoolCount) { for (uint32 i = 0; i < Pools[pool_id].Size; ++i) Pools[pool_id].Array[i].Clear(encode_decal_id(pool_id, i)); } }
void MultiFixedPoolDecalSystemClass::Clear_All_Decals(void) { for (uint32 i = 0; i < PoolCount; ++i) Clear_Pool(i); }
MultiFixedPoolDecalSystemClass::LogicalDecalClass & MultiFixedPoolDecalSystemClass::find_logical_decal(uint32 pool_id, uint32 slot_id) { return Pools[pool_id].Array[slot_id]; }
MultiFixedPoolDecalSystemClass::LogicalDecalClass & MultiFixedPoolDecalSystemClass::find_logical_decal(uint32 decal_id) { uint32 pool_id = 0; uint32 slot_id = 0; decode_decal_id(decal_id, pool_id, slot_id); return find_logical_decal(pool_id, slot_id); }
