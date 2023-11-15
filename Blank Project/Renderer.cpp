#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/MeshMaterial.h"
#include "../nclgl/MeshAnimation.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	quad = Mesh::GenerateQuad();

	heightMap = new HeightMap(TEXTUREDIR"/Coursework/TestHM2.png");

	waterTex = SOIL_load_OGL_texture(TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthTex = SOIL_load_OGL_texture(TEXTUREDIR"/Coursework/rock_boulder_dry_diff_1k.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR"/Coursework/rock_boulder_dry_nor_gl_1k.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"/Coursework/sb_right.png", TEXTUREDIR"/Coursework/sb_left.png",
		TEXTUREDIR"/Coursework/sb_top.png", TEXTUREDIR"/Coursework/sb_bot.png",
		TEXTUREDIR"/Coursework/sb_front.png", TEXTUREDIR"/Coursework/sb_back.png", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	if (!earthTex || !earthBump || !cubeMap || !waterTex) return;

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(waterTex, true);

	reflectShader = new Shader("ReflectVertex.glsl", "/Coursework/CWReflectFragment.glsl");
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	lightShader = new Shader("BumpVertex.glsl", "BumpFragment.glsl");

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !lightShader->LoadSuccess()) return;

	Vector3 heightmapSize = heightMap->GetHeightmapSize();

	camera = new Camera(-45.0f, 0.0f, heightmapSize * Vector3(0.5f, 5.0f, 0.5f));
	light = new Light(heightmapSize * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), heightmapSize.x);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	waterRotate = 0.0f;
	waterCycle = 0.0f;


	root = new SceneNode();	// SCENE GRAPH FUNCTIONALITY
	terrain = new SceneNode();
	terrain->SetMesh(heightMap);
	root->AddChild(terrain);


	//glEnable(GL_CULL_FACE);
	skinningShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl");
	if (!skinningShader->LoadSuccess()) return;

	//ship = Mesh::LoadFromMeshFile("/Coursework/Example1NoInterior_Grey.msh");
	//shipMaterial = new MeshMaterial("/Coursework/Example1NoInterior_Grey.mat");
	/*ship = Mesh::LoadFromMeshFile("Role_T.msh");
	shipAnim = new MeshAnimation("Role_T.anm");
	shipMaterial = new MeshMaterial("Role_T.mat");

	for (int i = 0; i < ship->GetSubMeshCount(); ++i) {
		const MeshMaterialEntry* matEntry = shipMaterial->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		shipMatTextures.emplace_back(texID);
	}

	currentFrame = 0;
	frameTime = 0.0f;*/

	/*SceneNode* shipNode = new SceneNode();
	shipNode->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	shipNode->SetTransform(Matrix4::Translation(Vector3(10.0f, 10.0f, 10.0f)));
	shipNode->SetModelScale(Vector3(10.0f, 10.0f, 10.0f));
	shipNode->SetBoundingRadius(100.0f);
	shipNode->SetMesh(ship);
	shipNode->SetTexture(earthTex);
	//root->AddChild(shipNode);*/

	sceneShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");

	ship = Mesh::LoadFromMeshFile("SP_Crystal01.msh");

	testTex = SOIL_load_OGL_texture(
		TEXTUREDIR"diffuse.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	SetTextureRepeating(testTex, true);

	for (int i = 1; i < 4; ++i)
	{
		SceneNode* s = new SceneNode();
		s->SetColour(Vector4(1.0f, 3.0f, 5.0f, 1.0f));
		s->SetTransform(Matrix4::Translation(heightmapSize * Vector3(0.4f * i, 0.5f, 0.3f * i)));
		s->SetModelScale(Vector3(200.0f, 200.0f, 200.0f));
		s->SetMesh(ship);
		s->SetTexture(testTex);
		root->AddChild(s);
	}

	init = true;
}

