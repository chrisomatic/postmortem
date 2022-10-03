#version 330 core

in vec2 tex_coord0;
out vec4 color;

uniform sampler2D image;
uniform vec3 tint_color;
uniform float opacity;

void main() {
   color = vec4(tint_color.xyz, opacity)*texture2D(image,tex_coord0.xy);
   //color = vec4(1.0,0.0,0.0,1.0);
}
