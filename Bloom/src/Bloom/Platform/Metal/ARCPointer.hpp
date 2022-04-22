#pragma once

#include <utl/concepts.hpp>
#include <Foundation/Foundation.hpp>
#include <string_view>

namespace bloom {

	namespace internal {
		template <typename T, typename U>
		std::true_type _isNSReferencing(NS::Referencing<T, U> const&);
		std::false_type _isNSReferencing(...);
		template <typename T>
		struct IsNSReferencing: decltype(_isNSReferencing(std::declval<T>())) {};
		
		template <typename T>
		concept NSReferencing = IsNSReferencing<T>::value;
		
		template <typename T>
		struct NSReferencingDeleter {
			void operator()(T* ptr) {
				ptr->release();
			}
		};
	}

	template <typename T>
	requires internal::IsNSReferencing<T>::value
	class ARCPointer: public std::unique_ptr<T, internal::NSReferencingDeleter<T>> {
		using _base = std::unique_ptr<T, internal::NSReferencingDeleter<T>>;
	public:
		ARCPointer() = default;
		ARCPointer(T* ptr): _base(ptr) {}
	};
	
	inline auto makeNSString(std::string_view str) {
		return NS::String::string(str.data(), NS::StringEncoding::ASCIIStringEncoding);
	}
	
}

