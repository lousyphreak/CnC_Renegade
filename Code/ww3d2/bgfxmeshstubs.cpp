#include "dx8renderer.h"
DX8MeshRendererClass TheDX8MeshRenderer;

DX8MeshRendererClass::DX8MeshRendererClass() : enable_lighting(true), camera(nullptr), texture_category_container_list_skin(nullptr), visible_decal_meshes(nullptr)
{
}

DX8MeshRendererClass::~DX8MeshRendererClass()
{
}

void DX8MeshRendererClass::Init()
{
}

void DX8MeshRendererClass::Shutdown()
{
}

void DX8MeshRendererClass::Flush()
{
}

void DX8MeshRendererClass::Clear_Pending_Delete_Lists()
{
}

void DX8MeshRendererClass::Log_Statistics_String(bool)
{
}

void DX8MeshRendererClass::Request_Log_Statistics()
{
}

void DX8MeshRendererClass::Register_Mesh_Type(MeshClass *)
{
}

void DX8MeshRendererClass::Unregister_Mesh_Type(MeshClass *)
{
}

void DX8MeshRendererClass::Add_To_Render_List(DecalMeshClass *)
{
}

void DX8MeshRendererClass::Invalidate()
{
}
