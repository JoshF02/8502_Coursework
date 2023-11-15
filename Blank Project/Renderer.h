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
	SceneNode* terrain;

	bool SetTexture(GLuint texID, GLuint unit, const std::string& uniformName, Shader* s, GLenum target);

	SceneNode* npcNode;
	Mesh* npc;
	Shader* npcShader;
	MeshAnimation* anim;
	MeshMaterial* npcMat;
	vector<GLuint> npcMatTextures;
	int currentFrame;
	float frameTime;
	GLuint nodeTex;

	Shader* matShader;
	Mesh* shipMesh;
	GLuint texture;	// remove once i get mat working
	MeshMaterial* shipMat;
};
