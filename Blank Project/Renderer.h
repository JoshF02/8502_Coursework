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
	void DrawNode(SceneNode* n);
	bool SetTexture(GLuint texID, GLuint unit, const std::string& uniformName, Shader* s, GLenum target);
	void PresentScene();
	void DrawPostProcess();
	void DrawShadowScene();
	void DrawNodeShadows(SceneNode* n);
	int ApplyFloatingMovement(SceneNode* n, int count);

	Shader* reflectShader;
	Shader* skyboxShader;
	Shader* npcShader;
	Shader* processShader;
	Shader* processSceneShader;
	Shader* shadowSceneShader;
	Shader* shadowShader;
	Shader* simpleLitShader;
	Shader* sunShader;
	Shader* bloomShader;

	HeightMap* heightMap;
	Vector3 heightmapSize;
	Light* light;
	Light* pointLights[2];
	Camera* camera;
	Orbit* orbit;

	Mesh* quad;
	Mesh* processQuad;
	Mesh* orbitSun;
	Mesh* npc;
	MeshAnimation* anim;
	MeshMaterial* npcMat;
	vector<GLuint> npcMatTextures;
	Mesh* shipMesh;
	MeshMaterial* shipMat;
	vector<GLuint> shipMatTextures;

	GLuint cubeMap;
	GLuint waterTex;
	GLuint earthTex;
	GLuint earthBump;
	GLuint shipTexture;
	GLuint nodeTex;
	GLuint bufferFBO;
	GLuint processFBO;
	GLuint bufferBrightTex[2];
	GLuint bufferDepthTex;
	GLuint bufferColourTex;
	GLuint shadowTex;
	GLuint shadowFBO;
	GLuint sunTex;

	float waterRotate;
	float waterCycle;
	int currentFrame;
	float frameTime;
	float sceneTime;

	bool camAutoHasStarted;
	bool bloomEnabled;
	bool blurEnabled;
	bool nvEnabled;

	SceneNode* root;
	SceneNode* terrain;
	SceneNode* npcNode;
	SceneNode* terrainCentreNode;
	SceneNode* orbitSunNode;
	SceneNode* floatingGlowingOrb;
	SceneNode* floatingGlowingOrb2;
};
