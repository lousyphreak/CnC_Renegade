#include "vertmaterial.h"

#include "chunkio.h"

#include <cstring>

namespace {

struct HeadlessColor {
	float r;
	float g;
	float b;
	float a;
};

}

struct _D3DMATERIAL8 {
	HeadlessColor Diffuse;
	HeadlessColor Ambient;
	HeadlessColor Specular;
	HeadlessColor Emissive;
	float Power;
};

static unsigned int g_unique_material_id = 1;
static const unsigned int D3DMCS_MATERIAL = 0;
static const unsigned int D3DMCS_COLOR1 = 1;
static const unsigned int D3DMCS_COLOR2 = 2;

VertexMaterialClass * VertexMaterialClass::Presets[VertexMaterialClass::PRESET_COUNT] = { NULL, NULL };

VertexMaterialClass::VertexMaterialClass(void)
	: Material(new _D3DMATERIAL8()), Flags(0), AmbientColorSource(D3DMCS_MATERIAL), EmissiveColorSource(D3DMCS_MATERIAL), DiffuseColorSource(D3DMCS_MATERIAL), UseLighting(false), UniqueID(0), CRC(0), CRCDirty(true)
{
	std::memset(Material, 0, sizeof(*Material));
	for (int i = 0; i < MeshBuilderClass::MAX_STAGES; ++i) {
		Mapper[i] = NULL;
		UVSource[i] = i;
	}
	Set_Ambient(1.0f, 1.0f, 1.0f);
	Set_Diffuse(1.0f, 1.0f, 1.0f);
	Set_Opacity(1.0f);
}

VertexMaterialClass::VertexMaterialClass(const VertexMaterialClass & src) : VertexMaterialClass() { *this = src; }
VertexMaterialClass::~VertexMaterialClass(void)
{
	for (int i = 0; i < MeshBuilderClass::MAX_STAGES; ++i) {
		REF_PTR_RELEASE(Mapper[i]);
	}
	delete Material;
}

VertexMaterialClass & VertexMaterialClass::operator = (const VertexMaterialClass & src)
{
	if (this != &src) {
		Name = src.Name;
		Flags = src.Flags;
		AmbientColorSource = src.AmbientColorSource;
		EmissiveColorSource = src.EmissiveColorSource;
		DiffuseColorSource = src.DiffuseColorSource;
		UseLighting = src.UseLighting;
		UniqueID = src.UniqueID;
		CRCDirty = true;
		*Material = *src.Material;
		for (int i = 0; i < MeshBuilderClass::MAX_STAGES; ++i) {
			REF_PTR_SET(Mapper[i], src.Mapper[i]);
			UVSource[i] = src.UVSource[i];
		}
	}
	return *this;
}

