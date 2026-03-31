#pragma once

#include "Renderer/Pipeline.hpp"

namespace Vanta {

	class OpenGLPipeline : public Pipeline
	{
	public:
		OpenGLPipeline(const PipelineSpecification& spec);
		virtual ~OpenGLPipeline();

		virtual PipelineSpecification& GetSpecification() { return m_Specification; }
		virtual const PipelineSpecification& GetSpecification() const { return m_Specification; }

		virtual void Invalidate() override;
		virtual void SetUniformBuffer(Ref<UniformBuffer> uniformBuffer, uint32_t binding, uint32_t set = 0) override {}

		virtual void Bind() override;
	private:
		PipelineSpecification m_Specification;
		uint32_t m_VertexArrayRendererID = 0;
	};

}