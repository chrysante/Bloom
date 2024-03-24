#include "Bloom/Application/MessageSystem.h"

using namespace bloom;

MessageSystem::MessageSystem(Tag<void>) {}

std::shared_ptr<MessageSystem> MessageSystem::Create() {
    return std::make_shared<MessageSystem>(Tag<void>{});
}

void MessageSystem::sendNow(std::any message) {
    auto& set = listenerSets[std::type_index(message.type())];
    for (auto& [index, listener]: set) {
        listener(message);
    }
}

void MessageSystem::sendAsync(std::any message) {
    std::lock_guard lock(bufferMutex);
    buffer.push_back(std::move(message));
}

void MessageSystem::flush() {
    /// Guard this here to prevent feedback loops
    if (flushing) {
        return;
    }
    flushing = true;
    auto messages = [this] {
        std::lock_guard lock(bufferMutex);
        return std::move(buffer);
    }();
    for (auto& msg: messages) {
        try {
            sendNow(std::move(msg));
        }
        catch (...) {
            std::terminate();
        }
    }
    flushing = false;
}

void MessageSystem::unregister(ListenerID ID) {
    auto itr = listenerSets.find(ID.type);
    BL_EXPECT(itr != listenerSets.end(), "Invalid ID");
    auto& set = itr->second;
    auto setItr = set.find(ID.index);
    BL_EXPECT(setItr != set.end(), "Invalid ID");
    set.erase(setItr);
}

Emitter MessageSystem::makeEmitter() { return Emitter(shared_from_this()); }

Receiver MessageSystem::makeReceiver() { return Receiver(shared_from_this()); }

Receiver::Receiver(std::weak_ptr<MessageSystem> messageSystem):
    messageSystem(std::move(messageSystem)) {}

void Receiver::unregisterAll() {
    auto msg = messageSystem.lock();
    if (!msg) {
        return;
    }
    for (auto& ID: registeredIDs) {
        msg->unregister(ID);
    }
    registeredIDs.clear();
}
