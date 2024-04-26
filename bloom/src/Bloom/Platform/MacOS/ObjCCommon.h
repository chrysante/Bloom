#ifndef BLOOM_PLATFORM_MACOS_OBJCCOMMON_H
#define BLOOM_PLATFORM_MACOS_OBJCCOMMON_H

#if !__has_feature(objc_arc)
#warning ARC is disabled. This build will leak memory
#endif

#endif // BLOOM_PLATFORM_MACOS_OBJCCOMMON_H
