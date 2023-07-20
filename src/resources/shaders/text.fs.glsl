#version 330
in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D tex;
uniform vec3 text_color;

void main(void)
{
    fragColor = texture(tex, texCoord);
    fragColor.rgb = text_color;
}