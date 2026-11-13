#include <atomic>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

// Exercise the production trace without an engine, injected hooks or file IO.
namespace RE
{
    struct hkbCharacter {};
    struct Character
    {
        std::uint32_t id;
        std::uint32_t GetFormID() const noexcept { return id; }
    };
}

static bool cacheLocked{};
static bool loggedWhileLocked{};
static bool sinkThrows{};
static unsigned records{};
static unsigned heartbeats{};
namespace logger
{
    template <class... Args>
    void info(const char* format, Args&&...)
    {
        loggedWhileLocked |= cacheLocked;
        if (sinkThrows) throw std::runtime_error("simulated diagnostic sink failure");
        ++records;
        heartbeats += std::string_view{format}.find("Player motion hook reached") != std::string_view::npos;
    }
}

#include "MotionDiagnostics.h"

static unsigned checks{};
#define CHECK(value) do { ++checks; if (!(value)) { std::cerr << "FAILED line " << __LINE__ << ": " #value "\n"; return 1; } } while (false)

int main()
{
    using namespace amr::diagnostics;
    using Kind = Trace::Kind;
    RE::hkbCharacter playerIdentity, npcIdentity;
    RE::Character player{0x14}, npc{0x123};
    CHECK(!captureEnabled.load());
    for (const auto kind : {Kind::activation, Kind::deactivation, Kind::translation, Kind::rotation}) {
        Trace trace{kind};
        CHECK(!trace.capturing);
        trace.SetClip("MCO_attack1");
        trace.SetPlayer(&player, &playerIdentity);
        CHECK(!trace.enabled && !trace.firstPlayer);
        CHECK(trace.clip[0] == '\0');
    }
    CHECK(records == 0);
    CHECK(!playerGraph.load() && !announcedPlayer.load() && initialActivations.load() == 0);
    captureEnabled.store(true);
    SampleBudget concurrent;
    std::atomic<unsigned> samples{};
    std::vector<std::thread> workers;
    for (unsigned n = 0; n < 8; ++n) {
        workers.emplace_back([&] {
            for (unsigned i = 0; i < 10000; ++i) if (concurrent.Take()) ++samples;
        });
    }
    for (auto& worker : workers) worker.join();
    CHECK(samples.load() == 64);
    CHECK(!concurrent.Take());

    for (unsigned i = 0; i < 100; ++i) {
        Trace trace{Kind::activation};
        trace.SetClip("MCO_attack1");
        trace.graph = &npcIdentity;
    }
    CHECK(records == 9);  // First eight plus the sampled ninth; unclassified cap is 16 attempts.
    {
        Trace trace{Kind::translation};
        trace.SetClip("idle");
        trace.SetPlayer(&player, &playerIdentity);
        CHECK(trace.enabled);
        struct Guard {
            Guard() { cacheLocked = true; }
            ~Guard() { cacheLocked = false; }
        } guard;
        CHECK(records == 9);
    }
    CHECK(!loggedWhileLocked);
    CHECK(heartbeats == 1);
    CHECK(records == 11);
    CHECK(playerGraph.load() == &playerIdentity);
    for (unsigned i = 0; i < 50; ++i) {
        Trace trace{Kind::translation};
        trace.SetClip("MCO_attack1");
        trace.SetPlayer(&npc, &npcIdentity);
    }
    CHECK(records == 11);
    {
        Trace trace{Kind::activation};
        trace.SetClip("MCO_attack1");
        trace.graph = &npcIdentity;
    }
    CHECK(records == 11);
    for (const auto kind : {Kind::translation, Kind::rotation}) {
        const auto before = records;
        for (unsigned i = 0; i < 10000; ++i) {
            Trace trace{kind};
            trace.SetClip("MCO_attack1");
            trace.SetPlayer(&player, &playerIdentity);
            trace.output = {1, 2, 3, 4};
        }
        CHECK(records == before + (kind == Kind::translation ? 63 : 64));
    }
    CHECK(heartbeats == 1);
    {
        sinkThrows = true;
        Trace trace{Kind::deactivation};
        trace.SetClip("MCO_attack1");
        trace.graph = &playerIdentity;
    }
    sinkThrows = false;
    CHECK(!loggedWhileLocked);
    {
        Trace trace{Kind::deactivation};
        trace.SetClip("MCO_attack1");
        trace.graph = &playerIdentity;
    }
    CHECK(records == 139);
    captureEnabled.store(false);
    const auto before = records;
    for (const auto kind : {Kind::activation, Kind::deactivation, Kind::translation, Kind::rotation}) {
        Trace trace{kind};
        trace.SetClip("MCO_attack1");
        trace.SetPlayer(&player, &playerIdentity);
    }
    CHECK(records == before);
    CHECK(heartbeats == 1);
    std::cout << "PASS: " << checks << " production diagnostic trace checks\n";
}
