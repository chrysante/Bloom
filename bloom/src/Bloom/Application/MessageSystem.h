#ifndef BLOOM_APPLICATION_MESSAGESYSTEM_H
#define BLOOM_APPLICATION_MESSAGESYSTEM_H

#include <utl/functional.hpp>
#include <utl/messenger.hpp>

#include "Bloom/Core/Base.h"

namespace bloom {

/// Message type with arbitrary user defined behaviour
struct BLOOM_API CustomCommand {
    std::function<void()> function;
};

/// Tokens to disambiguate when messages should be send
enum class DispatchToken { NextFrame, Now };

/// Message sender. Classes that want to send messages should privately inherit
/// from this class
class Emitter: private utl::emitter<utl::buffered_messenger> {
    friend class Messenger;

    using emitter::emitter;

public:
    /// Dispatches a message
    void dispatch(DispatchToken token, std::any message) {
        using enum DispatchToken;
        switch (token) {
        case NextFrame:
            emitter::send(std::move(message));
            break;
        case Now:
            emitter::send_now(std::move(message));
            break;
        }
    }

    /// Dispatches a `CustomCommand` constructed from \p function
    template <std::invocable F>
    void dispatch(DispatchToken token, F&& function) {
        dispatch(token, CustomCommand{ std::forward<F>(function) });
    }
};

/// Message receiver. Classes that want to receive messages should privately
/// inherit from this class
class Receiver: private utl::receiver<utl::buffered_messenger> {
public:
    friend class Messenger;

    using MyBase = utl::receiver<utl::buffered_messenger>;

    using receiver::receiver;

public:
    using receiver::listen;
};

class MessageSystem {
public:
    MessageSystem(): m(std::make_unique<utl::buffered_messenger>()) {}

    Emitter makeEmitter() { return Emitter(m); }

    Receiver makeReceiver() { return Receiver(m); }

    void flush() { m->flush(); }

private:
    std::shared_ptr<utl::buffered_messenger> m;
};

} // namespace bloom

#endif // BLOOM_APPLICATION_MESSAGESYSTEM_H
