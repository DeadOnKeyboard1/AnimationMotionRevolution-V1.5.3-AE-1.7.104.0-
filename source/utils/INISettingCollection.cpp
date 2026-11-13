#include "utils/INISettingCollection.h"

#include "utils/Logger.h"

namespace utils
{
	INISettingCollection::INISettingCollection() noexcept
	{
		REL::Relocation<std::uintptr_t> __vTable(*reinterpret_cast<std::uintptr_t*>(this));

		// Use the Skyrim's INISettingCollection virtual functions
		static std::uintptr_t* skyrimsVTable = REL::Relocation<std::uintptr_t*>{ vTableId }.get();

		// Replace all except the destructor (index 0)
		for (int i = 1; i < 10; i++) {
			__vTable.write_vfunc(i, skyrimsVTable[i]);
		}
	}

	bool INISettingCollection::ReadFromFile(std::string_view a_fileName)
	{
		const auto iniPath = std::filesystem::path(REL::Module::get().filePath().data()).parent_path() / "Data" / "SKSE" / "Plugins" / a_fileName;
		const auto path = iniPath.string();
		if (path.size() >= sizeof(subKey)) {
			logger::warn("INI path exceeds the native setting collection capacity; retaining defaults");
			return false;
		}
		strcpy_s(subKey, path.c_str());

		if (_this()->OpenHandle(false)) {
			_this()->ReadAllSettings();
			_this()->CloseHandle();

			return true;
		}

		return false;
	}
}

