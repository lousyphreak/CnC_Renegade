$input a_position, a_normal, a_color0, a_texcoord0, a_texcoord1
$output v_color0, v_texcoord0, v_texcoord1, v_fogFactor, v_worldPos, v_viewDepth, v_worldNormal, v_shadowProj0, v_shadowProj1, v_shadowProj2

#include <bgfx_shader.sh>
#include "shadow_vs.sh"

// u_meshFogConfig.x = fogEnabled (0 or 1)
// u_meshFogConfig.y = fogStart
// u_meshFogConfig.z = fogEnd
// u_meshFogConfig.w = rangeFogSign (1.0 = range, -1.0 = planar)
uniform vec4 u_meshFogConfig;

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
    v_color0 = a_color0;
    v_texcoord0 = a_texcoord0;
    v_texcoord1 = a_texcoord1;

    v_worldPos = mul(u_model[0], vec4(a_position, 1.0)).xyz;
    v_worldNormal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);
    ComputeShadowProjections(v_worldPos, v_shadowProj0, v_shadowProj1, v_shadowProj2);
    vec3 viewPos = mul(u_modelView, vec4(a_position, 1.0)).xyz;
    v_viewDepth = -viewPos.z;

    v_fogFactor = 0.0;
    if (u_meshFogConfig.x > 0.5) {
        float dist = u_meshFogConfig.w > 0.0 ? length(viewPos) : abs(viewPos.z);
        float factor = (u_meshFogConfig.z - dist) / max(u_meshFogConfig.z - u_meshFogConfig.y, 1.0e-4);
        v_fogFactor = 1.0 - clamp(factor, 0.0, 1.0);
    }
}
