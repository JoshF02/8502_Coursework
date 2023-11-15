#pragma once
#include "../nclgl/OGLRenderer.h"

class Camera;
class Shader;
class HeightMap;
class SceneNode;
class MeshMaterial;
class MeshAnimation;

class Renderer : public OGLRenderer {
public:
	Renderer(Window& parent);
	~Renderer(void);
	void RenderScene() override;
	void UpdateScene(float dt) override;

protected:
	void DrawHeightmap();
	void DrawWater();
	void DrawSkybox();

	Shader* lightShader;
	Shader* reflectShader;
	Shader* skyboxShader;

	HeightMap* heightMap;
	Mesh* quad;

	Light* light;
	Camera* camera;

	GLuint cubeMap;
	GLuint waterTex;
	GLuint earthTex;
	GLuint earthBump;

	float waterRotate;
	float waterCycle;

	void DrawNode(SceneNode* n);

	SceneNode* root;

	bool SetTexture(GLuint texID, GLuint unit, const std::string& uniformName, Shader* s, GLenum target);

	Shader* skinningShader;
	Mesh* ship;
	MeshMaterial* shipMaterial;
	MeshAnimation* shipAnim;
	vector<GLuint> shipMatTextures;

	int currentFrame;
	float frameTime;

	void RenderSkinnedMesh();

	SceneNode* shipNode;
	GLuint testTex;

	Shader* sceneShader;
	SceneNode* terrain;
};
