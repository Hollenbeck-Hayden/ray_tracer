in vec2 UV;

uniform sampler2D mySampler;

out vec4 color;


void main() {
	color = texture(mySampler, UV);
}
