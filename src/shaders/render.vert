#version 330

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_diffuse;

out vec3 f_position_camera_space;
out vec3 f_normal_camera_space;
out vec3 f_light_pos_camera_space;
out vec3 f_diffuse;

uniform vec3 u_light_pos;
uniform mat4 u_mv_mat;
uniform mat4 u_mvp_mat;
uniform mat4 u_norm_mat;
uniform mat4 u_light_mat;

void main() {
    gl_Position = u_mvp_mat * vec4(in_position, 1.0);

    f_position_camera_space  = (u_mv_mat * vec4(in_position, 1.0)).xyz;
    f_normal_camera_space    = (u_norm_mat * vec4(in_normal, 0.0)).xyz;
    f_light_pos_camera_space = (u_light_mat * vec4(u_light_pos, 1.0)).xyz;

    f_diffuse = in_diffuse;
}
