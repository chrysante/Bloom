#pragma once

#include <string>
#include <mtl/mtl.hpp>
#include <utl/messenger.hpp>
#include <utl/hashmap.hpp>
#include <yaml-cpp/yaml.h>

#include "Poppy/Core/Debug.hpp"
#include "Bloom/Application/MessageSystem.hpp"
#include "Bloom/Application/InputEvent.hpp"

namespace bloom { class Window; }

namespace poppy {
	
	struct ViewDescription {
		std::string name;
		
		mtl::int2 size = { 300, 300 };
		mtl::float2 padding = -1;
		
		int id = -1; // Used to restore serialized Views. -1 means the View will use a new ID.
	};
	
	class Editor;
	
	class View:
		protected bloom::Emitter,
		protected bloom::Reciever
	{
		friend class Editor;
		
	public:
		/// MARK: Initialization
		View(std::string title): View(ViewDescription{ title }) {}
		View(ViewDescription const& = {});
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
		
		std::string_view title() const { return desc.pub.name; }
		ViewDescription const& description() const { return desc.pub; }
		
		mtl::float2 windowSpaceToViewSpace(mtl::float2) const;
		mtl::float2 viewSpaceToWindowSpace(mtl::float2) const;
		
		Editor& editor() const { return *desc.editor; }
		bloom::Window& window() const { return *desc.window; }
		
		/// MARK: Modifiers
		void setFocused();
		void maximize();
		void restore();
		void toggleMaximize();
		
		void setPadding(mtl::float2 padding);
		
	private:
		/// MARK: Virtual Interface
		virtual void init() {};
		virtual void shutdown() {};
		virtual void frame() = 0;
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
		
		virtual bool isUnique() const { return false; }
		
	private:
		struct ViewDescPrivate {
			ViewDescription pub;
			
			mtl::int2 position = 0;
			mtl::int2 windowSize = 0;
			
			void* imguiWindow = nullptr;
			Editor* editor = nullptr;
			bloom::Window* window = nullptr;
			
			bool hasPaddingX = false;
			bool hasPaddingY = false;
			bool open = true;
			bool maximized = false;
		};
		
		ViewDescPrivate desc;
	};
	
	class UniqueView: public View {
		friend class Editor;
	public:
		using View::View;
		
	private:
		bool isUnique() const override { return true; }
	};
	
	class ViewRegistry {
	public:
		using Factory = utl::function<std::unique_ptr<View>()>;
		
		static void add(std::string name, Factory factory) {
			auto [_, success] = instance().factories.insert({ name, factory });
			if (!success) {
				poppyLog(fatal, "View '{}' is already registered.");
				poppyDebugfail();
			}
		}
		
		static Factory getFactory(std::string name) {
			auto const itr = instance().factories.find(name);
			if (itr == instance().factories.end()) {
				return Factory{};
			}
			return itr->second;
		}

		static void forEach(std::invocable<std::string, Factory> auto&& block) {
			for (auto&& [name, factory]: instance().factories) {
				block(name, factory);
			}
		}
		
	private:
		static ViewRegistry& instance() {
			static ViewRegistry instance;
			return instance;
		}
		
	private:
		utl::hashmap<std::string, Factory> factories;
	};

#define POPPY_REGISTER_VIEW(Type, Name)                                       \
UTL_STATIC_INIT {                                                             \
	::poppy::ViewRegistry::add(Name, [](){ return std::make_unique<Type>(); }); \
};
	
}


#include "Bloom/Core/Serialize.hpp"

BLOOM_MAKE_TEXT_SERIALIZER(poppy::ViewDescription,
						   name,
						   size,
						   padding,
						   id);
