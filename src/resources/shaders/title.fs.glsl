#version 330
in vec4 color;
in vec2 texCoord;
out vec4 fragColor;

uniform sampler2D tex;
uniform float hover;
float d = 0.01;
vec4 hover_color = vec4(1.0, 0.0, 0.0, 1.0);

void main(void)
{
	vec2 uv = texCoord;
	if((uv.y - d/uv.x < 0) || ((uv.y) - d/(1 - uv.x) < 0))
	{
		fragColor = vec4(0.0, 1.0, 0.0, 1.0);
	}
	else
	{
		fragColor = texture2D(tex, texCoord);
		fragColor.rgb = mix(fragColor.rgb, hover_color.rgb, 0.5*hover);
	}
}