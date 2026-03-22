#include "boxrobj.h"

#include <cstdio>

BoxLoaderClass _BoxLoader;

bool BoxRenderObjClass::IsInitted = false;
int BoxRenderObjClass::DisplayMask = 0;

BoxRenderObjClass::BoxRenderObjClass(void)
{
    Name[0] = '\0';
    Color.Set(1.0f, 1.0f, 1.0f);
    ObjSpaceCenter.Set(0.0f, 0.0f, 0.0f);
    ObjSpaceExtent.Set(1.0f, 1.0f, 1.0f);
    Opacity = 1.0f;
}

BoxRenderObjClass::BoxRenderObjClass(const W3dBoxStruct &)
    : BoxRenderObjClass()
{
}

BoxRenderObjClass::BoxRenderObjClass(const BoxRenderObjClass &src)
    : RenderObjClass(src)
{
    *this = src;
}

BoxRenderObjClass &BoxRenderObjClass::operator=(const BoxRenderObjClass &that)
{
    if (this != &that) {
        RenderObjClass::operator=(that);
        Set_Name(that.Get_Name());
        Color = that.Color;
        ObjSpaceCenter = that.ObjSpaceCenter;
        ObjSpaceExtent = that.ObjSpaceExtent;
        Opacity = that.Opacity;
    }
    return *this;
}

int BoxRenderObjClass::Get_Num_Polys(void) const { return 0; }
const char *BoxRenderObjClass::Get_Name(void) const { return Name; }
void BoxRenderObjClass::Set_Name(const char *name)
{
    if (name == nullptr) {
        Name[0] = '\0';
        return;
    }
    std::snprintf(Name, sizeof(Name), "%s", name);
}
void BoxRenderObjClass::Set_Color(const Vector3 &color) { Color = color; }
void BoxRenderObjClass::Init(void) { IsInitted = true; }
void BoxRenderObjClass::Shutdown(void) { IsInitted = false; }
void BoxRenderObjClass::Set_Box_Display_Mask(int mask) { DisplayMask = mask; }
int BoxRenderObjClass::Get_Box_Display_Mask(void) { return DisplayMask; }
void BoxRenderObjClass::render_box(RenderInfoClass &, const Vector3 &, const Vector3 &) {}
void BoxRenderObjClass::vis_render_box(SpecialRenderInfoClass &, const Vector3 &, const Vector3 &) {}

AABoxRenderObjClass::AABoxRenderObjClass(void) : BoxRenderObjClass() { update_cached_box(); }
AABoxRenderObjClass::AABoxRenderObjClass(const W3dBoxStruct &def) : BoxRenderObjClass(def) { update_cached_box(); }
AABoxRenderObjClass::AABoxRenderObjClass(const AABoxRenderObjClass &src) : BoxRenderObjClass(src), CachedBox(src.CachedBox) {}
AABoxRenderObjClass::AABoxRenderObjClass(const AABoxClass &box) : BoxRenderObjClass(), CachedBox(box) {}
AABoxRenderObjClass &AABoxRenderObjClass::operator=(const AABoxRenderObjClass &that)
{
    BoxRenderObjClass::operator=(that);
    CachedBox = that.CachedBox;
    return *this;
}
RenderObjClass *AABoxRenderObjClass::Clone(void) const { return new AABoxRenderObjClass(*this); }
int AABoxRenderObjClass::Class_ID(void) const { return RenderObjClass::CLASSID_UNKNOWN; }
void AABoxRenderObjClass::Render(RenderInfoClass &) {}
void AABoxRenderObjClass::Special_Render(SpecialRenderInfoClass &) {}
void AABoxRenderObjClass::Set_Transform(const Matrix3D &m) { RenderObjClass::Set_Transform(m); update_cached_box(); }
void AABoxRenderObjClass::Set_Position(const Vector3 &v) { RenderObjClass::Set_Position(v); update_cached_box(); }
bool AABoxRenderObjClass::Cast_Ray(RayCollisionTestClass &) { return false; }
bool AABoxRenderObjClass::Cast_AABox(AABoxCollisionTestClass &) { return false; }
bool AABoxRenderObjClass::Cast_OBBox(OBBoxCollisionTestClass &) { return false; }
bool AABoxRenderObjClass::Intersect_AABox(AABoxIntersectionTestClass &) { return false; }
bool AABoxRenderObjClass::Intersect_OBBox(OBBoxIntersectionTestClass &) { return false; }
void AABoxRenderObjClass::Get_Obj_Space_Bounding_Sphere(SphereClass &sphere) const { sphere.Init(ObjSpaceCenter, ObjSpaceExtent.Length()); }
void AABoxRenderObjClass::Get_Obj_Space_Bounding_Box(AABoxClass &box) const { box.Init(ObjSpaceCenter, ObjSpaceExtent); }
void AABoxRenderObjClass::update_cached_box(void) { CachedBox.Init(ObjSpaceCenter, ObjSpaceExtent); }

