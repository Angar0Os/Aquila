#include <memory>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <core/window.h>
#include <core/gpu/device.h>

#include <core/input/system/inputSystem.h>
#include <core/input/system/inputMapper.h>

#include <graphics/render/renderer.h>
#include <graphics/render/mesh.h>

#include <loaders/meshLoader.h>

#include <chrono>

#pragma comment(lib, "AquilaEngine_x64_Debug")

int main(int argc, char** argv)
{
	auto window = std::make_unique<core::Window>(core::WindowDesc{
		.appName = "Aquila - Restir Showdown",
		.windowSize = { 1080, 720 },
		.isFullscreen = false,
		.isResizable = true,
		.exclusiveFullscreen = false
	});

	auto gpu = std::make_unique<core::gpu::Device>(*window);
	auto renderer = std::make_unique<graphics::render::Renderer>(*gpu);

	core::input::system::InputSystem input(window->GetHandle());
	core::input::system::InputMapper mapper;

	mapper.SetContext("Debug");

	mapper.Action("ToggleFullscreen") = core::input::utils::E_KEYS::KEY_F11;
	mapper.Action("ToggleFullscreen") = std::function<void()>([&window]
		{
			window->ToggleFullscreen();
		});

	mapper.PushContext("Debug");

	//auto scene = loaders::MeshLoader::LoadGLTFScene(*gpu, "assets/models/testSceneQuentin.glb", *renderer->materialLayout);
	auto helmet = loaders::MeshLoader::LoadGLTF(*gpu, "assets/models/DamagedHelmet.glb", *renderer->materialLayout);
	auto plane = loaders::MeshLoader::LoadGLTF(*gpu, "assets/models/sphere.glb", *renderer->materialLayout);

	renderer->PushLight(glm::vec3(1.0f, 1.5f, 4.0f), glm::vec3(1.0f, 0.0f, 0.0f), 10.0f, 20.0f);

	glm::mat4 transform = glm::rotate(
		glm::mat4(1.0f),
		glm::radians(45.0f),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);

	glm::mat4 planeTransform = glm::translate(
		glm::mat4(1.0f),
		glm::vec3(0.0f, -2.0f, 0.0f)
	);

	//glm::mat4 transform(0.0f);

	auto startTime = std::chrono::steady_clock::now();

	while (!window->ShouldClose())
	{
		window->PollEvents();

		input.Update();
		mapper.Update(input);

		if (window->WasFramebufferResized())
		{
			gpu->RequestResize();
		}

		float time = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();

		float angle = time * 0.5f;
		glm::vec3 sunDir = glm::normalize(glm::vec3(
			cos(angle),
			-0.6f,
			sin(angle)
		));

		renderer->SetSunDirection(sunDir);
		
		/*for (auto& obj : scene)
			renderer->PushMesh(obj.mesh.get(), obj.worldTransform);*/

		renderer->PushMesh(helmet.get(), transform);
		renderer->PushMesh(plane.get(), planeTransform);

		auto image = gpu->AcquireNextImage();
		if (!image)
		{
			continue;
		}

		renderer->Render(gpu->AcquireCommandBuffer(), *image);
		gpu->Present();
	}
}