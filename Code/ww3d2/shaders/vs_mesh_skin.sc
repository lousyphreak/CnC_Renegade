$input a_position, a_normal, a_color0, a_texcoord0, a_texcoord1, a_texcoord2
$output v_color0, v_texcoord0, v_texcoord1, v_fogFactor, v_worldPos, v_viewDepth, v_worldNormal

#include <bgfx_shader.sh>
#include "skin_common.sh"

uniform vec4 u_meshFogConfig;

void main()
{
    vec3 skinnedPosition;
    vec3 skinnedNormal;
    ApplyRigidSkinning(a_position, a_normal, a_texcoord2, skinnedPosition, skinnedNormal);

    vec4 skinnedPosition4 = vec4(skinnedPosition, 1.0);
    gl_Position = mul(u_modelViewProj, skinnedPosition4);
    v_color0 = a_color0;
    v_texcoord0 = a_texcoord0;
    v_texcoord1 = a_texcoord1;

    v_worldPos = mul(u_model[0], skinnedPosition4).xyz;
    v_worldNormal = normalize(mul(u_model[0], vec4(skinnedNormal, 0.0)).xyz);
    vec3 viewPos = mul(u_modelView, skinnedPosition4).xyz;
    v_viewDepth = -viewPos.z;

    v_fogFactor = 0.0;
    if (u_meshFogConfig.x > 0.5) {
        float dist = u_meshFogConfig.w > 0.0 ? length(viewPos) : abs(viewPos.z);
        float factor = (u_meshFogConfig.z - dist) / max(u_meshFogConfig.z - u_meshFogConfig.y, 1.0e-4);
        v_fogFactor = 1.0 - clamp(factor, 0.0, 1.0);
    }
}
