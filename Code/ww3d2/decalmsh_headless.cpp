#include "decalmsh.h"

DecalMeshClass::DecalMeshClass(MeshClass *parent, DecalSystemClass *system)
    : Parent(parent),
      DecalSystem(system),
      NextVisible(nullptr)
{
}

DecalMeshClass::~DecalMeshClass() = default;

RigidDecalMeshClass::RigidDecalMeshClass(MeshClass *parent, DecalSystemClass *system)
    : DecalMeshClass(parent, system)
{
}

RigidDecalMeshClass::~RigidDecalMeshClass() = default;

void RigidDecalMeshClass::Render() {}

bool RigidDecalMeshClass::Create_Decal(DecalGeneratorClass *, const OBBoxClass &, SimpleDynVecClass<uint32_t> &, const DynamicVectorClass<Vector3> *)
{
    return false;
}

bool RigidDecalMeshClass::Delete_Decal(uint32_t)
{
    return false;
}

int RigidDecalMeshClass::Process_Material_Run(int)
{
    return 0;
}

SkinDecalMeshClass::SkinDecalMeshClass(MeshClass *parent, DecalSystemClass *system)
    : DecalMeshClass(parent, system)
{
}

SkinDecalMeshClass::~SkinDecalMeshClass() = default;

void SkinDecalMeshClass::Render() {}

bool SkinDecalMeshClass::Create_Decal(DecalGeneratorClass *, const OBBoxClass &, SimpleDynVecClass<uint32_t> &, const DynamicVectorClass<Vector3> *)
{
    return false;
}

bool SkinDecalMeshClass::Delete_Decal(uint32_t)
{
    return false;
}

int SkinDecalMeshClass::Process_Material_Run(int)
{
    return 0;
}
