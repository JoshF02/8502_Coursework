#version 330 core
uniform sampler2D diffuseTex;

in Vertex {
	vec2 texCoord;
} IN;

out vec4 fragColour;

void main(void) {
	vec4 val = texture2D(diffuseTex, IN.texCoord);
	vec4 blank = vec4(0.0, 0.0, 0.0, 1.0);

	float brightness = dot(val.rgb, vec3(0.2126, 0.7152, 0.0722));
	
	if (brightness > 0.4)
		fragColour = vec4(val.rgb, 1.0);

	else 
		fragColour = blank;
		
}