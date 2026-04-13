// Shared definitions for all mesh shaders.
// Included by vs_mesh_*.sc and fs_mesh_*.sc.

// Stage color/alpha operation enum — must match C++ StageColorOp.
#define STAGE_DISABLE        0.0
#define STAGE_MODULATE       1.0
#define STAGE_SELECT_TEXTURE 2.0
#define STAGE_SELECT_CURRENT 3.0
#define STAGE_ADD            4.0
#define STAGE_ADDSMOOTH      5.0
#define STAGE_SUBTRACT       6.0
#define STAGE_BLEND_TEX_ALPHA 7.0
#define STAGE_BLEND_CUR_ALPHA 8.0

// Fog mode constants (fragment shader)
#define FOG_NONE           0.0
#define FOG_ENABLE         1.0
#define FOG_SCALE_FRAGMENT 2.0
#define FOG_WHITE          3.0

vec3 ApplyColorOp(float op, vec4 current, vec4 texel)
{
    if (op < 0.5) return current.rgb;
    if (op < 1.5) return current.rgb * texel.rgb;
    if (op < 2.5) return texel.rgb;
    if (op < 3.5) return current.rgb;
    if (op < 4.5) return clamp(current.rgb + texel.rgb, vec3_splat(0.0), vec3_splat(1.0));
    if (op < 5.5) return clamp(current.rgb + texel.rgb * (vec3_splat(1.0) - current.rgb), vec3_splat(0.0), vec3_splat(1.0));
    if (op < 6.5) return clamp(current.rgb - texel.rgb, vec3_splat(0.0), vec3_splat(1.0));
    if (op < 7.5) return mix(current.rgb, texel.rgb, texel.a);
    if (op < 8.5) return mix(current.rgb, texel.rgb, current.a);
    return current.rgb;
}

float ApplyAlphaOp(float op, float currentAlpha, vec4 texel)
{
    if (op < 0.5) return currentAlpha;
    if (op < 1.5) return currentAlpha * texel.a;
    if (op < 2.5) return texel.a;
    if (op < 3.5) return currentAlpha;
    if (op < 4.5) return clamp(currentAlpha + texel.a, 0.0, 1.0);
    if (op < 5.5) return clamp(currentAlpha + texel.a * (1.0 - currentAlpha), 0.0, 1.0);
    return currentAlpha;
}

void ApplyFog(inout vec3 color, float fogMode, float fogAmount, vec3 fogColor)
{
    if (fogMode > 0.5) {
        float fog = clamp(fogAmount, 0.0, 1.0);
        if (fogMode < 1.5)
            color = mix(color, fogColor, fog);
        else if (fogMode < 2.5)
            color = color * (1.0 - fog);
        else
            color = mix(color, vec3_splat(1.0), fog);
    }
}

float ComputeLinearFog(vec3 viewPosition, float fogStart, float fogEnd, float rangeFogSign)
{
    float fogDistance = rangeFogSign > 0.0 ? length(viewPosition) : abs(viewPosition.z);
    float fogFactor = (fogEnd - fogDistance) / max(fogEnd - fogStart, 1.0e-4);
    return 1.0 - clamp(fogFactor, 0.0, 1.0);
}
