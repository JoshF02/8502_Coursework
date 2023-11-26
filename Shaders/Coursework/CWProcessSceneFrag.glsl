#version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D differentTex;
uniform bool bloomEnabled;
uniform bool nvEnabled;

in Vertex {
	vec2 texCoord;
} IN;

out vec4 fragColour;

void main(void) {
	vec4 t0 = texture2D(diffuseTex, IN.texCoord);	// blurred (bright) fragments
	vec4 t1 = texture2D(differentTex, IN.texCoord);	// all colour fragments, not blurred

	if (bloomEnabled)
		fragColour = t1 + t0;
	
	else
		fragColour = vec4(t0.rgb + t1.rgb * 0.0001, t0.a);


	if (nvEnabled) {	// for night vision effect, increases gamma to brighten image + reduces red and blue values to give green tint
		float gamma = 3.2;
		fragColour.rgb = pow(fragColour.rgb, vec3(1.0/gamma));
		fragColour.rb *= 0.6;
	}
}