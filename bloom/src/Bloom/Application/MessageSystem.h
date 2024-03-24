#ifndef BLOOM_APPLICATION_MESSAGESYSTEM_H
#define BLOOM_APPLICATION_MESSAGESYSTEM_H

#include <any>
#include <concepts>
#include <functional>
#include <memory>
#include <typeindex>
#include <vector>

#include <utl/hashtable.hpp>

#include "Bloom/Core/Base.h"
#include "Bloom/Core/Debug.h"

namespace bloom {

/// Message type with arbitrary user defined behaviour
struct BLOOM_API CustomCommand {
    std::function<void()> function;
};

/// Tokens to disambiguate when messages should be send
enum class DispatchToken { NextFrame, Now };

/// Used to identify registered listeners
struct ListenerID {
    int64_t index;
    std::type_index type;

    bool operator==(ListenerID const&) const = default;
};

class Emitter;
class Receiver;

/// Send messages from one component to another without the components knowing
/// about each other. `Emitter`s can send messages to the system and `Receiver`s
/// can listen the sent messages
class MessageSystem: public std::enable_shared_from_this<MessageSystem> {
    template <typename>
    struct Tag {};

public:
    /// Takes private `Tag` class because object must be created through the
    /// static `Create` function
    MessageSystem(Tag<void>);

    /// Not copyable
    MessageSystem(MessageSystem const&) = delete;

    /// Creates a message system. Since emitters and receivers hold shared and
    /// weak references to this class, this shall only exist in shared pointers
    static std::shared_ptr<MessageSystem> Create();

    /// Immediate calls all registered listeners that listen for the type of \p
    /// message and returns
    void sendNow(std::any message);

    /// Stores \p message in an internal buffer
    void sendAsync(std::any message) { buffer.push_back(std::move(message)); }

    /// Registers \p listener for listening to any messages sent to this system
    /// Call `unregister` with the returned ID to stop listening
    template <typename T, std::invocable<T> F>
    [[nodiscard]] ListenerID listen(F&& f) {
        using U = std::decay_t<T>;
        std::type_index type = typeid(U);
        listenerSets[type][index] =
            [f = std::forward<F>(f)](std::any const& message) {
            std::invoke(f, std::any_cast<U const&>(message));
        };
        return ListenerID{ index++, type };
    }

    /// \overload with automatic argument deduction
    template <typename F>
        requires requires(F&& f) { std::function{ f }; }
    [[nodiscard]] ListenerID listen(F&& f) {
        using FunctionType = decltype(std::function{ f });
        return [&]<typename R, typename T>(Tag<std::function<R(T)>>) {
            return listen<T>(std::forward<F>(f));
        }(Tag<FunctionType>{});
    }

    /// Calls `sendNow()` on all messages in the internal buffer
    void flush();

    /// Stops the listener with \p ID from listening to this system
    void unregister(ListenerID ID);

    /// Creates an emitter that emits to this message system
    Emitter makeEmitter();

    /// Creates an receiver that listens to this message system
    Receiver makeReceiver();

private:
    using TypedSet =
        utl::hashmap<int64_t, std::function<void(std::any const&)>>;

    utl::hashmap<std::type_index, TypedSet> listenerSets;
    std::vector<std::any> buffer;
    int64_t index = 0;
    bool flushing = false;
};

/// Message sender. Classes that want to send messages should privately inherit
/// from this class
class Emitter {
public:
    Emitter() = default;

    explicit Emitter(std::shared_ptr<MessageSystem> messageSystem):
        messageSystem(std::move(messageSystem)) {}

    ///
    void assignEmitter(Emitter emitter) { *this = std::move(emitter); }

    /// Dispatches a message
    void dispatch(DispatchToken token, std::any message) const {
        BL_ASSERT(messageSystem, "Cannot dispatch without a message system");
        switch (token) {
        case DispatchToken::NextFrame:
            messageSystem->sendAsync(std::move(message));
            break;
        case DispatchToken::Now:
            messageSystem->sendNow(std::move(message));
            break;
        }
    }

    /// Dispatches a `CustomCommand` constructed from \p function
    template <std::invocable F>
    void dispatch(DispatchToken token, F&& function) const {
        dispatch(token, CustomCommand{ std::forward<F>(function) });
    }

private:
    std::shared_ptr<MessageSystem> messageSystem;
};

/// Message receiver. Classes that want to receive messages should privately
/// inherit from this class
class Receiver {
public:
    Receiver() = default;

    explicit Receiver(std::weak_ptr<MessageSystem> messageSystem);

    Receiver(Receiver&&) = default;
    Receiver& operator=(Receiver&&) = default;

    ~Receiver() { unregisterAll(); };

    ///
    void assignReceiver(Receiver receiver) { *this = std::move(receiver); }

    ///
    template <typename T, std::invocable<T> F>
    void listen(F&& listener) {
        listenImpl([&](auto* msg) {
            return msg->template listen<T>(std::forward<F>(listener));
        });
    }

    /// \overload
    template <typename F>
    void listen(F&& listener) {
        listenImpl(
            [&](auto* msg) { return msg->listen(std::forward<F>(listener)); });
    }

    ///
    void unregisterAll();

private:
    void listenImpl(auto callback) {
        if (auto msg = messageSystem.lock()) {
            registeredIDs.push_back(callback(msg.get()));
        }
    }

    std::weak_ptr<MessageSystem> messageSystem;
    std::vector<ListenerID> registeredIDs;
};

} // namespace bloom

template <>
struct std::hash<bloom::ListenerID> {
    size_t operator()(bloom::ListenerID ID) const {
        return utl::hash_combine(ID.index, ID.type);
    }
};

#endif // BLOOM_APPLICATION_MESSAGESYSTEM_H
