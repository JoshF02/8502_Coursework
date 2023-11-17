#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/MeshMaterial.h"
#include "../nclgl/MeshAnimation.h"

const int POST_PASSES = 10;

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

	camera = new Camera(-45.0f, 0.0f, heightmapSize * Vector3(0.5f, 5.0f, 0.5f), heightmapSize);
	light = new Light(heightmapSize * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), heightmapSize.x);	// POINT LIGHT

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



	npc = Mesh::LoadFromMeshFile("/Coursework/Monster_Crab.msh");	// ANIMATED CHARACTER
	npcShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl");
	anim = new MeshAnimation("/Coursework/Monster_Crab.anm");
	npcMat = new MeshMaterial("/Coursework/Monster_Crab.mat");

	for (int i = 0; i < npc->GetSubMeshCount(); ++i)
	{
		const MeshMaterialEntry* matEntry = npcMat->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		npcMatTextures.emplace_back(texID);
	}

	currentFrame = 0;
	frameTime = 0.0f;

	npcNode = new SceneNode();
	npcNode->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	npcNode->SetTransform(Matrix4::Translation(heightmapSize * Vector3(0.6f, 0.4f, 0.45f)));
	npcNode->SetModelScale(Vector3(300.0f, 300.0f, 300.0f));
	npcNode->SetMesh(npc);
	npcNode->SetAniTexture(npcMatTextures);
	npcNode->SetAnimation(anim);
	root->AddChild(npcNode);





	matShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");	// STATIC MESHES
	shipMesh = Mesh::LoadFromMeshFile("/Coursework/Example1NoInterior_Grey.msh");	
	shipMat = new MeshMaterial("/Coursework/Example1NoInterior_Grey.mat");	// change to w/interio for submission, but loads slow
	shipTexture = SOIL_load_OGL_texture(TEXTUREDIR"brick.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);

	for (int i = 0; i < shipMesh->GetSubMeshCount(); ++i)
	{
		const MeshMaterialEntry* matEntry = shipMat->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		shipMatTextures.emplace_back(texID);
	}

	for (int i = 1; i < 4; ++i)
	{
		SceneNode* s = new SceneNode(shipMesh);
		s->SetTransform(Matrix4::Translation(heightmapSize * Vector3(0.2f * i, 2.0f, 0.08f * i)));
		s->SetModelScale(Vector3(100.0f, 100.0f, 100.0f));
		s->SetBoundingRadius(50.0f);
		s->SetTexture(shipTexture);
		//s->SetColour(Vector4(0, 1, 0, 1));	// colours the texture, can remove
		root->AddChild(s);
	}




	processQuad = Mesh::GenerateQuad();
	processShader = new Shader("TexturedVertex.glsl", "ProcessFrag.glsl");
	sceneShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	if (!processShader->LoadSuccess() || !sceneShader->LoadSuccess()) {
		return;
	}

	//Scene Depth Texture
	glGenTextures(1, &bufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	//Colour Texture
	for (int i = 0; i < 2; ++i) {
		glGenTextures(1, &bufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &processFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0]) return;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	postEnabled = false;
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

	delete npc;
	//delete npcNode;
	//delete terrain
	delete npcShader;
	delete anim;
	delete npcMat;

	delete matShader;
	delete shipMesh;
	//delete shipMat;

	delete processShader;
	delete processQuad;
	glDeleteTextures(2, bufferColourTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &processFBO);
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	waterRotate += dt * 2.0f;
	waterCycle += dt * 0.25f;

	frameTime -= dt;
	while (frameTime < 0.0f)
	{
		currentFrame = (currentFrame + 1) % anim->GetFrameCount();
		frameTime += 1.0f / anim->GetFrameRate();
	}
	npcNode->SetCurrentFrame(currentFrame);

	root->Update(dt);

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_P)) {
		postEnabled = true;
	}

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_O)) {
		postEnabled = false;
	}
}

