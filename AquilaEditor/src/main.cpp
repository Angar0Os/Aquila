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

#include <imgui/context.h>
#include <imgui/imgui.h>

#include <core/debug/cpuProfiler.h>

#pragma comment(lib, "AquilaEngine_x64_Debug")

#define AQUILA_EDITOR

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
    bool showUI = true;

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

    mapper.Action("HideUI") = core::input::utils::E_KEYS::KEY_TAB;
    mapper.Action("HideUI") = std::function<void()>([&]() {
        showUI = !showUI;
        });

    mapper.Action("Quit") = core::input::utils::E_KEYS::KEY_ESCAPE;
    mapper.Action("Quit") = std::function<void()>([&]() {
        window->Close();
        });

    mapper.Action("NewTimeline") = core::input::utils::E_KEYS::KEY_N;

    mapper.Action("CameraLookLeft") = core::input::utils::E_KEYS::KEY_J;
    mapper.Action("CameraLookRight") = core::input::utils::E_KEYS::KEY_L;
    mapper.Action("CameraLookUp") = core::input::utils::E_KEYS::KEY_I;
    mapper.Action("CameraLookDown") = core::input::utils::E_KEYS::KEY_O;

    auto testScene = loaders::MeshLoader::LoadGLTF(
        *gpu,
        "assets/models/testSceneQuentin.glb",
        renderer->GetTextureLibrary(),
        renderer->GetMaterialLibrary()
    );

    auto helmet = loaders::MeshLoader::LoadGLTF(
        *gpu,
        "assets/models/DamagedHelmet.glb",
        renderer->GetTextureLibrary(),
        renderer->GetMaterialLibrary()
    );

    glm::vec3 sunDirection = glm::vec3(0.0f, 0.02f, 0.0f);
    graphics::render::GPULight sun
    {
        .position = glm::vec3(0.0f),
        .type = core::gpu::utils::ELightType::Sun,
        .direction = sunDirection,
        .color = glm::vec3(1.0f),
        .intensity = 1.0f
    };

    const glm::vec3 helmetPosition = glm::vec3(0.0f, 1.0f, 0.0f);

    graphics::render::GPULight greenLight
    {
        .position = glm::vec3(-4.0f, 1.0f, 0.0f),
        .type = core::gpu::utils::ELightType::Point,
        .direction = glm::vec3(0.0f),
        .radius = 0.0f,
        .color = glm::vec3(0.1f, 1.0f, 0.5f),
        .intensity = 0.5f
    };

    graphics::render::GPULight redLight
    {
        .position = glm::vec3(4.0f, 1.0f, 0.0f),
        .type = core::gpu::utils::ELightType::Point,
        .direction = glm::vec3(0.0f),
        .radius = 0.0f,
        .color = glm::vec3(1.0f, 0.15f, 0.35f),
        .intensity = 0.5f
    };

    graphics::render::GPULight cyanLight
    {
        .position = glm::vec3(0.0f, 1.0f, -4.0f),
        .type = core::gpu::utils::ELightType::Point,
        .direction = glm::vec3(0.0f),
        .radius = 0.0f,
        .color = glm::vec3(0.4f, 0.9f, 1.0f),
        .intensity = 0.5f
    };

    glm::mat4 planeTransform(1.0f);

    glm::mat4 helmetTransform =
        glm::translate(glm::mat4(1.0f), helmetPosition) *
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    glm::vec3 cameraPos = glm::vec3(-2.0f, 2.5f, 5.0f);
    glm::vec3 cameraTarget = helmetPosition;

    mapper.PushContext("Debug");

    auto previousTime = std::chrono::steady_clock::now();

       audioSystem->Play(
           audio::MusicInfo{
               .path = "assets/music/evoke_quentin.mp3",
               .baseVolume = 0.1f,
               .tempo = 95.0f
           }
       );

    constexpr float CameraSpeed = 5.0f;
    constexpr float CameraLookSpeed = 3.0f;

    while (!window->ShouldClose())
    {
        AQUILA_PROFILE_FRAME_BEGIN();

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

        renderer->PushLight(sun);
        renderer->PushLight(greenLight);
        renderer->PushLight(redLight);
        renderer->PushLight(cyanLight);

        renderer->PushMesh(testScene[1].get(), planeTransform);
        renderer->PushMesh(helmet[0].get(), helmetTransform);

        auto image = gpu->AcquireNextImage();

        if (!image)
        {
            AQUILA_PROFILE_FRAME_END();
            continue;
        }


        auto cmdBuf = gpu->AcquireCommandBuffer();

        cmdBuf->Record([&]() {
            AQUILA_PROFILE_SCOPE("Renderer::Render");
            renderer->Render(cmdBuf, *image);
#ifdef AQUILA_EDITOR
            if (showUI)
            {
                AQUILA_PROFILE_SCOPE("ImGui");
                imgui->BeginFrame(cmdBuf, image);
                ImGui::ShowDemoWindow();
                imgui->EndFrame(cmdBuf, image);
            }
#endif
            cmdBuf->TransitionImageLayout(
                *image,
                core::gpu::utils::EImageLayout::Present,
                false
            );
            });

        cmdBuf->Submit(*gpu);
        gpu->ReleaseCommandBuffer(cmdBuf);

        gpu->Present();

        AQUILA_PROFILE_FRAME_END();
    }

    return 0;
}