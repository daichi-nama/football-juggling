#version 330

in vec3 f_position_camera_space;
in vec3 f_normal_camera_space;
in vec3 f_light_pos_camera_space;
in vec3 f_diffuse;

out vec4 out_color;

uniform vec3 u_specColor;
uniform vec3 u_ambiColor;
uniform float u_shininess;

void main() {
    vec3 V = normalize(-f_position_camera_space);
    vec3 N = normalize(f_normal_camera_space);
    vec3 L = normalize(f_light_pos_camera_space - f_position_camera_space);
    vec3 H = normalize(V + L);

    float ndotl   = max(0.0, dot(N, L));
    float ndoth   = max(0.0, dot(N, H));
    vec3 diffuse  = f_diffuse * ndotl;
    vec3 specular = u_specColor * pow(ndoth, u_shininess);
    vec3 ambient  = u_ambiColor;

    out_color = vec4(diffuse + specular + ambient, 1.0);
}
