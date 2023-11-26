#version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D bumpTex;
uniform sampler2D shadowTex;
uniform vec3 cameraPos;

uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;

uniform vec4 light2Colour;
uniform vec3 light2Pos;
uniform float light2Radius;

uniform vec4 light3Colour;
uniform vec3 light3Pos;
uniform float light3Radius;

in Vertex{
	vec3 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
	vec4 shadowProj;
} IN;

out vec4 fragColour;

void main(void){
	vec3 incident = normalize(lightPos - IN.worldPos);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

	mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));

	vec4 diffuse = texture(diffuseTex, IN.texCoord);
	vec3 normal = texture(bumpTex, IN.texCoord).rgb;
	normal = normalize(TBN * normalize(normal * 2.0 - 1.0));

	float lambert = max(dot(incident, normal), 0.0f);
	float distance = length(lightPos - IN.worldPos);
	float attenuation = 1.0f - clamp(distance / lightRadius, 0.0, 1.0);

	float specFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
	specFactor = pow(specFactor, 60.0);

	float shadow = 1.0;

	vec3 shadowNDC = IN.shadowProj.xyz / IN.shadowProj.w;
	if(abs(shadowNDC.x) < 1.0f && abs(shadowNDC.y) < 1.0f && abs(shadowNDC.z) < 1.0f){
		vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		float shadowZ = texture(shadowTex, biasCoord.xy).x;
		if(shadowZ < biasCoord.z){
			shadow = 0.0f;
		}
	}

	vec3 surface = (diffuse.rgb * lightColour.rgb);
	fragColour.rgb = surface * lambert * attenuation;
	fragColour.rgb += (lightColour.rgb * specFactor) * attenuation * 0.33;
	fragColour.rgb *= shadow;
	fragColour.rgb += surface * 0.1f;
	fragColour.a = diffuse.a;






	
	
	// add effects of other 2 lights in small radius around them, using additive blending
	vec4 temp;

	distance = length(light2Pos - IN.worldPos);
	if (distance < 2000.0f) {
		incident = normalize(light2Pos - IN.worldPos);
		halfDir = normalize(incident + viewDir);

		lambert = max(dot(incident, normal), 0.0f);

		attenuation = 1.0f - clamp(distance / light2Radius, 0.0, 1.0);

		specFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
		specFactor = pow(specFactor, 60.0);

		shadow = 1.0;

		shadowNDC = IN.shadowProj.xyz / IN.shadowProj.w;
		if(abs(shadowNDC.x) < 1.0f && abs(shadowNDC.y) < 1.0f && abs(shadowNDC.z) < 1.0f){
			vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
			float shadowZ = texture(shadowTex, biasCoord.xy).x;
			if(shadowZ < biasCoord.z){
				shadow = 0.0f;
			}
		}

		surface = (diffuse.rgb * light2Colour.rgb);
		
		temp.rgb = surface * lambert * attenuation;
		temp.rgb += (light2Colour.rgb * specFactor) * attenuation * 0.33;
		temp.rgb *= shadow;
		temp.rgb += surface * 0.1f;
		temp.a = diffuse.a;

		fragColour += (temp * 2);
	}

	distance = length(light3Pos - IN.worldPos);
	if (distance < 2000.0f) {
		incident = normalize(light3Pos - IN.worldPos);
		halfDir = normalize(incident + viewDir);

		lambert = max(dot(incident, normal), 0.0f);

		attenuation = 1.0f - clamp(distance / light3Radius, 0.0, 1.0);

		specFactor = clamp(dot(halfDir, normal), 0.0, 1.0);
		specFactor = pow(specFactor, 60.0);

		shadow = 1.0;

		shadowNDC = IN.shadowProj.xyz / IN.shadowProj.w;
		if(abs(shadowNDC.x) < 1.0f && abs(shadowNDC.y) < 1.0f && abs(shadowNDC.z) < 1.0f){
			vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
			float shadowZ = texture(shadowTex, biasCoord.xy).x;
			if(shadowZ < biasCoord.z){
				shadow = 0.0f;
			}
		}

		surface = (diffuse.rgb * light3Colour.rgb);
		
		temp.rgb = surface * lambert * attenuation;
		temp.rgb += (light3Colour.rgb * specFactor) * attenuation * 0.33;
		temp.rgb *= shadow;
		temp.rgb += surface * 0.1f;
		temp.a = diffuse.a;

		fragColour += (temp * 2);
	}
}