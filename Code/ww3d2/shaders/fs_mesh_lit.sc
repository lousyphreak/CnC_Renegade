$input v_color0, v_texcoord0, v_texcoord1, v_fogFactor

#include <bgfx_shader.sh>
#include "mesh_common.sh"

SAMPLER2D(s_texColor0, 0);
SAMPLER2D(s_texColor1, 1);

// u_meshFragConfig.x = stage0ColorOp
// u_meshFragConfig.y = stage1ColorOp
// u_meshFragConfig.z = alphaTestRef (-1 = disabled)
// u_meshFragConfig.w = stage0AlphaOp
uniform vec4 u_meshFragConfig;

// u_meshFragConfig2.x = stage1AlphaOp
// u_meshFragConfig2.y = fogMode (0=none, 1=enable, 2=scale_fragment, 3=white)
// u_meshFragConfig2.z = unused
// u_meshFragConfig2.w = unused
uniform vec4 u_meshFragConfig2;

uniform vec4 u_meshFogColor;

void main()
{
    vec4 tex0 = texture2D(s_texColor0, v_texcoord0);
    vec4 tex1 = texture2D(s_texColor1, v_texcoord1);
    vec4 current = v_color0;

    current = vec4(
        ApplyColorOp(u_meshFragConfig.x, current, tex0),
        ApplyAlphaOp(u_meshFragConfig.w, current.a, tex0));

    current = vec4(
        ApplyColorOp(u_meshFragConfig.y, current, tex1),
        ApplyAlphaOp(u_meshFragConfig2.x, current.a, tex1));

    ApplyFog(current.rgb, u_meshFragConfig2.y, v_fogFactor, u_meshFogColor.rgb);

    if (u_meshFragConfig.z >= 0.0) {
        if (current.a < u_meshFragConfig.z) {
            discard;
        }
    }

    gl_FragColor = current;
}
