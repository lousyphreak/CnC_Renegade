$input v_color0, v_texcoord0

#include <bgfx_shader.sh>

SAMPLER2D(s_texColor0, 0);
SAMPLER2D(s_texColor1, 1);

uniform vec4 u_movieYuvConfig;

void main()
{
    vec2 chroma = texture2D(s_texColor1, v_texcoord0).rg - vec2(0.5, 0.5);
    float luma = texture2D(s_texColor0, v_texcoord0).r;

    vec3 rgb;
    if (u_movieYuvConfig.x > 0.5) {
        rgb.r = luma + 1.40234375 * chroma.y;
        rgb.g = luma - 0.34375 * chroma.x - 0.71484375 * chroma.y;
        rgb.b = luma + 1.7734375 * chroma.x;
    } else {
        float scaled_luma = max(luma - (16.0 / 255.0), 0.0) * 1.16438356;
        rgb.r = scaled_luma + 1.59602715 * chroma.y;
        rgb.g = scaled_luma - 0.3917616 * chroma.x - 0.81296802 * chroma.y;
        rgb.b = scaled_luma + 2.01723214 * chroma.x;
    }

    gl_FragColor = vec4(v_color0.rgb * saturate(rgb), v_color0.a);
}
