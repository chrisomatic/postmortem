#version 400 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 tex_coord;
layout (location = 2) in mat4 model;
layout (location = 6) in vec4 sprite_rect;
layout (location = 7) in vec3 color;
layout (location = 8) in float opacity;
layout (location = 9) in uint tex_unit;
layout (location = 10) in uint ignore_light;
layout (location = 11) in uint blending_mode;

out vec2 tex_coord0;
out vec3 color0;
out float opacity0;
flat out uint image_index0;
flat out uint ignore_light0;
flat out uint blending_mode0;

out vec2 to_light_vector[16];

uniform vec2 light_pos[16];
uniform mat4 view;
uniform mat4 projection;

void main()
{
    tex_coord0.x = sprite_rect.x + sprite_rect.z*tex_coord.x;
    tex_coord0.y = sprite_rect.y + sprite_rect.w*tex_coord.y;

    color0 = color;
    opacity0 = opacity;
    image_index0 = tex_unit;
    ignore_light0 = ignore_light;
    blending_mode0 = blending_mode;

    //if(tex_unit == 0)
    //{
    //    color0 = vec3(1.0,0.0,0.0);
    //}

    vec4 world_pos = model * vec4(position.xy,0.0,1.0);
    for(int i = 0; i < 16; ++i)
    {
        to_light_vector[i] = light_pos[i] - world_pos.xy;
    }

    gl_Position = projection * view * world_pos;
}
