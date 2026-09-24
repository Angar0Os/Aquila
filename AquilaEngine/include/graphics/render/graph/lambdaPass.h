#ifndef AQUILA_ENGINE_GRAPHICS_RENDER_GRAPH_LAMBDA_PASS_H
#define AQUILA_ENGINE_GRAPHICS_RENDER_GRAPH_LAMBDA_PASS_H
#pragma once

#include <functional>
#include <graphics/render/graph/pass.h>

namespace graphics::render::graph
{
	class LambdaPass final : public Pass
	{
	public:
		using ExecuteFn = std::function<void()>;

		LambdaPass(PassCreateInfo _createInfo, ExecuteFn _execute)
			: Pass(std::move(_createInfo))
			, m_execute(std::move(_execute))
		{
		}

		void Execute() override
		{
			if (m_execute)
			{
				m_execute();
			}
		}

	private:
		ExecuteFn m_execute;
	};
}

#endif //AQUILA_ENGINE_GRAPHICS_RENDER_GRAPH_LAMBDA_PASS_H