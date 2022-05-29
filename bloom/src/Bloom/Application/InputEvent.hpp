#pragma once

#include <mtl/mtl.hpp>
#include <utl/common.hpp>
#include <array>
#include <iosfwd>
#include <variant>
#include <type_traits>
#include <concepts>

#include "Input.hpp"
#include "Bloom/Core/Debug.hpp"

namespace bloom {
	
	struct BLOOM_API InputEventBase {
		ModFlags modifierFlags;
	};
	
	struct BLOOM_API MouseEvent: InputEventBase {
		mtl::double2 locationInWindow;
	};
	
	struct BLOOM_API MouseDownEvent: MouseEvent {
		MouseButton button;
		long clickCount;
	};
	
	struct BLOOM_API MouseUpEvent: MouseEvent {
		MouseButton button;
	};
	
	struct BLOOM_API MouseMoveEvent: MouseEvent {
		mtl::double2 offset;
	};
	
	struct BLOOM_API MouseDragEvent: MouseMoveEvent {
		MouseButton button;
	};
	
	struct BLOOM_API ScrollEvent: MouseEvent {
		mtl::double2 offset;
		bool isTrackpad;
	};
	
	struct BLOOM_API MagnificationEvent: MouseEvent {
		double offset;
	};
	
	struct BLOOM_API KeyEvent: InputEventBase {
		Key key;
		int repeat;
	};
	
	enum struct BLOOM_API InputEventType: unsigned {
		none              = 0,
		
		leftMouseDown     = 1 <<  0,
		rightMouseDown    = 1 <<  1,
		otherMouseDown    = 1 <<  2,
		mouseDown         = leftMouseDown | rightMouseDown | otherMouseDown,
		
		leftMouseUp       = 1 <<  3,
		rightMouseUp      = 1 <<  4,
		otherMouseUp      = 1 <<  5,
		mouseUp           = leftMouseUp | rightMouseUp | otherMouseUp,
		
		mouseMoved        = 1 <<  6,
		
		leftMouseDragged  = 1 <<  7,
		rightMouseDragged = 1 <<  8,
		otherMouseDragged = 1 <<  9,
		mouseDragged      = leftMouseDragged | rightMouseDragged | otherMouseDragged,
		
		scrollWheel       = 1 << 10,
		magnify           = 1 << 11,
		
		mouse             = mouseDown | mouseUp | mouseMoved |mouseDragged | scrollWheel | magnify,
		
		keyDown           = 1 << 12,
		keyUp             = 1 << 13,
		key               = keyDown | keyUp
	};
	
	UTL_ENUM_OPERATORS(InputEventType);
	
	inline bool isMouseEvent(InputEventType type) {
		return (int)type != 0 && (int)type <= (int)InputEventType::magnify;
	}
	
	std::string_view toString(InputEventType);
	std::ostream& operator<<(std::ostream&, InputEventType);
	
	namespace internal {
		
		template <InputEventType>
		struct ToInputEvent;
		
		template <> struct ToInputEvent<InputEventType::leftMouseDown>     { using type = MouseDownEvent; };
		template <> struct ToInputEvent<InputEventType::rightMouseDown>    { using type = MouseDownEvent; };
		template <> struct ToInputEvent<InputEventType::otherMouseDown>    { using type = MouseDownEvent; };
		template <> struct ToInputEvent<InputEventType::mouseDown>         { using type = MouseDownEvent; };
		
		template <> struct ToInputEvent<InputEventType::leftMouseUp>       { using type = MouseUpEvent; };
		template <> struct ToInputEvent<InputEventType::rightMouseUp>      { using type = MouseUpEvent; };
		template <> struct ToInputEvent<InputEventType::otherMouseUp>      { using type = MouseUpEvent; };
		template <> struct ToInputEvent<InputEventType::mouseUp>           { using type = MouseUpEvent; };
		
		template <> struct ToInputEvent<InputEventType::mouseMoved>        { using type = MouseMoveEvent; };
		
		template <> struct ToInputEvent<InputEventType::leftMouseDragged>  { using type = MouseDragEvent; };
		template <> struct ToInputEvent<InputEventType::rightMouseDragged> { using type = MouseDragEvent; };
		template <> struct ToInputEvent<InputEventType::otherMouseDragged> { using type = MouseDragEvent; };
		template <> struct ToInputEvent<InputEventType::mouseDragged>      { using type = MouseDragEvent; };
		
		template <> struct ToInputEvent<InputEventType::scrollWheel>       { using type = ScrollEvent; };
		template <> struct ToInputEvent<InputEventType::magnify>           { using type = MagnificationEvent; };
		
		template <> struct ToInputEvent<InputEventType::keyDown>           { using type = KeyEvent; };
		template <> struct ToInputEvent<InputEventType::keyUp>             { using type = KeyEvent; };
		template <> struct ToInputEvent<InputEventType::key>               { using type = KeyEvent; };
		
		
		template <typename E>
		struct IsInputEvent: std::bool_constant<std::is_base_of_v<InputEventBase, std::decay_t<E>>>{};
	}
	
	class BLOOM_API InputEvent {
	public:
		template <typename E> requires (internal::IsInputEvent<E>::value)
		InputEvent(InputEventType type, E e): _type(type), _union(std::move(e)) {}
		
		auto visit(auto&& f) const {
			return std::visit(f, _union);
		}
		
		template <typename E> requires (internal::IsInputEvent<E>::value)
		E& get() { return std::get<E>(_union); }
		template <typename E> requires (internal::IsInputEvent<E>::value)
		E const& get() const { return std::get<E>(_union); }
		
		template <InputEventType Type, typename E = typename internal::ToInputEvent<Type>::type>
		void dispatch(std::invocable<E const> auto&& f) {
			if (!(Type & _type) || handled()) {
				return;
			}
			
			if constexpr (std::predicate<decltype(f), E const>) {
				_handled = std::invoke(f, std::get<E>(_union));
			}
			else {
				std::invoke(f, std::get<E>(_union));
			}
		}
		
		template <InputEventType Type, typename E = typename internal::ToInputEvent<Type>::type>
		void dispatch(std::invocable<E const> auto&& f) const
			requires (!std::predicate<decltype(f), E const>)
		{
			if (!(Type & _type) || handled()) {
				return;
			}
			
			std::invoke(f, std::get<E>(_union));
		}
		
		InputEventType type() const { return _type; }
		bool handled() const { return _handled; }
		
		ModFlags modifierFlags() const {
			return visit([](auto& e) {
				return e.modifierFlags;
			});
		}
		
	private:
		friend class Application;
		friend class Input;
		using EventUnion = std::variant<
			MouseEvent,
			MouseDownEvent,
			MouseUpEvent,
			MouseMoveEvent,
			MouseDragEvent,
			ScrollEvent,
			MagnificationEvent,
			KeyEvent	
		>;
		InputEventType _type;
		EventUnion _union;
		bool _handled = false;
	};
	
	InputEvent inputEventFromGLFWMouseButton(Input const&, int button, int action, int mods);
	InputEvent inputEventFromGLFWCursorPos(Input const&, double xpos, double ypos);
	InputEvent inputEventFromGLFWScroll(Input const&, double xoffset, double yoffset);
	InputEvent inputEventFromGLFWKey(Input const&, int key, int scancode, int action, int mods);
	
}

