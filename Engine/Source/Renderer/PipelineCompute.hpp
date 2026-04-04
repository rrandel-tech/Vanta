#pragma once

#include "Core/Base.hpp"
#include "Renderer/Shader.hpp"
#include "Renderer/RenderCommandBuffer.hpp"

namespace Vanta {

	class PipelineCompute : public RefCounted
	{
	public:
		virtual void Begin(Ref<RenderCommandBuffer> renderCommandBuffer = nullptr) = 0;
		virtual void End() = 0;

		virtual Ref<Shader> GetShader() = 0;

		static Ref<PipelineCompute> Create(Ref<Shader> computeShader);
	};

}
