#include "pch.h"
#include <RE/H/hkaAnnotationTrack.h>
#include "CharacterClipAnimMotionMap.h"
#include "utils/HookValidation.h"
#include "utils/CallPatchRollback.h"
#include "MotionDiagnostics.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <Windows.h>

static bool failNextAllocation{};
void* operator new(std::size_t size)
{
    if (std::exchange(failNextAllocation, false)) throw std::bad_alloc{};
    if (auto result = std::malloc(size ? size : 1)) return result;
    throw std::bad_alloc{};
}
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }

static unsigned checks{};
#define CHECK(value) do { ++checks; if (!(value)) { std::cerr << "FAILED line " << __LINE__ << ": " #value "\n"; return 1; } } while (false)

int main(int argc, char** argv)
{
    if (argc == 3 && std::string_view(argv[1]) == "--reject-wrapper") {
        const auto library = ::LoadLibraryW(std::filesystem::path(argv[2]).c_str());
        CHECK(library != nullptr);
        using Load = bool (*)(const SKSE::LoadInterface*);
        const auto load = reinterpret_cast<Load>(::GetProcAddress(library, "SKSEPlugin_Load"));
        CHECK(load != nullptr);
        SKSE::Impl::SKSEInterface mock{};
        mock.runtimeVersion = REL::Version{1, 7, 104, 0}.pack();
        CHECK(!load(reinterpret_cast<const SKSE::LoadInterface*>(&mock)));
        CHECK(::FreeLibrary(library) != 0);
        std::cout << "PASS: " << checks << " wrapper rejection checks\n";
        return 0;
    }
    if (argc == 3 && std::string_view(argv[1]) == "--module-override") {
        const std::filesystem::path expected(argv[2]);
        const auto library = ::LoadLibraryW(expected.c_str());
        CHECK(library != nullptr);
        const auto& module = REL::Module::get();
        CHECK(reinterpret_cast<void*>(module.pointer()) == reinterpret_cast<void*>(library));
        CHECK(std::filesystem::path(module.filePath().data()) == expected);
        CHECK(::FreeLibrary(library) != 0);
        std::cout << "PASS: " << checks << " explicit loaded-module override checks\n";
        return 0;
    }
    if (argc == 3 && std::string_view(argv[1]) == "--module-name") {
        const auto& module = REL::Module::get();
        const std::filesystem::path expected(argv[2]);
        CHECK(std::filesystem::path(module.filename().data()).filename() == expected);
        CHECK(module.filename().size() == std::wstring_view(module.filename().data()).size());
        CHECK(reinterpret_cast<void*>(module.pointer()) == reinterpret_cast<void*>(::GetModuleHandleW(nullptr)));
        CHECK(std::filesystem::path(module.filePath().data()).is_absolute());
        CHECK(std::filesystem::path(module.filePath().data()).filename() == expected);
        CHECK(REL::GetFileVersion(module.filePath()) == module.version());
        std::cout << "PASS: " << checks << " module discovery checks\n";
        return 0;
    }

    CHECK(amr::diagnostics::IsAttackClip("MCO_ClipGenerator_MCO_attack1"));
    CHECK(amr::diagnostics::IsAttackClip("AttackPowerStart"));
    CHECK(amr::diagnostics::IsAttackClip("DodgeLeft"));
    CHECK(!amr::diagnostics::IsAttackClip("IdleDefault"));
    CHECK(!amr::diagnostics::IsAttackClip(""));
    CHECK(std::string_view(amr::diagnostics::Label("Q:\\private\\animation.hkx").data()) == "animation.hkx");
    CHECK(std::string_view(amr::diagnostics::Label("Q:/private/animation.hkx").data()) == "animation.hkx");
    CHECK(std::string_view(amr::diagnostics::Label("Q:animation.hkx").data()) == ">DoK<");
    CHECK(std::string_view(amr::diagnostics::Label("a\nb").data()) == "a?b");
    CHECK(std::string_view(amr::diagnostics::Label(std::string(200, 'a')).data()).size() == 71);
    amr::diagnostics::SampleBudget budget;
    unsigned samples{};
    for (unsigned i = 0; i < 100000; ++i) samples += budget.Take();
    CHECK(samples == 64);
    CHECK(!budget.Take());

    CHECK(std::holds_alternative<Translation>(ParseMotionText(1, "animmotion +1 -2 3\t")));
    CHECK(std::get<Translation>(ParseMotionText(1, "animmotion +1 -2 3")).delta.y == -2);
    for (const auto text : {"animmotion 1 2", "animmotion 1 2 3 junk", "animmotion nan 2 3",
             "animmotion inf 2 3", "animmotion 1x 2 3", "animmotion +-1 2 3", "animmotion ++1 2 3",
             "animrotation ", "animrotation junk", "animrotation 90x", "animrotation 90 1",
             "animrotation 1e999", "unrelated annotation"}) {
        CHECK(std::holds_alternative<std::monostate>(ParseMotionText(1, text)));
    }
    CHECK(std::holds_alternative<std::monostate>(ParseMotionText(-1, "animrotation 90")));
    CHECK(std::holds_alternative<std::monostate>(ParseMotionText(std::numeric_limits<float>::infinity(), "animrotation 90")));
    const auto rotation = std::get<Rotation>(ParseMotionText(1, "animrotation 90")).delta;
    CHECK(std::abs(rotation.w - 0.70710678f) < 0.00001f);
    CHECK(std::abs(rotation.z - 0.70710678f) < 0.00001f);
    CHECK(std::isfinite(std::get<Rotation>(ParseMotionText(1, "animrotation 3.4e38")).delta.w));
    static_assert(std::is_trivially_copyable_v<RE::hkaAnnotationTrack::Annotation>);
    const auto annotation = std::bit_cast<RE::hkaAnnotationTrack::Annotation>(
        std::array<std::byte, sizeof(RE::hkaAnnotationTrack::Annotation)>{});
    CHECK(std::holds_alternative<std::monostate>(ParseAnnotation(annotation)));

    RE::MotionDataContainer native{};
    CHECK(native.IsTranslationDataAligned() && native.IsRotationDataAligned());
    native.translationDataPtr = 1;
    native.rotationDataPtr = 1;
    CHECK(!native.IsTranslationDataAligned() && !native.IsRotationDataAligned());

    AnimMotionData motion;
    CHECK(motion.Activate() && motion.activeCount == 2);
    CHECK(!motion.Deactivate() && motion.activeCount == 1);
    CHECK(motion.Deactivate());
    motion.activeCount = std::numeric_limits<std::int32_t>::max();
    CHECK(!motion.Activate());
    motion.activeCount = 0;
    CHECK(!motion.Activate() && motion.Deactivate());
    motion.activeCount = -1;
    CHECK(!motion.Activate() && motion.Deactivate());

    auto cache = hooks::CharacterClipAnimMotionMap::GetSingleton();
    // Opaque identities are never dereferenced by this cache.
    const auto actor = reinterpret_cast<const RE::hkbCharacter*>(1);
    const auto animation = reinterpret_cast<const RE::hkaAnimation*>(2);
    const auto otherAnimation = reinterpret_cast<const RE::hkaAnimation*>(3);
    const std::string name("test clip");
    Translation translation{1, {1, 2, 3}};
    CHECK(!cache->Get(actor, name));
    CHECK(!cache->Add(nullptr, name, animation, &translation));
    auto data = cache->Add(actor, name, animation, &translation);
    CHECK(data && data->animation == animation && data->translationList.size() == 1);
    data = cache->Add(actor, name, otherAnimation, &translation);
    CHECK(data && data->animation == otherAnimation && data->activeCount == 1);
    CHECK(cache->Get(actor, name) == data);
    cache->Remove(actor, name);
    CHECK(!cache->Get(actor, name));
    CHECK(cache->Add(actor, name, animation, &translation));
    spdlog::default_logger()->set_level(spdlog::level::off);
    failNextAllocation = true;
    data = cache->Add(actor, name, otherAnimation, &translation);
    CHECK(!failNextAllocation && !data && !cache->Get(actor, name));

    const std::array<std::uint8_t, 11> rsi{0x48,0x8b,0xf2,0x48,0x8b,0xd9,0xe8,0,0,0,0};
    const std::array<std::uint8_t, 8> r14{0x4c,0x8b,0xf2,0xe8,0,0,0,0};
    CHECK(utils::DeactivationContext(rsi, 6) == utils::ContextRegister::rsi);
    CHECK(utils::DeactivationContext(r14, 3) == utils::ContextRegister::r14);
    CHECK(!utils::IsCallBoundary(rsi, 1));
    CHECK(!utils::IsCallBoundary(rsi, 7));
    CHECK(!utils::IsCallBoundary({}, 0));
    CHECK(!utils::IsCallBoundary(rsi, std::numeric_limits<std::size_t>::max()));
    CHECK(!utils::DeactivationContext(std::span(r14).first(6), 3));
    const std::array<std::uint8_t, 10> fake{0xb8,0,0x48,0x8b,0xf2,0xe8,0,0,0,0};
    CHECK(utils::IsCallBoundary(fake, 5));
    CHECK(!utils::DeactivationContext(fake, 5));

    std::array<std::array<std::uint8_t, 5>, 6> calls{};
    utils::CallPatchRollback::Sites sites{};
    for (std::size_t i = 0; i < calls.size(); ++i) {
        calls[i] = {0xE8, static_cast<std::uint8_t>(i), 0, 0, 0};
        sites[i] = reinterpret_cast<std::uintptr_t>(calls[i].data());
    }
    const auto originalCalls = calls;
    const auto write = [](std::uintptr_t destination, const void* source, std::size_t size) {
        std::memcpy(reinterpret_cast<void*>(destination), source, size);
    };
    static_assert(!std::is_copy_constructible_v<utils::CallPatchRollback>);
    // Cover failure after every installation stage, including after the last CALL.
    for (std::size_t changed = 0; changed <= calls.size(); ++changed) {
        try {
            utils::CallPatchRollback rollback(sites, write);
            for (std::size_t i = 0; i < changed; ++i) calls[i].fill(0xCC);
            throw std::runtime_error("Simulated load completion failure");
        } catch (const std::runtime_error&) {}
        CHECK(calls == originalCalls);
    }
    {
        utils::CallPatchRollback rollback(sites, write);
        calls[0][0] = 0xE9;
        rollback.Commit();
    }
    CHECK(calls[0][0] == 0xE9);

    if (argc == 2) {
        // The supplied reference prologue is tested by the same decoder as the runtime gate.
        const std::filesystem::path directory(argv[1]);
        std::ifstream file(directory / "deactivate.bin", std::ios::binary);
        std::vector<std::uint8_t> prologue(std::istreambuf_iterator<char>{file}, {});
        CHECK(prologue.size() == 31);
        CHECK(utils::DeactivationContext(prologue, 0x1A) == utils::ContextRegister::rsi);
        const auto checkCalls = [&](const char* name, std::initializer_list<std::size_t> offsets) {
            std::ifstream input(directory / name, std::ios::binary);
            std::vector<std::uint8_t> code(std::istreambuf_iterator<char>{input}, {});
            return std::all_of(offsets.begin(), offsets.end(), [&](auto offset) { return utils::IsCallBoundary(code, offset); });
        };
        CHECK(checkCalls("activate.bin", {0x66E}));
        CHECK(checkCalls("motion.bin", {0x298, 0x2AA, 0x35C, 0x36D}));
    }
    std::cout << "PASS: " << checks << " regression checks\n";
}
