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
	skyboxShader = new Shader("SkyboxVertex.glsl", "/Coursework/CWSkyboxFragment.glsl");

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess()) {
		return;
	}

	heightmapSize = heightMap->GetHeightmapSize();

	camera = new Camera(-20, 0, (heightmapSize * Vector3(1.0f, 5.0f, 1.0f)) + Vector3(heightmapSize.x / 2, 0, heightmapSize.x / 2), heightmapSize);
	camAutoHasStarted = false;
	light = new Light(heightmapSize * Vector3(0.5f, 30.5f, 0.5f), Vector4(1, 1, 1, 1), heightmapSize.x * 4);	// POINT LIGHT
	light->SetInitialRadius();

	projMatrix = Matrix4::Perspective(1.0f, 25000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	waterRotate = 0.0f;
	waterCycle = 0.0f;


	root = new SceneNode();	// SCENE GRAPH FUNCTIONALITY
	terrain = new SceneNode();
	terrain->SetMesh(heightMap);
	terrain->SetModelScale(Vector3(2.0, 2.0, 2.0));
	root->AddChild(terrain);



	npc = Mesh::LoadFromMeshFile("/Coursework/Monster_Crab.msh");	// ANIMATED CHARACTER
	npcShader = new Shader("/Coursework/CWLitSkinningVertex.glsl", "PerPixelFragment.glsl");
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





	shipMesh = Mesh::LoadFromMeshFile("/Coursework/Example1NoInterior_Grey.msh");	// STATIC MESHES
	shipMat = new MeshMaterial("/Coursework/Example1NoInterior_Grey.mat");	// change to w/interior for submission, but loads slow
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
		SceneNode* s = new SceneNode(shipMesh, Vector4(1,1,1,1), true);	// mesh is complex
		s->SetTransform(Matrix4::Translation(heightmapSize * Vector3(0.2f * i, 2.0f, 0.08f * i)));
		s->SetModelScale(Vector3(100.0f, 100.0f, 100.0f));
		s->SetBoundingRadius(50.0f);
		s->SetTexture(shipTexture);
		s->SetOriginalTransform();
		//s->SetColour(Vector4(0, 1, 0, 1));	// colours the texture, can remove
		root->AddChild(s);
	}




	processQuad = Mesh::GenerateQuad();	// POST PROCESSING
	processShader = new Shader("TexturedVertex.glsl", "ProcessFrag.glsl");
	processSceneShader = new Shader("TexturedVertex.glsl", "/Coursework/CWProcessSceneFrag.glsl");
	bloomShader = new Shader("TexturedVertex.glsl", "/Coursework/CWBloomFrag.glsl");
	simpleTexShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	if (!processShader->LoadSuccess() || !processSceneShader->LoadSuccess() || !bloomShader->LoadSuccess() || !simpleTexShader->LoadSuccess()) {
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

	//Bright Texture (for bloom)
	for (int i = 0; i < 2; ++i) {
		glGenTextures(1, &bufferBrightTex[i]);
		glBindTexture(GL_TEXTURE_2D, bufferBrightTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	//Colour Texture
	glGenTextures(1, &bufferColourTex);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);


	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &processFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferBrightTex[0], 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bufferColourTex, 0);	// extra code to draw to new colour tex
	GLenum buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferBrightTex[0] || !bufferColourTex) return;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);

	bloomEnabled = false;
	blurEnabled = false;



	


	shadowSceneShader = new Shader("ShadowSceneVert.glsl", "ShadowSceneFrag.glsl");	// SHADOWS
	shadowShader = new Shader("ShadowVert.glsl", "ShadowFrag.glsl");
	simpleLitShader = new Shader("PerPixelVertex.glsl", "PerPixelFragment.glsl");
	sunShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	if (!shadowSceneShader->LoadSuccess() || !shadowShader->LoadSuccess() || !simpleLitShader->LoadSuccess() || !sunShader->LoadSuccess()) return;

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

	
	// static sphere for testing
	SceneNode* test = new SceneNode(Mesh::LoadFromMeshFile("Sphere.msh"));
	test->SetTransform(Matrix4::Translation(Vector3(heightmapSize.x / 2, 1000.0f, heightmapSize.z / 2) * terrain->GetModelScale().x));
	test->SetModelScale(Vector3(250.0f, 250.0f, 250.0f));
	test->SetTexture(earthTex);
	terrain->AddChild(test);


	sceneTime = 0.0f;


	// sun orbiting around centre of terrain
	orbitSun = Mesh::LoadFromMeshFile("Sphere.msh");
	sunTex = SOIL_load_OGL_texture(TEXTUREDIR"/Coursework/2k_sun.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	SetTextureRepeating(sunTex, true);

	// for planets, use other 2k textures from https://www.solarsystemscope.com/textures/ and use simpleLitShader
	
	Vector3 terrainCentrePos = Vector3(heightmapSize.x / 2, 0.0f, heightmapSize.z / 2) * terrain->GetModelScale().x;
	terrainCentreNode = new SceneNode();
	terrainCentreNode->SetTransform(Matrix4::Translation(terrainCentrePos));
	root->AddChild(terrainCentreNode);

	Vector3 sunRelativePosition = Vector3(12000.0f, 0.0f, 0.0f);
	orbitSunNode = new SceneNode(orbitSun);
	orbitSunNode->SetTransform(Matrix4::Translation(sunRelativePosition));
	orbitSunNode->SetModelScale(Vector3(500.0f, 500.0f, 500.0f));
	orbitSunNode->SetTexture(sunTex);
	terrainCentreNode->AddChild(orbitSunNode);

	orbit = new Orbit(0.0f, terrainCentrePos, sunRelativePosition + terrainCentrePos, 0.05f);

	init = true;
}

