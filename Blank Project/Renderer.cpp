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

	if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess()) return;

	heightmapSize = heightMap->GetHeightmapSize();

	camera = new Camera(-45.0f, 0.0f, heightmapSize * Vector3(0.5f, 5.0f, 0.5f), heightmapSize);
	light = new Light(heightmapSize * Vector3(0.5f, 30.5f, 0.5f), Vector4(1, 1, 1, 1), heightmapSize.x * 2);	// POINT LIGHT

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
	//npcShader = new Shader("SkinningVertex.glsl", "TexturedFragment.glsl");
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





	//matShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");	// replaced with simplelitshader / sunshader
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

	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Sphere.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cylinder.msh"));
	sceneMeshes.emplace_back(Mesh::LoadFromMeshFile("Cone.msh"));

	sceneTransforms.resize(3);

	sceneTime = 0.0f;

	//shadowMeshesNode = new SceneNode(sceneMeshes[0]);
	shadowMeshesNode = new SceneNode(NULL, Vector4(1,1,1,1), sceneMeshes);
	terrain->AddChild(shadowMeshesNode);	// later can change to individual nodes for each




	// sun orbiting around centre of terrain
	orbitSun = Mesh::LoadFromMeshFile("Sphere.msh");
	
	Vector3 terrainCentrePos = Vector3(heightmapSize.x / 2, 0.0f, heightmapSize.z / 2);
	terrainCentreNode = new SceneNode();
	terrainCentreNode->SetTransform(Matrix4::Translation(terrainCentrePos));
	root->AddChild(terrainCentreNode);

	Vector3 sunRelativePosition = Vector3(5000.0f, 0.0f, 0.0f);
	orbitSunNode = new SceneNode(orbitSun);
	orbitSunNode->SetTransform(Matrix4::Translation(sunRelativePosition));
	orbitSunNode->SetModelScale(Vector3(500.0f, 500.0f, 500.0f));
	orbitSunNode->SetTexture(earthTex);
	terrainCentreNode->AddChild(orbitSunNode);

	orbit = new Orbit(0.0f, terrainCentrePos, sunRelativePosition + terrainCentrePos, 0.05f);

	init = true;
}

Renderer::~Renderer(void) {
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

	//delete matShader;
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

	//root->Update(dt);	// moved to bottom

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_P)) {
		postEnabled = true;
	}

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_O)) {
		postEnabled = false;
	}

	// MOVE POINT LIGHT AROUND - FOR SHADOW TESTING, LATER ADD MULTIPLE POINT LIGHTS TO USE FOR SHADOWS INSTEAD (so whole terrain isnt shadowed)
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

	for (int i = 0; i < shadowMeshesNode->GetMeshes().size(); ++i) {
		Vector3 t = Vector3(-10 + (5 * i), 2.0f + sin(sceneTime * i), 0);
		sceneTransforms[i] = Matrix4::Translation(Vector3(4500.0f, 400.0f, 4500.0f)) *
			Matrix4::Scale(Vector3(100.0f, 100.0f, 100.0f)) *
			Matrix4::Translation(t) * Matrix4::Rotation(sceneTime * 10 * (i+1), Vector3(1, 0, 0));
	}

	// ORBITING LIGHT MANAGEMENT
	Vector3 orbitPos = orbit->CalculateRelativePosition();
	orbitSunNode->SetTransform(Matrix4::Translation(orbitPos));
	light->SetPosition(orbitPos + terrainCentreNode->GetTransform().GetPositionVector());	// light position = sun local pos + terrain centre global pos

	if (orbitPos.y < -750.0f) light->SetRadius(0.0f);
	else light->SetRadius(heightmapSize.x * 2);	// can maybe reduce to just heightmapSize.x

	root->Update(dt);
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
		DrawShadowScene();	// note - shadows arent drawn on water
		DrawNode(root);
		DrawWater();
	}
}


void Renderer::DrawScene() {	// POSTPROCESSING FUNCTION, USING FOR TESTING
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
	SetShaderLight(*light);

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
	if (n->GetMesh() != NULL || !n->GetMeshes().empty())
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
		else if (n == shadowMeshesNode) {
			BindShader(simpleLitShader);	// use simple shader (no bump maps) so that shadows arent drawn on the objects
			SetShaderLight(*light);

			glUniform3fv(glGetUniformLocation(simpleLitShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());
			SetTexture(earthTex, 0, "diffuseTex", simpleLitShader, GL_TEXTURE_2D);
			textureMatrix.ToIdentity();

			// DRAW OBJECTS - make these child nodes instead
			for (int i = 0; i < n->GetMeshes().size(); ++i) {
				modelMatrix = sceneTransforms[i];
				UpdateShaderMatrices();
				sceneMeshes[i]->Draw();
			}
		}
		else if (n == orbitSunNode) {
			BindShader(sunShader);
			//SetShaderLight(*light);
			SetTexture(n->GetTexture(), 0, "diffuseTex", sunShader, GL_TEXTURE_2D);	// for texture uncomment this and comment the texture bind below
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
			//SetTexture(n->GetTexture(), 0, "diffuseTex", matShader, GL_TEXTURE_2D);	// for texture uncomment this and comment the texture bind below
			UpdateShaderMatrices();

			Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
			glUniformMatrix4fv(glGetUniformLocation(simpleLitShader->GetProgram(), "modelMatrix"), 1, false, model.values);
			glUniform4fv(glGetUniformLocation(simpleLitShader->GetProgram(), "colour"), 1, (float*)&n->GetColour());

			glUniform3fv(glGetUniformLocation(simpleLitShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

			nodeTex = n->GetTexture();
			glUniform1i(glGetUniformLocation(simpleLitShader->GetProgram(), "useTexture"), nodeTex);

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

	projMatrix = Matrix4::Perspective(1, 10000, 1, 90);	// MODIFY SHADOW POINT LIGHT VARIABLES HERE
	shadowMatrix = projMatrix * viewMatrix;
	
	// REMOVING TERRAIN DRAWING HERE STOPS TERRAIN FROM DRAWING SHADOWS (shadows still drawn on it though)
	//modelMatrix = terrain->GetWorldTransform() * Matrix4::Scale(terrain->GetModelScale());	// DRAW TERRAIN
	//UpdateShaderMatrices();
	//terrain->Draw(*this);

	/*for (int i = 0; i < shadowMeshesNode->GetMeshes().size(); ++i) {	// DRAW OBJECTS
		modelMatrix = sceneTransforms[i];
		UpdateShaderMatrices();
		sceneMeshes[i]->Draw();
	}*/
	DrawNodeShadows(root);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	viewMatrix = camera->BuildViewMatrix();	// RESET VIEW AND PROJ MATRIX
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
}


void Renderer::DrawNodeShadows(SceneNode* n) {
	if (n != terrain && n != orbitSunNode) {	// dont draw shadows for these

		if (n->GetMesh()) {
			modelMatrix = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());	// DRAW TERRAIN
			UpdateShaderMatrices();
			n->Draw(*this);
		}

		if (!n->GetMeshes().empty()) {
			for (int i = 0; i < sceneMeshes.size(); ++i) {	// DRAW SHADOW OBJECTS - REPLACE WITH INDIVIDUAL NODES LATER
				modelMatrix = sceneTransforms[i];
				UpdateShaderMatrices();
				sceneMeshes[i]->Draw();
			}
		}
	}

	for (vector<SceneNode*>::const_iterator i = n->GetChildIteratorStart(); i != n->GetChildIteratorEnd(); ++i) {
		DrawNodeShadows(*i);
	}
}

