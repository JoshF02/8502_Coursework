#include "../NCLGL/window.h"
#include "Renderer.h"
#include "SpaceRenderer.h"
#include "../nclgl/OGLRenderer.h"

int main()	{
	Window w("Coursework Scene", 1280, 720, false);	// bool sets fullscreen

	if(!w.HasInitialised()) {
		return -1;
	}
	
	OGLRenderer* renderer = new Renderer(w);	// careful with performance - renderer created on heap not stack
	if(!(*renderer).HasInitialised()) {
		return -1;
	}

	bool useSpaceRenderer = false;

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);


	while(w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)) {
		(*renderer).UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
		(*renderer).RenderScene();
		(*renderer).SwapBuffers();

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_Z)) {	// toggle renderer with z key
			if (!useSpaceRenderer) {
				delete renderer;
				renderer = new SpaceRenderer(w);
				if (!(*renderer).HasInitialised()) {
					return -1;
				}
			}
			else {
				delete renderer;
				renderer = new Renderer(w);
				if (!(*renderer).HasInitialised()) {
					return -1;
				}
			}
			useSpaceRenderer = !useSpaceRenderer;
		}
		

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
			Shader::ReloadAllShaders();
		}
	}
	delete renderer;
	return 0;
}