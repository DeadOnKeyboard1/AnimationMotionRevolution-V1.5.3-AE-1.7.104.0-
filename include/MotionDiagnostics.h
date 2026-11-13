#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <string_view>

namespace amr::diagnostics
{
    class SampleBudget
    {
    public:
        bool Take() noexcept
        {
            auto value = attempts.load(std::memory_order_relaxed);
            while (value < limit) {
                if (attempts.compare_exchange_weak(value, value + 1, std::memory_order_relaxed)) {
                    return value < 8 || (value - 8) % 32 == 0;
                }
            }
            return false;
        }

    private:
        static constexpr std::uint32_t limit = 8 + 56 * 32;
        std::atomic<std::uint32_t> attempts{};
    };

    inline bool IsAttackClip(std::string_view name) noexcept
    {
        constexpr auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
        for (const auto word : {std::string_view{"attack"}, std::string_view{"mco"}, std::string_view{"dodge"}}) {
            const auto found = std::search(name.begin(), name.end(), word.begin(), word.end(),
                [&](char a, char b) { return lower(a) == b; });
            if (found != name.end()) return true;
        }
        return false;
    }

    inline std::array<char, 72> Label(std::string_view text) noexcept
    {
        const auto slash = text.find_last_of("\\/");
        if (slash != std::string_view::npos) text.remove_prefix(slash + 1);
        // A label is never an operational path or a full replacer path.
        if (text.find(':') != std::string_view::npos) text = ">DoK<";
        std::array<char, 72> result{};
        const auto count = std::min(text.size(), result.size() - 1);
        for (std::size_t i = 0; i < count; ++i) {
            result[i] = text[i] >= 32 && text[i] < 127 ? text[i] : '?';
        }
        return result;
    }

#ifdef AMR_MOTION_DIAGNOSTICS
    // Set once during plugin initialization, before hooks are installed.
    inline std::atomic<bool> captureEnabled{false};
    inline std::atomic<const RE::hkbCharacter*> playerGraph{};
    inline SampleBudget activationBudget;
    inline SampleBudget translationBudget;
    inline SampleBudget rotationBudget;
    inline SampleBudget deactivationBudget;
    inline std::atomic<unsigned> initialActivations{};
    inline std::atomic<bool> announcedPlayer{};

    struct Trace
    {
        enum class Kind { activation, deactivation, translation, rotation };
        explicit Trace(Kind value) noexcept : kind(value), capturing(captureEnabled.load(std::memory_order_relaxed)) {}
        Kind kind;
        const bool capturing;
        bool enabled{};
        bool player{};
        bool firstPlayer{};
        bool bound{};
        bool custom{};
        bool cacheHit{};
        const RE::hkbCharacter* graph{};  // Identity only; never dereferenced or retained by a worker.
        std::array<char, 72> clip{};
        std::array<char, 72> animation{};
        std::size_t tracks{};
        std::size_t annotations{};
        std::size_t translations{};
        std::size_t rotations{};
        int active{};
        float time{};
        std::array<float, 4> output{};

        void SetClip(const char* text) noexcept
        {
            if (!capturing) return;
            const auto view = text ? std::string_view{text} : std::string_view{};
            enabled = IsAttackClip(view);
            clip = Label(view);
        }

        void SetPlayer(RE::Character* actor, const RE::hkbCharacter* value) noexcept
        {
            if (!capturing) return;
            player = actor && actor->GetFormID() == 0x14;
            graph = value;
            if (player && graph) playerGraph.store(graph, std::memory_order_relaxed);
            firstPlayer = player && !announcedPlayer.exchange(true, std::memory_order_relaxed);
            // Collector labels need not be named like attack generators. Do not filter player output by name.
            enabled = player;
        }

        ~Trace() noexcept
        {
            if (!capturing) return;
            // This object precedes the cache guard: formatting and disk IO run after unlocking.
            try {
                if (firstPlayer) {
                    logger::info("[AMR-DIAG] Player motion hook reached; graph_available={}", graph != nullptr);
                }
                if (!enabled) return;
                if (kind == Kind::activation || kind == Kind::deactivation) {
                    const auto knownPlayer = playerGraph.load(std::memory_order_relaxed);
                    player = graph && graph == knownPlayer;
                    if (!player && (knownPlayer || initialActivations.fetch_add(1, std::memory_order_relaxed) >= 16)) return;
                }
                auto& budget = kind == Kind::activation ? activationBudget :
                    kind == Kind::deactivation ? deactivationBudget :
                    kind == Kind::translation ? translationBudget : rotationBudget;
                if (!budget.Take()) return;
                if (kind == Kind::activation || kind == Kind::deactivation) {
                    logger::info("[AMR-DIAG] {} scope={} clip={} animation={} bound={} tracks={} annotations={} cached={} customT={} customR={} active={}",
                        kind == Kind::activation ? "activate" : "deactivate", player ? "player" : "unclassified",
                        clip.data(), animation.data(), bound, tracks, annotations, cacheHit, translations, rotations, active);
                } else {
                    logger::info("[AMR-DIAG] {} scope=player clip={} graph={} cached={} customT={} customR={} route={} time={:.4f} output=({:.4f},{:.4f},{:.4f},{:.4f})",
                        kind == Kind::translation ? "translation" : "rotation", clip.data(), graph != nullptr,
                        cacheHit, translations, rotations, custom ? "annotation" : "native", time,
                        output[0], output[1], output[2], output[3]);
                }
            } catch (...) {
                // Diagnostic sink failures must not alter the engine result or unwind through Havok.
            }
        }
    };
#endif
}
