#pragma once

#include "Bloom/Core/Base.hpp"

#include <cstddef>
#include <atomic>


namespace bloom {
	
	class BLOOM_API HardwareResourceHandle {
	protected:
		using Deleter = void(*)(void*);
		
	public:
		HardwareResourceHandle() = default;
		HardwareResourceHandle(void* native, Deleter deleter):
			_refcount(new std::atomic<std::size_t>{ 0 }),
			_native(native),
			_deleter(deleter)
		{}
		HardwareResourceHandle(HardwareResourceHandle&& rhs) noexcept:
			_refcount(rhs._refcount),
			_native(rhs._native),
			_deleter(rhs._deleter)
		{
			rhs._refcount = nullptr;
		}
		HardwareResourceHandle(HardwareResourceHandle const& rhs):
			_refcount(rhs._refcount),
			_native(rhs._native),
			_deleter(rhs._deleter)
		{
			if (_refcount) {
				++*_refcount;
			}
		}
		HardwareResourceHandle& operator=(HardwareResourceHandle&& rhs) noexcept {
			if (this == &rhs || this->_native == rhs._native) {
				return *this;
			}
			release();
			this->_refcount = rhs._refcount;
			this->_native = rhs._native;
			this->_deleter = rhs._deleter;
			
			rhs._refcount = nullptr;
			
			return *this;
		}
		HardwareResourceHandle& operator=(HardwareResourceHandle const& rhs) {
			if (this == &rhs || this->_native == rhs._native) {
				return *this;
			}
			release();
			this->_refcount = rhs._refcount;
			this->_native = rhs._native;
			this->_deleter = rhs._deleter;
					
			if (_refcount) {
				++*_refcount;
			}
			
			return *this;
		}
		~HardwareResourceHandle() { release(); }
		
		void* nativeHandle() const { return _native; };
		
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
	
	class BLOOM_API HardwareResourceView {
	protected:
		HardwareResourceView(HardwareResourceHandle const& resource): _native(resource.nativeHandle()) {}
		HardwareResourceView(void* native): _native(native) {}
		
	public:
		HardwareResourceView() = default;
		HardwareResourceView(std::nullptr_t): HardwareResourceView() {}
		
		void* nativeHandle() const { return _native; };
		
		explicit operator bool() const { return _native; }
		
	private:
		void* _native = nullptr;
	};
	
}
