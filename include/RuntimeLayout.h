#pragma once

#include <RE/H/hkbClipGenerator.h>
#include <RE/H/hkaAnimation.h>
#include <RE/M/MotionDataContainer.h>

// The fields AMR actually reads must retain its established SE/AE layout.
static_assert(sizeof(RE::hkbClipGenerator) == 0x110);
static_assert(offsetof(RE::hkbClipGenerator, name) == 0x38);
static_assert(offsetof(RE::hkbClipGenerator, animationName) == 0x48);
static_assert(offsetof(RE::hkbClipGenerator, binding) == 0xA0);
static_assert(offsetof(RE::hkaAnimationBinding, animation) == 0x18);
static_assert(sizeof(RE::hkaAnimation) == 0x38);
static_assert(offsetof(RE::hkaAnimation, duration) == 0x14);
static_assert(offsetof(RE::hkaAnimation, annotationTracks) == 0x28);
static_assert(sizeof(RE::hkaAnnotationTrack::Annotation) == 0x10);
static_assert(offsetof(RE::hkaAnnotationTrack::Annotation, time) == 0);
static_assert(offsetof(RE::hkaAnnotationTrack::Annotation, text) == 8);
static_assert(offsetof(RE::hkbContext, character) == 0);
static_assert(offsetof(RE::BShkbAnimationGraph, characterInstance) == 0xC0);
static_assert(sizeof(RE::MotionDataContainer) == 0x20);
static_assert(offsetof(RE::MotionDataContainer, translationDataPtr) == 0);
static_assert(offsetof(RE::MotionDataContainer, translationSegCount) == 8);
static_assert(offsetof(RE::MotionDataContainer, rotationDataPtr) == 0x10);
static_assert(offsetof(RE::MotionDataContainer, rotationSegCount) == 0x18);
