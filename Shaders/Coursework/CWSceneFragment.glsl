#version 330 core

uniform sampler2D diffuseTex;
uniform int useTexture;

in Vertex{
	vec2 texCoord;
	vec4 colour;
} IN;

out vec4 fragColour;

void main(void){
	fragColour = IN.colour;
	if(useTexture > 0) {
		fragColour *= texture(diffuseTex, IN.texCoord);

		int scaleDownFactor = 6;	// reduce brightness of sun surface - same amount as increase in CWCombineFrag, result is brighter background but same sun
		fragColour /= scaleDownFactor;
		fragColour.a *= scaleDownFactor;
	}
}