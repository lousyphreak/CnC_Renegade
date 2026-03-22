#include "statistics.h"

#include "wwstring.h"

namespace Debug_Statistics {

namespace {
StringClass g_record_texture_string(true);
RecordTextureMode g_record_texture_mode = RECORD_TEXTURE_NONE;
}

void Record_Texture_Mode(RecordTextureMode m)
{
    g_record_texture_mode = m;
}

RecordTextureMode Get_Record_Texture_Mode()
{
    return g_record_texture_mode;
}

void Record_Texture(TextureClass *) {}
int Get_Record_Texture_Size() { return 0; }
int Get_Record_Lightmap_Texture_Size() { return 0; }
int Get_Record_Procedural_Texture_Size() { return 0; }
int Get_Record_Texture_Count() { return 0; }
int Get_Record_Texture_Change_Count() { return 0; }
int Get_Record_Lightmap_Texture_Count() { return 0; }
int Get_Record_Procedural_Texture_Count() { return 0; }
const StringClass &Get_Record_Texture_String() { return g_record_texture_string; }
void Record_DX8_Skin_Polys_And_Vertices(int, int) {}
void Record_DX8_Polys_And_Vertices(int, int, const ShaderClass &) {}
void Record_Sorting_Polys_And_Vertices(int, int) {}
int Get_DX8_Polygons() { return 0; }
int Get_DX8_Vertices() { return 0; }
int Get_DX8_Skin_Renders() { return 0; }
int Get_DX8_Skin_Polygons() { return 0; }
int Get_DX8_Skin_Vertices() { return 0; }
int Get_Sorting_Polygons() { return 0; }
int Get_Sorting_Vertices() { return 0; }
void Begin_Statistics() {}
void End_Statistics() {}

} // namespace Debug_Statistics
