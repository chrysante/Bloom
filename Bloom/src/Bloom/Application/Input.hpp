#pragma once

#include <mtl/mtl.hpp>
#include <bitset>

namespace bloom {
	
	enum struct Key {
		none = 0,
		A, S, D, F, H, G, Z, X, C, V, B, Q, W, E, R, Y, T, O, U, I, P, L, J, K, N, M,
		_1, _2, _3, _4, _5, _6, _7, _8, _9, _0,
		equal, minus,
		rightBracket,
		leftBracket,
		apostrophe,
		semicolon,
		backslash,
		comma,
		slash,
		period,
		graveAccent,
		keypadDecimal,
		keypadMultiply,
		keypadAdd,
		numLock,
		keypadDivide,
		keypadEnter,
		keypadSubtract,
		keypadEqual,
		keypad0,
		keypad1,
		keypad2,
		keypad3,
		keypad4,
		keypad5,
		keypad6,
		keypad7,
		keypad8,
		keypad9,
		enter,
		tab,
		space,
		backspace,
		escape,
		capsLock,
		leftCtrl,
		leftShift,
		leftAlt,
		leftSuper,
		rightCtrl,
		rightShift,
		rightAlt,
		rightSuper,
		F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
		printScreen,
		menu,
		insert,
		home,
		pageUp,
		delete_,
		end,
		pageDown,
		leftArrow,
		rightArrow,
		downArrow,
		upArrow,
		_count
	};
	
	enum struct MouseButton {
		none = 0, left, right, other, _count
	};
	
	class Event;
	
	class Input {
	public:
		mtl::float2 mousePosition() const { return _mousePosition; }
		mtl::float2 mouseOffset() const { return _mouseOffset; }
		
		mtl::float2 scrollOffset() const { return _scrollOffset; }
		
		bool mouseDown(MouseButton button) const { return _mouseButtons[(std::size_t)button]; }
		bool keyDown(Key key) const { return _keys[(std::size_t)key]; }
		
	private:
		friend class Application;
		
		void _setFromEvent(Event const&);
		
		void _setKey(Key k, bool down) {
			_keys[(std::size_t)k] = down;
		}
		void _setMouseButton(MouseButton b, bool down) {
			_mouseButtons[(std::size_t)b] = down;
		}
		
		void _clearOffsets() {
			_mouseOffset = 0;
			_scrollOffset = 0;
		}
		
	private:
		mtl::float2 _mousePosition = 0;
		mtl::float2 _mouseOffset = 0;
		mtl::float2 _scrollOffset = 0;
		std::bitset<(std::size_t)Key::_count> _keys{};
		std::bitset<(std::size_t)MouseButton::_count> _mouseButtons{};
	};
	
}
