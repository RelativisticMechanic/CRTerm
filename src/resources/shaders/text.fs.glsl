#version 420

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

layout(binding = 0) uniform sampler2D tex;
uniform float alpha;
uniform vec3 text_color;

void main(void)
{
    fragColor = vec4(text_color, texture(tex, texCoord).a);
    
    /* This allows us to draw rectangles when we set alpha to a value other than 0. */
    if(alpha != 1.0)
    {
        fragColor.a = alpha;
    }
}