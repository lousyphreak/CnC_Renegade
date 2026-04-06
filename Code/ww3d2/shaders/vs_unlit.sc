$input a_position, a_normal, a_color0, a_texcoord0
$output v_color0, v_texcoord0, v_fog

#include "vs_common.sc"

void main()
{
    vec4 object_position = vec4(a_position, 1.0);
    gl_Position = mul(u_modelViewProj, object_position);

    vec3 camera_position = vec3(0.0, 0.0, 0.0);
    vec3 normal = vec3(0.0, 0.0, 1.0);
    if (u_fogState.x > 0.5 || u_texGenState.x > 0.5)
    {
        camera_position = transform_world_view_position(a_position);
        normal = normalize_safe(transform_world_view_normal(a_normal));
    }

    v_color0 = resolve_material_color(a_color0);
    v_texcoord0 = (u_texGenState.x > 0.5 || u_texGenState.y > 0.5)
        ? compute_stage0_texcoord_from_camera(camera_position, normal, a_texcoord0)
        : a_texcoord0;
    v_fog = u_fogState.x > 0.5 ? compute_fog_from_camera_position(camera_position) : 0.0;
}
