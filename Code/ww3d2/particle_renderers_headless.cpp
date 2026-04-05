#include "pointgr.h"
#include "linegrp.h"
#include "seglinerenderer.h"

#include "matrix3d.h"
#include "shader.h"

Vector3 PointGroupClass::_TriVertexLocationOrientationTable[256][3] = {};
Vector3 PointGroupClass::_QuadVertexLocationOrientationTable[256][4] = {};
Vector3 PointGroupClass::_ScreenspaceVertexLocationSizeTable[2][3] = {};
Vector2 *PointGroupClass::_TriVertexUVFrameTable[5] = {};
Vector2 *PointGroupClass::_QuadVertexUVFrameTable[5] = {};
VertexMaterialClass *PointGroupClass::PointMaterial = nullptr;

PointGroupClass::PointGroupClass(void)
	: PointLoc(nullptr),
	  PointDiffuse(nullptr),
	  APT(nullptr),
	  PointSize(nullptr),
	  PointOrientation(nullptr),
	  PointFrame(nullptr),
	  PointCount(0),
	  FrameRowColumnCountLog2(0),
	  Texture(nullptr),
	  Shader(ShaderClass::_PresetOpaqueShader),
	  PointMode(TRIS),
	  Flags(0),
	  DefaultPointSize(1.0f),
	  DefaultPointColor(1.0f, 1.0f, 1.0f),
	  DefaultPointAlpha(1.0f),
	  DefaultPointOrientation(0),
	  DefaultPointFrame(0),
	  VPXMin(0.0f),
	  VPYMin(0.0f),
	  VPXMax(0.0f),
	  VPYMax(0.0f)
{
}

PointGroupClass::~PointGroupClass(void) = default;

void PointGroupClass::Set_Arrays(ShareBufferClass<Vector3> *locs, ShareBufferClass<Vector4> *diffuse, ShareBufferClass<uint32_t> *apt, ShareBufferClass<float> *sizes, ShareBufferClass<uint8_t> *orientations, ShareBufferClass<uint8_t> *frames, int active_point_count, float vpxmin, float vpymin, float vpxmax, float vpymax)
{
	PointLoc = locs;
	PointDiffuse = diffuse;
	APT = apt;
	PointSize = sizes;
	PointOrientation = orientations;
	PointFrame = frames;
	PointCount = active_point_count >= 0 ? active_point_count : (locs != nullptr ? locs->Get_Count() : 0);
	VPXMin = vpxmin;
	VPYMin = vpymin;
	VPXMax = vpxmax;
	VPYMax = vpymax;
}

void PointGroupClass::Set_Point_Size(float size) { DefaultPointSize = size; }
float PointGroupClass::Get_Point_Size(void) { return DefaultPointSize; }
void PointGroupClass::Set_Point_Color(Vector3 color) { DefaultPointColor = color; }
Vector3 PointGroupClass::Get_Point_Color(void) { return DefaultPointColor; }
void PointGroupClass::Set_Point_Alpha(float alpha) { DefaultPointAlpha = alpha; }
float PointGroupClass::Get_Point_Alpha(void) { return DefaultPointAlpha; }
void PointGroupClass::Set_Point_Orientation(uint8_t orientation) { DefaultPointOrientation = orientation; }
uint8_t PointGroupClass::Get_Point_Orientation(void) { return DefaultPointOrientation; }
void PointGroupClass::Set_Point_Frame(uint8_t frame) { DefaultPointFrame = frame; }
uint8_t PointGroupClass::Get_Point_Frame(void) { return DefaultPointFrame; }
void PointGroupClass::Set_Point_Mode(PointModeEnum mode) { PointMode = mode; }
PointGroupClass::PointModeEnum PointGroupClass::Get_Point_Mode(void) { return PointMode; }
void PointGroupClass::Set_Flag(FlagsType flag, bool onoff) { if (onoff) { Flags |= (1u << flag); } else { Flags &= ~(1u << flag); } }
int PointGroupClass::Get_Flag(FlagsType flag) { return (Flags & (1u << flag)) != 0; }
void PointGroupClass::Set_Texture(TextureClass *texture) { Texture = texture; }
TextureClass *PointGroupClass::Get_Texture(void) { return Texture; }
TextureClass *PointGroupClass::Peek_Texture(void) { return Texture; }
void PointGroupClass::Set_Shader(ShaderClass shader) { Shader = shader; }
ShaderClass PointGroupClass::Get_Shader(void) { return Shader; }
uint8_t PointGroupClass::Get_Frame_Row_Column_Count_Log2(void) { return FrameRowColumnCountLog2; }
void PointGroupClass::Set_Frame_Row_Column_Count_Log2(uint8_t frccl2) { FrameRowColumnCountLog2 = frccl2; }
int PointGroupClass::Get_Polygon_Count(void) { return PointMode == QUADS ? PointCount * 2 : PointCount; }
void PointGroupClass::Render(RenderInfoClass &) {}
void PointGroupClass::Update_Arrays(Vector3 *, Vector4 *, float *, uint8_t *, uint8_t *, int, int, int &, int &) {}
void PointGroupClass::_Init(void) {}
void PointGroupClass::_Shutdown(void) {}