void Renderer::RenderScene() {
	if (postEnabled) {
		glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		DrawSkybox();
		DrawNode(root);	//DrawHeightmap();
		DrawWater();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//DrawScene();


		// NOTES ON BROKEN PPROCESSING
		// - skybox mostly works but looks weird
		// - terrain doesnt appear at all
		// - water makes the processing texture solid colour
		DrawPostProcess();
		PresentScene();
	}
	else {
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		DrawSkybox();
		DrawNode(root);
		DrawWater();
	}
}


void Renderer::DrawScene() {
	//glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	//glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	BindShader(sceneShader);
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);
	heightMap->Draw();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
	if (n->GetMesh())
	{
		if (n == npcNode)
		{
			BindShader(npcShader);

			glUniform1i(glGetUniformLocation(npcShader->GetProgram(), "diffuseTex"), 0);
			UpdateShaderMatrices();

			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(npcShader->GetProgram(), "modelMatrix"), 1, false, model.values);
			glUniform4fv(glGetUniformLocation(npcShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

			glUniform3fv(glGetUniformLocation(npcShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

			nodeTex = npcMatTextures[0];

			glUniform1i(glGetUniformLocation(npcShader->GetProgram(), "useTexture"), nodeTex);

			vector<Matrix4> frameMatrices;

			const Matrix4* invBindPose = npc->GetInverseBindPose();
			const Matrix4* frameData = anim->GetJointData(currentFrame);

			for (unsigned int i = 0; i < npc->GetJointCount(); ++i)
			{
				frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
			}

			int j = glGetUniformLocation(npcShader->GetProgram(), "joints");
			glUniformMatrix4fv(j, frameMatrices.size(), false,
				(float*)frameMatrices.data());

			for (int i = 0; i < npc->GetSubMeshCount(); ++i)
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, npcMatTextures[i]);
				npc->DrawSubMesh(i);
			}
		}
		else if (n == terrain)
		{
			//FADE LIGHT COLOUR TO RED OVER TIME
			//light->SetColour(Vector4(light->GetColour().x, light->GetColour().y - 0.0001, light->GetColour().z - 0.0001, light->GetColour().w));

			BindShader(lightShader);
			SetShaderLight(*light);
			glUniform3fv(glGetUniformLocation(lightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

			SetTexture(earthTex, 0, "diffuseTex", lightShader, GL_TEXTURE_2D);
			SetTexture(earthBump, 1, "bumpTex", lightShader, GL_TEXTURE_2D);

			modelMatrix.ToIdentity();
			textureMatrix.ToIdentity();

			UpdateShaderMatrices();

			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(lightShader->GetProgram(), "modelMatrix"), 1, false, model.values);

			glUniform4fv(glGetUniformLocation(lightShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

			glUniform1i(glGetUniformLocation(lightShader->GetProgram(), "useTexture"), 0);

			n->Draw(*this);
		}
		else
		{
			BindShader(matShader);
			//SetTexture(n->GetTexture(), 0, "diffuseTex", matShader, GL_TEXTURE_2D);	// for texture uncomment this and comment the texture bind below
			UpdateShaderMatrices();

			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(matShader->GetProgram(), "modelMatrix"), 1, false, model.values);
			glUniform4fv(glGetUniformLocation(matShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

			glUniform3fv(glGetUniformLocation(matShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

			nodeTex = n->GetTexture();
			glUniform1i(glGetUniformLocation(matShader->GetProgram(), "useTexture"), nodeTex);

			for (int i = 0; i < shipMesh->GetSubMeshCount(); ++i)	// for simple mesh instead use n->Draw(*this); 
			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, shipMatTextures[i]);
				shipMesh->DrawSubMesh(i);
			}
		}
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




void Renderer::DrawPostProcess() {
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(processShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(processShader->GetProgram(), "sceneTex"), 0);
	for (int i = 0; i < POST_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 0);

		glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
		processQuad->Draw();

		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
		processQuad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::PresentScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(sceneShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	processQuad->Draw();
}
