#include "pch.h"
#include "Haoyue/Renderer/Renderer2D.h"

#include "Haoyue/Renderer/Pipeline.h"
#include "Haoyue/Renderer/Shader.h"
#include "Haoyue/Renderer/Renderer.h"
#include "Haoyue/Renderer/RenderCommandBuffer.h"

#include <glm/gtc/matrix_transform.hpp>

// TEMP
#include "Haoyue/Vulkan/VulkanRenderCommandBuffer.h"

namespace Haoyue {

	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TexCoord;
		float TexIndex;
		float TilingFactor;
	};

	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	struct CircleVertex
	{
		glm::vec3 WorldPosition;
		float Thickness;
		glm::vec2 LocalPosition;
		glm::vec4 Color;
	};

	struct Renderer2DData
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		static const uint32_t MaxLines = 10000;
		static const uint32_t MaxLineVertices = MaxLines * 2;
		static const uint32_t MaxLineIndices = MaxLines * 6;

		Ref<Pipeline> QuadPipeline;
		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;
		Ref<Material> QuadMaterial;

		Ref<Texture2D> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		Ref<Pipeline> CirclePipeline;
		Ref<VertexBuffer> CircleVertexBuffer;
		uint32_t CircleIndexCount = 0;
		CircleVertex* CircleVertexBufferBase = nullptr;
		CircleVertex* CircleVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		glm::vec4 QuadVertexPositions[4];

		// Lines
		Ref<Pipeline> LinePipeline;
		Ref<VertexBuffer> LineVertexBuffer;
		Ref<IndexBuffer> LineIndexBuffer;
		Ref<Material> LineMaterial;

		uint32_t LineIndexCount = 0;
		LineVertex* LineVertexBufferBase = nullptr;
		LineVertex* LineVertexBufferPtr = nullptr;

		glm::mat4 CameraViewProj;
		bool DepthTest = true;

		float LineWidth = 1.0f;

		Renderer2D::Statistics Stats;

		Ref<RenderCommandBuffer> CommandBuffer;
		Ref<UniformBufferSet> UniformBufferSet;

		struct UBCamera
		{
			glm::mat4 ViewProjection;
		};
	};

	static Renderer2DData* s_Data = nullptr;

	void Renderer2D::Init()
	{
		s_Data = new Renderer2DData;

		FramebufferSpecification framebufferSpec;
		framebufferSpec.Attachments = { ImageFormat::RGBA32F, ImageFormat::Depth };
		framebufferSpec.Samples = 1;
		framebufferSpec.ClearOnLoad = false;
		framebufferSpec.ClearColor = { 0.1f, 0.5f, 0.5f, 1.0f };

		Ref<Framebuffer> framebuffer = Framebuffer::Create(framebufferSpec);

		RenderPassSpecification renderPassSpec;
		renderPassSpec.TargetFramebuffer = framebuffer;
		renderPassSpec.DebugName = "Renderer2D";
		Ref<RenderPass> renderPass = RenderPass::Create(renderPassSpec);

		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Renderer2D-Quad";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D");
			pipelineSpecification.RenderPass = renderPass;
			pipelineSpecification.BackfaceCulling = false;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" },
				{ ShaderDataType::Float2, "a_TexCoord" },
				{ ShaderDataType::Float, "a_TexIndex" },
				{ ShaderDataType::Float, "a_TilingFactor" }
			};
			s_Data->QuadPipeline = Pipeline::Create(pipelineSpecification);

			s_Data->QuadVertexBuffer = VertexBuffer::Create(s_Data->MaxVertices * sizeof(QuadVertex));
			s_Data->QuadVertexBufferBase = new QuadVertex[s_Data->MaxVertices];

			uint32_t* quadIndices = new uint32_t[s_Data->MaxIndices];

			uint32_t offset = 0;
			for (uint32_t i = 0; i < s_Data->MaxIndices; i += 6)
			{
				quadIndices[i + 0] = offset + 0;
				quadIndices[i + 1] = offset + 1;
				quadIndices[i + 2] = offset + 2;

				quadIndices[i + 3] = offset + 2;
				quadIndices[i + 4] = offset + 3;
				quadIndices[i + 5] = offset + 0;

				offset += 4;
			}

			s_Data->QuadIndexBuffer = IndexBuffer::Create(quadIndices, s_Data->MaxIndices);
			delete[] quadIndices;
		}

		s_Data->WhiteTexture = Renderer::GetWhiteTexture();

		// Set all texture slots to 0
		s_Data->TextureSlots[0] = s_Data->WhiteTexture;

		s_Data->QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
		s_Data->QuadVertexPositions[1] = { -0.5f,  0.5f, 0.0f, 1.0f };
		s_Data->QuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f, 1.0f };
		s_Data->QuadVertexPositions[3] = { 0.5f, -0.5f, 0.0f, 1.0f };

		// Lines
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Renderer2D-Line";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D_Line");
			pipelineSpecification.RenderPass = renderPass;
			pipelineSpecification.Topology = PrimitiveTopology::Lines;
			pipelineSpecification.LineWidth = 2.0f;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_Position" },
				{ ShaderDataType::Float4, "a_Color" }
			};
			s_Data->LinePipeline = Pipeline::Create(pipelineSpecification);

			s_Data->LineVertexBuffer = VertexBuffer::Create(s_Data->MaxLineVertices * sizeof(LineVertex));
			s_Data->LineVertexBufferBase = new LineVertex[s_Data->MaxLineVertices];

			uint32_t* lineIndices = new uint32_t[s_Data->MaxLineIndices];
			for (uint32_t i = 0; i < s_Data->MaxLineIndices; i++)
				lineIndices[i] = i;

			s_Data->LineIndexBuffer = IndexBuffer::Create(lineIndices, s_Data->MaxLineIndices);
			delete[] lineIndices;
		}

		// Circles
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "Renderer2D-Circle";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Renderer2D_Circle");
			pipelineSpecification.RenderPass = renderPass;
			pipelineSpecification.Layout = {
				{ ShaderDataType::Float3, "a_WorldPosition" },
				{ ShaderDataType::Float,  "a_Thickness" },
				{ ShaderDataType::Float2, "a_LocalPosition" },
				{ ShaderDataType::Float4, "a_Color" }
			};
			s_Data->CirclePipeline = Pipeline::Create(pipelineSpecification);

			s_Data->CircleVertexBuffer = VertexBuffer::Create(s_Data->MaxVertices * sizeof(QuadVertex));
			s_Data->CircleVertexBufferBase = new CircleVertex[s_Data->MaxVertices];
		}

		s_Data->CommandBuffer = RenderCommandBuffer::Create(0, "Renderer2D");

		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		s_Data->UniformBufferSet = UniformBufferSet::Create(framesInFlight);
		s_Data->UniformBufferSet->Create(sizeof(Renderer2DData::UBCamera), 0);

		s_Data->QuadMaterial = Material::Create(s_Data->QuadPipeline->GetSpecification().Shader, "QuadMaterial");
		s_Data->LineMaterial = Material::Create(s_Data->LinePipeline->GetSpecification().Shader, "LineMaterial");
	}

	void Renderer2D::Shutdown()
	{
		delete s_Data;
	}

	void Renderer2D::BeginScene(const glm::mat4& viewProj, bool depthTest)
	{
		s_Data->CameraViewProj = viewProj;
		s_Data->DepthTest = depthTest;

		Renderer::Submit([viewProj]() mutable
			{
				uint32_t bufferIndex = Renderer::GetCurrentFrameIndex();
				s_Data->UniformBufferSet->Get(0, 0, bufferIndex)->RT_SetData(&viewProj, sizeof(Renderer2DData::UBCamera));
			});

		s_Data->QuadIndexCount = 0;
		s_Data->QuadVertexBufferPtr = s_Data->QuadVertexBufferBase;

		s_Data->LineIndexCount = 0;
		s_Data->LineVertexBufferPtr = s_Data->LineVertexBufferBase;

		s_Data->CircleIndexCount = 0;
		s_Data->CircleVertexBufferPtr = s_Data->CircleVertexBufferBase;

		s_Data->TextureSlotIndex = 1;
	}

	void Renderer2D::EndScene()
	{
		s_Data->CommandBuffer->Begin();
		Renderer::BeginRenderPass(s_Data->CommandBuffer, s_Data->QuadPipeline->GetSpecification().RenderPass);

		uint32_t dataSize = (uint8_t*)s_Data->QuadVertexBufferPtr - (uint8_t*)s_Data->QuadVertexBufferBase;
		if (dataSize)
		{
			s_Data->QuadVertexBuffer->SetData(s_Data->QuadVertexBufferBase, dataSize);

			for (uint32_t i = 0; i < s_Data->TextureSlotIndex; i++)
			{
				//s_Data->QuadMaterial->Set("")
				//s_Data->TextureSlots[i]->Bind(i);
			}

			Renderer::RenderGeometry(s_Data->CommandBuffer, s_Data->QuadPipeline, s_Data->UniformBufferSet, s_Data->QuadMaterial, s_Data->QuadVertexBuffer, s_Data->QuadIndexBuffer, glm::mat4(1.0f), s_Data->QuadIndexCount);

			s_Data->Stats.DrawCalls++;
		}

		dataSize = (uint8_t*)s_Data->LineVertexBufferPtr - (uint8_t*)s_Data->LineVertexBufferBase;
		if (dataSize)
		{
			s_Data->LineVertexBuffer->SetData(s_Data->LineVertexBufferBase, dataSize);

			Ref<RenderCommandBuffer> renderCommandBuffer = s_Data->CommandBuffer;
			Renderer::Submit([renderCommandBuffer]()
				{
					uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
					VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetCommandBuffer(frameIndex);
					vkCmdSetLineWidth(commandBuffer, s_Data->LineWidth);
				});
			Renderer::RenderGeometry(s_Data->CommandBuffer, s_Data->LinePipeline, s_Data->UniformBufferSet, s_Data->LineMaterial, s_Data->LineVertexBuffer, s_Data->LineIndexBuffer, glm::mat4(1.0f), s_Data->LineIndexCount);

			s_Data->Stats.DrawCalls++;
		}

		Renderer::EndRenderPass(s_Data->CommandBuffer);
		s_Data->CommandBuffer->End();
		s_Data->CommandBuffer->Submit();
	}

	Ref<RenderPass> Renderer2D::GetTargetRenderPass()
	{
		return s_Data->QuadPipeline->GetSpecification().RenderPass;
	}

	void Renderer2D::SetTargetRenderPass(Ref<RenderPass> renderPass)
	{
		if (renderPass != s_Data->QuadPipeline->GetSpecification().RenderPass)
		{
			{
				PipelineSpecification pipelineSpecification = s_Data->QuadPipeline->GetSpecification();
				pipelineSpecification.RenderPass = renderPass;
				s_Data->QuadPipeline = Pipeline::Create(pipelineSpecification);
			}

			{
				PipelineSpecification pipelineSpecification = s_Data->LinePipeline->GetSpecification();
				pipelineSpecification.RenderPass = renderPass;
				s_Data->LinePipeline = Pipeline::Create(pipelineSpecification);
			}
		}
	}

	void Renderer2D::FlushAndReset()
	{
		EndScene();

		s_Data->QuadIndexCount = 0;
		s_Data->QuadVertexBufferPtr = s_Data->QuadVertexBufferBase;

		s_Data->TextureSlotIndex = 1;
	}

	void Renderer2D::FlushAndResetLines()
	{

	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
	{
		constexpr size_t quadVertexCount = 4;
		const float textureIndex = 0.0f; // White Texture
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };
		const float tilingFactor = 1.0f;

		if (s_Data->QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[i];
			s_Data->QuadVertexBufferPtr->Color = color;
			s_Data->QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data->QuadVertexBufferPtr++;
		}

		s_Data->QuadIndexCount += 6;

		s_Data->Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		constexpr glm::vec2 textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data->QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data->TextureSlotIndex; i++)
		{
			if (*s_Data->TextureSlots[i].Raw() == *texture.Raw())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data->TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
				FlushAndReset();

			textureIndex = (float)s_Data->TextureSlotIndex;
			s_Data->TextureSlots[s_Data->TextureSlotIndex] = texture;
			s_Data->TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[i];
			s_Data->QuadVertexBufferPtr->Color = color;
			s_Data->QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
			s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_Data->QuadVertexBufferPtr++;
		}

		s_Data->QuadIndexCount += 6;

		s_Data->Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, color);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		if (s_Data->QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		const float textureIndex = 0.0f; // White Texture
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[0];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[1];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[2];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[3];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadIndexCount += 6;

		s_Data->Stats.QuadCount++;
	}

	void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawQuad({ position.x, position.y, 0.0f }, size, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		if (s_Data->QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data->TextureSlotIndex; i++)
		{
			if (*s_Data->TextureSlots[i].Raw() == *texture.Raw())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data->TextureSlotIndex;
			s_Data->TextureSlots[s_Data->TextureSlotIndex] = texture;
			s_Data->TextureSlotIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[0];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[1];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[2];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[3];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadIndexCount += 6;

		s_Data->Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		if (s_Data->QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		const float textureIndex = 0.0f; // White Texture
		const float tilingFactor = 1.0f;

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[0];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[1];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[2];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[3];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadIndexCount += 6;

		s_Data->Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		DrawRotatedQuad({ position.x, position.y, 0.0f }, size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		if (s_Data->QuadIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		constexpr glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data->TextureSlotIndex; i++)
		{
			if (*s_Data->TextureSlots[i].Raw() == *texture.Raw())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_Data->TextureSlotIndex;
			s_Data->TextureSlots[s_Data->TextureSlotIndex] = texture;
			s_Data->TextureSlotIndex++;
		}

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[0];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 0.0f, 0.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[1];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 1.0f, 0.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[2];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 1.0f, 1.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadVertexBufferPtr->Position = transform * s_Data->QuadVertexPositions[3];
		s_Data->QuadVertexBufferPtr->Color = color;
		s_Data->QuadVertexBufferPtr->TexCoord = { 0.0f, 1.0f };
		s_Data->QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data->QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data->QuadVertexBufferPtr++;

		s_Data->QuadIndexCount += 6;

		s_Data->Stats.QuadCount++;
	}

	void Renderer2D::DrawRotatedRect(const glm::vec2& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		DrawRotatedRect({ position.x, position.y, 0.0f }, size, rotation, color);
	}

	void Renderer2D::DrawRotatedRect(const glm::vec3& position, const glm::vec2& size, float rotation, const glm::vec4& color)
	{
		if (s_Data->LineIndexCount >= Renderer2DData::MaxLineIndices)
			FlushAndResetLines();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });

		glm::vec3 positions[4] =
		{
			transform * s_Data->QuadVertexPositions[0],
			transform * s_Data->QuadVertexPositions[1],
			transform * s_Data->QuadVertexPositions[2],
			transform * s_Data->QuadVertexPositions[3]
		};

		for (int i = 0; i < 4; i++)
		{
			auto& v0 = positions[i];
			auto& v1 = positions[(i + 1) % 4];

			s_Data->LineVertexBufferPtr->Position = v0;
			s_Data->LineVertexBufferPtr->Color = color;
			s_Data->LineVertexBufferPtr++;

			s_Data->LineVertexBufferPtr->Position = v1;
			s_Data->LineVertexBufferPtr->Color = color;
			s_Data->LineVertexBufferPtr++;

			s_Data->LineIndexCount += 2;
			s_Data->Stats.LineCount++;
		}
	}

	void Renderer2D::DrawCircle(const glm::vec2& position, float radius, const glm::vec4& color, float thickness)
	{
		DrawCircle({ position.x, position.y, 0.0f }, radius, color, thickness);
	}

	void Renderer2D::DrawCircle(const glm::vec3& position, float radius, const glm::vec4& color, float thickness)
	{
		if (s_Data->CircleIndexCount >= Renderer2DData::MaxIndices)
			FlushAndReset();

		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::scale(glm::mat4(1.0f), { radius * 2.0f, radius * 2.0f, 1.0f });

		for (int i = 0; i < 4; i++)
		{
			s_Data->CircleVertexBufferPtr->WorldPosition = transform * s_Data->QuadVertexPositions[i];
			s_Data->CircleVertexBufferPtr->Thickness = thickness;
			s_Data->CircleVertexBufferPtr->LocalPosition = s_Data->QuadVertexPositions[i] * 2.0f;
			s_Data->CircleVertexBufferPtr->Color = color;
			s_Data->CircleVertexBufferPtr++;

			s_Data->CircleIndexCount += 6;
			s_Data->Stats.QuadCount++;
		}

	}

	void Renderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color)
	{
		if (s_Data->LineIndexCount >= Renderer2DData::MaxLineIndices)
			FlushAndResetLines();

		s_Data->LineVertexBufferPtr->Position = p0;
		s_Data->LineVertexBufferPtr->Color = color;
		s_Data->LineVertexBufferPtr++;

		s_Data->LineVertexBufferPtr->Position = p1;
		s_Data->LineVertexBufferPtr->Color = color;
		s_Data->LineVertexBufferPtr++;

		s_Data->LineIndexCount += 2;

		s_Data->Stats.LineCount++;
	}

	void Renderer2D::DrawAABB(Ref<Mesh> mesh, const glm::mat4& transform, const glm::vec4& color)
	{
		const auto& meshAssetSubmeshes = mesh->GetMeshAsset()->GetSubmeshes();
		auto& submeshes = mesh->GetSubmeshes();
		for (uint32_t submeshIndex : submeshes)
		{
			const Submesh& submesh = meshAssetSubmeshes[submeshIndex];
			auto& aabb = submesh.BoundingBox;
			auto aabbTransform = transform * submesh.Transform;
			DrawAABB(aabb, aabbTransform);
		}
	}

	void Renderer2D::DrawAABB(const AABB& aabb, const glm::mat4& transform, const glm::vec4& color /*= glm::vec4(1.0f)*/)
	{
		glm::vec4 min = { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f };
		glm::vec4 max = { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f };

		glm::vec4 corners[8] =
		{
			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Max.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Max.z, 1.0f },

			transform * glm::vec4 { aabb.Min.x, aabb.Min.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Min.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Max.y, aabb.Min.z, 1.0f },
			transform * glm::vec4 { aabb.Max.x, aabb.Min.y, aabb.Min.z, 1.0f }
		};

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i], corners[(i + 1) % 4], color);

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i + 4], corners[((i + 1) % 4) + 4], color);

		for (uint32_t i = 0; i < 4; i++)
			Renderer2D::DrawLine(corners[i], corners[i + 4], color);
	}

	void Renderer2D::SetLineWidth(float lineWidth)
	{
		s_Data->LineWidth = lineWidth;
	}

	void Renderer2D::ResetStats()
	{
		memset(&s_Data->Stats, 0, sizeof(Statistics));
	}

	Renderer2D::Statistics Renderer2D::GetStats()
	{
		return s_Data->Stats;
	}

}