OBBoxRenderObjClass::OBBoxRenderObjClass(void) : BoxRenderObjClass() { update_cached_box(); }
OBBoxRenderObjClass::OBBoxRenderObjClass(const W3dBoxStruct &def) : BoxRenderObjClass(def) { update_cached_box(); }
OBBoxRenderObjClass::OBBoxRenderObjClass(const OBBoxRenderObjClass &src) : BoxRenderObjClass(src), CachedBox(src.CachedBox) {}
OBBoxRenderObjClass::OBBoxRenderObjClass(const OBBoxClass &box) : BoxRenderObjClass(), CachedBox(box) {}
OBBoxRenderObjClass &OBBoxRenderObjClass::operator=(const OBBoxRenderObjClass &that)
{
    BoxRenderObjClass::operator=(that);
    CachedBox = that.CachedBox;
    return *this;
}
RenderObjClass *OBBoxRenderObjClass::Clone(void) const { return new OBBoxRenderObjClass(*this); }
int OBBoxRenderObjClass::Class_ID(void) const { return RenderObjClass::CLASSID_UNKNOWN; }
void OBBoxRenderObjClass::Render(RenderInfoClass &) {}
void OBBoxRenderObjClass::Special_Render(SpecialRenderInfoClass &) {}
void OBBoxRenderObjClass::Set_Transform(const Matrix3D &m) { RenderObjClass::Set_Transform(m); update_cached_box(); }
void OBBoxRenderObjClass::Set_Position(const Vector3 &v) { RenderObjClass::Set_Position(v); update_cached_box(); }
bool OBBoxRenderObjClass::Cast_Ray(RayCollisionTestClass &) { return false; }
bool OBBoxRenderObjClass::Cast_AABox(AABoxCollisionTestClass &) { return false; }
bool OBBoxRenderObjClass::Cast_OBBox(OBBoxCollisionTestClass &) { return false; }
bool OBBoxRenderObjClass::Intersect_AABox(AABoxIntersectionTestClass &) { return false; }
bool OBBoxRenderObjClass::Intersect_OBBox(OBBoxIntersectionTestClass &) { return false; }
void OBBoxRenderObjClass::Get_Obj_Space_Bounding_Sphere(SphereClass &sphere) const { sphere.Init(ObjSpaceCenter, ObjSpaceExtent.Length()); }
void OBBoxRenderObjClass::Get_Obj_Space_Bounding_Box(AABoxClass &box) const { box.Init(ObjSpaceCenter, ObjSpaceExtent); }
OBBoxClass &OBBoxRenderObjClass::Get_Box(void) { update_cached_box(); return CachedBox; }
void OBBoxRenderObjClass::update_cached_box(void) { CachedBox = OBBoxClass(ObjSpaceCenter, ObjSpaceExtent); }

PrototypeClass *BoxLoaderClass::Load_W3D(ChunkLoadClass &) { return nullptr; }
BoxPrototypeClass::BoxPrototypeClass(W3dBoxStruct box) : Definition(box) {}
const char *BoxPrototypeClass::Get_Name(void) const { return Definition.Name; }
int BoxPrototypeClass::Get_Class_ID(void) const { return RenderObjClass::CLASSID_UNKNOWN; }
RenderObjClass *BoxPrototypeClass::Create(void) { return new OBBoxRenderObjClass(Definition); }
