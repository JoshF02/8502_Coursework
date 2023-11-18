#pragma once
#include "../nclgl/OGLRenderer.h"
#include "Orbit.h"

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
	void DrawSkybox();

	Shader* sceneShader;
	Shader* pointlightShader;
	Shader* combineShader;
	Shader* skyboxShader;
	GLuint bufferFBO;
	GLuint bufferColourTex;
	GLuint bufferNormalTex;
	GLuint bufferDepthTex;

	GLuint pointLightFBO;
	GLuint lightDiffuseTex;
	GLuint lightSpecularTex;
	GLuint cubeMap;

	Light* pointLights;
	Mesh* sphere;
	Mesh* quad;
	Camera* camera;
	GLuint earthTex;
	GLuint earthBump;

	float radius;
	float sceneTime;

	Orbit* sunOrbit;
	GLuint sunTex;
	Matrix4 sunTransform;
	Shader* sunShader;
	Light* sunLight;
};