Renderer::~Renderer(void) {	// need to check deletes
	delete camera;
	delete heightMap;
	delete quad;
	delete reflectShader;
	delete skyboxShader;
	delete light;
	delete root;

	delete npc;
	//delete npcNode;
	//delete terrain
	delete npcShader;
	delete anim;
	delete npcMat;

	delete shipMesh;
	//delete shipMat;

	delete processShader;
	delete processSceneShader;
	delete processQuad;
	glDeleteTextures(2, bufferBrightTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &processFBO);


	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);

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

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_P)) {
		bloomEnabled = true;
		blurEnabled = false;
	}

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_O)) {
		bloomEnabled = false;
		blurEnabled = true;
	}

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_I)) {
		bloomEnabled = false;
		blurEnabled = false;
	}


	sceneTime += dt;

	if (!camAutoHasStarted) {	// start automatic camera movement
		camera->TriggerAuto();
		camAutoHasStarted = true;
	}

	ApplyFloatingMovement(root, 1);


	// ORBITING LIGHT MANAGEMENT
	Vector3 orbitPos = orbit->CalculateRelativePosition();
	orbitSunNode->SetTransform(Matrix4::Translation(orbitPos));
	light->SetPosition(orbitPos + terrainCentreNode->GetTransform().GetPositionVector());	// light position = sun local pos + terrain centre global pos

	if (orbitPos.y < -750.0f) light->SetRadius(0.0f);
	else light->SetRadius(light->GetInitialRadius());	


	root->Update(dt);
}

int Renderer::ApplyFloatingMovement(SceneNode* n, int count) {
	if (n->GetMesh() == shipMesh) {;
		n->SetTransform(n->GetOriginalTransform() *
			Matrix4::Translation(Vector3(2.0f + cos(sceneTime * count) * 10, 2.0f + sin(sceneTime * count) * 50, 2.0f + sin(sceneTime * count) * 10)));
			
		count++;
	}

	for (vector<SceneNode*>::const_iterator i = n->GetChildIteratorStart(); i != n->GetChildIteratorEnd(); ++i) {
		count = ApplyFloatingMovement(*i, count);
	}

	return count;
}

