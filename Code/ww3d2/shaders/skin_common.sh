SAMPLER2D(s_skinPalette, 3);
uniform vec4 u_skinPaletteInfo;

vec4 FetchSkinPaletteTexel(float texelOffset)
{
    vec2 uv = vec2(
        (texelOffset + 0.5) * u_skinPaletteInfo.y,
        (u_skinPaletteInfo.x + 0.5) * u_skinPaletteInfo.z);
    return texture2DLod(s_skinPalette, uv, 0.0);
}

void ApplyRigidSkinning(vec3 position, vec3 normal, float boneIndex, out vec3 skinnedPosition, out vec3 skinnedNormal)
{
    float baseOffset = boneIndex * 3.0;
    vec4 row0 = FetchSkinPaletteTexel(baseOffset + 0.0);
    vec4 row1 = FetchSkinPaletteTexel(baseOffset + 1.0);
    vec4 row2 = FetchSkinPaletteTexel(baseOffset + 2.0);

    skinnedPosition = vec3(
        dot(row0.xyz, position) + row0.w,
        dot(row1.xyz, position) + row1.w,
        dot(row2.xyz, position) + row2.w);
    skinnedNormal = vec3(
        dot(row0.xyz, normal),
        dot(row1.xyz, normal),
        dot(row2.xyz, normal));
}

vec3 ApplyRigidSkinningPosition(vec3 position, float boneIndex)
{
    float baseOffset = boneIndex * 3.0;
    vec4 row0 = FetchSkinPaletteTexel(baseOffset + 0.0);
    vec4 row1 = FetchSkinPaletteTexel(baseOffset + 1.0);
    vec4 row2 = FetchSkinPaletteTexel(baseOffset + 2.0);

    return vec3(
        dot(row0.xyz, position) + row0.w,
        dot(row1.xyz, position) + row1.w,
        dot(row2.xyz, position) + row2.w);
}
