#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/MeshMaterial.h"
#include "../nclgl/MeshAnimation.h"

const int POST_PASSES = 10;	// more passes = stronger blur
#define SHADOWSIZE 2048

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {

	// Shaders
	reflectShader = new Shader("ReflectVertex.glsl", "/Coursework/CWReflectFragment.glsl");
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	npcShader = new Shader("/Coursework/CWLitSkinningVertex.glsl", "PerPixelFragment.glsl");
	processShader = new Shader("TexturedVertex.glsl", "ProcessFrag.glsl");
	processSceneShader = new Shader("TexturedVertex.glsl", "/Coursework/CWProcessSceneFrag.glsl");
	bloomShader = new Shader("TexturedVertex.glsl", "/Coursework/CWBloomFrag.glsl");
	shadowSceneShader = new Shader("ShadowSceneVert.glsl", "/Coursework/CWShadowSceneFrag.glsl");
	shadowShader = new Shader("ShadowVert.glsl", "ShadowFrag.glsl");
	simpleLitShader = new Shader("PerPixelVertex.glsl", "PerPixelFragment.glsl");
	sunShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !npcShader->LoadSuccess() || !processShader->LoadSuccess() || 
		!processSceneShader->LoadSuccess() || !bloomShader->LoadSuccess() || !shadowSceneShader->LoadSuccess() || 
		!shadowShader->LoadSuccess() || !simpleLitShader->LoadSuccess() || !sunShader->LoadSuccess()) {
		return;
	}



	// Terrain and Scene Graph
	quad = Mesh::GenerateQuad();
	heightMap = new HeightMap(TEXTUREDIR"/Coursework/TestHM2.png");
	heightmapSize = heightMap->GetHeightmapSize();

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

	root = new SceneNode();
	terrain = new SceneNode();
	terrain->SetMesh(heightMap);
	terrain->SetModelScale(Vector3(2.0, 2.0, 2.0));
	root->AddChild(terrain);

	waterRotate = 0.0f;
	waterCycle = 0.0f;



	// Camera and Sun Light
	camera = new Camera(0, 0, (heightmapSize * Vector3(1.0f, 5.0f, 1.0f)) + Vector3(heightmapSize.x / 2, 0, heightmapSize.x / 2), heightmapSize);
	camAutoHasStarted = false;
	projMatrix = Matrix4::Perspective(1.0f, 25000.0f, (float)width / (float)height, 45.0f);

	light = new Light(heightmapSize * Vector3(0.5f, 30.5f, 0.5f), Vector4(1, 1, 1, 1), heightmapSize.x * 4);
	light->SetInitialRadius();


	
	// Animated NPC Character (Crab)
	npc = Mesh::LoadFromMeshFile("/Coursework/Monster_Crab.msh");
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
	npcNode->SetTransform(Matrix4::Translation(heightmapSize * 2 * Vector3(0.6f, 0.4f, 0.45f)));
	npcNode->SetModelScale(Vector3(300.0f, 300.0f, 300.0f));
	npcNode->SetMesh(npc);
	npcNode->SetAniTexture(npcMatTextures);
	npcNode->SetAnimation(anim);
	root->AddChild(npcNode);




	// REMEMBER TO CHANGE BACK TO SHIP MESH, CHANGE ISCOMPLEX TO TRUE

	// Ship Meshes
	shipMesh = Mesh::LoadFromMeshFile("/Coursework/cliff.msh");
	shipMat = new MeshMaterial("/Coursework/Rock.mat");	// change to w/interior for submission, but loads slow
	shipTexture = SOIL_load_OGL_texture(TEXTUREDIR"/Coursework/muddy+terrain.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);

	/*for (int i = 0; i < shipMesh->GetSubMeshCount(); ++i)
	{
		const MeshMaterialEntry* matEntry = shipMat->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		shipMatTextures.emplace_back(texID);
	}*/

	for (int i = 1; i < 4; ++i)
	{
		SceneNode* s = new SceneNode(shipMesh, Vector4(1,1,1,1), false);
		s->SetTransform(Matrix4::Translation(heightmapSize * 2 * Vector3((0.62f / i) + 0.14f , 2.0f + (0.02f * i), (0.18f * i) + 0.15f)) *
		Matrix4::Rotation(30.0f * i * i, Vector3(0, 1, 0)));
		s->SetModelScale(Vector3(100.0f, 100.0f, 100.0f));
		s->SetBoundingRadius(50.0f);
		s->SetTexture(shipTexture);
		s->SetOriginalTransform();
		root->AddChild(s);
	}



	// Post Processing
	processQuad = Mesh::GenerateQuad();	
	
	//Scene Depth Texture
	glGenTextures(1, &bufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	//Bright Texture (For Bloom)
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

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bufferColourTex, 0);	
	GLenum buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, buffers);	// draw to bright tex and colour tex

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferBrightTex[0] || !bufferColourTex) return;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	bloomEnabled = false;
	blurEnabled = false;
	nvEnabled = false;



	// Shadows
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

	

	// Point lights for glowing orbs
	pointLights[0] = new Light(heightmapSize * Vector3(0.25f, 0.025f, 0.25f), Vector4(1, 1, 0, 1), heightmapSize.x / 2);
	pointLights[1] = new Light(heightmapSize * Vector3(1.75f, 0.025f, 1.75f), Vector4(0, 1, 1, 1), heightmapSize.x / 2);

	// Glowing orbs
	floatingGlowingOrb = new SceneNode(Mesh::LoadFromMeshFile("Sphere.msh"));
	floatingGlowingOrb->SetTransform(Matrix4::Translation(Vector3(0.25f, 3.0f, 0.25f) * heightmapSize));
	floatingGlowingOrb->SetModelScale(Vector3(250.0f, 250.0f, 250.0f));
	floatingGlowingOrb->SetColour(Vector4(1, 1, 0, 1));
	terrain->AddChild(floatingGlowingOrb);

	floatingGlowingOrb2 = new SceneNode(Mesh::LoadFromMeshFile("Sphere.msh"));
	floatingGlowingOrb2->SetTransform(Matrix4::Translation(Vector3(1.75f, 3.0f, 1.75f) * heightmapSize));
	floatingGlowingOrb2->SetModelScale(Vector3(250.0f, 250.0f, 250.0f));
	floatingGlowingOrb2->SetColour(Vector4(0, 1, 1, 1));
	terrain->AddChild(floatingGlowingOrb2);



	// sun orbiting around centre of terrain
	orbitSun = Mesh::LoadFromMeshFile("Sphere.msh");
	sunTex = SOIL_load_OGL_texture(TEXTUREDIR"/Coursework/2k_sun.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	SetTextureRepeating(sunTex, true);	// for planets, use other 2k textures from https://www.solarsystemscope.com/textures/ and use simpleLitShader

	Vector3 terrainCentrePos = Vector3(heightmapSize.x / 2, 0.0f, heightmapSize.z / 2) * terrain->GetModelScale().x;
	terrainCentreNode = new SceneNode();
	terrainCentreNode->SetTransform(Matrix4::Translation(terrainCentrePos));
	root->AddChild(terrainCentreNode);

	Vector3 sunRelativePosition = Vector3(12000.0f, 0.0f, 0.0f);
	orbit = new Orbit(0.0f, terrainCentrePos, sunRelativePosition + terrainCentrePos, 0.25f);
	orbitSunNode = new SceneNode(orbitSun);
	orbitSunNode->SetTransform(Matrix4::Translation(sunRelativePosition));
	orbitSunNode->SetModelScale(Vector3(500.0f, 500.0f, 500.0f));
	orbitSunNode->SetTexture(sunTex);
	orbitSunNode->SetColour(Vector4(1, 1, 0, 1));
	terrainCentreNode->AddChild(orbitSunNode);

	sceneTime = 0.0f;
	init = true;
}

