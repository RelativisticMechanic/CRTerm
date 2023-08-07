#version 330

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D tex;
uniform sampler2D crt_background;
uniform sampler2D noise_texture;

uniform float time;
uniform vec2 resolution;

// Glow effect settings
const float blurSizeDefault = 1.0/512.0;
const float intensity = 1.0;
const float speed = 30.0;
const float flicker_fraction = 0.15;

// Theme settings
uniform vec3 back_color;
const vec4 frame_color = vec4(0.20, 0.21, 0.25, 1.0);
const float background_brightness = 0.25;
const float crt_noise_fraction = 0.10;

// CRT Effect settings
uniform float warp; 
float scan = 0.75;
float scanline_speed = 0.5;
float scanline_intensity = 0.10;
float scanline_spread = 0.2;
float vigenette_intensity = 0.25;
float vignette_brightness = 50.0;

// CRT Frame Settings
float frameShadowCoeff = 15.0;
float screenShadowCoeff = 15.0;
vec2 margin = vec2(0.03, 0.03);


/* The CRT glowing text effect, downsample, then upscale to cause a glowy blur */
vec4 crtGlow(in sampler2D texture, in vec2 uv, in float blurSize)
{
    vec4 sum = vec4(0);
    vec2 texcoord = uv.xy;
    int j;
    int i;

    sum += texture(texture, vec2(texcoord.x - 4.0*blurSize, texcoord.y)) * 0.05;
    sum += texture(texture, vec2(texcoord.x - 3.0*blurSize, texcoord.y)) * 0.09;
    sum += texture(texture, vec2(texcoord.x - 2.0*blurSize, texcoord.y)) * 0.12;
    sum += texture(texture, vec2(texcoord.x - blurSize, texcoord.y)) * 0.15;
    sum += texture(texture, vec2(texcoord.x, texcoord.y)) * 0.16;
    sum += texture(texture, vec2(texcoord.x + blurSize, texcoord.y)) * 0.15;
    sum += texture(texture, vec2(texcoord.x + 2.0*blurSize, texcoord.y)) * 0.12;
    sum += texture(texture, vec2(texcoord.x + 3.0*blurSize, texcoord.y)) * 0.09;
    sum += texture(texture, vec2(texcoord.x + 4.0*blurSize, texcoord.y)) * 0.05;
        
    sum += texture(texture, vec2(texcoord.x, texcoord.y - 4.0*blurSize)) * 0.05;
    sum += texture(texture, vec2(texcoord.x, texcoord.y - 3.0*blurSize)) * 0.09;
    sum += texture(texture, vec2(texcoord.x, texcoord.y - 2.0*blurSize)) * 0.12;
    sum += texture(texture, vec2(texcoord.x, texcoord.y - blurSize)) * 0.15;
    sum += texture(texture, vec2(texcoord.x, texcoord.y)) * 0.16;
    sum += texture(texture, vec2(texcoord.x, texcoord.y + blurSize)) * 0.15;
    sum += texture(texture, vec2(texcoord.x, texcoord.y + 2.0*blurSize)) * 0.12;
    sum += texture(texture, vec2(texcoord.x, texcoord.y + 3.0*blurSize)) * 0.09;
    sum += texture(texture, vec2(texcoord.x, texcoord.y + 4.0*blurSize)) * 0.05;

    vec4 result = sum * (intensity + flicker_fraction * intensity * sin(time*speed)) + texture(texture, texcoord);
    
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

    /* Calculate normal vector to the center */
    vec2 normal = staticCoords - vec2(0.4, 0.4);
    normal = normal / length(normal);

    /* Sample glow + blur for a decent reflection of the content on frame */
    for(int i = 0; i < 20; i++)
    {
        color = mix(color, crtGlow(tex, staticCoords - (i/256.0)*normal, 1/256.0).rgb, 0.01);
    }

    return vec4(color*alpha, alpha);
}
/* End of cool-retro-term code */

void main(void)
{
    /* Turn texCoord into distorted CRT coordinates */
    vec2 uv = texCoord;
    vec2 dc = abs(0.5-uv);
    dc *= dc;
    
    uv.x -= 0.5; uv.x *= 1.0 + (dc.y * (0.3 * warp)); uv.x += 0.5;
    uv.y -= 0.5; uv.y *= 1.0 + (dc.x * (0.3 * warp)); uv.y += 0.5;

    if ((uv.y > 1.0 || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0) && warp > 0.0)
    {
        /* If we are out of bounds, draw the frame */
        fragColor = crtFrame(texCoord, uv);
    }
    else {
        float apply = abs(sin(texCoord.y)*0.5*scan);
        /* Add glow effect */
    	fragColor = vec4(mix(crtGlow(tex, uv, blurSizeDefault).rgb, vec3(0.0),apply),1.0);
        /* Add glowy noise */
        fragColor += 0.05 * vec4(back_color, 1.0) * length(crtGlow(noise_texture, uv, blurSizeDefault));
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