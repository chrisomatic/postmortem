#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 tex_coord;
layout (location = 2) in mat4 model;
layout (location = 6) in vec4 sprite_rect;
layout (location = 7) in vec3 color;
layout (location = 8) in float opacity;

out vec2 tex_coord0;
out vec3 color0;
out float opacity;

out vec2 to_light_vector[16];

uniform vec2 light_pos[16];
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
