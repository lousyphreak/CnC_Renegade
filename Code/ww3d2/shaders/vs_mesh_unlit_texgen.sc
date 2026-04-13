$input a_position, a_normal, a_color0, a_color1, a_texcoord0, a_texcoord1
$output v_color0, v_texcoord0, v_texcoord1, v_fogFactor, v_worldPos, v_viewDepth, v_worldNormal

#include <bgfx_shader.sh>

// u_meshFogConfig.x = fogEnabled (0 or 1)
// u_meshFogConfig.y = fogStart
// u_meshFogConfig.z = fogEnd
// u_meshFogConfig.w = rangeFogSign (1.0 = range, -1.0 = planar)
uniform vec4 u_meshFogConfig;

// u_meshTexgenMode.x = texgen mode stage 0 (0=passthrough, 1=camera_normal, 2=camera_position, 3=reflection)
// u_meshTexgenMode.y = texgen UV source stage 0
// u_meshTexgenMode.z = texgen mode stage 1
// u_meshTexgenMode.w = texgen UV source stage 1
uniform vec4 u_meshTexgenMode;

// u_meshTexTransformFlags.x = stage0 (0=disabled, 1=count2, 2=count3_projected)
// u_meshTexTransformFlags.y = stage1
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
        coordinate = uvSource > 0.5 ? vec4(texcoord1, 0.0, 1.0) : vec4(texcoord0, 0.0, 1.0);
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
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_color0 = a_color0;

    vec3 viewPos = mul(u_modelView, vec4(a_position, 1.0)).xyz;
    vec3 viewNormal = normalize(mul(u_modelView, vec4(a_normal, 0.0)).xyz);

    v_texcoord0 = ResolveTexcoord(0, a_texcoord0, a_texcoord1, viewPos, viewNormal);
    v_texcoord1 = ResolveTexcoord(1, a_texcoord0, a_texcoord1, viewPos, viewNormal);

    v_worldPos = mul(u_model[0], vec4(a_position, 1.0)).xyz;
    v_worldNormal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);
    v_viewDepth = -viewPos.z;

    v_fogFactor = 0.0;
    if (u_meshFogConfig.x > 0.5) {
        float dist = u_meshFogConfig.w > 0.0 ? length(viewPos) : abs(viewPos.z);
        float factor = (u_meshFogConfig.z - dist) / max(u_meshFogConfig.z - u_meshFogConfig.y, 1.0e-4);
        v_fogFactor = 1.0 - clamp(factor, 0.0, 1.0);
    }
}
