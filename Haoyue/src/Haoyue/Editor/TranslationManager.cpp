#include "pch.h"
#include "TranslationManager.h"

#include <fstream>
#include <filesystem>


namespace Haoyue {

	TranslationManager::Language TranslationManager::s_CurrentLanguage = TranslationManager::Language::English;
	std::unordered_map<std::string, std::string> TranslationManager::s_Translations;
	std::unordered_map<TranslationManager::Language, std::string> TranslationManager::s_LanguageStrings;
	bool TranslationManager::s_Initialized = false;

	std::unordered_map<std::string, std::string>& TranslationManager::GetTRCache()
	{
		static std::unordered_map<std::string, std::string> cache;
		return cache;
	}

	void TranslationManager::Init()
	{
		if (s_Initialized) return;

		s_LanguageStrings[Language::English] = "en";
		s_LanguageStrings[Language::Chinese] = "zh";

		// 默认加载英文（英文直接使用键作为值）
		SetLanguage(Language::Chinese);

		s_Initialized = true;
	}

	void TranslationManager::Shutdown()
	{
		s_Translations.clear();
		s_LanguageStrings.clear();
		ClearTRCache();
		s_Initialized = false;
	}

	void TranslationManager::SetLanguage(Language language)
	{
		if (s_CurrentLanguage == language) return;
		UnloadTranslations(s_CurrentLanguage);
		s_CurrentLanguage = language;
		ClearTRCache();
		LoadTranslations(language);
	}

	const std::string& TranslationManager::GetLocaleString(Language language)
	{
		static std::string emptyString = "";
		auto it = s_LanguageStrings.find(language);
		return it != s_LanguageStrings.end() ? it->second : emptyString;
	}

	std::string TranslationManager::Translate(const std::string& key)
	{
		if (s_CurrentLanguage == Language::English) return key;

		auto it = s_Translations.find(key);
		if (it != s_Translations.end()) {
			if (it->second.empty()) {
				return key;
			}
			return it->second;
		}

		// 如果找不到翻译，回退到英文（返回键）
		// 同时将缺失的翻译添加到文件中
		SaveMissingTranslation(key);
		return key;
	}

	void TranslationManager::LoadTranslations(Language language)
	{
		if (language == Language::English) return;

		std::string locale = GetLocaleString(language);
		if (locale.empty()) return;

		std::filesystem::path translationPath = std::filesystem::current_path().parent_path() / "Haoyue-Editor" / "Resources" / "Translations" / (locale + ".lang");
		std::ifstream file(translationPath);

		if (!file.is_open())
		{
			HY_WARN("Could not open translation file: {0}", translationPath.string());
			return;
		}

		std::string line; 
		while (std::getline(file, line))
		{
			// 跳过空行和注释行
			if (line.empty() || line[0] == '#' || line[0] == ';')
				continue;

			// 查找分隔符
			size_t delimiterPos = line.find('=');
			if (delimiterPos != std::string::npos)
			{
				std::string key = line.substr(0, delimiterPos);
				std::string value = line.substr(delimiterPos + 1);

				// 去除首尾空格
				key.erase(0, key.find_first_not_of(" \t"));
				key.erase(key.find_last_not_of(" \t") + 1);
				value.erase(0, value.find_first_not_of(" \t"));
				value.erase(value.find_last_not_of(" \t") + 1);

				s_Translations[key] = value;
			}
		}

		file.close();
		HY_INFO("Loaded translations for language: {0}", locale);
	}

	void TranslationManager::UnloadTranslations(Language language)
	{
		s_Translations.clear();
	}

	void TranslationManager::ClearTRCache()
	{
		GetTRCache().clear();
	}
	
	void TranslationManager::SaveMissingTranslation(const std::string& key)
	{
		// 只有在非英文环境下才保存缺失的翻译
		if (s_CurrentLanguage == Language::English) return;

		std::string locale = GetLocaleString(s_CurrentLanguage);
		if (locale.empty()) return;

		std::filesystem::path translationPath = std::filesystem::current_path().parent_path() / "Haoyue-Editor" / "Resources" / "translations" / (locale + ".lang");

		std::ofstream file(translationPath, std::ios::app);
		if (file.is_open())
		{
			file << std::endl;
			file << key << "=" << std::endl;
			file.close();
			s_Translations[key] = "***";
			
			HY_INFO("Added missing translation key to {0}.lang: {1}", locale, key);
		}
		else
		{
			HY_WARN("Could not open translation file for writing: {0}", translationPath.string());
		}
	}

}
