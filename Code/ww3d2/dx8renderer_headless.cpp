#include "dx8renderer.h"

DX8MeshRendererClass TheDX8MeshRenderer;

DX8TextureCategoryClass::DX8TextureCategoryClass(
    DX8FVFCategoryContainer *container_,
    TextureClass **,
    ShaderClass shd,
    VertexMaterialClass *mat,
    int pass_)
    : pass(pass_),
      shader(shd),
      material(mat),
      container(container_),
      render_task_head(nullptr)
{
    for (int i = 0; i < MAX_TEXTURE_STAGES; ++i) {
        textures[i] = nullptr;
    }
}

DX8TextureCategoryClass::~DX8TextureCategoryClass() = default;
void DX8TextureCategoryClass::Add_Render_Task(DX8PolygonRendererClass *, MeshClass *) {}
void DX8TextureCategoryClass::Render(void) {}
unsigned DX8TextureCategoryClass::Add_Mesh(Vertex_Split_Table &, unsigned, unsigned, IndexBufferClass *, unsigned) { return 0; }
void DX8TextureCategoryClass::Log(bool) {}
void DX8TextureCategoryClass::Remove_Polygon_Renderer(DX8PolygonRendererClass *) {}
void DX8TextureCategoryClass::Add_Polygon_Renderer(DX8PolygonRendererClass *, DX8PolygonRendererClass *) {}

DX8FVFCategoryContainer::DX8FVFCategoryContainer(unsigned fvf, bool is_sorting)
    : visible_matpass_head(nullptr),
      visible_matpass_tail(nullptr),
      index_buffer(nullptr),
      used_indices(0),
      FVF(fvf),
      passes(0),
      uv_coordinate_channels(0),
      sorting(is_sorting),
      AnythingToRender(false),
      AnyDelayedPassesToRender(false)
{
}

DX8FVFCategoryContainer::~DX8FVFCategoryContainer() = default;
unsigned DX8FVFCategoryContainer::Define_FVF(MeshModelClass *, unsigned int *, bool) { return 0; }
void DX8FVFCategoryContainer::Change_Polygon_Renderer_Texture(DX8PolygonRendererList &, TextureClass *, TextureClass *, unsigned, unsigned) {}
void DX8FVFCategoryContainer::Change_Polygon_Renderer_Material(DX8PolygonRendererList &, VertexMaterialClass *, VertexMaterialClass *, unsigned) {}
void DX8FVFCategoryContainer::Remove_Texture_Category(DX8TextureCategoryClass *) {}
void DX8FVFCategoryContainer::Add_Visible_Material_Pass(MaterialPassClass *, MeshClass *) { AnythingToRender = true; }

DX8RigidFVFCategoryContainer::DX8RigidFVFCategoryContainer(unsigned fvf, bool is_sorting)
    : DX8FVFCategoryContainer(fvf, is_sorting),
      vertex_buffer(nullptr),
      used_vertices(0),
      delayed_matpass_head(nullptr),
      delayed_matpass_tail(nullptr)
{
}

DX8RigidFVFCategoryContainer::~DX8RigidFVFCategoryContainer() = default;
void DX8RigidFVFCategoryContainer::Add_Mesh(MeshClass *) {}
void DX8RigidFVFCategoryContainer::Log(bool) {}
bool DX8RigidFVFCategoryContainer::Check_If_Mesh_Fits(MeshModelClass *) { return true; }
void DX8RigidFVFCategoryContainer::Render(void) {}
void DX8RigidFVFCategoryContainer::Add_Delayed_Visible_Material_Pass(MaterialPassClass *, MeshClass *) {}
void DX8RigidFVFCategoryContainer::Render_Delayed_Procedural_Material_Passes(void) {}

DX8SkinFVFCategoryContainer::DX8SkinFVFCategoryContainer(bool is_sorting)
    : DX8FVFCategoryContainer(0, is_sorting),
      VisibleVertexCount(0),
      VisibleSkinHead(nullptr)
{
}

DX8SkinFVFCategoryContainer::~DX8SkinFVFCategoryContainer() = default;
void DX8SkinFVFCategoryContainer::Render(void) {}
void DX8SkinFVFCategoryContainer::Add_Mesh(MeshClass *) {}
void DX8SkinFVFCategoryContainer::Log(bool) {}
bool DX8SkinFVFCategoryContainer::Check_If_Mesh_Fits(MeshModelClass *) { return true; }
void DX8SkinFVFCategoryContainer::Add_Visible_Skin(MeshClass *) {}
void DX8SkinFVFCategoryContainer::Reset() {}

DX8MeshRendererClass::DX8MeshRendererClass()
    : enable_lighting(false),
      camera(nullptr),
      texture_category_container_list_skin(nullptr),
      visible_decal_meshes(nullptr)
{
}

DX8MeshRendererClass::~DX8MeshRendererClass() = default;
void DX8MeshRendererClass::Init() {}
void DX8MeshRendererClass::Shutdown() {}
void DX8MeshRendererClass::Flush() {}
void DX8MeshRendererClass::Clear_Pending_Delete_Lists() {}
void DX8MeshRendererClass::Log_Statistics_String(bool) {}
void DX8MeshRendererClass::Request_Log_Statistics() {}
void DX8MeshRendererClass::Register_Mesh_Type(MeshClass *) {}
void DX8MeshRendererClass::Unregister_Mesh_Type(MeshClass *) {}
void DX8MeshRendererClass::Add_To_Render_List(DecalMeshClass *) {}
void DX8MeshRendererClass::Invalidate() {}
void DX8MeshRendererClass::Render_Decal_Meshes(void) {}
