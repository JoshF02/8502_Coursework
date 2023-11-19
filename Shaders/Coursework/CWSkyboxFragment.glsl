#version 330 core

uniform samplerCube cubeTex;
//uniform int postEnabled;
uniform bool postEnabled;

in Vertex{
	vec3 viewDir;
} IN;

out vec4 fragColour[2];

void main(void) {
	vec4 val = texture(cubeTex, normalize(IN.viewDir));
	/*vec4 blank = vec4(0.0, 0.0, 0.0, 1.0);	// 0 ALPHA = BLANK, 1 ALPHA = BLACK?

	float brightness = dot(val.rgb, vec3(0.2126, 0.7152, 0.0722));


	switch (postEnabled) { // brightTex = fragColour[0] = frags to blur

	case 2:	// bloom - brightTex = bright frags, colourTex = all frags
		if (brightness > 0.4) {	// threshold, was 1.0
			fragColour[0] = vec4(val.rgb, 1.0);
		}
		else {
			fragColour[0] = blank;
		}
		
		fragColour[1] = val;
		break;

	case 1: // blur - brightTex = all frags, colourTex = blank
		fragColour[0] = val;
		fragColour[1] = blank;	// BLUR BROKEN SOMEHOW
		break;

	case 0: // none - both = all frags (no blurring is done, only 0 is used)
		fragColour[0] = val;
		fragColour[1] = val;
		break;
	}*/

	vec4 blank = vec4(0.0, 0.0, 0.0, 1.0);

	if (postEnabled == true) {
	//switch (postEnabled) {
	//case 2:
		float brightness = dot(val.rgb, vec3(0.2126, 0.7152, 0.0722));
		
		if (brightness > 0.4) {	
		fragColour[0] = vec4(val.rgb, 1.0);
		}
		else {
			fragColour[0] = blank;
		}
		
		fragColour[1] = val;
	}
		//break;
	else {
		fragColour[0] = val;
		fragColour[1] = val;
	}
	/*case 1:
		fragColour[0] = val;
		fragColour[1] = blank;	
		break;

	case 0: 
		fragColour[0] = val;
		fragColour[1] = val;
		break;
	}*/
}



