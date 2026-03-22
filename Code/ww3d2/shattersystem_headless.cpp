#include "shattersystem.h"

void ShatterSystem::Init(void) {}
void ShatterSystem::Shutdown(void) {}
void ShatterSystem::Shatter_Mesh(MeshClass *, const Vector3 &, const Vector3 &) {}
int ShatterSystem::Get_Fragment_Count(void) { return 0; }
RenderObjClass *ShatterSystem::Get_Fragment(int) { return nullptr; }
RenderObjClass *ShatterSystem::Peek_Fragment(int) { return nullptr; }
void ShatterSystem::Release_Fragments(void) {}
void ShatterSystem::Reset_Clip_Pools(void) {}
void ShatterSystem::Process_Clip_Pools(const Matrix3D &, MeshClass *, MeshMtlParamsClass &) {}
