#include "Shaders.h"

uint32_t loadShader(GPU_ShaderEnum type, const char* filename)
{
	SDL_RWops* rwops;
	uint32_t shader;
	int file_size;

	rwops = SDL_RWFromFile(filename, "rb");
	if (rwops == NULL)
	{
		GPU_PushErrorCode("loadShader", GPU_ERROR_FILE_NOT_FOUND, "Shader file \"%s\" not found", filename);
		return 0;
	}
	file_size = SDL_RWseek(rwops, 0, SEEK_END);
	SDL_RWseek(rwops, 0, SEEK_SET);

	char* source = (char*)malloc((file_size + 1) * sizeof(char));
	SDL_RWread(rwops, source, sizeof(char), file_size);

	source[file_size] = '\0';
	shader = GPU_CompileShader(type, source);
	free(source);
	return shader;
}

GPU_ShaderBlock loadShaderProgram(uint32_t* p, std::string name)
{
	std::string vsname = name + ".vs.glsl";
	std::string fsname = name + ".fs.glsl";
	
	uint32_t v = loadShader(GPU_VERTEX_SHADER, vsname.c_str());
	uint32_t f = loadShader(GPU_PIXEL_SHADER, fsname.c_str());

	if (!v)
		GPU_LogError("Unable to load vertex shader: %s, error:\n%s\n", vsname.c_str(), GPU_GetShaderMessage());
	if (!f)
		GPU_LogError("Unable to load fragment shader: %s, error:\n%s\n", fsname.c_str(), GPU_GetShaderMessage());

	*p = GPU_LinkShaders(v, f);
	if (!(*p))
	{
		GPU_LogError("Unable to link shaders [%s] [%s], error:\n%s\n", vsname.c_str(), fsname.c_str(), GPU_GetShaderMessage());
		GPU_ShaderBlock b = { -1, -1, -1, -1 };
		return b;
	}
	GPU_ShaderBlock block = GPU_LoadShaderBlock(*p, "gpu_Vertex", "gpu_TexCoord", "gpu_Color", "gpu_ModelViewProjectionMatrix");
	GPU_ActivateShaderProgram(*p, &block);
	return block;
}