Renderer::~Renderer(void) {
	delete reflectShader;
	delete skyboxShader;
	delete npcShader;
	delete processShader;
	delete processSceneShader;
	delete shadowSceneShader;
	delete shadowShader;
	delete simpleLitShader;
	delete sunShader;
	delete bloomShader;

	delete heightMap;
	delete light;
	delete pointLights[0];
	delete pointLights[1];
	delete camera;
	delete orbit;

	delete quad;
	delete processQuad;
	delete orbitSun;
	delete npc;
	delete anim;
	delete npcMat;
	delete shipMesh;
	delete shipMat;

	delete root;

	glDeleteTextures(2, bufferBrightTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(1, &bufferColourTex);
	glDeleteTextures(1, &shadowTex);
	glDeleteTextures(1, &sunTex);

	glDeleteTextures(1, &cubeMap);
	glDeleteTextures(1, &waterTex);
	glDeleteTextures(1, &earthTex);
	glDeleteTextures(1, &earthBump);
	glDeleteTextures(1, &shipTexture);
	glDeleteTextures(1, &nodeTex);

	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &processFBO);
	glDeleteFramebuffers(1, &shadowFBO);
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

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_N)) {
		nvEnabled = !nvEnabled;
	}

	if (!camAutoHasStarted) {	// start automatic camera movement
		camera->TriggerAuto();
		camAutoHasStarted = true;
	}

	ApplyFloatingMovement(root, 1);

	Vector3 orbitPos = orbit->CalculateRelativePosition();	// manage orbiting sun
	orbitSunNode->SetTransform(Matrix4::Translation(orbitPos));
	light->SetPosition(orbitPos + terrainCentreNode->GetTransform().GetPositionVector());	// light position = sun local pos + terrain centre global pos

	if (orbitPos.y < -750.0f) light->SetRadius(0.0f);
	else light->SetRadius(light->GetInitialRadius());	


	sceneTime += dt;
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
	if (bloomEnabled || blurEnabled || nvEnabled) {
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
			BindShader(shadowSceneShader);
			SetShaderLight(*light);
			
			glUniform3fv(glGetUniformLocation(shadowSceneShader->GetProgram(), "light2Pos"), 1, (float*)&(*pointLights[0]).GetPosition());
			glUniform4fv(glGetUniformLocation(shadowSceneShader->GetProgram(), "light2Colour"), 1, (float*)&(*pointLights[0]).GetColour());
			glUniform1f(glGetUniformLocation(shadowSceneShader->GetProgram(), "light2Radius"), (*pointLights[0]).GetRadius());

			glUniform3fv(glGetUniformLocation(shadowSceneShader->GetProgram(), "light3Pos"), 1, (float*)&(*pointLights[1]).GetPosition());
			glUniform4fv(glGetUniformLocation(shadowSceneShader->GetProgram(), "light3Colour"), 1, (float*)&(*pointLights[1]).GetColour());
			glUniform1f(glGetUniformLocation(shadowSceneShader->GetProgram(), "light3Radius"), (*pointLights[1]).GetRadius());

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
		else if (n == orbitSunNode || n == floatingGlowingOrb || n == floatingGlowingOrb2) {
			BindShader(sunShader);
			SetTexture(n->GetTexture(), 0, "diffuseTex", sunShader, GL_TEXTURE_2D);
			UpdateShaderMatrices();

			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(sunShader->GetProgram(), "modelMatrix"), 1, false, model.values);
			glUniform4fv(glGetUniformLocation(sunShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

			glUniform3fv(glGetUniformLocation(sunShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

			glUniform1i(glGetUniformLocation(sunShader->GetProgram(), "useTexture"), 0);	// sets to 0 - use colour not texture

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
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferBrightTex[1], 0);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		BindShader(bloomShader);
		textureMatrix.ToIdentity();
		UpdateShaderMatrices();
		SetTexture(bufferBrightTex[0], 0, "diffuseTex", bloomShader, GL_TEXTURE_2D);
		processQuad->Draw();
	}

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferBrightTex[0], 0);

	BindShader(processShader);
	textureMatrix.ToIdentity();
	UpdateShaderMatrices();


	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(processShader->GetProgram(), "sceneTex"), 0);

	if (bloomEnabled) {	// draws bright frags back to bufferBrightTex[0] ready for blurring
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferBrightTex[0], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(), "isVertical"), 0);
		glBindTexture(GL_TEXTURE_2D, bufferBrightTex[1]);
		processQuad->Draw();
	}

	if (bloomEnabled || blurEnabled) {
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
	glUniform1i(glGetUniformLocation(processSceneShader->GetProgram(), "nvEnabled"), nvEnabled);

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

