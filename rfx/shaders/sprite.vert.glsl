#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 tex_coord;

out vec2 tex_coord0;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform int num_sprites_in_row;
uniform int sprite_index;

void main()
{
    vec2 tex = tex_coord;

    int row = sprite_index / num_sprites_in_row;
    int col = sprite_index % num_sprites_in_row;

    tex.x = (tex.x + col) / num_sprites_in_row;
    tex.y = (tex.y + row  + (num_sprites_in_row -1)) / num_sprites_in_row;
    
    tex_coord0 = tex;
    gl_Position = projection * view * model * vec4(position.xy,0.0,1.0);
}
