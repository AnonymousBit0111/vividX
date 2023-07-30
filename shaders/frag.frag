#version 450



layout(location = 0) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

void main() {

    outColor = vec4(fragColor);

    // outColor *= 0.1;

    // outColor += (1/distance(gl_FragCoord.xy,ubo.pos)) *10 ;
}
