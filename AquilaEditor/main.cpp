#include <memory>

#include <core/window.h>
#include <core/gpu/device.h>

#include <graphics/render/renderer.h>
#include <graphics/render/mesh.h>

#include <loaders/meshLoader.h>

#pragma comment(lib, "AquilaEngine_x64_Debug")

int main(int argc, char** argv)
{
	auto window = std::make_unique<core::Window>(core::WindowDesc {
		.appName = "Aquila - Restir Showdown",
		.windowSize = { 1080, 720 }
	});

	auto gpu = std::make_unique<core::gpu::Device>(*window);
	auto renderer = std::make_unique<graphics::render::Renderer>(*gpu);

	auto mesh = loaders::MeshLoader::LoadGLTF(*gpu, "assets/models/DamagedHelmet.glb", *renderer->materialLayout);
	glm::mat4 transform(1.0f);
	renderer->PushMesh(mesh.get(), transform);

	while (!window->ShouldClose())
	{
		auto image = gpu->AcquireNextImage();

		window->PollEvents();

		renderer->Render(gpu->AcquireCommandBuffer(), *image);

		gpu->Present();
	}

	return 0;
}