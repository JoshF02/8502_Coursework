#version 330 core

uniform sampler2D diffuseTex;
uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;
//uniform int postEnabled;
uniform bool postEnabled;

in Vertex {
	vec3 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 worldPos;
} IN;

out vec4 fragColour[2];

void main(void){
	vec3 incident = normalize(lightPos - IN.worldPos);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

	vec4 diffuse = texture(diffuseTex, IN.texCoord);

	float lambert = max(dot(incident, IN.normal), 0.0f);
	float distance = length(lightPos - IN.worldPos);
	float attenuation = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);

	float specFactor = clamp(dot(halfDir, IN.normal), 0.0, 1.0);
	specFactor = pow(specFactor, 60.0);

	vec3 surface = (diffuse.rgb * lightColour.rgb);

	vec4 val = vec4(0.0, 0.0, 0.0, 1.0);
	val.rgb = surface * lambert * attenuation;
	val.rgb += (lightColour.rgb * specFactor) * attenuation * 0.33;
	val.rgb += surface * 0.1f;
	val.a = diffuse.a;

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