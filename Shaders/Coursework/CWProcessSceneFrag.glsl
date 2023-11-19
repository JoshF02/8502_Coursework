#version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D differentTex;
//uniform float blend;
//uniform int postEnabled;
uniform bool postEnabled;

in Vertex {
	vec2 texCoord;
	//vec4 colour;
} IN;

out vec4 fragColour;
void main(void) {
	vec4 t0 = texture2D(diffuseTex, IN.texCoord);
	vec4 t1 = texture2D(differentTex, IN.texCoord);

	//fragColour = mix(t0, t1, 0.5);
	//fragColour = t1 + t0;	// adds brightTex to colourTex (additive blending)

	//fragColour = mix(t0, t1, t1.a)  * IN.colour;
	//fragColour = t0;

	/*switch (postEnabled) {

	case 0:
		fragColour = vec4(0.0, 1.0, 0.0, 1.0);	// bright green - this should never be called as shader only used if postEnabled > 0
	
	case 1:
		fragColour = t0;

	case 2:
		fragColour = t1 + t0;

	}*/

	fragColour = t1 + t0;
}