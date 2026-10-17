#version 330 core

in vec3 WorldPosition;
in float Height;
uniform float uJoinHeight;

out vec4 FragColor;

void main()
{
    vec3 tangentX = dFdx(WorldPosition);
    vec3 tangentY = dFdy(WorldPosition);
    vec3 normal = normalize(cross(tangentX, tangentY));
    vec3 lightDirection = normalize(vec3(-3.0, -4.0, 6.0));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float brightness = 0.25 + 0.75 * diffuse;

    vec3 baseColor = (Height >= uJoinHeight)
        ? vec3(0.60, 0.85, 0.35)
        : vec3(0.76, 0.50, 0.30);

    FragColor = vec4(baseColor * brightness, 1.0);
}
