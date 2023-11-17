#version 330 core

uniform sampler2D diffuseTex;
uniform samplerCube cubeTex;

uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;

in Vertex {
	vec4 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 worldPos;
} IN;

out vec4 fragColour;

void main(void){
	// reflection calculations to give environment mapping surface
	vec4 diffuse = texture(diffuseTex, IN.texCoord);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);

	vec3 reflectDir = reflect(-viewDir, normalize(IN.normal));
	vec4 reflectTex = texture(cubeTex, reflectDir);

	vec4 colouredTint = vec4(0, 1, 0, 1) * 0.0f;	// adjustable coloured tint

	vec4 surfaceColour = reflectTex + (diffuse * 0.25f) + colouredTint;	


	// lighting calculations to give realistic lighting and shadows
	vec3 incident = normalize(lightPos - IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

	float lambert = max(dot(incident, IN.normal), 0.0f);
	float distance = length(lightPos - IN.worldPos);
	float attenuation = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);

	float specFactor = clamp(dot(halfDir, IN.normal), 0.0, 1.0);
	specFactor = pow(specFactor, 60.0);

	vec3 surface = (surfaceColour.rgb * lightColour.rgb);
	fragColour.rgb = surface * lambert * attenuation;
	fragColour.rgb += (lightColour.rgb * specFactor) * attenuation * 0.33;
	fragColour.rgb += surface * 0.1f;
	fragColour.a = surfaceColour.a * 0.4;	// set transparency here
}