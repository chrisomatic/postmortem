#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec3 color;

out vec3 color0;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    color0 = color;
    gl_Position = projection * view*vec4(position.xy,0.0,1.0);
}
