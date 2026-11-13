#include "Settings.h"

#include "utils/INISettingCollection.h"

namespace settings
{
	using namespace utils;

	void Init(const std::string& a_iniFileName)
	{
		INISettingCollection* iniSettingCollection = INISettingCollection::GetSingleton();

		{
			using namespace debug;
			iniSettingCollection->AddSettings(
				MakeSetting("uLogLevel:Debug", static_cast<std::uint32_t>(logLevel)),
				MakeSetting("bEnableMotionDiagnostics:Debug", enableMotionDiagnostics));
		}

		if (!iniSettingCollection->ReadFromFile(a_iniFileName))
		{
			logger::warn("Could not read {}, falling back to default options", a_iniFileName);
		}

		{
			using namespace debug;
			if (const auto diagnostic = iniSettingCollection->GetSetting("bEnableMotionDiagnostics:Debug")) {
				enableMotionDiagnostics = diagnostic->GetBool();
			}
			const auto setting = iniSettingCollection->GetSetting("uLogLevel:Debug");
			if (setting) {
				const auto value = setting->GetUnsignedInteger();
				if (value <= static_cast<std::uint32_t>(logger::level::off)) logLevel = static_cast<logger::level>(value);
				else logger::warn("Invalid log level {}; retaining the default", value);
			}
		}
	}
}
