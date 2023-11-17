#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/MeshMaterial.h"
#include "../nclgl/MeshAnimation.h"

const int POST_PASSES = 10;
#define SHADOWSIZE 2048

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

	heightmapSize = heightMap->GetHeightmapSize();

	camera = new Camera(-45.0f, 0.0f, heightmapSize * Vector3(0.5f, 5.0f, 0.5f), heightmapSize);
	//camera = new Camera(-30.0f, 315.0f, Vector3(-8.0f, 5.0f, 8.0f));
	light = new Light(heightmapSize * Vector3(0.5f, 30.5f, 0.5f), Vector4(1, 1, 1, 1), heightmapSize.x * 2);	// POINT LIGHT
	//light = new Light(Vector3(-20.0f, 30.0f, -20.0f), Vector4(1, 1, 1, 1), 250.0f);

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




	processQuad = Mesh::GenerateQuad();	// POST PROCESSING
	processShader = new Shader("TexturedVertex.glsl", "ProcessFrag.glsl");
	processSceneShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	if (!processShader->LoadSuccess() || !processSceneShader->LoadSuccess()) {
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





	shadowSceneShader = new Shader("ShadowSceneVert.glsl", "ShadowSceneFrag.glsl");	// SHADOWS
	shadowShader = new Shader("ShadowVert.glsl", "ShadowFrag.glsl");
	if (!shadowSceneShader->LoadSuccess() || !shadowShader->LoadSuccess()) return;

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	sceneMeshes.emplace_back(Mesh::GenerateQuad());
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Sphere.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cylinder.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cone.msh"));

	//sceneDiffuse = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	//sceneBump = SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	//SetTextureRepeating(sceneDiffuse, true);
	//SetTextureRepeating(sceneBump, true);
	//glEnable(GL_DEPTH_TEST);

	sceneTransforms.resize(4);
	sceneTransforms[0] = //Matrix4::Translation(Vector3(0.0f, -500.0f, 0.0f)) *
		//Matrix4::Scale(Vector3(10.0f, 10.0f, 10.0f)) *
		Matrix4::Translation(Vector3(4500.0f, 400.0f, 4500.0f)) *
		Matrix4::Scale(Vector3(1000.0f, 1000.0f, 1000.0f)) *
		Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(10, 10, 1));// *
		//Matrix4::Translation(heightmapSize * Vector3(0.6f, 0.4f, 0.45f)) *
		//Matrix4::Translation(heightmapSize * Vector3(0.6f, 0.4f, 0.45f)) *
		//Matrix4::Scale(Vector3(300.0f, 300.0f, 300.0f));

	std::cout << "transformpos before: " << sceneTransforms[0] << "\n";
	sceneTransforms[0] = sceneTransforms[0] * Matrix4::Scale(Vector3(10.0f, 10.0f, 10.0f));
	std::cout << "transformpos after: " << sceneTransforms[0] << "\n";

	sceneTime = 0.0f;


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
	delete processSceneShader;
	delete processQuad;
	glDeleteTextures(2, bufferColourTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &processFBO);


	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);

	for (auto& i : sceneMeshes) {
		delete i;
	}

	delete shadowSceneShader;
	delete shadowShader;
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

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_U)) {
		light->SetPosition(light->GetPosition() + Vector3(0.0f, 0.0f, 25.0f));
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_J)) {
		light->SetPosition(light->GetPosition() + Vector3(0.0f, 0.0f, -25.0f));
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_H)) {
		light->SetPosition(light->GetPosition() + Vector3(25.0f, 0.0f, 0.0f));
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_K)) {
		light->SetPosition(light->GetPosition() + Vector3(-25.0f, 0.0f, 0.0f));
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_N)) {
		light->SetPosition(light->GetPosition() + Vector3(0.0f, 25.0f, 0.0f));
	}
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_M)) {
		light->SetPosition(light->GetPosition() + Vector3(0.0f, -25.0f, 0.0f));
	}


	sceneTime += dt;

	for (int i = 1; i < 4; ++i) {
		Vector3 t = Vector3(-10 + (5 * i), 2.0f + sin(sceneTime * i), 0);
		sceneTransforms[i] = Matrix4::Translation(Vector3(4500.0f, 400.0f, 4500.0f)) *
			Matrix4::Scale(Vector3(100.0f, 100.0f, 100.0f)) *
			Matrix4::Translation(t) * Matrix4::Rotation(sceneTime * 10 * i, Vector3(1, 0, 0));//*
			//Matrix4::Translation(heightmapSize * Vector3(0.6f, 0.4f, 0.45f)) *
			//Matrix4::Translation(Vector3(100.0f, 100.0f, 100.0f)) *
			//Matrix4::Scale(Vector3(300.0f, 300.0f, 300.0f));
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
		//DrawNode(root);
		//DrawWater();
		DrawShadowScene();

		DrawNode(root);
		//DrawMainScene();
	}
}


