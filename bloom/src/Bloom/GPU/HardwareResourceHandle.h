#ifndef BLOOM_GPU_HARDWARERESOURCEHANDLE_H
#define BLOOM_GPU_HARDWARERESOURCEHANDLE_H

#include <atomic>
#include <cstddef>

#include "Bloom/Core/Base.h"

namespace bloom {

class HardwareResourceView;

/// Reference counted resource handle. Increments the reference count when for
/// every copy of the handle and decrements it when the handle is destroyed or
/// `release()` is called manually, When the last handle is released, the
/// managed resource will be released with the given deleter.
///
/// Used as a base class for the specific resource handles
class HardwareResourceHandle {
protected:
    using Deleter = void (*)(void*);

public:
    /// \Returns the native resource handle
    void* nativeHandle() const { return _native; };

    /// Decrements the reference count. If this was the last handle with a
    /// reference to the managed resource the resource will be deleted
    void release() {
        if (!_refcount) {
            return;
        }
        if (*_refcount == 0) {
            _deleter(nativeHandle());
            delete _refcount;
        }
        else {
            --*_refcount;
        }
        _refcount = nullptr;
    }

    /// \Returns `*this != nullptr`
    explicit operator bool() const { return *this != nullptr; }

    /// \Returns `true` if \p *this manages the same resource as \p rhs
    bool operator==(HardwareResourceHandle const& rhs) const {
        return _refcount && rhs._refcount &&
               nativeHandle() == rhs.nativeHandle();
    }

    /// \Returns `true` if \p *this manages the same resource that \p rhs points
    /// to
    bool operator==(HardwareResourceView rhs) const;

    /// \Returns `true` if the handle manages no resource
    bool operator==(std::nullptr_t) const {
        return !_refcount || !nativeHandle();
    }

protected:
    /// Constructs an empty handle
    HardwareResourceHandle() = default;

    /// Constructs a handle managing a native resource
    HardwareResourceHandle(void* native, Deleter deleter):
        _refcount(new std::atomic<std::size_t>{ 0 }),
        _native(native),
        _deleter(deleter) {}

    ///  All special member functions are protected to prevent object slicing

    HardwareResourceHandle(HardwareResourceHandle&& rhs) noexcept:
        _refcount(rhs._refcount), _native(rhs._native), _deleter(rhs._deleter) {
        rhs._refcount = nullptr;
    }

    HardwareResourceHandle(HardwareResourceHandle const& rhs):
        _refcount(rhs._refcount), _native(rhs._native), _deleter(rhs._deleter) {
        if (_refcount) {
            ++*_refcount;
        }
    }

    HardwareResourceHandle& operator=(HardwareResourceHandle&& rhs) noexcept {
        if (this == &rhs) {
            return *this;
        }
        release();
        _refcount = rhs._refcount;
        _native = rhs._native;
        _deleter = rhs._deleter;
        rhs._refcount = nullptr;
        return *this;
    }

    HardwareResourceHandle& operator=(HardwareResourceHandle const& rhs) {
        /// If we are the same handle of manage the same resource we return
        /// early
        if (this == &rhs || *this == rhs) {
            return *this;
        }
        release();
        _refcount = rhs._refcount;
        _native = rhs._native;
        _deleter = rhs._deleter;
        if (_refcount) {
            ++*_refcount;
        }
        return *this;
    }

    ~HardwareResourceHandle() { release(); }

private:
    /// This is nonnull _iff_ the handle manages a resource
    std::atomic<std::size_t>* _refcount = nullptr;
    void* _native = nullptr;
    Deleter _deleter = nullptr;
};

/// Non-owning view of a resource. Can be implicitly constructed from an owning
/// `HardwareResourceHandle`
class HardwareResourceView {
public:
    /// \Returns the native resource handle
    void* nativeHandle() const { return _native; };

    /// \Returns `*this != nullptr`
    explicit operator bool() const { return *this != nullptr; }

    /// \Returns `true` if \p *this views the same resource as \p rhs
    bool operator==(HardwareResourceView const& rhs) const {
        return nativeHandle() == rhs.nativeHandle();
    }

    /// \Returns `true` if the handle manages no resource
    bool operator==(std::nullptr_t) const { return !nativeHandle(); }

protected:
    HardwareResourceView() = default;
    HardwareResourceView(HardwareResourceHandle const& handle):
        _native(handle.nativeHandle()) {}
    HardwareResourceView(void* native): _native(native) {}

private:
    void* _native = nullptr;
};

} // namespace bloom

inline bool bloom::HardwareResourceHandle::operator==(
    HardwareResourceView rhs) const {
    return _refcount && nativeHandle() == rhs.nativeHandle();
}

#endif // BLOOM_GPU_HARDWARERESOURCEHANDLE_H