void Renderer::RenderScene() {
	if (bloomEnabled || blurEnabled) {
		DrawShadowScene();
		glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		projMatrix = Matrix4::Perspective(1.0f, 25000.0f, (float)width / (float)height, 45.0f);
		DrawSkybox();
		DrawNode(root);	
		DrawWater();

		DrawPostProcess();
		PresentScene();
	}
	else {
		DrawShadowScene();
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		DrawSkybox();
		DrawNode(root);
		DrawWater();
	}
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
	SetShaderLight(*light);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	SetTexture(waterTex, 0, "diffuseTex", reflectShader, GL_TEXTURE_2D);
	SetTexture(cubeMap, 2, "cubeTex", reflectShader, GL_TEXTURE_CUBE_MAP);

	Vector3 hSize = heightMap->GetHeightmapSize();
	float scaleFac = terrain->GetModelScale().x;	// scales water size with terrain size

	modelMatrix = Matrix4::Translation(hSize * 0.5f * scaleFac) * Matrix4::Scale(hSize * 0.5f * scaleFac) * Matrix4::Rotation(90, Vector3(1, 0, 0));

	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) * Matrix4::Scale(Vector3(10, 10, 10)) * Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();
	quad->Draw();
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh() != NULL)
	{
		if (n == npcNode)
		{
			BindShader(npcShader);
			SetShaderLight(*light);

			glUniform1i(glGetUniformLocation(npcShader->GetProgram(), "diffuseTex"), 0);
			
			UpdateShaderMatrices();

			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(npcShader->GetProgram(), "modelMatrix"), 1, false, model.values);
			glUniform4fv(glGetUniformLocation(npcShader->GetProgram(), "colour"), 1, (float*)&n->GetColour());

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

			glUniform4fv(glGetUniformLocation(shadowSceneShader->GetProgram(), "colour"), 1, (float*)&n->GetColour());

			glUniform1i(glGetUniformLocation(shadowSceneShader->GetProgram(), "useTexture"), 0);

			n->Draw(*this);
		}
		else if (n == orbitSunNode) {
			BindShader(sunShader);
			SetTexture(n->GetTexture(), 0, "diffuseTex", sunShader, GL_TEXTURE_2D);
			UpdateShaderMatrices();

			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(sunShader->GetProgram(), "modelMatrix"), 1, false, model.values);
			glUniform4fv(glGetUniformLocation(sunShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

			glUniform3fv(glGetUniformLocation(sunShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

			nodeTex = n->GetTexture();
			glUniform1i(glGetUniformLocation(sunShader->GetProgram(), "useTexture"), nodeTex);

			n->Draw(*this); 
		}
		else
		{
			BindShader(simpleLitShader);
			SetShaderLight(*light);
			UpdateShaderMatrices();

			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(simpleLitShader->GetProgram(), "modelMatrix"), 1, false, model.values);
			glUniform4fv(glGetUniformLocation(simpleLitShader->GetProgram(), "colour"), 1, (float*)&n->GetColour());

			glUniform3fv(glGetUniformLocation(simpleLitShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

			nodeTex = n->GetTexture();
			glUniform1i(glGetUniformLocation(simpleLitShader->GetProgram(), "useTexture"), nodeTex);

			if (n->meshIsComplex) {
				for (int i = 0; i < shipMesh->GetSubMeshCount(); ++i)	
				{
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, shipMatTextures[i]);
					shipMesh->DrawSubMesh(i);
				}
			}
			else {
				SetTexture(n->GetTexture(), 0, "diffuseTex", simpleLitShader, GL_TEXTURE_2D);
				n->Draw(*this);
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
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	glDisable(GL_DEPTH_TEST);

	if (bloomEnabled) {	// draws bright frags to bufferBrightTex[1] for bloom
		//glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferBrightTex[1], 0);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		BindShader(bloomShader);
		//modelMatrix.ToIdentity();
		//viewMatrix.ToIdentity();
		//projMatrix.ToIdentity();
		textureMatrix.ToIdentity();
		UpdateShaderMatrices();
		//glDisable(GL_DEPTH_TEST);

		SetTexture(bufferBrightTex[0], 0, "diffuseTex", bloomShader, GL_TEXTURE_2D);
		processQuad->Draw();
		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glEnable(GL_DEPTH_TEST);
	}

	//glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferBrightTex[0], 0);
	//glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	BindShader(processShader);
	//modelMatrix.ToIdentity();
	//viewMatrix.ToIdentity();
	//projMatrix.ToIdentity();
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();

	//glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(processShader->GetProgram(), "sceneTex"), 0);

	if (bloomEnabled) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferBrightTex[0], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 0);
		glBindTexture(GL_TEXTURE_2D, bufferBrightTex[1]);
		processQuad->Draw();
	}

	for (int i = 0; i < POST_PASSES; ++i) {

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferBrightTex[1], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 0);
		glBindTexture(GL_TEXTURE_2D, bufferBrightTex[0]);
		processQuad->Draw();

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferBrightTex[0], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 1);
		glBindTexture(GL_TEXTURE_2D, bufferBrightTex[1]);
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

	SetTexture(bufferBrightTex[0], 0, "diffuseTex", processSceneShader, GL_TEXTURE_2D);

	SetTexture(bufferColourTex, 1, "differentTex", processSceneShader, GL_TEXTURE_2D);

	glUniform1i(glGetUniformLocation(processSceneShader->GetProgram(), "bloomEnabled"), bloomEnabled);

	processQuad->Draw();
}




void Renderer::DrawShadowScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BindShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), heightmapSize * Vector3(0.5f, 0.0f, 0.5f));	// POINT TOWARDS CENTRE OF TERRAIN

	projMatrix = Matrix4::Perspective(1, 30000, 1, 90);	// MODIFY SHADOW POINT LIGHT VARIABLES HERE
	shadowMatrix = projMatrix * viewMatrix;
	
	
	DrawNodeShadows(root);


	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	viewMatrix = camera->BuildViewMatrix();	// RESET VIEW AND PROJ MATRIX
	projMatrix = Matrix4::Perspective(1.0f, 25000.0f, (float)width / (float)height, 45.0f);
}


void Renderer::DrawNodeShadows(SceneNode* n) {
	if (n != terrain && n != orbitSunNode) {	// dont draw shadows for these

		if (n->GetMesh()) {
			modelMatrix = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			UpdateShaderMatrices();
			n->Draw(*this);
		}
	}

	for (vector<SceneNode*>::const_iterator i = n->GetChildIteratorStart(); i != n->GetChildIteratorEnd(); ++i) {
		DrawNodeShadows(*i);
	}
}

