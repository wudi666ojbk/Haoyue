#pragma once

#include "Haoyue/Core/Ref.h"

#include "RendererTypes.h"

namespace Haoyue {

	class IndexBuffer : public RefCounted
	{
	public:
		virtual ~IndexBuffer() {}

		virtual uint32_t GetCount() const = 0;

		virtual uint32_t GetSize() const = 0;

		static Ref<IndexBuffer> Create(uint32_t size);
		static Ref<IndexBuffer> Create(void* data, uint32_t size = 0);
	};

}

