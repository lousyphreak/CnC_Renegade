$input v_color0, v_texcoord0, v_texcoord1

#include <bgfx_shader.sh>
#include "mesh_common.sh"

SAMPLER2D(s_texColor0, 0);
SAMPLER2D(s_texColor1, 1);

uniform vec4 u_meshFragConfig;
uniform vec4 u_meshFragConfig2;

void main()
{
    float current_alpha = v_color0.a;
    vec4 tex0 = texture2D(s_texColor0, v_texcoord0);
    vec4 tex1 = texture2D(s_texColor1, v_texcoord1);

    current_alpha = ApplyAlphaOp(u_meshFragConfig.w, current_alpha, tex0);
    current_alpha = ApplyAlphaOp(u_meshFragConfig2.x, current_alpha, tex1);

    if (u_meshFragConfig.z >= 0.0 && current_alpha < u_meshFragConfig.z) {
        discard;
    }

    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
