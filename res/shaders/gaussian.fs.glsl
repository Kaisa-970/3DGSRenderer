#version 430 core

in vec3 outColor;
in float opacity;
in vec3 conic;
in vec2 coordxy;
in float quadId;
in float distance;
out vec4 FragColor;

void main() {			
    float power = -0.5f * (conic.x * coordxy.x * coordxy.x + conic.z * coordxy.y * coordxy.y) - conic.y * coordxy.x * coordxy.y;
    if(power > 0.0f) discard;
    float alpha = min(0.99f, opacity * exp(power));
    if(alpha < 1.f / 255.f) discard;
    FragColor = vec4(outColor, alpha);
    //FragColor = vec4(outColor, 1.0);
    float value = quadId / 559263.0;
    // value = distance;
    // value = 1.0 / (1.0 + exp(-value));
    //FragColor = vec4(value, value, value, 1.0);
}