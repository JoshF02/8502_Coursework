#include "SpaceRenderer.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Light.h"
const int LIGHT_NUM = 100;

SpaceRenderer::SpaceRenderer(Window& parent) : OGLRenderer(parent) {
	sphere = Mesh::LoadFromMeshFile("Sphere.msh");
	quad = Mesh::GenerateQuad();

	earthTex = SOIL_load_OGL_texture(TEXTUREDIR"/Coursework/2k_moon.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	earthBump = SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);

	cubeMap = SOIL_load_OGL_cubemap(
		TEXTUREDIR"/Coursework/sb_right.png", TEXTUREDIR"/Coursework/sb_left.png",
		TEXTUREDIR"/Coursework/sb_top.png", TEXTUREDIR"/Coursework/sb_bot.png",
		TEXTUREDIR"/Coursework/sb_front.png", TEXTUREDIR"/Coursework/sb_back.png", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	camera = new Camera(-8.8f, 33.5f, Vector3(5140.0f, 1275.0f, 7700.0f));

	pointLights = new Light[LIGHT_NUM];

	radius = 2500.0f;
	float lightRadius = radius + 150.0f;

	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];

		float angleS = DegToRad(rand() % 360);
		float angleT = DegToRad(rand() % 360);
		l.SetPosition(Vector3(lightRadius * cos(angleS) * sin(angleT), lightRadius * sin(angleS) * sin(angleT), lightRadius * cos(angleT)));

		l.SetColour(Vector4(0, 0.5f + (float)(rand() / (float)RAND_MAX), 0.5f + (float)(rand() / (float)RAND_MAX), 1));
		l.SetRadius(250.0f + (rand() % 250));
		l.SetInitialRadius();
	}

	sceneShader = new Shader("BumpVertex.glsl", "BufferFragment.glsl");
	pointlightShader = new Shader("PointlightVertex.glsl", "PointlightFrag.glsl");
	combineShader = new Shader("CombineVert.glsl", "CombineFrag.glsl");
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	sunShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");

	if (!sceneShader->LoadSuccess() || !pointlightShader->LoadSuccess() || !combineShader->LoadSuccess() || !skyboxShader->LoadSuccess() || !sunShader->LoadSuccess()) return;

	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &pointLightFBO);

	GLenum buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

	GenerateScreenTexture(bufferDepthTex, true);
	GenerateScreenTexture(bufferColourTex);
	GenerateScreenTexture(bufferNormalTex);
	GenerateScreenTexture(lightDiffuseTex);
	GenerateScreenTexture(lightSpecularTex);

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, bufferNormalTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return;

	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightDiffuseTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, lightSpecularTex, 0);
	glDrawBuffers(2, buffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) return;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	sunOrbit = new Orbit(0.0f, Vector3(0,0,0), Vector3(5000.0f, 0.0f, 0.0f), 0.05f);
	sunTex = SOIL_load_OGL_texture(TEXTUREDIR"/Coursework/2k_sun.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	sunLight = new Light(Vector3(0,0,0), Vector4(1,0.75,0.5,1), 15000.0f);

	sceneTime = 0.0f;
	init = true;
}

SpaceRenderer::~SpaceRenderer(void) {
	delete sceneShader;
	delete combineShader;
	delete pointlightShader;

	delete camera;
	delete sphere;
	delete quad;
	delete[] pointLights;

	glDeleteTextures(1, &bufferColourTex);
	glDeleteTextures(1, &bufferNormalTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteTextures(1, &lightDiffuseTex);
	glDeleteTextures(1, &lightSpecularTex);

	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &pointLightFBO);
}

void SpaceRenderer::GenerateScreenTexture(GLuint& into, bool depth) {
	glGenTextures(1, &into);
	glBindTexture(GL_TEXTURE_2D, into);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint format = depth ? GL_DEPTH_COMPONENT24 : GL_RGBA8;
	GLuint type = depth ? GL_DEPTH_COMPONENT : GL_RGBA;

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, type, GL_UNSIGNED_BYTE, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void SpaceRenderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);

	sceneTime += dt;

	Vector3 orbitPos = sunOrbit->CalculateRelativePosition();
	sunTransform = Matrix4::Translation(orbitPos) * 
		Matrix4::Scale(Vector3(radius / 10, radius / 10, radius / 10)) * 
		Matrix4::Rotation((sceneTime * 50.0f), Vector3(0, 1, 0));

	sunLight->SetPosition(orbitPos + Vector3(0, 0, -500.0f));

	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		l.SetRadius(l.GetInitialRadius() * (sin(sceneTime + l.GetInitialRadius()) + 4) / 4);	// glowing effect
	}
}

void SpaceRenderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	FillBuffers();
	DrawPointLights();
	CombineBuffers();
}

void SpaceRenderer::FillBuffers() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	viewMatrix = camera->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 30000.0f, (float)width / (float)height, 45.0f);

	DrawSkybox();

	BindShader(sceneShader);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "bumpTex"), 1);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	modelMatrix = Matrix4::Scale(Vector3(radius, radius, radius));

	UpdateShaderMatrices();

	sphere->Draw();



	BindShader(sunShader);
	glUniform1i(glGetUniformLocation(sunShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sunTex);

	glUniform4fv(glGetUniformLocation(sunShader->GetProgram(), "nodeColour"), 1, (float*)&Vector4(1,1,1,1));
	glUniform1i(glGetUniformLocation(sunShader->GetProgram(), "useTexture"), 100);

	modelMatrix = sunTransform;
	UpdateShaderMatrices();
	sphere->Draw();



	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SpaceRenderer::DrawPointLights() {
	glBindFramebuffer(GL_FRAMEBUFFER, pointLightFBO);
	BindShader(pointlightShader);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glBlendFunc(GL_ONE, GL_ONE);
	glCullFace(GL_FRONT);
	glDepthFunc(GL_ALWAYS);
	glDepthMask(GL_FALSE);

	glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "depthTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);

	glUniform1i(glGetUniformLocation(pointlightShader->GetProgram(), "normTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferNormalTex);

	glUniform3fv(glGetUniformLocation(pointlightShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform2f(glGetUniformLocation(pointlightShader->GetProgram(), "pixelSize"), 1.0f / width, 1.0f / height);

	Matrix4 invViewProj = (projMatrix * viewMatrix).Inverse();
	glUniformMatrix4fv(glGetUniformLocation(pointlightShader->GetProgram(), "inverseProjView"), 1, false, invViewProj.values);

	UpdateShaderMatrices();
	for (int i = 0; i < LIGHT_NUM; ++i) {
		Light& l = pointLights[i];
		SetShaderLight(l);
		sphere->Draw();
	}

	SetShaderLight(*sunLight);
	sphere->Draw();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);

	glDepthMask(GL_TRUE);

	glClearColor(0.2f, 0.2f, 0.2f, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SpaceRenderer::CombineBuffers() {
	BindShader(combineShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "diffuseLight"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, lightDiffuseTex);

	glUniform1i(glGetUniformLocation(combineShader->GetProgram(), "specularLight"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, lightSpecularTex);

	quad->Draw();
}

void SpaceRenderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	quad->Draw();
	glDepthMask(GL_TRUE);
}