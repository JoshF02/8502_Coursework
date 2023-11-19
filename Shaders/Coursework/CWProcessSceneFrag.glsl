#version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D differentTex;
uniform float blend;

in Vertex {
	vec2 texCoord;
	//vec4 colour;
} IN;

out vec4 fragColour;
void main(void){
	vec4 t0 = texture2D(diffuseTex, IN.texCoord);
	vec4 t1 = texture2D(differentTex, IN.texCoord);

	//fragColour = mix(t0, t1, 0.5);
	//fragColour = (t1 * 0.95) + (t0 * 0.05);	// adds brightTex to colourTex (additive blending)
	fragColour = t1;

	//fragColour = mix(t0, t1, t1.a)  * IN.colour;
	//fragColour = t0;
}