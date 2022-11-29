#version 330 core

in vec2 tex_coord0;
in vec3 color0;
in float opacity0;
in vec2 to_light_vector[16];

out vec4 color;

uniform sampler2D image;
uniform vec3 ambient_color;
uniform vec3 light_color[16];
uniform vec3 light_atten[16];
uniform int  is_particle;

void main() {

    // texture color
    vec4 tex_color = texture2D(image,tex_coord0.xy);
    if(tex_color == vec4(1.0,0.0,1.0,1.0))
        discard;

    vec3 total_diffuse = vec3(0.0);

    // point lights
    for(int i = 0; i < 16; ++i)
    {
        float dist_to_light = length(to_light_vector[i]);
        float atten_factor = light_atten[i].x + light_atten[i].y*dist_to_light + light_atten[i].z*dist_to_light*dist_to_light;

        total_diffuse = total_diffuse + (light_color[i] / atten_factor);
    }

    total_diffuse = min(total_diffuse,vec3(1.0,1.0,1.0)); // cap the total diffuse
    total_diffuse = max(total_diffuse, ambient_color);

    if(is_particle == 1)
    {
        color = vec4(tex_color.rgb*color0,tex_color.a*opacity0);
    }
    else
    {
        color = vec4(total_diffuse, opacity0)*tex_color;
    }
}
