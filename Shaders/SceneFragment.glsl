#version 330 core

uniform sampler2D diffuseTex;
uniform int useTexture;
uniform bool postEnabled;

in Vertex{
	vec2 texCoord;
	vec4 colour;
} IN;

out vec4 fragColour[2];

void main(void){
	vec4 val = IN.colour;
	if(useTexture > 0) {
		val *= texture(diffuseTex, IN.texCoord);
	}

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