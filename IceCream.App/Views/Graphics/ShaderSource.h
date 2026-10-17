#pragma once

#include <GL/glew.h>

extern const char* VertexShaderSource;
extern const char* FragmentShaderSource;

GLuint CompileShader(GLenum shaderType, const char* source);
GLuint CreateShaderProgram();