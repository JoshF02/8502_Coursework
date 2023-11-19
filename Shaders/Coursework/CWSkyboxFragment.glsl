#version 330 core

uniform samplerCube cubeTex;
uniform bool postEnabled;
in Vertex{
	vec3 viewDir;
} IN;

out vec4 fragColour[2];

void main(void) {
	fragColour[1] = texture(cubeTex, normalize(IN.viewDir));

	float brightness = dot(fragColour[1].rgb, vec3(0.2126, 0.7152, 0.0722));



	// if pp on, brightTex = bright frags. If not, brightTex = blank

	if (postEnabled == true)
		if (brightness > 0.4)	// threshold, was 1.0
			fragColour[0] = vec4(fragColour[1].rgb, 1.0);
			//fragColour[0] = vec4(1.0, 0.0, 0.0, 1.0);
		else
			fragColour[0] = vec4(0.0, 0.0, 0.0, 1.0);
	
	else
		fragColour[0] = texture(cubeTex, normalize(IN.viewDir));
}