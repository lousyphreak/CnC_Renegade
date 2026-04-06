$input a_position, a_normal, a_color0, a_texcoord0
$output v_color0, v_texcoord0, v_fog

#include "vs_common.sc"

void main()
{
    vec4 object_position = vec4(a_position, 1.0);
    gl_Position = mul(u_modelViewProj, object_position);

    vec3 camera_position = transform_world_view_position(a_position);
    vec3 normal = normalize_safe(transform_world_view_normal(a_normal));

    v_color0 = compute_direct_light_color(camera_position, normal, a_color0);
    v_texcoord0 = compute_stage0_texcoord_from_camera(camera_position, normal, a_texcoord0);
    v_fog = compute_fog_from_camera_position(camera_position);
}
