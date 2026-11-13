#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <optional>
#include <span>
#include <hde64.h>

namespace utils
{
	enum class ContextRegister { r14, rsi };

	inline std::optional<hde64s> InstructionAt(std::span<const std::uint8_t> code, std::size_t offset) noexcept
	{
		if (offset >= code.size()) return std::nullopt;
		std::size_t cursor{};
		while (cursor <= offset) {
			std::uint8_t padded[16]{};
			const auto count = std::min(sizeof(padded), code.size() - cursor);
			std::memcpy(padded, code.data() + cursor, count);
			hde64s instruction{};
			const auto length = hde64_disasm(padded, &instruction);
			if (!length || instruction.flags & F_ERROR || length > count) return std::nullopt;
			if (cursor == offset) return instruction;
			cursor += length;
		}
		return std::nullopt;
	}

	inline bool IsCallBoundary(std::span<const std::uint8_t> code, std::size_t offset) noexcept
	{
		const auto instruction = InstructionAt(code, offset);
		return instruction && instruction->len == 5 && instruction->opcode == 0xe8;
	}

	inline std::optional<ContextRegister> DeactivationContext(std::span<const std::uint8_t> code, std::size_t offset) noexcept
	{
		if (!IsCallBoundary(code, offset) || offset < 3) return std::nullopt;
		// Accept the actual argument capture immediately before the CALL, optionally followed by this->rbx.
		for (const auto length : { std::size_t{3}, std::size_t{6} }) {
			if (offset < length) continue;
			const auto capture = offset - length;
			const auto instruction = InstructionAt(code, capture);
			if (!instruction || instruction->len != 3) continue;
			if (length == 6 && std::memcmp(code.data() + capture + 3, "\x48\x8b\xd9", 3) != 0) continue;
			if (std::memcmp(code.data() + capture, "\x4c\x8b\xf2", 3) == 0) return ContextRegister::r14;
			if (std::memcmp(code.data() + capture, "\x48\x8b\xf2", 3) == 0) return ContextRegister::rsi;
		}
		return std::nullopt;
	}
}
