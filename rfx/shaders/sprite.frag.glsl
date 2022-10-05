#version 330 core

in vec2 tex_coord0;
out vec4 color;

uniform sampler2D image;
uniform vec3 tint_color;
uniform float opacity;

void main() {
    vec4 tex_color = texture2D(image,tex_coord0.xy);
    //if(tex_color == vec4(1.0,0.0,1.0,1.0))
    //    discard;
    color = vec4(tint_color.xyz, opacity)*tex_color;
}
