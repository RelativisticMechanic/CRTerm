/* 
	Simple shader code for SDL-GPU. Handles most of shader initialization. 
	CRTerm expects that the path to shader be something like "shaders/myshader",
	it then looks for two files: "shaders/myshader.vs.glsl" and "shaders/myshaders.fs.gl"
	representing the vertex and fragment shader.
*/

#ifndef SHADERS_H
#define SHADERS_H

#include "SDL_gpu.h"
#include <string>
#include <cstdint>
#include <cstdlib>

uint32_t loadShader(GPU_ShaderEnum type, const char* filename);
GPU_ShaderBlock loadShaderProgram(uint32_t* p, std::string name);

#endif