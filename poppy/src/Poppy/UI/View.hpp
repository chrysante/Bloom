#pragma once

#include <optional>
#include <string>

#include <mtl/mtl.hpp>
#include <utl/hashmap.hpp>
#include <utl/messenger.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Application/InputEvent.hpp"
#include "Bloom/Application/MessageSystem.hpp"
#include "Poppy/Core/Debug.hpp"

namespace bloom {

class Window;

}

namespace poppy {

struct ViewDescription {
    std::string const& name() const { return _name_DONT_CHANGE_ME; }
    std::string _name_DONT_CHANGE_ME;

    std::string title;

    mtl::int2 size      = { 300, 300 };
    mtl::float2 padding = -1;

    int id = -1; // Used to restore serialized Views. -1 means the View will use
                 // a new ID.
};

struct ViewRegisterDescription {
    bool unique     = true;
    bool persistent = true;
};

class Editor;

class View: protected bloom::Emitter, protected bloom::Reciever {
    friend class Editor;

public:
    virtual ~View() = default;

    /// MARK: Queries
    bool focused() const;
    bool maximized() const { return desc.maximized; }
    bool shouldClose() const { return !desc.open; }

    int id() const { return desc.pub.id; }

    mtl::int2 size() const { return desc.pub.size; }
    mtl::int2 windowSize() const { return desc.windowSize; }
    mtl::int2 position() const { return desc.position; }
    mtl::float2 padding() const { return desc.pub.padding; }

    std::string_view name() const { return desc.pub.name(); }
    std::string_view title() const { return desc.pub.title; }
    ViewDescription const& description() const { return desc.pub; }

    mtl::float2 windowSpaceToViewSpace(mtl::float2) const;
    mtl::float2 viewSpaceToWindowSpace(mtl::float2) const;

    Editor& editor() const { return *desc.editor; }
    bloom::Window& window() const { return *desc.window; }

    ViewRegisterDescription registerDescription() const {
        return mRegisterDescription;
    }

    /// MARK: Modifiers
    void setFocused();
    void maximize();
    void restore();
    void toggleMaximize();

    void setPadding(mtl::float2 padding);

    void setTitle(std::string);

private:
    /// MARK: Virtual Interface
    virtual void init(){};
    virtual void shutdown(){};
    virtual void frame(){};
    virtual void onInput(bloom::InputEvent&) {}
    virtual YAML::Node serialize() const { return YAML::Node{}; }
    virtual void deserialize(YAML::Node) {}

protected:
    /// MARK: Convinience Helpers
    void displayEmptyWithReason(std::string_view reason) const;

private:
    /// MARK: Private
    void doInit();
    void doShutdown();
    void doFrame();
    void doOnInput(bloom::InputEvent&);

    YAML::Node doSerialize() const;
    static std::unique_ptr<View> doDeserialize(YAML::Node const&);

private:
    struct ViewDescPrivate {
        ViewDescription pub;

        mtl::int2 position   = 0;
        mtl::int2 windowSize = 0;

        void* imguiWindow     = nullptr;
        Editor* editor        = nullptr;
        bloom::Window* window = nullptr;

        bool hasPaddingX = false;
        bool hasPaddingY = false;
        bool open        = true;
        bool maximized   = false;
    };
    ViewDescPrivate desc;
    ViewRegisterDescription mRegisterDescription;
};

class ViewRegistry {
public:
    using Factory = utl::function<std::unique_ptr<View>()>;

    struct Entry {
        ViewRegisterDescription description;
        Factory factory;
        std::string name;
    };

    static void add(std::string name, Entry entry) {
        auto [_, success] = instance().entries.insert({ name, entry });
        if (!success) {
            poppyLog(fatal, "View '{}' is already registered.");
            BL_DEBUGFAIL();
        }
    }

    static std::optional<Entry> get(std::string name) {
        auto const itr = instance().entries.find(name);
        if (itr == instance().entries.end()) {
            return std::nullopt;
        }
        return itr->second;
    }

    static void forEach(std::invocable<std::string, Entry> auto&& block) {
        for (auto&& [name, entry]: instance().entries) {
            block(name, entry);
        }
    }

private:
    static ViewRegistry& instance() {
        static ViewRegistry instance;
        return instance;
    }

private:
    utl::hashmap<std::string, Entry> entries;
};

#define POPPY_REGISTER_VIEW(Type, Name, ...)                                   \
    UTL_STATIC_INIT {                                                          \
        ::poppy::ViewRegistry::Entry entry;                                    \
        entry.factory     = []() { return std::make_unique<Type>(); };         \
        entry.description = __VA_ARGS__;                                       \
        entry.name        = Name;                                              \
        ViewRegistry::add(Name, entry);                                        \
    };

} // namespace poppy

#include "Bloom/Core/Serialize.hpp"

BLOOM_MAKE_TEXT_SERIALIZER(
    poppy::ViewDescription, _name_DONT_CHANGE_ME, size, padding, id);
