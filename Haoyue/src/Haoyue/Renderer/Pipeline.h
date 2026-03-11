#pragma once

#include "Haoyue/Core/Ref.h"

#include "Haoyue/Renderer/VertexBuffer.h"
#include "Haoyue/Renderer/Shader.h"
#include "Haoyue/Renderer/RenderPass.h"

namespace Haoyue {

	struct PipelineSpecification
	{
		Ref<Shader> Shader;
		VertexBufferLayout Layout;
		Ref<RenderPass> RenderPass;

		std::string DebugName;
	};

	class Pipeline : public RefCounted
	{
	public:
		virtual ~Pipeline() = default;

		virtual PipelineSpecification& GetSpecification() = 0;
		virtual const PipelineSpecification& GetSpecification() const = 0;

		virtual void Invalidate() = 0;

		virtual void Bind() = 0;

		static Ref<Pipeline> Create(const PipelineSpecification& spec);
	};

}
