#pragma once
#include "../nclgl/OGLRenderer.h"

class Camera;
class Mesh;
class HeightMap;

class SpaceRenderer : public OGLRenderer
{
public:
	SpaceRenderer(void);
	SpaceRenderer(Window& parent);
	~SpaceRenderer(void);

	void RenderScene() override;
	void UpdateScene(float dt) override;

protected:
	void FillBuffers();
	void DrawPointLights();
	void CombineBuffers();
	void GenerateScreenTexture(GLuint& into, bool depth = false);

	Shader* sceneShader;
	Shader* pointlightShader;
	Shader* combineShader;
	GLuint bufferFBO;
	GLuint bufferColourTex;
	GLuint bufferNormalTex;
	GLuint bufferDepthTex;

	GLuint pointLightFBO;
	GLuint lightDiffuseTex;
	GLuint lightSpecularTex;

	//HeightMap* heightMap;
	Light* pointLights;
	Mesh* sphere;
	Mesh* quad;
	Camera* camera;
	GLuint earthTex;
	GLuint earthBump;

	float radius;
};
