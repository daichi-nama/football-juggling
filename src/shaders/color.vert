#version 330

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;

out vec3 f_color;

uniform mat4 u_mvp_mat;

void main() {
    gl_Position = u_mvp_mat * vec4(in_position, 1.0);

    f_color = in_color;
}
