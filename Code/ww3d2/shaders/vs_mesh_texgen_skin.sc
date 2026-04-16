$input a_position, a_normal, a_color0, a_texcoord0, a_texcoord1, a_texcoord2
$output v_color0, v_texcoord0, v_texcoord1, v_fogFactor, v_worldPos, v_viewDepth, v_worldNormal

#include <bgfx_shader.sh>
#include "skin_common.sh"

uniform vec4 u_meshFogConfig;
uniform vec4 u_meshTexgenMode;
uniform vec4 u_meshTexTransformFlags;
uniform mat4 u_meshTexTransform0;
uniform mat4 u_meshTexTransform1;

vec2 ResolveTexcoord(
    int stage,
    vec2 texcoord0,
    vec2 texcoord1,
    vec3 viewPosition,
    vec3 viewNormal)
{
    float texgenMode = stage == 0 ? u_meshTexgenMode.x : u_meshTexgenMode.z;
    float uvSource = stage == 0 ? u_meshTexgenMode.y : u_meshTexgenMode.w;
    float transformFlags = stage == 0 ? u_meshTexTransformFlags.x : u_meshTexTransformFlags.y;

    vec4 coordinate;
    if (texgenMode < 0.5) {
        coordinate = uvSource > 0.5 ? vec4(texcoord1, 1.0, 1.0) : vec4(texcoord0, 1.0, 1.0);
    } else if (texgenMode < 1.5) {
        coordinate = vec4(viewNormal, 1.0);
    } else if (texgenMode < 2.5) {
        coordinate = vec4(viewPosition, 1.0);
    } else {
        vec3 eyeVector = normalize(-viewPosition);
        vec3 reflection = viewNormal * (2.0 * dot(viewNormal, eyeVector)) - eyeVector;
        coordinate = vec4(reflection, 1.0);
    }

    if (transformFlags > 0.5) {
        mat4 texMatrix = stage == 0 ? u_meshTexTransform0 : u_meshTexTransform1;
        coordinate = mul(texMatrix, coordinate);
        if (transformFlags > 1.5) {
            float q = coordinate.z;
            if (abs(q) > 1.0e-6) {
                coordinate.xy /= q;
            }
        }
    }

    return coordinate.xy;
}

void main()
{
    vec3 skinnedPosition;
    vec3 skinnedNormal;
    ApplyRigidSkinning(a_position, a_normal, a_texcoord2, skinnedPosition, skinnedNormal);

    vec4 skinnedPosition4 = vec4(skinnedPosition, 1.0);
    gl_Position = mul(u_modelViewProj, skinnedPosition4);
    v_color0 = a_color0;

    vec3 viewPos = mul(u_modelView, skinnedPosition4).xyz;
    vec3 viewNormal = normalize(mul(u_modelView, vec4(skinnedNormal, 0.0)).xyz);

    v_texcoord0 = ResolveTexcoord(0, a_texcoord0, a_texcoord1, viewPos, viewNormal);
    v_texcoord1 = ResolveTexcoord(1, a_texcoord0, a_texcoord1, viewPos, viewNormal);

    v_worldPos = mul(u_model[0], skinnedPosition4).xyz;
    v_worldNormal = normalize(mul(u_model[0], vec4(skinnedNormal, 0.0)).xyz);
    v_viewDepth = -viewPos.z;

    v_fogFactor = 0.0;
    if (u_meshFogConfig.x > 0.5) {
        float dist = u_meshFogConfig.w > 0.0 ? length(viewPos) : abs(viewPos.z);
        float factor = (u_meshFogConfig.z - dist) / max(u_meshFogConfig.z - u_meshFogConfig.y, 1.0e-4);
        v_fogFactor = 1.0 - clamp(factor, 0.0, 1.0);
    }
}
