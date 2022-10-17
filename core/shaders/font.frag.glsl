#version 330 core

in vec2 tex_coord0;
out vec4 color;

uniform sampler2D image;
uniform vec4 fg_color;
uniform float px_range; // set to distance field's pixel range

float screenPxRange() {
    vec2 unitRange = vec2(px_range)/vec2(textureSize(image, 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(tex_coord0);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msd = texture(image, tex_coord0).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange()*(sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

    //color = fg_color;
    //color = vec4(msd,1.0);
    color = vec4(fg_color.rgb, opacity*fg_color.a);
    //color = vec4(1.0,0.0,0.0,1.0);
}
