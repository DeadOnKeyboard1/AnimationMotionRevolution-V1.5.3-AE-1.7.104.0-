auto moduleHandle = static_cast<REX::W32::HMODULE>(nullptr);
const auto required = REX::W32::GetEnvironmentVariableW(L"SKSE_RUNTIME", nullptr, 0);
if (required != 0) {
    std::wstring overrideName(required, L'\0');
    const auto length = REX::W32::GetEnvironmentVariableW(L"SKSE_RUNTIME", overrideName.data(), required);
    if (length != 0 && length < required) {
        overrideName.resize(length);
        moduleHandle = REX::W32::GetModuleHandleW(overrideName.c_str());
    }
}
if (!moduleHandle) {
    for (const auto name : {L"SkyrimVR.exe", L"SkyrimSE.exe", L"SkyrimSE.rne"}) {
        moduleHandle = REX::W32::GetModuleHandleW(name);
        if (moduleHandle) break;
    }
}
if (!moduleHandle) {
    moduleHandle = REX::W32::GetModuleHandleW(nullptr);
}
