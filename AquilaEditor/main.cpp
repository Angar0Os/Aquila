#include <memory>

#include <core/window.h>
#include <core/gpu/device.h>

#include <graphics/render/renderer.h>
#include <graphics/render/mesh.h>

#include <loaders/meshLoader.h>

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

#pragma comment(lib, "AquilaEngine_x64_Debug")

int main(int argc, char** argv)
{
	auto window = std::make_unique<core::Window>(core::WindowDesc {
		.appName = "Aquila - Restir Showdown",
		.windowSize = { 1080, 720 }
	});

	auto gpu = std::make_unique<core::gpu::Device>(*window);
	auto renderer = std::make_unique<graphics::render::Renderer>(*gpu);

	auto sphere = loaders::MeshLoader::LoadGLTF(*gpu, "assets/models/sphere.glb", *renderer->materialLayout);
	auto orbiter = loaders::MeshLoader::LoadGLTF(*gpu, "assets/models/DamagedHelmet.glb", *renderer->materialLayout); 

	auto startTime = std::chrono::steady_clock::now();

	while (!window->ShouldClose())
	{
		float time = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();

		glm::mat4 centerTransform(1.0f);
		renderer->PushMesh(sphere.get(), centerTransform);

		float radius = 3.0f;
		float speed = 1.0f;
		glm::mat4 orbitTransform = glm::translate(
			glm::mat4(1.0f),
			glm::vec3(radius * cos(time * speed), 0.0f, radius * sin(time * speed))
		);
		orbitTransform = glm::scale(orbitTransform, glm::vec3(0.5f)); 
		renderer->PushMesh(orbiter.get(), orbitTransform);

		auto image = gpu->AcquireNextImage();

		window->PollEvents();

		renderer->Render(gpu->AcquireCommandBuffer(), *image);

		gpu->Present();
	}

	return 0;
}