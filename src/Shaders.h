#ifndef SHADERS_H
#define SHADERS_H

#include "SDL_gpu.h"
#include <string>
#include <cstdint>
#include <cstdlib>

uint32_t loadShader(GPU_ShaderEnum type, const char* filename);
GPU_ShaderBlock loadShaderProgram(uint32_t* p, std::string name);

#endif