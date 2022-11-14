#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 tex_coord;

out vec2 tex_coord0;
out vec2 to_light_vector[16];

uniform vec2 light_pos[16];

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    tex_coord0 = tex_coord;

    vec4 world_pos = model * vec4(position.xy,0.0,1.0);
    for(int i = 0; i < 16; ++i)
    {
        to_light_vector[i] = light_pos[i] - world_pos.xy;
    }

    gl_Position = projection * view * world_pos;
}
