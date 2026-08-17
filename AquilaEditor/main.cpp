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

#include <audio/audioSystem.h>

#include <iostream>

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

	std::cout << "[main] materialLibrary=" << &renderer->GetMaterialLibrary() << "\n";

	auto audioSystem = std::make_unique<audio::AudioSystem>();

	core::input::system::InputSystem input(window->GetHandle());
	core::input::system::InputMapper mapper;

	mapper.SetContext("Debug");

	mapper.Action("ToggleFullscreen") = core::input::utils::E_KEYS::KEY_F11;
	mapper.Action("ToggleFullscreen") = std::function<void()>([&window]
		{
			window->ToggleFullscreen();
		});

	mapper.PushContext("Debug");

	auto testScene = loaders::MeshLoader::LoadGLTF(*gpu, "assets/models/testSceneQuentin.glb", renderer->GetTextureLibrary(), renderer->GetMaterialLibrary());
	//auto helmet = loaders::MeshLoader::LoadGLTF(*gpu, "assets/models/DamagedHelmet.glb", renderer->GetTextureLibrary(), renderer->GetMaterialLibrary());
	//auto plane = loaders::MeshLoader::LoadGLTF(*gpu, "assets/models/sphere.glb", renderer->GetTextureLibrary(), renderer->GetMaterialLibrary());

	renderer->PushLight(glm::vec3(1.0f, 3.0f, 4.0f), glm::vec3(1.0f, 1.0f, 1.0f), 10.0f, 20.0f);

	/*glm::mat4 transform = glm::rotate(
		glm::mat4(1.0f),
		glm::radians(45.0f),
		glm::vec3(1.0f, 0.0f, 0.0f)
	);

	glm::mat4 planeTransform = glm::translate(
		glm::mat4(1.0f),
		glm::vec3(0.0f, -2.0f, 0.0f)
	);*/

	glm::mat4 transform(1.0f);

	// Plane (index 0) stays at identity; the rest get spread out along X so they don't overlap.
	std::vector<glm::mat4> meshTransforms(testScene.size(), glm::mat4(1.0f));

	constexpr float kSpacing = 2.5f;
	constexpr float kHeight = 1.0f;

	for (size_t i = 1; i < meshTransforms.size(); ++i)
	{
		meshTransforms[i] = glm::translate(
			glm::mat4(1.0f),
			glm::vec3(static_cast<float>(i) * kSpacing, kHeight, 0.0f)
		);
	}

	auto startTime = std::chrono::steady_clock::now();

	audioSystem->Play(audio::MusicInfo{
		.path = "assets/music/test.mp3",
		.baseVolume = 1.0f,
		.tempo = 130.0f
		});

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

		for (size_t i = 0; i < testScene.size(); ++i)
			renderer->PushMesh(testScene[i].get(), meshTransforms[i]);

		//renderer->PushMesh(helmet[0].get(), transform);
		//renderer->PushMesh(plane[0].get(), planeTransform);

		auto image = gpu->AcquireNextImage();
		if (!image)
		{
			continue;
		}

		renderer->Render(gpu->AcquireCommandBuffer(), *image);
		gpu->Present();
	}
}