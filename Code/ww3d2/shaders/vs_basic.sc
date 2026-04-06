$input a_position, a_normal, a_color0, a_texcoord0
$output v_color0, v_texcoord0, v_fog

#include <bgfx_shader.sh>

void main()
{
    vec4 object_position = vec4(a_position, 1.0);
    gl_Position = mul(u_modelViewProj, object_position);
    v_color0 = a_color0;
    v_texcoord0 = a_texcoord0;
    v_fog = 0.0;
}
