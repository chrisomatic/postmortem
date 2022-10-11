#version 330 core

in vec3 color0;
out vec4 out_color;

uniform float opacity;

void main() {
    out_color = vec4(color0.xyz, opacity);
}

