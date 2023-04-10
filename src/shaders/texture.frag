#version 330

in vec2 f_texcoord;

out vec4 out_color;

uniform sampler2D u_texture;

void main() {
    out_color = texture(u_texture, f_texcoord);
}