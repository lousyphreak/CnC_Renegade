#include "dazzle.h"
#include "aabox.h"
#include "persistfactory.h"
#include "sphere.h"

namespace {

DazzleLayerClass * g_current_layer = NULL;
const DazzleVisibilityClass * g_visibility_handler = NULL;

class HeadlessDazzlePersistFactory : public PersistFactoryClass
{
public:
	uint32 Chunk_ID(void) const override { return 0; }
	PersistClass * Load(ChunkLoadClass &) const override { return NULL; }
	void Save(ChunkSaveClass &, PersistClass *) const override {}
};

HeadlessDazzlePersistFactory g_dazzle_persist_factory;

}

bool DazzleRenderObjClass::_dazzle_rendering_enabled = true;
DazzleLoaderClass _DazzleLoader;

DazzleLayerClass::DazzleLayerClass(void) : visible_lists(NULL) {}
DazzleLayerClass::~DazzleLayerClass(void) { delete [] visible_lists; }
void DazzleLayerClass::Render(CameraClass *) {}
int DazzleLayerClass::Get_Visible_Item_Count(unsigned int) const { return 0; }
void DazzleLayerClass::Clear_Visible_List(unsigned int) {}

float DazzleVisibilityClass::Compute_Dazzle_Visibility(RenderInfoClass &, DazzleRenderObjClass *, const Vector3 &) const { return 1.0f; }

DazzleRenderObjClass::DazzleRenderObjClass(unsigned)
{
}

DazzleRenderObjClass::DazzleRenderObjClass(const char *)
{
}

DazzleRenderObjClass::DazzleRenderObjClass(const DazzleRenderObjClass &)
{
}

DazzleRenderObjClass & DazzleRenderObjClass::operator=(const DazzleRenderObjClass &)
{
	return *this;
}

RenderObjClass * DazzleRenderObjClass::Clone(void) const
{
	return new DazzleRenderObjClass(*this);
}

void DazzleRenderObjClass::Render(RenderInfoClass &)
{
}

void DazzleRenderObjClass::Special_Render(SpecialRenderInfoClass &)
{
}

void DazzleRenderObjClass::Set_Transform(const Matrix3D &m)
{
	RenderObjClass::Set_Transform(m);
}

void DazzleRenderObjClass::Get_Obj_Space_Bounding_Sphere(SphereClass & sphere) const
{
	sphere.Init(Vector3(0, 0, 0), radius);
}

void DazzleRenderObjClass::Get_Obj_Space_Bounding_Box(AABoxClass & box) const
{
	box.Init(Vector3(0, 0, 0), Vector3(radius, radius, radius));
}

void DazzleRenderObjClass::Set_Layer(DazzleLayerClass *)
{
}

const PersistFactoryClass & DazzleRenderObjClass::Get_Factory(void) const
{
	return g_dazzle_persist_factory;
}

void DazzleRenderObjClass::Set_Current_Dazzle_Layer(DazzleLayerClass * layer) { g_current_layer = layer; }
void DazzleRenderObjClass::Init_Type(const DazzleInitClass &) {}
void DazzleRenderObjClass::Init_Lensflare(const LensflareInitClass &) {}
void DazzleRenderObjClass::Init_From_INI(const INIClass *) {}
unsigned DazzleRenderObjClass::Get_Type_ID(const char *) { return 0; }
const char * DazzleRenderObjClass::Get_Type_Name(unsigned int) { return "headless_dazzle"; }
DazzleTypeClass * DazzleRenderObjClass::Get_Type_Class(unsigned) { return NULL; }
unsigned DazzleRenderObjClass::Get_Lensflare_ID(const char *) { return 0; }
LensflareTypeClass * DazzleRenderObjClass::Get_Lensflare_Class(unsigned) { return NULL; }
void DazzleRenderObjClass::Deinit() {}
void DazzleRenderObjClass::Install_Dazzle_Visibility_Handler(const DazzleVisibilityClass * visibility_handler) { g_visibility_handler = visibility_handler; }

RenderObjClass * DazzlePrototypeClass::Create(void) { return NULL; }
WW3DErrorType DazzlePrototypeClass::Load_W3D(ChunkLoadClass &) { return WW3D_ERROR_OK; }
PrototypeClass * DazzleLoaderClass::Load_W3D(ChunkLoadClass &) { return NULL; }
