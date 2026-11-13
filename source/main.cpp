#include "Hooks.h"
#include "Settings.h"

#include "utils/Logger.h"
#include "MotionDiagnostics.h"

#include <ShlObj.h>

namespace SKSE::log
{
	std::optional<std::filesystem::path> amr_log_directory()
	{
		PWSTR buffer{ nullptr };
		const auto result = SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_DEFAULT, nullptr, std::addressof(buffer));
		std::unique_ptr<wchar_t[], decltype(&CoTaskMemFree)> knownPath(buffer, CoTaskMemFree);
		if (!knownPath || result != S_OK) {
			return std::nullopt;
		}

		std::filesystem::path path = knownPath.get();
		path /= "My Games";
		const auto gameDirectory = std::filesystem::path(REL::Module::get().filePath().data()).parent_path();
		path /= std::filesystem::exists(gameDirectory / "steam_api64.dll") ? "Skyrim Special Edition" : "Skyrim Special Edition GOG";
		path /= "SKSE";

		return path;
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	try {
		const SKSE::PluginDeclaration* plugin = SKSE::PluginDeclaration::GetSingleton();
		if (!a_skse || !plugin) return false;
		if (REL::Module::get().version() != a_skse->RuntimeVersion()) {
			logger::critical("Selected process image version does not match the SKSE game runtime; no native bindings resolved");
			return false;
		}

		if (!logger::init(plugin->GetName()))
		{
			return false;
		}

		logger::info("Loading {} {}...", plugin->GetName(), plugin->GetVersion().string("."));
		logger::flush();

		logger::info("Calling SKSE::Init...");
		logger::flush();
		SKSE::Init(a_skse, SKSE::InitInfo{ .log = false });
		const auto context = hooks::ValidateRuntimeLayout();
		if (!context) return false;

		logger::info("Calling settings::Init...");
		logger::flush();
		settings::Init(std::string(plugin->GetName()) + ".ini");

		logger::set_level(settings::debug::logLevel, settings::debug::logLevel);
#ifdef AMR_MOTION_DIAGNOSTICS
		amr::diagnostics::captureEnabled.store(settings::debug::enableMotionDiagnostics, std::memory_order_relaxed);
		if (settings::debug::enableMotionDiagnostics) {
			const auto level = std::min(settings::debug::logLevel, logger::level::info);
			logger::set_level(level, level);
			logger::info("[AMR-DIAG] Bounded motion diagnostic candidate enabled; revision=3; INI-controlled; all player motion labels included; movement logic unchanged. Up to 64 sampled records per category; initial activation records may be unclassified. Output is cumulative clip motion, not proof of actor displacement.");
		}
#endif

		logger::info("Calling hooks::Install...");
		logger::flush();
		return hooks::Install(*context);
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
