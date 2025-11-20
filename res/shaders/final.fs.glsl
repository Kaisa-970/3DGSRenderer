#version 430 core

in vec2 texCoord;
uniform sampler2D u_colorTexture;

out vec4 finalColor;

void main()
{
    finalColor = texture(u_colorTexture, texCoord);
    // float depth = finalColor.r;
    // depth = linearizeDepth(depth, 0.1, 100.0);
    // finalColor = vec4(vec3(depth) / 100.0, 1.0);
}