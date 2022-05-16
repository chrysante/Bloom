#pragma once

#include <mtl/mtl.hpp>
#include <utl/common.hpp>
#include <utl/messenger.hpp>
#include <array>
#include <iosfwd>
#include <variant>
#include <type_traits>
#include <concepts>

#include "Input.hpp"
#include "Bloom/Core/Debug.hpp"

namespace bloom {
	
	enum class InputModifierFlags: unsigned {
		none       = 0u,
		capsLock   = 1u << 0, // Set if Caps Lock key is pressed.
		shift      = 1u << 1, // Set if Shift key is pressed.
		control    = 1u << 2, // Set if Control key is pressed.
		option     = 1u << 3, // Set if Option or Alternate key is pressed.
		super      = 1u << 4, // Set if Command key is pressed.
		numericPad = 1u << 5, // Set if any key in the numeric keypad is pressed.
		help       = 1u << 6, // Set if the Help key is pressed.
		function   = 1u << 7  // Set if any function key is pressed.
	};
	
	UTL_ENUM_OPERATORS(InputModifierFlags);
	
	struct InputEventBase {
		InputModifierFlags modifierFlags;
	};
	
	struct MouseEvent: InputEventBase {
		mtl::double2 locationInWindow;
	};
	
	struct MouseDownEvent: MouseEvent {
		long clickCount;
	};
	
	struct MouseUpEvent: MouseEvent {
		
	};
	
	struct MouseDragEvent: MouseEvent {
		mtl::double2 offset;
	};
	
	struct ScrollEvent: MouseEvent {
		mtl::double2 offset;
		bool isTrackpad;
	};
	
	struct MagnificationEvent: MouseEvent {
		double offset;
	};
	
	struct MouseMoveEvent: MouseEvent {
		mtl::double2 offset;
	};
	
	struct KeyEvent: InputEventBase {
		Key key;
		int repeat;
	};
	
	enum struct InputEventType: unsigned {
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
		keyFlagsChanged   = 1 << 14,
		key               = keyDown | keyUp | keyFlagsChanged
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
		
		template <> struct ToInputEvent<InputEventType::leftMouseUp>       { using type = MouseUpEvent; };
		template <> struct ToInputEvent<InputEventType::rightMouseUp>      { using type = MouseUpEvent; };
		template <> struct ToInputEvent<InputEventType::otherMouseUp>      { using type = MouseUpEvent; };
		
		template <> struct ToInputEvent<InputEventType::mouseMoved>        { using type = MouseMoveEvent; };
		
		template <> struct ToInputEvent<InputEventType::leftMouseDragged>  { using type = MouseDragEvent; };
		template <> struct ToInputEvent<InputEventType::rightMouseDragged> { using type = MouseDragEvent; };
		template <> struct ToInputEvent<InputEventType::otherMouseDragged> { using type = MouseDragEvent; };
		
		template <> struct ToInputEvent<InputEventType::scrollWheel>       { using type = ScrollEvent; };
		template <> struct ToInputEvent<InputEventType::magnify>           { using type = MagnificationEvent; };
		
		template <> struct ToInputEvent<InputEventType::keyDown>           { using type = KeyEvent; };
		template <> struct ToInputEvent<InputEventType::keyUp>             { using type = KeyEvent; };
		
		
		template <typename E>
		struct IsInputEvent: std::bool_constant<std::is_base_of_v<InputEventBase, std::decay_t<E>>>{};
	}
	
	class InputEvent: public utl::message<InputEvent> {
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
			if (Type != _type || handled()) {
				return;
			}
			try {
				if constexpr (std::predicate<decltype(f), E const>) {
					_handled = std::invoke(f, std::get<E>(_union));
				}
				else {
					std::invoke(f, std::get<E>(_union));
				}
			}
			catch (std::bad_variant_access const&) {
				bloomDebugbreak();
				std::terminate();
			}
		}
		
		InputEventType type() const { return _type; }
		bool handled() const { return _handled; }
		
		InputModifierFlags modifierFlags() const {
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
	
}

