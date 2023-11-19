#version 330 core

uniform samplerCube cubeTex;
uniform bool postEnabled;
in Vertex{
	vec3 viewDir;
} IN;

out vec4 fragColour[2];

void main(void) {
	vec4 val = texture(cubeTex, normalize(IN.viewDir));

	float brightness = dot(val.rgb, vec3(0.2126, 0.7152, 0.0722));



	// if pp on, brightTex = bright frags. If not, brightTex = blank

	if (postEnabled == true) {
		if (brightness > 0.8) {	// threshold, was 1.0
			fragColour[0] = vec4(val.rgb, 1.0);
			//fragColour[0] = vec4(1.0, 0.0, 0.0, 1.0);
		}
		else {
			fragColour[0] = vec4(0.0, 0.0, 0.0, 1.0);
		}
		
		fragColour[1] = val;
	}
	else {
		fragColour[0] = texture(cubeTex, normalize(IN.viewDir));
		fragColour[1] = vec4(0.0, 0.0, 0.0, 1.0);
	}
}



// NOT BEING USED