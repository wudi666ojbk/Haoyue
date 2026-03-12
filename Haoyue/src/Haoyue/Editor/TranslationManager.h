#pragma once

#include <string>
#include <unordered_map>
#include <memory>

namespace Haoyue {

	class TranslationManager
	{
	public:
		enum class Language
		{
			English = 0,
			Chinese = 1
		};

	public:
		static void Init();
		static void Shutdown();

		static void SetLanguage(Language language);
		static Language GetLanguage() { return s_CurrentLanguage; }

		static const std::string& GetLocaleString(Language language);

		static std::string Translate(const std::string& key);

		static void ClearTRCache();

		static std::unordered_map<std::string, std::string>& GetTRCache();

	private:
		static void LoadTranslations(Language language);
		static void UnloadTranslations(Language language);
		static void SaveMissingTranslation(const std::string& key);

	private:
		static Language s_CurrentLanguage;
		
		static std::unordered_map<std::string, std::string> s_Translations;
		static std::unordered_map<Language, std::string> s_LanguageStrings;
		
		static bool s_Initialized;
	};

	static const char* TR(const std::string& key)
	{
		auto& cache = TranslationManager::GetTRCache();
		
		auto it = cache.find(key);
		if (it != cache.end()) {
			return it->second.c_str();
		}
		
		cache[key] = TranslationManager::Translate(key);
		return cache[key].c_str();
	}

}