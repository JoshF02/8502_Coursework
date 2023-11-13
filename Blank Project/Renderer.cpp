#include "Renderer.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {	// RENDERER CODE FROM T8 (TERRAIN)
	heightMap = new HeightMap(TEXTUREDIR"/Coursework/TestHM.png");
	camera = new Camera(-40, 270, Vector3());

	Vector3 dimensions = heightMap->GetHeightmapSize();
	camera->SetPosition(dimensions * Vector3(0.5f, 2, 0.5f));

	shader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");

	if (!shader->LoadSuccess()) return;

	terrainTex = SOIL_load_OGL_texture(TEXTUREDIR"/Coursework/soil_ground.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	if (!terrainTex) return;

	SetTextureRepeating(terrainTex, true);

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	init = true;


	root = new SceneNode();	// SCENE GRAPH FUNCTIONALITY

	SceneNode* terrain = new SceneNode();
	terrain->SetMesh(heightMap);
	root->AddChild(terrain);


	cubeMap = SOIL_load_OGL_cubemap(	// SKYBOX FUNCTIONALITY
		TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg",
		TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg",
		TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg", SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	if (!skyboxShader->LoadSuccess()) return;

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	quad = Mesh::GenerateQuad();

}

Renderer::~Renderer(void) {
	delete root;
	delete shader;
	delete camera;
	delete heightMap;

	delete quad;
	delete skyboxShader;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	root->Update(dt);
}

void Renderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	DrawSkybox();


	BindShader(shader);
	UpdateShaderMatrices();

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, terrainTex);

	//heightMap->Draw();
	DrawNode(root);
}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
		glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(), "modelMatrix"), 1, false, model.values);

		glUniform4fv(glGetUniformLocation(shader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

		glUniform1i(glGetUniformLocation(shader->GetProgram(), "useTexture"), 0);

		n->Draw(*this);
	}

	for (vector<SceneNode*>::const_iterator i = n->GetChildIteratorStart(); i != n->GetChildIteratorEnd(); ++i) {
		DrawNode(*i);
	}
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	quad->Draw();
	glDepthMask(GL_TRUE);
}

