$input a_position, a_normal, a_color0, a_color1, a_texcoord0, a_texcoord1
$output v_color0, v_texcoord0, v_texcoord1, v_fogFactor

#include <bgfx_shader.sh>

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

    v_fogFactor = 0.0;
    if (u_meshFogConfig.x > 0.5) {
        vec3 viewPos = mul(u_modelView, vec4(a_position, 1.0)).xyz;
        float dist = u_meshFogConfig.w > 0.0 ? length(viewPos) : abs(viewPos.z);
        float factor = (u_meshFogConfig.z - dist) / max(u_meshFogConfig.z - u_meshFogConfig.y, 1.0e-4);
        v_fogFactor = 1.0 - clamp(factor, 0.0, 1.0);
    }
}
