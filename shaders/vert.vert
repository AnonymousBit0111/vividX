#version 450

layout(binding = 0) uniform UniformBufferObjects { mat4 models[440000]; } ubos;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec4 fragColor;


layout( push_constant ) uniform constants
{
	vec4 data;
	mat4 render_matrix;
} PushConstants;


void main() {

    mat4 model = ubos.models[gl_InstanceIndex];


    gl_Position = PushConstants.render_matrix * model *vec4(inPosition.x,inPosition.y, 0.0, 1.0);
    fragColor = vec4(inColor,1);
}