#pragma once

#include "Haoyue/Core/Ref.h"

#include "Haoyue/Renderer/VertexBuffer.h"
#include "Haoyue/Renderer/Shader.h"
#include "Haoyue/Renderer/RenderPass.h"
#include "Haoyue/Renderer/UniformBuffer.h"

namespace Haoyue {

	enum class PrimitiveTopology
	{
		None = 0,
		Points,
		Lines,
		Triangles,
		LineStrip,
		TriangleStrip,
		TriangleFan
	};

	struct PipelineSpecification
	{
		Ref<Shader> Shader;
		VertexBufferLayout Layout;
		Ref<RenderPass> RenderPass;
		PrimitiveTopology Topology = PrimitiveTopology::Triangles;
		bool BackfaceCulling = true;
		bool DepthTest = true;
		bool DepthWrite = true;
		bool Wireframe = false;
		float LineWidth = 1.0f;

		std::string DebugName;
	};

	class Pipeline : public RefCounted
	{
	public:
		virtual ~Pipeline() = default;

		virtual PipelineSpecification& GetSpecification() = 0;
		virtual const PipelineSpecification& GetSpecification() const = 0;

		virtual void Invalidate() = 0;

		virtual void SetUniformBuffer(Ref<UniformBuffer> uniformBuffer, uint32_t binding, uint32_t set = 0) = 0;

		static Ref<Pipeline> Create(const PipelineSpecification& spec);
	};

}