void Renderer::DrawScene() {
	//glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	//glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	BindShader(processSceneShader);
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);
	UpdateShaderMatrices();
	glUniform1i(glGetUniformLocation(processSceneShader->GetProgram(), "diffuseTex"), 0);
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

			BindShader(shadowSceneShader);
			SetShaderLight(*light);
			glUniform3fv(glGetUniformLocation(shadowSceneShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

			SetTexture(earthTex, 0, "diffuseTex", shadowSceneShader, GL_TEXTURE_2D);
			SetTexture(earthBump, 1, "bumpTex", shadowSceneShader, GL_TEXTURE_2D);
			SetTexture(shadowTex, 2, "shadowTex", shadowSceneShader, GL_TEXTURE_2D);

			modelMatrix.ToIdentity();
			textureMatrix.ToIdentity();

			UpdateShaderMatrices();

			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(shadowSceneShader->GetProgram(), "modelMatrix"), 1, false, model.values);

			glUniform4fv(glGetUniformLocation(shadowSceneShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

			glUniform1i(glGetUniformLocation(shadowSceneShader->GetProgram(), "useTexture"), 0);

			n->Draw(*this);

			
			
			// DRAW OBJECTS
			for (int i = 1; i < 4; ++i) {
				modelMatrix = sceneTransforms[i];
				UpdateShaderMatrices();
				sceneMeshes[i]->Draw();
			}
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
	BindShader(processSceneShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	glUniform1i(glGetUniformLocation(processSceneShader->GetProgram(), "diffuseTex"), 0);
	processQuad->Draw();
}




void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BindShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), heightmapSize * Vector3(0.5f, 0.0f, 0.5f));	// POINT TOWARDS CENTRE OF TERRAIN
	//viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(light->GetPosition().x, -100000.0f, light->GetPosition().z));	// POINT DOWNWARDS
	projMatrix = Matrix4::Perspective(1, 10000, 1, 90);
	shadowMatrix = projMatrix * viewMatrix;

	// DRAW TERRAIN
	//Matrix4 model = terrain->GetWorldTransform() * Matrix4::Scale(terrain->GetModelScale());
	//glUniformMatrix4fv(glGetUniformLocation(shadowShader->GetProgram(), "modelMatrix"), 1, false, model.values);
	//modelMatrix = terrain->GetWorldTransform() * Matrix4::Scale(terrain->GetModelScale());
	//UpdateShaderMatrices();
	//terrain->Draw(*this);
	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();
	Matrix4 model = terrain->GetWorldTransform() * Matrix4::Scale(terrain->GetModelScale());
	glUniformMatrix4fv(glGetUniformLocation(shadowShader->GetProgram(), "modelMatrix"), 1, false, model.values);
	glUniform4fv(glGetUniformLocation(shadowShader->GetProgram(), "nodeColour"), 1, (float*)&terrain->GetColour());
	glUniform1i(glGetUniformLocation(shadowShader->GetProgram(), "useTexture"), 0);
	terrain->Draw(*this);

	for (int i = 1; i < 4; ++i) {	// DRAW OBJECTS (REMOVE QUAD FROM SLOT 0 LATER)
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// RESET VIEW AND PROJ MATRIX
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
}

void Renderer::DrawMainScene() {
	BindShader(shadowSceneShader);
	SetShaderLight(*light);
	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	glUniform1i(glGetUniformLocation(shadowSceneShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(shadowSceneShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(shadowSceneShader->GetProgram(), "shadowTex"), 2);

	glUniform3fv(glGetUniformLocation(shadowSceneShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	for (int i = 0; i < 4; ++i) {
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}
}
