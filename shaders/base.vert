#version 460 core


layout (location = 0) in vec3 a_Pos;


out vec2 v_Pos;

void main() {
	v_Pos = a_Pos.xy;
	gl_Position = vec4(a_Pos, 1.0);
}