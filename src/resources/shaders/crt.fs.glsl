#version 420

#define PI  3.14159265358
#define TAU 6.28318530718
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

// Glow effect settings
const float glow_size = 1.0 / 512.0;
const float intensity = 1.5;
const float flicker_fraction = 0.15;
const float flicker_speed = 20.0;

// Theme settings
uniform vec3 back_color;
const vec4 frame_color = vec4(0.4, 0.4, 0.4, 1.0);
const float background_brightness = 0.25;
const float crt_noise_fraction = 0.10;

// CRT Effect settings
uniform float warp; 
float scan = 0.75;
float scanline_opacity = 0.1;
float scanline_density = 0.3;
float scanline_speed = 0.5;
float scanline_intensity = 0.15;
float scanline_spread = 0.2;
float vigenette_intensity = 0.25;
float vignette_brightness = 50.0;

// CRT Frame Settings
float frameShadowCoeff = 15.0;
float screenShadowCoeff = 15.0;
vec2 margin = vec2(0.03, 0.03);

/* Sample from the old frame and current framge and interpolate */
vec4 burnInEffect(in vec2 uv, in sampler2D prev_frame, in sampler2D curr_frame)
{
    vec4 old_color = texture(prev_frame, uv);
    vec4 new_color = texture(curr_frame, uv);
    highp float decay = clamp((1.0/(current_frame_time - older_frame_time)) * 120.0, 0.0, 1.0);
    old_color *= decay;
    return max(old_color, new_color);
}

