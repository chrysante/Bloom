#ifndef POPPY_UI_VIEW_H
#define POPPY_UI_VIEW_H

#include <optional>
#include <string>

#include <utl/hashmap.hpp>
#include <utl/messenger.hpp>
#include <vml/vml.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Application/InputEvent.h"
#include "Bloom/Application/MessageSystem.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/UI/CommandPalette.h"

namespace bloom {

class Window;

} // namespace bloom

namespace poppy {

class Editor;

///
struct ViewDescription {
    ///
    std::string name;

    ///
    std::string title;

    ///
    vml::int2 size = { 300, 300 };

    ///
    vml::float2 padding = -1;

    ///
    int id = -1; // Used to restore serialized Views. -1 means the View will use
                 // a new ID.
};

struct ViewRegisterDescription {
    bool unique = true;
    bool persistent = true;
};

class View: protected bloom::Emitter, protected bloom::Receiver {
    friend class Editor;

public:
    virtual ~View() = default;

    bool focused() const;
    bool maximized() const { return desc.maximized; }
    bool shouldClose() const { return !desc.open; }

    int id() const { return desc.pub.id; }

    vml::int2 size() const { return desc.pub.size; }
    vml::int2 windowSize() const { return desc.windowSize; }
    vml::int2 position() const { return desc.position; }
    vml::float2 padding() const { return desc.pub.padding; }

    std::string_view name() const { return desc.pub.name; }
    std::string_view title() const { return desc.pub.title; }
    ViewDescription const& description() const { return desc.pub; }

    vml::float2 windowSpaceToViewSpace(vml::float2) const;
    vml::float2 viewSpaceToWindowSpace(vml::float2) const;

    Editor& editor() const { return *desc.editor; }
    bloom::Window& window() const { return *desc.window; }

    ViewRegisterDescription registerDescription() const {
        return mRegisterDescription;
    }

    void setFocused();
    void maximize();
    void restore();
    void toggleMaximize();

    void setPadding(vml::float2 padding);

    void setTitle(std::string);

    /// \Returns the attached command palette
    CommandPalette const& commandPalette() const { return cmdPalette; }

    /// Used by window backend to close imgui window of closing platform window
    void setClosing() { desc.open = false; }

protected:
    /// \Returns a mutable reference to the attached command palette
    CommandPalette& commandPaletteWrite() { return cmdPalette; }

    ///
    void setBGColor(std::optional<vml::float4> color) { desc.bgColor = color; }

private:
    virtual void init() {};
    virtual void shutdown() {};
    virtual void frame() {};
    virtual void onInput(bloom::InputEvent&) {}
    virtual YAML::Node serialize() const { return YAML::Node{}; }
    virtual void deserialize(YAML::Node) {}

    void doInit();
    void doShutdown();
    void doFrame();
    void doOnInput(bloom::InputEvent&);

    YAML::Node doSerialize() const;
    static std::unique_ptr<View> doDeserialize(YAML::Node const&);

    struct ViewDescPrivate {
        ViewDescription pub;

        std::optional<vml::float4> bgColor = {};
        vml::int2 position = 0;
        vml::int2 windowSize = 0;

        void* imguiWindow = nullptr;
        Editor* editor = nullptr;
        bloom::Window* window = nullptr;

        bool hasPaddingX = false;
        bool hasPaddingY = false;
        bool open = true;
        bool maximized = false;
        bool isMoveable = true;
    };
    ViewDescPrivate desc;
    ViewRegisterDescription mRegisterDescription;
    CommandPalette cmdPalette;
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
            Logger::Error("View is already registered");
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
        entry.factory = []() { return std::make_unique<Type>(); };             \
        entry.description = __VA_ARGS__;                                       \
        entry.name = Name;                                                     \
        ViewRegistry::add(Name, entry);                                        \
    }

} // namespace poppy

#include "Bloom/Core/Serialize.h"

BLOOM_MAKE_TEXT_SERIALIZER(poppy::ViewDescription, name, size, padding, id);

#endif // POPPY_UI_VIEW_H
