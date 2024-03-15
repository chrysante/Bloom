#ifndef BLOOM_PLATFORM_METAL_OBJCBRIDGING_H
#define BLOOM_PLATFORM_METAL_OBJCBRIDGING_H

/// Retains \p object to be used outside of Objective-C without ARC references
/// The returned pointer must be released with a call to `bloom_release` to
/// prevent memory leaks
static inline void* bloom_retain(id object) {
    return (__bridge_retained void*)object;
}

/// Releases an object retained by `bloom_retain`
static inline void bloom_release(void* object) {
    (void)(__bridge_transfer id)object;
}

#endif // BLOOM_PLATFORM_METAL_OBJCBRIDGING_H
