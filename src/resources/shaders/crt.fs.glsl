#version 330

#define GOLDEN_RATIO (1.61803398874989484820)
#define GOLDEN_RATIO_MIN_ONE (GOLDEN_RATIO - 1.0)
#define LINE (0.01666f)
#define SCANLINE_SPEED 1.0

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D tex;
uniform sampler2D crt_background;
uniform float time;
uniform vec2 resolution;

// Glow effect settings
const float blurSize = 1.5/512.0;
const float intensity = 1.4;
const float speed = 10.0;

// Theme settings
uniform vec3 back_color;
const float background_brightness = 0.25;
const float crt_noise_fraction = 0.1;

// CRT Effect settings
uniform float warp; 
float scan = 0.75;
float scanline_speed = 0.50;
float scanline_intensity = 0.15;
float scanline_spread = 0.2;

/* The CRT glowing text effect, downsample, then upscale to cause a glowy blur */
vec4 crtGlow(in vec2 uv)
{
    vec4 sum = vec4(0);
    vec2 texcoord = uv.xy;
    int j;
    int i;

    sum += texture(tex, vec2(texcoord.x - 4.0*blurSize, texcoord.y)) * 0.05;
    sum += texture(tex, vec2(texcoord.x - 3.0*blurSize, texcoord.y)) * 0.09;
    sum += texture(tex, vec2(texcoord.x - 2.0*blurSize, texcoord.y)) * 0.12;
    sum += texture(tex, vec2(texcoord.x - blurSize, texcoord.y)) * 0.15;
    sum += texture(tex, vec2(texcoord.x, texcoord.y)) * 0.16;
    sum += texture(tex, vec2(texcoord.x + blurSize, texcoord.y)) * 0.15;
    sum += texture(tex, vec2(texcoord.x + 2.0*blurSize, texcoord.y)) * 0.12;
    sum += texture(tex, vec2(texcoord.x + 3.0*blurSize, texcoord.y)) * 0.09;
    sum += texture(tex, vec2(texcoord.x + 4.0*blurSize, texcoord.y)) * 0.05;
        
    sum += texture(tex, vec2(texcoord.x, texcoord.y - 4.0*blurSize)) * 0.05;
    sum += texture(tex, vec2(texcoord.x, texcoord.y - 3.0*blurSize)) * 0.09;
    sum += texture(tex, vec2(texcoord.x, texcoord.y - 2.0*blurSize)) * 0.12;
    sum += texture(tex, vec2(texcoord.x, texcoord.y - blurSize)) * 0.15;
    sum += texture(tex, vec2(texcoord.x, texcoord.y)) * 0.16;
    sum += texture(tex, vec2(texcoord.x, texcoord.y + blurSize)) * 0.15;
    sum += texture(tex, vec2(texcoord.x, texcoord.y + 2.0*blurSize)) * 0.12;
    sum += texture(tex, vec2(texcoord.x, texcoord.y + 3.0*blurSize)) * 0.09;
    sum += texture(tex, vec2(texcoord.x, texcoord.y + 4.0*blurSize)) * 0.05;

    vec4 result = sum * (intensity + 0.5*sin(time*speed)) + texture(tex, texcoord);
    
    return result;
}

float crtNoise(vec2 pos, float evolve) {
    
    // Loop the evolution (over a very long period of time).
    float e = fract((evolve*0.01));
    
    // Coordinates
    float cx  = pos.x*e;
    float cy  = pos.y*e;
    
    // Generate a "random" black or white value
    return fract(23.0*fract(2.0/fract(fract(cx*2.4/cy*23.0+pow(abs(cy/22.4),3.3))*fract(cx*evolve/pow(abs(cy),0.050)))));
}

void main(void)
{
    // squared distance from center
    vec2 uv = texCoord;
    vec2 dc = abs(0.5-uv);
    dc *= dc;
    
    // warp the fragment coordinates
    uv.x -= 0.5; uv.x *= 1.0 + (dc.y * (0.3 * warp)); uv.x += 0.5;
    uv.y -= 0.5; uv.y *= 1.0 + (dc.x * (0.3 * warp)); uv.y += 0.5;

    // sample inside boundaries, otherwise set to transparent
    if (uv.y > 1.0 || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0)
    {
        fragColor = vec4(0.0,0.0,0.0,1.0);
    }
    else {
        float apply = abs(sin(texCoord.y)*0.5*scan);
    	fragColor = vec4(mix(crtGlow(uv).rgb, vec3(0.0),apply),1.0);
        fragColor = vec4(mix(fragColor.rgb, length(texture(crt_background, uv).rgb)*back_color, background_brightness),1.0);
        // Add a scanline going up and down
        fragColor.rgb += scanline_intensity * exp(-1.0*abs((1/scanline_spread) * sin(abs(uv.y - abs(cos(scanline_speed*time)))))) * back_color;
        // Add noise
        fragColor.rgb = mix(fragColor.rgb, vec3(crtNoise(uv, time)), crt_noise_fraction);
    }
}