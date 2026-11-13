#pragma once

#include <charconv>
#include <cmath>
#include <limits>
#include <numbers>

template <typename T>
struct Motion
{
	float time;
	T delta;
};

using Translation = Motion<RE::NiPoint3>;
using Rotation = Motion<RE::NiQuaternion>;

inline bool ParseMotionNumber(std::string_view& text, float& value) noexcept
{
	const auto begin = text.find_first_not_of(" \t\r\n");
	if (begin == std::string_view::npos) return false;
	text.remove_prefix(begin);
	const auto end = text.find_first_of(" \t\r\n");
	auto token = text.substr(0, end);
	if (token.starts_with('+')) {
		token.remove_prefix(1);
		if (token.starts_with('-')) return false;
	}
	if (token.empty() || token.starts_with('+')) return false;
	const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
	if (result.ec != std::errc{} || result.ptr != token.data() + token.size() || !std::isfinite(value)) return false;
	text.remove_prefix(end == std::string_view::npos ? text.size() : end);
	return true;
}

inline std::variant<std::monostate, Translation, Rotation>
	ParseMotionText(float time, std::string_view text) noexcept
{
	constexpr std::string_view animmotionPrefix = "animmotion ";
	constexpr std::string_view animrotationPrefix = "animrotation ";

	if (!std::isfinite(time) || time < 0.0f) return {};

	if (text.starts_with(animmotionPrefix))
	{
		auto remaining = text.substr(animmotionPrefix.size());
		RE::NiPoint3 translation{};
		if (!ParseMotionNumber(remaining, translation.x) || !ParseMotionNumber(remaining, translation.y) ||
			!ParseMotionNumber(remaining, translation.z) || remaining.find_first_not_of(" \t\r\n") != std::string_view::npos) return {};
		return Translation{ time, translation };
	}
	else if (text.starts_with(animrotationPrefix))
	{
		auto remaining = text.substr(animrotationPrefix.size());
		float yawDegrees{};
		if (!ParseMotionNumber(remaining, yawDegrees) || remaining.find_first_not_of(" \t\r\n") != std::string_view::npos) return {};

		float roll = 0.0f;
		float pitch = 0.0f;
		float yaw = (yawDegrees / 180.0f) * std::numbers::pi_v<float>;

		RE::NiQuaternion rotation
		{
			std::cos(roll / 2) * std::cos(pitch / 2) * std::cos(yaw / 2) + std::sin(roll / 2) * std::sin(pitch / 2) * std::sin(yaw / 2),
			std::sin(roll / 2) * std::cos(pitch / 2) * std::cos(yaw / 2) - std::cos(roll / 2) * std::sin(pitch / 2) * std::sin(yaw / 2),
			std::cos(roll / 2) * std::sin(pitch / 2) * std::cos(yaw / 2) + std::sin(roll / 2) * std::cos(pitch / 2) * std::sin(yaw / 2),
			std::cos(roll / 2) * std::cos(pitch / 2) * std::sin(yaw / 2) - std::sin(roll / 2) * std::sin(pitch / 2) * std::cos(yaw / 2)
		};

		return Rotation{ time, rotation };
	}

	return { };
}

inline std::variant<std::monostate, Translation, Rotation>
	ParseAnnotation(const RE::hkaAnnotationTrack::Annotation& annotation) noexcept
{
	const auto text = annotation.text.c_str();
	return text ? ParseMotionText(annotation.time, text) : std::variant<std::monostate, Translation, Rotation>{};
}