/* The CRT glowing text effect, downsample, then upscale to cause a glowy blur */
vec4 crtGlow(in sampler2D crt_texture, in vec2 uv, in float blurSize)
{
    vec4 sum = vec4(0.0);
    sum += texture(crt_texture, vec2(uv.x - 4.0*blurSize, uv.y)) * 0.05;
    sum += texture(crt_texture, vec2(uv.x - 3.0*blurSize, uv.y)) * 0.09;
    sum += texture(crt_texture, vec2(uv.x - 2.0*blurSize, uv.y)) * 0.12;
    sum += texture(crt_texture, vec2(uv.x - blurSize, uv.y)) * 0.15;
    sum += texture(crt_texture, vec2(uv.x, uv.y)) * 0.16;
    sum += texture(crt_texture, vec2(uv.x + blurSize, uv.y)) * 0.15;
    sum += texture(crt_texture, vec2(uv.x + 2.0*blurSize, uv.y)) * 0.12;
    sum += texture(crt_texture, vec2(uv.x + 3.0*blurSize, uv.y)) * 0.09;
    sum += texture(crt_texture, vec2(uv.x + 4.0*blurSize, uv.y)) * 0.05;
        
        // blur in y (vertical)
    // take nine samples, with the distance blurSize between them
    sum += texture(crt_texture, vec2(uv.x, uv.y - 4.0*blurSize)) * 0.05;
    sum += texture(crt_texture, vec2(uv.x, uv.y - 3.0*blurSize)) * 0.09;
    sum += texture(crt_texture, vec2(uv.x, uv.y - 2.0*blurSize)) * 0.12;
    sum += texture(crt_texture, vec2(uv.x, uv.y - blurSize)) * 0.15;
    sum += texture(crt_texture, vec2(uv.x, uv.y)) * 0.16;
    sum += texture(crt_texture, vec2(uv.x, uv.y + blurSize)) * 0.15;
    sum += texture(crt_texture, vec2(uv.x, uv.y + 2.0*blurSize)) * 0.12;
    sum += texture(crt_texture, vec2(uv.x, uv.y + 3.0*blurSize)) * 0.09;
    sum += texture(crt_texture, vec2(uv.x, uv.y + 4.0*blurSize)) * 0.05;

    vec4 result = sum * (intensity + flicker_fraction * intensity * sin(time*flicker_speed)) + text_brightness_multiplier * burnInEffect(uv, older_frame, crt_texture);
    
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

vec3 vigenette(in vec2 uv, in vec3 oricol)
{
    float vig = (uv.x*uv.y - uv.x*uv.x*uv.y - uv.x*uv.y*uv.y + uv.x*uv.x*uv.y*uv.y) * vignette_brightness;
    vig = pow(vig, vigenette_intensity);
    return vig * oricol;
}

/* 
    The following code was borrowed from Cool-Retro-Term.
    It creates a nice frame around the terminal screen.
*/
float rand(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float max2(vec2 v)
{
    return max(v.x, v.y);
}

float min2(vec2 v)
{
    return min(v.x, v.y);
}

float prod2(vec2 v)
{
    return v.x * v.y;
}

float sum2(vec2 v)
{
    return v.x + v.y;
}

vec2 positiveLog(vec2 x) 
{
    return clamp(log(x), vec2(0.0), vec2(100.0));
}

vec4 crtFrame(in vec2 staticCoords, in vec2 uv)
{
    vec2 coords = uv * (vec2(1.0) + margin * 2.0) - margin;

    vec2 vignetteCoords = staticCoords * (1.0 - staticCoords.yx);
    float vignette = pow(prod2(vignetteCoords) * 15.0, 0.25);

    vec3 color = frame_color.rgb * vec3(1.0 - vignette);
    float alpha = 0.0;

    float frameShadow = max2(positiveLog(-coords * frameShadowCoeff + vec2(1.0)) + positiveLog(coords * frameShadowCoeff - (vec2(frameShadowCoeff) - vec2(1.0))));
    frameShadow = max(sqrt(frameShadow), 0.0);
    color *= frameShadow;
    alpha = sum2(1.0 - step(vec2(0.0), coords) + step(vec2(1.0), coords));
    alpha = clamp(alpha, 0.0, 1.0);
    alpha *= mix(1.0, 0.9, frameShadow);

    float screenShadow = 1.0 - prod2(positiveLog(coords * screenShadowCoeff + vec2(1.0)) * positiveLog(-coords * screenShadowCoeff + vec2(screenShadowCoeff + 1.0)));
    alpha = max(0.8 * screenShadow, alpha);

    vec4 final_color = vec4(color*alpha, alpha);
    return final_color;
}
/* End of cool-retro-term code */

vec2 BrownConradyDistortion(in vec2 uv)
{
    // positive values of K1 give barrel distortion
    float k1 = warp / 100.0;
    float k2 = warp / 100.0;
    uv = uv * 2.0 - 1.0; // brown conrady takes [-1:1]
    float r2 = uv.x * uv.x + uv.y * uv.y;
    uv *= 1.0 + k1 * r2 + k2 * r2 * r2;
    uv = uv * 0.5 + 0.5; // [0:1]
    // using the distortion param as a scale factor, to keep the image close to the viewport dims
    float scale = abs(k1) < 1.0 ? 1.0 - abs(k1) : 1.0 / (k1 + 1.0);		
    uv = uv * scale - (scale * 0.5) + 0.5; // scale from center
    return uv;
}

void main(void)
{
    /* Turn texCoord into distorted CRT coordinates */
    vec2 uv = BrownConradyDistortion(texCoord);

    if ((uv.y > 1.0 || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0) && warp > 0.0)
    {
        /* If we are out of bounds, draw the frame */
        fragColor = crtFrame(texCoord, uv);
    }
    else {
        float apply = abs(sin(texCoord.y)*0.5*scan);
        /* Add glow effect */
    	fragColor = vec4(mix(crtGlow(tex, uv, glow_size).rgb, back_color,apply),1.0);
        /* Add scanline */
        fragColor.rgb += fract(smoothstep(-1.0, 0.0, uv.y - 1.0 * fract(time * 0.1976))) * scanline_intensity * back_color;
        /* Mix with background image */
        fragColor = vec4(mix(fragColor.rgb, (texture(crt_background, uv).rgb), background_brightness),1.0);
        /* Add noise */
        fragColor.rgb = mix(fragColor.rgb, vec3(crtNoise(uv, time)), crt_noise_fraction);
        /* Add vigenette */
        fragColor.rgb = vigenette(uv, fragColor.rgb);
    }
}