LineGroupClass::LineGroupClass(void)
	: StartLineLoc(nullptr),
	  EndLineLoc(nullptr),
	  LineDiffuse(nullptr),
	  TailDiffuse(nullptr),
	  ALT(nullptr),
	  LineSize(nullptr),
	  LineUCoord(nullptr),
	  LineCount(0),
	  Texture(nullptr),
	  Shader(ShaderClass::_PresetOpaqueShader),
	  Flags(0),
	  DefaultLineSize(1.0f),
	  DefaultLineColor(1.0f, 1.0f, 1.0f),
	  DefaultLineAlpha(1.0f),
	  DefaultLineUCoord(0.0f),
	  DefaultTailDiffuse(1.0f, 1.0f, 1.0f, 1.0f),
	  LineMode(TETRAHEDRON)
{
}

LineGroupClass::~LineGroupClass(void) = default;
void LineGroupClass::Set_Arrays(ShareBufferClass<Vector3> *startlocs, ShareBufferClass<Vector3> *endlocs, ShareBufferClass<Vector4> *diffuse, ShareBufferClass<Vector4> *taildiffuse, ShareBufferClass<uint32_t> *alt, ShareBufferClass<float> *sizes, ShareBufferClass<float> *ucoords, int active_line_count)
{
	StartLineLoc = startlocs;
	EndLineLoc = endlocs;
	LineDiffuse = diffuse;
	TailDiffuse = taildiffuse;
	ALT = alt;
	LineSize = sizes;
	LineUCoord = ucoords;
	LineCount = active_line_count >= 0 ? active_line_count : (startlocs != nullptr ? startlocs->Get_Count() : 0);
}
void LineGroupClass::Set_Line_Size(float size) { DefaultLineSize = size; }
float LineGroupClass::Get_Line_Size(void) { return DefaultLineSize; }
void LineGroupClass::Set_Line_Color(const Vector3 &color) { DefaultLineColor = color; }
Vector3 LineGroupClass::Get_Line_Color(void) { return DefaultLineColor; }
void LineGroupClass::Set_Tail_Diffuse(const Vector4 &tdiffuse) { DefaultTailDiffuse = tdiffuse; }
Vector4 LineGroupClass::Get_Tail_Diffuse(void) { return DefaultTailDiffuse; }
void LineGroupClass::Set_Line_Alpha(float alpha) { DefaultLineAlpha = alpha; }
float LineGroupClass::Get_Line_Alpha(void) { return DefaultLineAlpha; }
void LineGroupClass::Set_Line_UCoord(float ucoord) { DefaultLineUCoord = ucoord; }
float LineGroupClass::Get_Line_UCoord(void) { return DefaultLineUCoord; }
void LineGroupClass::Set_Flag(FlagsType flag, bool on) { if (on) { Flags |= (1u << flag); } else { Flags &= ~(1u << flag); } }
int LineGroupClass::Get_Flag(FlagsType flag) { return (Flags & (1u << flag)) != 0; }
void LineGroupClass::Set_Texture(TextureClass *texture) { Texture = texture; }
TextureClass *LineGroupClass::Get_Texture(void) { return Texture; }
TextureClass *LineGroupClass::Peek_Texture(void) { return Texture; }
void LineGroupClass::Set_Shader(const ShaderClass &shader) { Shader = shader; }
ShaderClass LineGroupClass::Get_Shader(void) { return Shader; }
void LineGroupClass::Set_Line_Mode(LineModeType linemode) { LineMode = linemode; }
LineGroupClass::LineModeType LineGroupClass::Get_Line_Mode(void) { return LineMode; }
int LineGroupClass::Get_Polygon_Count(void) { return LineCount; }
void LineGroupClass::Render(RenderInfoClass &) {}

SegLineRendererClass::SegLineRendererClass(void)
	: Texture(nullptr),
	  Shader(ShaderClass::_PresetOpaqueShader),
	  Width(1.0f),
	  Color(1.0f, 1.0f, 1.0f),
	  Opacity(1.0f),
	  SubdivisionLevel(0),
	  NoiseAmplitude(0.0f),
	  MergeAbortFactor(1.5f),
	  TextureTileFactor(1.0f),
	  LastUsedSyncTime(0),
	  CurrentUVOffset(0.0f, 0.0f),
	  UVOffsetDeltaPerMS(0.0f, 0.0f),
	  Bits(DEFAULT_BITS)
{
}

SegLineRendererClass::SegLineRendererClass(const SegLineRendererClass &that) = default;
SegLineRendererClass &SegLineRendererClass::operator=(const SegLineRendererClass &that) = default;
SegLineRendererClass::~SegLineRendererClass(void) = default;
void SegLineRendererClass::Init(const W3dEmitterLinePropertiesStruct &) {}
TextureClass *SegLineRendererClass::Get_Texture(void) const { return Texture; }
void SegLineRendererClass::Set_Texture(TextureClass *texture) { Texture = texture; }
void SegLineRendererClass::Set_Texture_Tile_Factor(float factor) { TextureTileFactor = factor; }
void SegLineRendererClass::Set_Current_UV_Offset(const Vector2 &offset) { CurrentUVOffset = offset; }
void SegLineRendererClass::Render(RenderInfoClass &, const Matrix3D &, uint32_t, Vector3 *, const SphereClass &) {}
void SegLineRendererClass::Reset_Line(void) {}
void SegLineRendererClass::subdivision_util(uint32_t, const Vector3 *, const float *, uint32_t *, Vector3 *, float *) {}