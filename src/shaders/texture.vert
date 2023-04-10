#version 330

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;

out vec2 f_texcoord;

uniform mat4 u_mvp_mat;

void main() {
    gl_Position = u_mvp_mat * vec4(in_position, 1.0);

    vec2 tmp   = in_uv;
    tmp.y      = -tmp.y;
    f_texcoord = tmp;
}
