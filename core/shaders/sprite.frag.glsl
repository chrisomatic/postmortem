#version 330 core

in vec2 tex_coord0;
out vec4 color;

uniform sampler2D image;
uniform vec3 tint_color;
uniform float opacity;
uniform float brightness;

void main() {
    vec4 tex_color = texture2D(image,tex_coord0.xy);
    if(tex_color == vec4(1.0,0.0,1.0,1.0))
        discard;
    vec4 diffuse = vec4(tint_color.xyz, opacity)*tex_color;
    color = vec4(mix(vec3(0.0,0.0,0.0),diffuse.xyz,brightness),diffuse.a);
}
