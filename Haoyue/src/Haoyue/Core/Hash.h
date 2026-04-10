#pragma once

#include <string>

namespace Haoyue {

	class Hash
	{
		static uint32_t GenerateFNVHash(const char* str);
		static uint32_t GenerateFNVHash(const std::string& string);
	};
}

