#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out float Height;
out vec3 WorldPosition;

void main()
{
    Height = aPosition.z;
    vec4 position = uModel * vec4(aPosition, 1.0);
    WorldPosition = position.xyz;
    gl_Position = uProjection * uView * position;
}
