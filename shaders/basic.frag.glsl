#version 330 core

uniform sampler2D sampler;
in vec2 tex_coord0;

out vec4 frag_color;

void main() {
   vec4 color1 = texture2D(sampler,tex_coord0.xy);
   frag_color = color1; //vec4(1.0,0.0,0.0,1.0);
}
