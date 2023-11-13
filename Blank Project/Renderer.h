#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/HeightMap.h"

class Renderer : public OGLRenderer	{
public:
	Renderer(Window &parent);
	 ~Renderer(void);

	 void RenderScene()				override;
	 void UpdateScene(float msec)	override;

protected:
	void DrawNode(SceneNode* n);

	SceneNode* root;
	Camera* camera;
	Shader* shader;
	HeightMap* heightMap;
	GLuint terrainTex;

	void DrawSkybox();

	Shader* skyboxShader;
	GLuint cubeMap;
	Mesh* quad;

};
