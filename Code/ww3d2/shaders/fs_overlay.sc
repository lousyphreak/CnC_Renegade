$input v_color0, v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor0, 0);

uniform vec4 u_overlayConfig;

void main()
{
    vec4 texel = texture2D(s_texColor0, v_texcoord0);

    // u_overlayConfig.x: 1.0 = use texture, 0.0 = no texture (vertex color only)
    // u_overlayConfig.y: 1.0 = use texture alpha as a vertex-color mask
    vec4 color = u_overlayConfig.x > 0.5
        ? (u_overlayConfig.y > 0.5
            ? vec4(v_color0.rgb, v_color0.a * texel.a)
            : vec4(v_color0.rgb * texel.rgb, v_color0.a * texel.a))
        : v_color0;

    gl_FragColor = color;
}
