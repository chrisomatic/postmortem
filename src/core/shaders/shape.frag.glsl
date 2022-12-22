#version 330 core

out vec4 out_color;

uniform vec3 color;
uniform float opacity;

void main() {
    out_color = vec4(color.xyz, opacity);
}
