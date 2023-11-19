#version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D differentTex;
uniform bool postEnabled;

in Vertex {
	vec2 texCoord;
	//vec4 colour;
} IN;

out vec4 fragColour;
void main(void) {
	vec4 t0 = texture2D(diffuseTex, IN.texCoord);	// blurred (bright) fragments
	vec4 t1 = texture2D(differentTex, IN.texCoord);	// all colour fragments, not blurred

	//fragColour = vec4(t1.rgb * 1 + t0.rgb * 0.0001, t0.a);

	fragColour = t1 + t0; // adds brightTex to colourTex (additive blending)
}