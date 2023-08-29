#version 420
in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

layout(binding = 0) uniform sampler2D tex;
layout(binding = 1) uniform sampler2D crt_background;
layout(binding = 2) uniform sampler2D noise_texture;
layout(binding = 3) uniform sampler2D older_frame;

uniform float time;
uniform float current_frame_time;
uniform float older_frame_time;
uniform vec2 resolution;

const float text_brightness_multiplier = 3.0;

// Theme settings
uniform vec3 back_color;

// CRT Effect settings
uniform float warp; 

void main(void)
{
    fragColor = mix(texture(tex, texCoord) * text_brightness_multiplier, texture(crt_background, texCoord), 0.3);
}