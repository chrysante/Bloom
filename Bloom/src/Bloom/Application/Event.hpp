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
	
//	template <typename E>
//	void forEach(std::invocable<E> auto&&);
	
	enum class EventModifierFlags: unsigned {
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
	
//	template <>
//	void forEach<EventModifierFlags>(std::invocable<EventModifierFlags> auto&& f) {
//		for (int i = 0; i <= 7; ++i) {
//			f((EventModifierFlags)(1u << i));
//		}
//	}
	
	UTL_ENUM_OPERATORS(EventModifierFlags);
	
	struct EventBase {
		EventModifierFlags modifierFlags;
	};
	
	struct MouseEvent: EventBase {
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
	
	struct KeyEvent: EventBase {
		Key key;
		int repeat;
	};
	
	enum struct EventType: unsigned {
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
	
	UTL_ENUM_OPERATORS(EventType);
	
	inline bool isMouseEvent(EventType type) {
		return (int)type != 0 && (int)type <= (int)EventType::magnify;
	}
	
	std::string_view toString(EventType);
	std::ostream& operator<<(std::ostream&, EventType);
	
	namespace internal {
		
		template <EventType>
		struct ToEvent;
		
		template <> struct ToEvent<EventType::leftMouseDown>     { using type = MouseDownEvent; };
		template <> struct ToEvent<EventType::rightMouseDown>    { using type = MouseDownEvent; };
		template <> struct ToEvent<EventType::otherMouseDown>    { using type = MouseDownEvent; };
		
		template <> struct ToEvent<EventType::leftMouseUp>       { using type = MouseUpEvent; };
		template <> struct ToEvent<EventType::rightMouseUp>      { using type = MouseUpEvent; };
		template <> struct ToEvent<EventType::otherMouseUp>      { using type = MouseUpEvent; };
		
		template <> struct ToEvent<EventType::mouseMoved>        { using type = MouseMoveEvent; };
		
		template <> struct ToEvent<EventType::leftMouseDragged>  { using type = MouseDragEvent; };
		template <> struct ToEvent<EventType::rightMouseDragged> { using type = MouseDragEvent; };
		template <> struct ToEvent<EventType::otherMouseDragged> { using type = MouseDragEvent; };
		
		template <> struct ToEvent<EventType::scrollWheel>       { using type = ScrollEvent; };
		template <> struct ToEvent<EventType::magnify>           { using type = MagnificationEvent; };
		
		template <> struct ToEvent<EventType::keyDown>           { using type = KeyEvent; };
		template <> struct ToEvent<EventType::keyUp>             { using type = KeyEvent; };
		
		
		template <typename E>
		struct IsEvent: std::bool_constant<std::is_base_of_v<EventBase, std::decay_t<E>>>{};
	}
	
	class Event {
	public:
		template <typename E> requires (internal::IsEvent<E>::value)
		Event(EventType type, E e): _type(type), _union(std::move(e)) {}
		
		auto visit(auto&& f) const {
			return std::visit(f, _union);
		}
		
		template <typename E> requires (internal::IsEvent<E>::value)
		E& get() { return std::get<E>(_union); }
		template <typename E> requires (internal::IsEvent<E>::value)
		E const& get() const { return std::get<E>(_union); }
		
		template <EventType Type, typename E = typename internal::ToEvent<Type>::type>
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
		
		EventType type() const { return _type; }
		bool handled() const { return _handled; }
		
		EventModifierFlags modifierFlags() const {
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
		EventType _type;
		EventUnion _union;
		bool _handled = false;
	};
	
}

