in vec3 pos;
in vec2 uv;

uniform mat4 mvp;

out vec2 UV;


void main() {
	gl_Position = mvp * vec4(pos, 1);

	UV = uv;
}


