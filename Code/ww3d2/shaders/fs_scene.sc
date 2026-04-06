$input v_color0, v_texcoord0, v_fog

#include "fs_common.sc"

void main()
{
    vec4 color = sample_base_color(v_texcoord0, v_color0);
    color = apply_scene_fog(color, clamp(v_fog, 0.0, 1.0));
    gl_FragColor = finalize_color(color);
}
