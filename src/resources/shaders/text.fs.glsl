#version 420

in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

layout(binding = 0) uniform sampler2D tex;

uniform float alpha;
uniform vec3 text_color;

void main(void)
{
    vec4 text_color = vec4(text_color, texture(tex, texCoord).a);
    /* This allows us to draw rectangles when we set alpha to a value other than 0. */
    if(alpha != 1.0)
    {
        text_color.a = alpha;
    }
    fragColor = text_color; 
}