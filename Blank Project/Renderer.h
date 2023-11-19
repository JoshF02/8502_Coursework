#pragma once
#include "../nclgl/OGLRenderer.h"
#include "Orbit.h"

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

	//Shader* matShader;
	Mesh* shipMesh;
	GLuint shipTexture;	// remove once i get mat working
	MeshMaterial* shipMat;
	vector<GLuint> shipMatTextures;


	void PresentScene();
	void DrawPostProcess();
	void DrawScene();	// only using for testing - comparing to my scene drawing

	Mesh* processQuad;
	Shader* processShader;
	Shader* processSceneShader; // might not need
	GLuint bufferFBO;
	GLuint processFBO;
	GLuint bufferBrightTex[2];
	GLuint bufferDepthTex;

	GLuint bufferColourTex;
	//int postEnabled;	// 0 = disabled, 1 = blur, 2 = bloom
	bool bloomEnabled;


	void DrawShadowScene();

	GLuint shadowTex;
	GLuint shadowFBO;
	float sceneTime;
	Shader* shadowSceneShader;
	Shader* shadowShader;
	//vector<Mesh*> sceneMeshes;
	//vector<Matrix4> sceneTransforms;
	//SceneNode* shadowMeshesNode;

	Vector3 heightmapSize;

	Shader* simpleLitShader;
	Shader* sunShader;

	Orbit* orbit;
	Mesh* orbitSun;
	SceneNode* terrainCentreNode;
	SceneNode* orbitSunNode;
	GLuint sunTex;

	void DrawNodeShadows(SceneNode* n);

	int ApplyFloatingMovement(SceneNode* n, int count);

	bool camAutoHasStarted;
};
