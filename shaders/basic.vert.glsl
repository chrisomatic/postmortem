#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 tex_coord;

out vec2 tex_coord0;

uniform mat4 model;
uniform mat4 projection;

void main()
{
    tex_coord0 = tex_coord;
    //gl_Position = projection * model * vec4(position.xy,0.0,1.0);
    gl_Position = model*vec4(position.xy,0.0,1.0);
}
