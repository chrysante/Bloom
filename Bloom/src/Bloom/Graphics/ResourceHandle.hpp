#pragma once

#include "Bloom/Core/Base.hpp"

#include <cstddef>
#include <atomic>


namespace bloom {
	
	class BLOOM_API ResourceHandle {
	protected:
		using Deleter = void(*)(void*);
		
	public:
		ResourceHandle() = default;
		ResourceHandle(void* native, Deleter deleter):
			_refcount(new std::atomic<std::size_t>{ 0 }),
			_native(native),
			_deleter(deleter)
		{}
		ResourceHandle(ResourceHandle&& rhs) noexcept:
			_refcount(rhs._refcount),
			_native(rhs._native),
			_deleter(rhs._deleter)
		{
			rhs._refcount = nullptr;
		}
		ResourceHandle(ResourceHandle const& rhs):
			_refcount(rhs._refcount),
			_native(rhs._native),
			_deleter(rhs._deleter)
		{
			if (_refcount) {
				++*_refcount;
			}
		}
		ResourceHandle& operator=(ResourceHandle&& rhs) noexcept {
			release();
			this->_refcount = rhs._refcount;
			this->_native = rhs._native;
			this->_deleter = rhs._deleter;
			
			rhs._refcount = nullptr;
			
			return *this;
		}
		ResourceHandle& operator=(ResourceHandle const& rhs) {
			release();
			this->_refcount = rhs._refcount;
			this->_native = rhs._native;
			this->_deleter = rhs._deleter;
					
			if (_refcount) {
				++*_refcount;
			}
			
			return *this;
		}
		~ResourceHandle() { release(); }
		
		void* nativeHandle() { return _native; };
		void const* nativeHandle() const { return _native; };
		
		void release() {
			if (!_refcount) {
				return;
			}
			if (*_refcount == 0) {
				_deleter(_native);
				delete _refcount;
				_refcount = nullptr;
			}
			else {
				--*_refcount;
			}
		}
		
		explicit operator bool() const { return _refcount && _native; }
		
	private:
		std::atomic<std::size_t>* _refcount = nullptr;
		void* _native = nullptr;
		Deleter _deleter = nullptr;
	};
	
	class BLOOM_API ResourceView {
	protected:
		ResourceView(ResourceHandle& resource): _native(resource.nativeHandle()) {}

	public:
		ResourceView() = default;
		ResourceView(std::nullptr_t): ResourceView() {} 
		
		void* nativeHandle() { return _native; };
		void const* nativeHandle() const { return _native; };
		
		explicit operator bool() const { return _native; }
		
	private:
		void* _native = nullptr;
	};
	
}
