#include <memory>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <iostream>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <core/window.h>
#include <core/gpu/device.h>
#include <core/gpu/commandBuffer.h>

#include <core/input/system/inputSystem.h>
#include <core/input/system/inputMapper.h>


#include <graphics/render/renderer.h>
#include <graphics/render/mesh.h>
#include <graphics/render/light.h>

#include <loaders/meshLoader.h>

#include <audio/audioSystem.h>

#include <demo/syncTimeline.h>

#include <imgui/context.h>
#include <imgui/imgui.h>

#pragma comment(lib, "AquilaEngine_x64_Debug")

int main(int argc, char** argv)
{
    bool cameraDebugMode = true;
 
    auto window = std::make_unique<core::Window>(
        core::WindowDesc{
            .appName = "Aquila - Restir Showdown",
            .windowSize = { 1080, 720 },
            .isFullscreen = false,
            .isResizable = true,
            .exclusiveFullscreen = false
        }
    );

    auto gpu = std::make_unique<core::gpu::Device>(*window);
    auto renderer = std::make_unique<graphics::render::Renderer>(*gpu);
    auto audioSystem = std::make_unique<audio::AudioSystem>();
    auto imgui = std::make_unique<imgui::Context>(*window, *gpu);

    core::input::system::InputSystem input(window->GetHandle());
    core::input::system::InputMapper mapper;

    mapper.SetContext("Debug");

    mapper.Action("ToggleFullscreen") = core::input::utils::E_KEYS::KEY_F11; 
    mapper.Action("ToggleFullscreen") = std::function<void()>([&window]() {
         window->ToggleFullscreen();
    });

    mapper.Action("ToggleMusic") = core::input::utils::E_KEYS::KEY_SPACE;
    mapper.Action("ToggleMusic") = std::function<void()>([&]() {
         audioSystem->Pause();
    });

    mapper.Action("RestartMusic") = core::input::utils::E_KEYS::KEY_R;
    mapper.Action("RestartMusic") = std::function<void()>([&]() {
          audioSystem->Restart();
    });

    mapper.Action("PreviousRows") = core::input::utils::E_KEYS::KEY_C;
    mapper.Action("PreviousRows") = std::function<void()>([&]() {
          audioSystem->SeekRows(-10.0);
    });

    mapper.Action("NextRows") = core::input::utils::E_KEYS::KEY_V;
    mapper.Action("NextRows") = std::function<void()>([&]() {
          audioSystem->SeekRows(10.0);
    });

    mapper.Action("NewTimeline") = core::input::utils::E_KEYS::KEY_N;

    mapper.Action("CameraLookLeft") = core::input::utils::E_KEYS::KEY_J;
    mapper.Action("CameraLookRight") = core::input::utils::E_KEYS::KEY_L;
    mapper.Action("CameraLookUp") = core::input::utils::E_KEYS::KEY_I;
    mapper.Action("CameraLookDown") = core::input::utils::E_KEYS::KEY_O;

    demo::SyncTimeline timeline;
    const std::string timelinePath = "assets/demo.timeline";

    auto testScene = loaders::MeshLoader::LoadGLTF(
            *gpu,
            "assets/models/testSceneQuentin.glb",
            renderer->GetTextureLibrary(),
            renderer->GetMaterialLibrary()
        );

    glm::vec3 sunDirection = glm::normalize(glm::vec3(-0.1f, -0.9f, -0.3f));
    graphics::render::GPULight sun
    {
        .position = glm::vec3(0.0f),
        .type = core::gpu::utils::ELightType::Sun,
        .direction = sunDirection,
        .color = glm::vec3(1.0f),
        .intensity = 1.0f
    };

    graphics::render::GPULight light
    {
        .position = glm::vec3(3.0f, 5.5f, 3.0f),
        .type = core::gpu::utils::ELightType::Point,
        .direction = glm::vec3(0.0f),
        .radius = 0.0f,
        .color = glm::vec3(1.0f, 0.2f, 1.0f),
        .intensity = 100.0f
    };


    std::vector<glm::mat4> meshTransforms(testScene.size(), glm::mat4(1.0f));

    glm::vec3 cameraPos = glm::vec3(0.0f, 4.0f, 10.0f);
    glm::vec3 cameraTarget = glm::vec3(5.3603, -1.4502, -2.02124);

    mapper.Action("NewTimeline") = std::function<void()>([&]() {
        if (!input.IsKeyHeld(
            core::input::utils::E_KEYS::KEY_LEFT_CTRL))
        {
            return;
        }

        std::vector<std::string> meshTrackNames;
        std::vector<glm::vec3> meshPositions;

        meshTrackNames.reserve(testScene.size());
        meshPositions.reserve(testScene.size());

        for (size_t i = 0; i < testScene.size(); ++i)
        {
            meshTrackNames.push_back("mesh." + std::to_string(i)); 
            meshPositions.push_back(glm::vec3(meshTransforms[i][3]));
        }

        timeline.CreateNewFile(
            timelinePath,
            meshTrackNames,
            meshPositions,
            cameraPos,
            cameraTarget,
            sunDirection
        );

        std::cout << "Created timeline: " << timelinePath << "\n";
    });


    mapper.Action("DumpCamera") = core::input::utils::E_KEYS::KEY_Q;

    mapper.Action("DumpCamera") = std::function<void()>([&]() {
        const float row = static_cast<float>(audioSystem->GetCurrentRow());

        timeline.AddKeyframeAndSave("camera.pos", row, glm::vec4(cameraPos, 0.0f));
        timeline.AddKeyframeAndSave("camera.target", row, glm::vec4(cameraTarget, 0.0f));
    });

    mapper.PushContext("Debug");

    if (std::filesystem::exists(timelinePath))
    {
        if (timeline.LoadFromFile(timelinePath))
        {
            std::cout << "Timeline loaded: " << timelinePath << "\n";
        }
    }
    else
    {
        std::cout << "No timeline found.\n" << "Press Ctrl+N to create: " << timelinePath << "\n";
    }

   

    auto previousTime = std::chrono::steady_clock::now();

 /*   audioSystem->Play(
        audio::MusicInfo{
            .path = "assets/music/evoke_quentin.mp3",
            .baseVolume = 0.1f,
            .tempo = 95.0f
        }
    );*/

    constexpr float CameraSpeed = 5.0f;
    constexpr float CameraLookSpeed = 3.0f;

    while (!window->ShouldClose())
    {
        window->PollEvents();

        input.Update();
        mapper.Update(input);

        if (window->WasFramebufferResized())
        {
            gpu->RequestResize();
        }

        const auto now = std::chrono::steady_clock::now();
        const float deltaTime = std::chrono::duration<float>(now - previousTime).count();
        previousTime = now;

        const float row = static_cast<float>(audioSystem->GetCurrentRow());

        timeline.ReloadIfChanged();

        if (!cameraDebugMode)
        {
            const glm::vec4 timelineCameraPos = timeline.Evaluate("camera.pos", row);
            const glm::vec4 timelineCameraTarget = timeline.Evaluate("camera.target", row);

            cameraPos = glm::vec3(timelineCameraPos);
            cameraTarget = glm::vec3(timelineCameraTarget);
        }

        glm::vec3 cameraForward = glm::normalize(cameraTarget - cameraPos);
        glm::vec3 cameraRight = glm::normalize(glm::cross(cameraForward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 cameraMove(0.0f);

        if (input.IsKeyHeld(core::input::utils::E_KEYS::KEY_UP))
        {
            cameraMove += cameraForward;
        }

        if (input.IsKeyHeld(core::input::utils::E_KEYS::KEY_DOWN))
        {
            cameraMove -= cameraForward;
        }

        if (input.IsKeyHeld(core::input::utils::E_KEYS::KEY_RIGHT))
        {
            cameraMove += cameraRight;
        }

        if (input.IsKeyHeld(core::input::utils::E_KEYS::KEY_LEFT))
        {
            cameraMove -= cameraRight;
        }

        if (input.IsKeyHeld(core::input::utils::E_KEYS::KEY_RIGHT_SHIFT))
        {
            cameraMove.y += 1.0f;
        }

        if (input.IsKeyHeld(core::input::utils::E_KEYS::KEY_RIGHT_ALT))
        {
            cameraMove.y -= 1.0f;
        }

        if (glm::length(cameraMove) > 0.0f)
        {
            cameraMove = glm::normalize(cameraMove) * CameraSpeed * deltaTime;
            cameraPos += cameraMove;
        }

        glm::vec3 cameraLookMove(0.0f);

        if (input.IsKeyHeld(core::input::utils::E_KEYS::KEY_J))
        {
            cameraLookMove -= cameraRight;
        }

        if (input.IsKeyHeld(core::input::utils::E_KEYS::KEY_L))
        {
            cameraLookMove += cameraRight;
        }

        if (input.IsKeyHeld(core::input::utils::E_KEYS::KEY_I))
        {
            cameraLookMove += glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (input.IsKeyHeld(core::input::utils::E_KEYS::KEY_K))
        {
            cameraLookMove -= glm::vec3(0.0f, 1.0f, 0.0f);
        }

        if (glm::length(cameraLookMove) > 0.0f)
        {
            cameraLookMove = glm::normalize(cameraLookMove) * CameraLookSpeed * deltaTime;
            cameraTarget += cameraLookMove;
        }

        renderer->SetCamera(cameraPos);
        renderer->SetCameraTarget(cameraTarget);

        for (size_t i = 0; i < testScene.size(); ++i)
        {
            const std::string trackName = "mesh." + std::to_string(i) + ".pos";
            const glm::vec4 position = timeline.Evaluate(trackName, row);

            meshTransforms[i] = glm::translate(glm::mat4(1.0f), glm::vec3(position));
        }

        renderer->PushLight(sun);
        renderer->PushLight(light);

        for (size_t i = 0; i < testScene.size(); ++i)
        {
            renderer->PushMesh(testScene[i].get(), meshTransforms[i]);
        }

        auto image = gpu->AcquireNextImage();

        if (!image)
        {
            continue;
        }


        auto cmdBuf = gpu->AcquireCommandBuffer();

        cmdBuf->Record([&]() {
            renderer->Render(cmdBuf, *image); 

            imgui->BeginFrame(cmdBuf, image);
            ImGui::ShowDemoWindow();
            imgui->EndFrame(cmdBuf, image); 

            cmdBuf->TransitionImageLayout(
                *image,
                core::gpu::utils::EImageLayout::ColorAttachment,
                core::gpu::utils::EImageLayout::Present,
                false
            );
        }); 

        cmdBuf->Submit(*gpu);
        gpu->ReleaseCommandBuffer(cmdBuf);

        gpu->Present();
    }

    return 0;
}