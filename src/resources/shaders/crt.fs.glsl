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
const float blurSize = 1.0/512.0;
const float intensity = 1.2;
const float speed = 20.0;

// Theme settings
uniform vec3 back_color;
const float background_brightness = 0.25;
const float crt_noise_fraction = 0.1;

// CRT Effect settings
float warp = 0.85; // simulate curvature of CRT monitor
float scan = 0.75; // simulate darkness between scanlines
float scanline_speed = 0.85;
float scanline_intensity = 0.08;

vec4 crtGlow(in vec2 uv)
{
    vec4 sum = vec4(0);
    vec2 texcoord = uv.xy;
    int j;
    int i;

    //thank you! http://www.gamerendering.com/2008/10/11/gaussian-blur-filter-shader/ for the 
    // blur tutorial
    // blur in y (vertical)
    // take nine samples, with the distance blurSize between them
    sum += texture(tex, vec2(texcoord.x - 4.0*blurSize, texcoord.y)) * 0.05;
    sum += texture(tex, vec2(texcoord.x - 3.0*blurSize, texcoord.y)) * 0.09;
    sum += texture(tex, vec2(texcoord.x - 2.0*blurSize, texcoord.y)) * 0.12;
    sum += texture(tex, vec2(texcoord.x - blurSize, texcoord.y)) * 0.15;
    sum += texture(tex, vec2(texcoord.x, texcoord.y)) * 0.16;
    sum += texture(tex, vec2(texcoord.x + blurSize, texcoord.y)) * 0.15;
    sum += texture(tex, vec2(texcoord.x + 2.0*blurSize, texcoord.y)) * 0.12;
    sum += texture(tex, vec2(texcoord.x + 3.0*blurSize, texcoord.y)) * 0.09;
    sum += texture(tex, vec2(texcoord.x + 4.0*blurSize, texcoord.y)) * 0.05;
        
    // blur in y (vertical)
    // take nine samples, with the distance blurSize between them
    sum += texture(tex, vec2(texcoord.x, texcoord.y - 4.0*blurSize)) * 0.05;
    sum += texture(tex, vec2(texcoord.x, texcoord.y - 3.0*blurSize)) * 0.09;
    sum += texture(tex, vec2(texcoord.x, texcoord.y - 2.0*blurSize)) * 0.12;
    sum += texture(tex, vec2(texcoord.x, texcoord.y - blurSize)) * 0.15;
    sum += texture(tex, vec2(texcoord.x, texcoord.y)) * 0.16;
    sum += texture(tex, vec2(texcoord.x, texcoord.y + blurSize)) * 0.15;
    sum += texture(tex, vec2(texcoord.x, texcoord.y + 2.0*blurSize)) * 0.12;
    sum += texture(tex, vec2(texcoord.x, texcoord.y + 3.0*blurSize)) * 0.09;
    sum += texture(tex, vec2(texcoord.x, texcoord.y + 4.0*blurSize)) * 0.05;

    //increase blur with intensity!
    vec4 result = sum * (intensity + 0.5*sin(time*speed)) + texture(tex, texcoord);
    // Amberify
    //float av = (result.r + result.g + result.b)/3.0;
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


vec3 distort(in vec2 new)
{
    // Chromatic aberration
    vec2 chrom;
    chrom = vec2(mod(abs(sin(time)*8.0), 0.01), 0.0); // Strength of the color shift
    
    vec3 col;
    col.r = texture(tex, vec2(new.x+chrom.x, new.y)).r;
    col.g = texture(tex, vec2(new.x, new.y)).g;
    col.b = texture(tex, vec2(new.x-chrom.x, new.y)).b;
    
    col = ((new.x >= 0.0) && (new.x <= 1.0)) && ((new.y >= 0.0) && (new.y <= 1.0)) ? col : vec3(0.0);
    return col;
}

float random (vec2 st) {
    return fract(sin(dot(st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

float blend(const in float x, const in float y) {
	return (x < 0.5) ? (2.0 * x * y) : (1.0 - 2.0 * (1.0 - x) * (1.0 - y));
}

vec3 blend(const in vec3 x, const in vec3 y, const in float opacity) {
	vec3 z = vec3(blend(x.r, y.r), blend(x.g, y.g), blend(x.b, y.b));
	return z * opacity + x * (1.0 - opacity);
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
        // Add scanline going up and down
        fragColor.rgb += scanline_intensity*(exp(-5.0*abs(sin(abs((1-uv.y) - abs(cos(scanline_speed*time)*cos(scanline_speed*time)))))))*back_color;
        // Add noise
        fragColor.rgb = mix(fragColor.rgb, vec3(crtNoise(uv, time)), crt_noise_fraction);
    }
}