#include <memory>

#include <core/window.h>
#include <core/gpu/device.h>

#pragma comment(lib, "AquilaEngine_x64_Debug")

int main(int argc, char** argv)
{
	auto window = std::make_unique<core::Window>(core::WindowDesc {
		.appName = "Aquila - Restir Showdown",
		.windowSize = { 1080, 720 }
	});

	auto gpu = std::make_unique<core::gpu::Device>(*window);

	while (!window->ShouldClose())
	{
		window->PollEvents();
	}

	return 0;
}