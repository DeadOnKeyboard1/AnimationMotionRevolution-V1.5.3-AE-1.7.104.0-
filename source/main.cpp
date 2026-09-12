#include "Hooks.h"
#include "Settings.h"

#include "utils/Logger.h"

#include <ShlObj.h>

namespace SKSE::log
{
	std::optional<std::filesystem::path> log_directory()
	{
		PWSTR buffer{ nullptr };
		const auto result = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));
		std::unique_ptr<wchar_t[], decltype(&CoTaskMemFree)> knownPath(buffer, CoTaskMemFree);
		if (!knownPath || result != S_OK) {
			return std::nullopt;
		}

		std::filesystem::path path = knownPath.get();
		path /= "My Games";
		path /= std::filesystem::exists("steam_api64.dll") ? "Skyrim Special Edition" : "Skyrim Special Edition GOG";
		path /= "SKSE";

		return path;
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	try {
		const SKSE::PluginDeclaration* plugin = SKSE::PluginDeclaration::GetSingleton();

		if (!logger::init(plugin->GetName()))
		{
			return false;
		}

		logger::info("Loading {} {}...", plugin->GetName(), plugin->GetVersion().string("."));
		logger::flush();

		logger::info("Calling SKSE::Init...");
		logger::flush();
		SKSE::Init(a_skse);

		logger::info("Calling settings::Init...");
		logger::flush();
		settings::Init(std::string(plugin->GetName()) + ".ini");

		logger::set_level(settings::debug::logLevel, settings::debug::logLevel);

		logger::info("Calling hooks::Install...");
		logger::flush();
		hooks::Install();

		logger::set_level(logger::level::info, logger::level::info);
		logger::info("Succesfully loaded!");
		logger::flush();

		logger::set_level(settings::debug::logLevel, settings::debug::logLevel);

		return true;
	} catch (const std::exception& e) {
		logger::critical("Exception occurred during SKSEPluginLoad: {}", e.what());
		logger::flush();
		return false;
	} catch (...) {
		logger::critical("Unknown exception occurred during SKSEPluginLoad");
		logger::flush();
		return false;
	}
}
