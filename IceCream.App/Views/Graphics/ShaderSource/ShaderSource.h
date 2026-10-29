#pragma once

#include <GL/glew.h>

extern const char* VertexShaderSource;
extern const char* FragmentShaderSource;
extern const char* LineVertexShaderSource;
extern const char* LineFragmentShaderSource;
extern const char* ShadowVertexShaderSource;
extern const char* ShadowFragmentShaderSource;

GLuint CreateShadowShaderProgram();
GLuint CompileShader(GLenum shaderType, const char* source);
GLuint CreateShaderProgram();
GLuint CreateLineShaderProgram();
