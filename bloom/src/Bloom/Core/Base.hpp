#pragma once

#if defined(__cplusplus) && !defined(__METAL_VERSION__)
#	define BLOOM_CPP
#elif defined(__METAL_VERSION__)
#	define BLOOM_METAL
#else
#	error Unsupported Language
#endif

#if defined(BLOOM_CPP)
#	define BLM_CONSTANT constexpr
#elif defined(BLOOM)
#	define BLM_CONSTANT constant
#endif

#if defined(__GNUC__) || defined(__clang__)
#	define BLOOM_API __attribute__((visibility("default")))
#	define BLOOM_WEAK_API BLOOM_API __attribute__((weak))
#elif defined(_MSC_VER)
#	define BLOOM_API __declspec(dllexport)
#	define BLOOM_WEAK_API
#endif

#ifdef __APPLE__
#	define BLOOM_PLATFORM_APPLE
#endif
