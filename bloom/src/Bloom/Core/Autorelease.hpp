#include "Bloom/Core/Base.hpp"

#ifdef BLOOM_PLATFORM_APPLE

#define BLOOM_AUTORELEASE_BEGIN ::bloom::autoreleased([&]{
#define BLOOM_AUTORELEASE_END   });

#else

#define BLOOM_AUTORELEASE_BEGIN
#define BLOOM_AUTORELEASE_END   


#endif

#ifdef BLOOM_PLATFORM_APPLE

#include <utl/functional.hpp>

namespace bloom {
	
	void BLOOM_API autoreleased(utl::function<void()> const&);
	
}


#endif
