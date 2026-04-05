#include "texproject.h"

#include "camera.h"
#include "render2d.h"
#include "texture.h"
#include "vertmaterial.h"

TexProjectClass::TexProjectClass(void)
	: Flags(DEFAULT_FLAGS), DesiredIntensity(1.0f), Intensity(1.0f), Attenuation(1.0f), MaterialPass(NEW_REF(MaterialPassClass, ())), Mapper1(NULL), RenderTarget(NULL), HFov(90.0f), VFov(90.0f), XMin(-10.0f), XMax(10.0f), YMin(-10.0f), YMax(10.0f), ZNear(1.0f), ZFar(1000.0f)
{
	VertexMaterialClass * material = NEW_REF(VertexMaterialClass, ());
	material->Set_Mapper(Mapper);
	MaterialPass->Set_Material(material);
	material->Release_Ref();
	Init_Multiplicative();
}

TexProjectClass::~TexProjectClass(void)
{
	REF_PTR_RELEASE(Mapper1);
	REF_PTR_RELEASE(MaterialPass);
	REF_PTR_RELEASE(RenderTarget);
}

void TexProjectClass::Set_Texture_Size(int size) { Flags &= ~SIZE_MASK; Flags |= (static_cast<uint32_t>(size) << SIZE_SHIFT); }
int TexProjectClass::Get_Texture_Size(void) { return static_cast<int>((Flags & SIZE_MASK) >> SIZE_SHIFT); }
void TexProjectClass::Set_Flag(uint32_t flag,bool onoff) { if (onoff) Flags |= flag; else Flags &= ~flag; }
bool TexProjectClass::Get_Flag(uint32_t flag) const { return (Flags & flag) == flag; }
void TexProjectClass::Set_Intensity(float intensity,bool immediate) { DesiredIntensity = intensity; if (immediate) Intensity = intensity; }
float TexProjectClass::Get_Intensity(void) { return DesiredIntensity; }
bool TexProjectClass::Is_Intensity_Zero(void) { return Intensity == 0.0f && DesiredIntensity == 0.0f; }
void TexProjectClass::Set_Attenuation(float attenuation) { Attenuation = attenuation; }
float TexProjectClass::Get_Attenuation(void) { return Attenuation; }
void TexProjectClass::Enable_Attenuation(bool onoff) { Set_Flag(ATTENUATE, onoff); }
bool TexProjectClass::Is_Attenuation_Enabled(void) { return Get_Flag(ATTENUATE); }
void TexProjectClass::Enable_Depth_Gradient(bool onoff) { Set_Flag(USE_DEPTH_GRADIENT, onoff); }
bool TexProjectClass::Is_Depth_Gradient_Enabled(bool) { return Get_Flag(USE_DEPTH_GRADIENT); }
void TexProjectClass::Init_Multiplicative(void) { Set_Flag(ADDITIVE, false); MaterialPass->Set_Shader(ShaderClass::_PresetMultiplicativeShader); }
void TexProjectClass::Init_Additive(void) { Set_Flag(ADDITIVE, true); MaterialPass->Set_Shader(ShaderClass::_PresetAdditiveShader); }
void TexProjectClass::Set_Perspective_Projection(float hfov,float vfov,float znear,float zfar) { HFov = hfov; VFov = vfov; ZNear = znear; ZFar = zfar; ProjectorClass::Set_Perspective_Projection(hfov, vfov, znear, zfar); Set_Flag(PERSPECTIVE, true); }
void TexProjectClass::Set_Ortho_Projection(float xmin,float xmax,float ymin,float ymax,float znear,float zfar) { XMin = xmin; XMax = xmax; YMin = ymin; YMax = ymax; ZNear = znear; ZFar = zfar; ProjectorClass::Set_Ortho_Projection(xmin, xmax, ymin, ymax, znear, zfar); Set_Flag(PERSPECTIVE, false); }
void TexProjectClass::Set_Texture(TextureClass * texture) { MaterialPass->Set_Texture(texture); }
TextureClass * TexProjectClass::Get_Texture(void) const { return MaterialPass->Get_Texture(); }
TextureClass * TexProjectClass::Peek_Texture(void) const { return MaterialPass->Peek_Texture(); }
MaterialPassClass * TexProjectClass::Peek_Material_Pass(void) { return MaterialPass; }
bool TexProjectClass::Compute_Perspective_Projection(RenderObjClass *,const Vector3 &,float,float) { return true; }
bool TexProjectClass::Compute_Perspective_Projection(const AABoxClass &,const Matrix3D &,const Vector3 &,float,float) { return true; }
bool TexProjectClass::Compute_Ortho_Projection(RenderObjClass *,const Vector3 &,float,float) { return true; }
bool TexProjectClass::Compute_Ortho_Projection(const AABoxClass &,const Matrix3D &,const Vector3 &,float,float) { return true; }
bool TexProjectClass::Needs_Render_Target(void) { return false; }
void TexProjectClass::Set_Render_Target(TextureClass * render_target) { REF_PTR_SET(RenderTarget, render_target); Set_Texture(RenderTarget); }
TextureClass * TexProjectClass::Peek_Render_Target(void) { return RenderTarget; }
bool TexProjectClass::Compute_Texture(RenderObjClass *,SpecialRenderInfoClass *) { return true; }
void TexProjectClass::Pre_Render_Update(const Matrix3D &) {}
void TexProjectClass::Update_WS_Bounding_Volume(void) { ProjectorClass::Update_WS_Bounding_Volume(); Vector3 extent; WorldBoundingVolume.Compute_Axis_Aligned_Extent(&extent); Set_Cull_Box(AABoxClass(WorldBoundingVolume.Center, extent)); }
void TexProjectClass::Configure_Camera(CameraClass & camera) { camera.Set_Transform(Transform); camera.Set_Clip_Planes(0.01f, ZFar); }
