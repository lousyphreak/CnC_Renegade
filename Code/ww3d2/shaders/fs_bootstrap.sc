$input v_color0, v_texcoord0, v_fog

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor, 0);
uniform vec4 u_fogState;
uniform vec4 u_fogColor;

void main()
{
    vec4 color = texture2D(s_texColor, v_texcoord0) * v_color0;
    float fog = clamp(v_fog, 0.0, 1.0);
    if (u_fogState.x > 0.5)
    {
        if (u_fogState.x < 1.5)
        {
            color.rgb = mix(color.rgb, u_fogColor.rgb, fog);
        }
        else if (u_fogState.x < 2.5)
        {
            color *= (1.0 - fog);
        }
        else
        {
            color.rgb = mix(color.rgb, vec3(1.0, 1.0, 1.0), fog);
        }
    }
    if (color.a < u_alphaRef)
    {
        discard;
    }

    gl_FragColor = color;
}
