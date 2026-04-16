$input a_position, a_color0, a_texcoord0, a_texcoord1, a_texcoord2
$output v_color0, v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>
#include "skin_common.sh"

void main()
{
    vec3 skinnedPosition = ApplyRigidSkinningPosition(a_position, a_texcoord2);
    gl_Position = mul(u_modelViewProj, vec4(skinnedPosition, 1.0));
    v_color0 = a_color0;
    v_texcoord0 = a_texcoord0;
    v_texcoord1 = a_texcoord1;
}