float VertexMaterialClass::Get_Shininess(void) const { return Material->Power; }
void VertexMaterialClass::Set_Shininess(float shin) { CRCDirty = true; Material->Power = shin; }
float VertexMaterialClass::Get_Opacity(void) const { return Material->Diffuse.a; }
void VertexMaterialClass::Set_Opacity(float o) { CRCDirty = true; Material->Diffuse.a = o; Material->Ambient.a = o; Material->Specular.a = o; Material->Emissive.a = o; }
void VertexMaterialClass::Get_Ambient(Vector3 * set_color) const { if (set_color) set_color->Set(Material->Ambient.r, Material->Ambient.g, Material->Ambient.b); }
void VertexMaterialClass::Set_Ambient(const Vector3 & color) { Set_Ambient(color.X, color.Y, color.Z); }
void VertexMaterialClass::Set_Ambient(float r,float g,float b) { CRCDirty = true; Material->Ambient.r = r; Material->Ambient.g = g; Material->Ambient.b = b; }
void VertexMaterialClass::Get_Diffuse(Vector3 * set_color) const { if (set_color) set_color->Set(Material->Diffuse.r, Material->Diffuse.g, Material->Diffuse.b); }
void VertexMaterialClass::Set_Diffuse(const Vector3 & color) { Set_Diffuse(color.X, color.Y, color.Z); }
void VertexMaterialClass::Set_Diffuse(float r,float g,float b) { CRCDirty = true; Material->Diffuse.r = r; Material->Diffuse.g = g; Material->Diffuse.b = b; }
void VertexMaterialClass::Get_Specular(Vector3 * set_color) const { if (set_color) set_color->Set(Material->Specular.r, Material->Specular.g, Material->Specular.b); }
void VertexMaterialClass::Set_Specular(const Vector3 & color) { Set_Specular(color.X, color.Y, color.Z); }
void VertexMaterialClass::Set_Specular(float r,float g,float b) { CRCDirty = true; Material->Specular.r = r; Material->Specular.g = g; Material->Specular.b = b; }
void VertexMaterialClass::Get_Emissive(Vector3 * set_color) const { if (set_color) set_color->Set(Material->Emissive.r, Material->Emissive.g, Material->Emissive.b); }
void VertexMaterialClass::Set_Emissive(const Vector3 & color) { Set_Emissive(color.X, color.Y, color.Z); }
void VertexMaterialClass::Set_Emissive(float r,float g,float b) { CRCDirty = true; Material->Emissive.r = r; Material->Emissive.g = g; Material->Emissive.b = b; }
void VertexMaterialClass::Set_Ambient_Color_Source(ColorSourceType src) { CRCDirty = true; AmbientColorSource = src == COLOR1 ? D3DMCS_COLOR1 : src == COLOR2 ? D3DMCS_COLOR2 : D3DMCS_MATERIAL; }
VertexMaterialClass::ColorSourceType VertexMaterialClass::Get_Ambient_Color_Source(void) { return AmbientColorSource == D3DMCS_COLOR1 ? COLOR1 : AmbientColorSource == D3DMCS_COLOR2 ? COLOR2 : MATERIAL; }
void VertexMaterialClass::Set_Emissive_Color_Source(ColorSourceType src) { CRCDirty = true; EmissiveColorSource = src == COLOR1 ? D3DMCS_COLOR1 : src == COLOR2 ? D3DMCS_COLOR2 : D3DMCS_MATERIAL; }
VertexMaterialClass::ColorSourceType VertexMaterialClass::Get_Emissive_Color_Source(void) { return EmissiveColorSource == D3DMCS_COLOR1 ? COLOR1 : EmissiveColorSource == D3DMCS_COLOR2 ? COLOR2 : MATERIAL; }
void VertexMaterialClass::Set_Diffuse_Color_Source(ColorSourceType src) { CRCDirty = true; DiffuseColorSource = src == COLOR1 ? D3DMCS_COLOR1 : src == COLOR2 ? D3DMCS_COLOR2 : D3DMCS_MATERIAL; }
VertexMaterialClass::ColorSourceType VertexMaterialClass::Get_Diffuse_Color_Source(void) { return DiffuseColorSource == D3DMCS_COLOR1 ? COLOR1 : DiffuseColorSource == D3DMCS_COLOR2 ? COLOR2 : MATERIAL; }
void VertexMaterialClass::Set_UV_Source(int stage,int array_index) { if (stage >= 0 && stage < MeshBuilderClass::MAX_STAGES) { CRCDirty = true; UVSource[stage] = array_index; } }
int VertexMaterialClass::Get_UV_Source(int stage) { return stage >= 0 && stage < MeshBuilderClass::MAX_STAGES ? UVSource[stage] : 0; }
WW3DErrorType VertexMaterialClass::Load_W3D(ChunkLoadClass &) { return WW3D_ERROR_OK; }
WW3DErrorType VertexMaterialClass::Save_W3D(ChunkSaveClass &) { return WW3D_ERROR_OK; }
void VertexMaterialClass::Init_From_Material3(const W3dMaterial3Struct &) {}
void VertexMaterialClass::Init() { if (Presets[PRELIT_DIFFUSE] == NULL) { Presets[PRELIT_DIFFUSE] = NEW_REF(VertexMaterialClass, ()); Presets[PRELIT_DIFFUSE]->Set_Lighting(false); } if (Presets[PRELIT_NODIFFUSE] == NULL) { Presets[PRELIT_NODIFFUSE] = NEW_REF(VertexMaterialClass, ()); Presets[PRELIT_NODIFFUSE]->Set_Diffuse(0.0f, 0.0f, 0.0f); } }
void VertexMaterialClass::Shutdown() { for (int i = 0; i < PRESET_COUNT; ++i) { if (Presets[i] != NULL) { Presets[i]->Release_Ref(); Presets[i] = NULL; } } }
VertexMaterialClass * VertexMaterialClass::Get_Preset(PresetType type) { Init(); VertexMaterialClass * material = Presets[type]; if (material != NULL) material->Add_Ref(); return material; }
void VertexMaterialClass::Make_Unique() { UniqueID = g_unique_material_id++; CRCDirty = true; }
void VertexMaterialClass::Apply(void) const {}
void VertexMaterialClass::Apply_Null(void) {}
unsigned long VertexMaterialClass::Compute_CRC(void) const { return static_cast<unsigned long>(Flags ^ UniqueID ^ static_cast<unsigned int>(UseLighting)); }