Renderer::~Renderer(void) {
	delete camera;
	delete heightMap;
	delete quad;
	delete reflectShader;
	delete skyboxShader;
	delete lightShader;
	delete light;
	delete root;

	delete skinningShader;
	delete ship;
	delete shipMaterial;
	delete shipAnim;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	waterRotate += dt * 2.0f;
	waterCycle += dt * 0.25f;

	root->Update(dt);

	/*frameTime -= dt;
	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % shipAnim->GetFrameCount();
		frameTime += 1.0f / shipAnim->GetFrameRate();
	}*/
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	DrawSkybox();
	DrawNode(root);	//DrawHeightmap();
	DrawWater();
	//DrawNode(shipNode);
	//RenderSkinnedMesh();
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	quad->Draw();
	glDepthMask(GL_TRUE);
}

void Renderer::DrawWater() {
	BindShader(reflectShader);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	SetTexture(waterTex, 0, "diffuseTex", reflectShader, GL_TEXTURE_2D);
	SetTexture(cubeMap, 2, "cubeTex", reflectShader, GL_TEXTURE_CUBE_MAP);

	Vector3 hSize = heightMap->GetHeightmapSize();

	modelMatrix = Matrix4::Translation(hSize * 0.5f) * Matrix4::Scale(hSize * 0.5f) * Matrix4::Rotation(90, Vector3(1, 0, 0));

	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) * Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();
	quad->Draw();
}

void Renderer::DrawNode(SceneNode* n) {
	if (n == terrain) {
		BindShader(lightShader);
		SetShaderLight(*light);
		glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

		SetTexture(earthTex, 0, "diffuseTex", lightShader, GL_TEXTURE_2D);
		SetTexture(earthBump, 1, "bumpTex", lightShader, GL_TEXTURE_2D);

		modelMatrix.ToIdentity();
		textureMatrix.ToIdentity();

		UpdateShaderMatrices();

		if (n->GetMesh()) {
			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(lightShader->GetProgram(), "modelMatrix"), 1, false, model.values);

			glUniform4fv(glGetUniformLocation(lightShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

			glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "useTexture"), 0);

			n->Draw(*this);
		}
	}
	else {
		BindShader(sceneShader);

		glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
		UpdateShaderMatrices();

		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
		glUniformMatrix4fv(glGetUniformLocation(sceneShader->GetProgram(), "modelMatrix"), 1, false, model.values);
		glUniform4fv(glGetUniformLocation(sceneShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

		glUniform3fv(glGetUniformLocation(sceneShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

		testTex = n->GetTexture();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, testTex);

		glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "useTexture"), testTex);

		n->Draw(*this);
	}

	for (vector<SceneNode*>::const_iterator i = n->GetChildIteratorStart(); i != n->GetChildIteratorEnd(); ++i) {
		DrawNode(*i);
	}
}

bool Renderer::SetTexture(GLuint texID, GLuint unit, const std::string& uniformName, Shader* s, GLenum target) {	// target is type of texture e.g. GL_TEXTURE_2D
	GLint uniformID = glGetUniformLocation(s->GetProgram(), uniformName.c_str());
	if (uniformID < 0) {
		std::cout << "Trying to bind invalid 2D texture uniform!\n";
		return false;
	}

	if (currentShader != s) {
		std::cout << "Trying to set shader uniform on wrong shader!\n";
		return false;
	}

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(target, texID);

	glUniform1i(uniformID, unit);

	return true;
}

void Renderer::RenderSkinnedMesh() {
	BindShader(skinningShader);
	glUniform1i(glGetUniformLocation(skinningShader->GetProgram(), "diffuseTex"), 0);

	//Matrix4 testModelMatrix = Matrix4::Scale(Vector3(10.0f, 10.0f, 10.0f));

	UpdateShaderMatrices();

	//glUniformMatrix4fv(glGetUniformLocation(skinningShader->GetProgram(), "modelMatrix"), 1, false, testModelMatrix.values);

	vector<Matrix4> frameMatrices;

	const Matrix4* invBindPose = ship->GetInverseBindPose();
	const Matrix4* frameData = shipAnim->GetJointData(currentFrame);

	for (unsigned int i = 0; i < ship->GetJointCount(); ++i) {
		frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
	}

	int j = glGetUniformLocation(skinningShader->GetProgram(), "joints");
	glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());

	for (int i = 0; i < ship->GetSubMeshCount(); ++i) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shipMatTextures[i]);
		ship->DrawSubMesh(i);
	}
}
