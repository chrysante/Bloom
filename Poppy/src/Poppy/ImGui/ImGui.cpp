#define IMGUI_DEFINE_MATH_OPERATORS

#include "ImGui.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "Bloom/Application/Event.hpp"
#include "Poppy/Debug.hpp"
#include <cstring>

// Functions
static ImGuiKey KeyCodeToImGuiKey(bloom::Key k) {
	using namespace bloom;
	switch (k) {
		case Key::A: return ImGuiKey_A;
		case Key::S: return ImGuiKey_S;
		case Key::D: return ImGuiKey_D;
		case Key::F: return ImGuiKey_F;
		case Key::H: return ImGuiKey_H;
		case Key::G: return ImGuiKey_G;
		case Key::Z: return ImGuiKey_Z;
		case Key::X: return ImGuiKey_X;
		case Key::C: return ImGuiKey_C;
		case Key::V: return ImGuiKey_V;
		case Key::B: return ImGuiKey_B;
		case Key::Q: return ImGuiKey_Q;
		case Key::W: return ImGuiKey_W;
		case Key::E: return ImGuiKey_E;
		case Key::R: return ImGuiKey_R;
		case Key::Y: return ImGuiKey_Y;
		case Key::T: return ImGuiKey_T;
		case Key::_1: return ImGuiKey_1;
		case Key::_2: return ImGuiKey_2;
		case Key::_3: return ImGuiKey_3;
		case Key::_4: return ImGuiKey_4;
		case Key::_6: return ImGuiKey_6;
		case Key::_5: return ImGuiKey_5;
		case Key::equal: return ImGuiKey_Equal;
		case Key::_9: return ImGuiKey_9;
		case Key::_7: return ImGuiKey_7;
		case Key::minus: return ImGuiKey_Minus;
		case Key::_8: return ImGuiKey_8;
		case Key::_0: return ImGuiKey_0;
		case Key::rightBracket: return ImGuiKey_RightBracket;
		case Key::O: return ImGuiKey_O;
		case Key::U: return ImGuiKey_U;
		case Key::leftBracket: return ImGuiKey_LeftBracket;
		case Key::I: return ImGuiKey_I;
		case Key::P: return ImGuiKey_P;
		case Key::L: return ImGuiKey_L;
		case Key::J: return ImGuiKey_J;
		case Key::apostrophe: return ImGuiKey_Apostrophe;
		case Key::K: return ImGuiKey_K;
		case Key::semicolon: return ImGuiKey_Semicolon;
		case Key::backslash: return ImGuiKey_Backslash;
		case Key::comma: return ImGuiKey_Comma;
		case Key::slash: return ImGuiKey_Slash;
		case Key::N: return ImGuiKey_N;
		case Key::M: return ImGuiKey_M;
		case Key::period: return ImGuiKey_Period;
		case Key::graveAccent: return ImGuiKey_GraveAccent;
		case Key::keypadDecimal: return ImGuiKey_KeypadDecimal;
		case Key::keypadMultiply: return ImGuiKey_KeypadMultiply;
		case Key::keypadAdd: return ImGuiKey_KeypadAdd;
		case Key::numLock: return ImGuiKey_NumLock;
		case Key::keypadDivide: return ImGuiKey_KeypadDivide;
		case Key::keypadEnter: return ImGuiKey_KeypadEnter;
		case Key::keypadSubtract: return ImGuiKey_KeypadSubtract;
		case Key::keypadEqual: return ImGuiKey_KeypadEqual;
		case Key::keypad0: return ImGuiKey_Keypad0;
		case Key::keypad1: return ImGuiKey_Keypad1;
		case Key::keypad2: return ImGuiKey_Keypad2;
		case Key::keypad3: return ImGuiKey_Keypad3;
		case Key::keypad4: return ImGuiKey_Keypad4;
		case Key::keypad5: return ImGuiKey_Keypad5;
		case Key::keypad6: return ImGuiKey_Keypad6;
		case Key::keypad7: return ImGuiKey_Keypad7;
		case Key::keypad8: return ImGuiKey_Keypad8;
		case Key::keypad9: return ImGuiKey_Keypad9;
		case Key::enter: return ImGuiKey_Enter;
		case Key::tab: return ImGuiKey_Tab;
		case Key::space: return ImGuiKey_Space;
		case Key::delete_: return ImGuiKey_Backspace;
		case Key::escape: return ImGuiKey_Escape;
		case Key::capsLock: return ImGuiKey_CapsLock;
		case Key::leftCtrl: return ImGuiKey_LeftCtrl;
		case Key::leftShift: return ImGuiKey_LeftShift;
		case Key::leftAlt: return ImGuiKey_LeftAlt;
		case Key::leftSuper: return ImGuiKey_LeftSuper;
		case Key::rightCtrl: return ImGuiKey_RightCtrl;
		case Key::rightShift: return ImGuiKey_RightShift;
		case Key::rightAlt: return ImGuiKey_RightAlt;
		case Key::rightSuper: return ImGuiKey_RightSuper;
		case Key::F5: return ImGuiKey_F5;
		case Key::F6: return ImGuiKey_F6;
		case Key::F7: return ImGuiKey_F7;
		case Key::F3: return ImGuiKey_F3;
		case Key::F8: return ImGuiKey_F8;
		case Key::F9: return ImGuiKey_F9;
		case Key::F11: return ImGuiKey_F11;
		case Key::F10: return ImGuiKey_F10;
		case Key::menu: return ImGuiKey_Menu;
		case Key::F12: return ImGuiKey_F12;
		case Key::insert: return ImGuiKey_Insert;
		case Key::home: return ImGuiKey_Home;
		case Key::pageUp: return ImGuiKey_PageUp;
		case Key::F4: return ImGuiKey_F4;
		case Key::end: return ImGuiKey_End;
		case Key::F2: return ImGuiKey_F2;
		case Key::pageDown: return ImGuiKey_PageDown;
		case Key::F1: return ImGuiKey_F1;
		case Key::leftArrow: return ImGuiKey_LeftArrow;
		case Key::rightArrow: return ImGuiKey_RightArrow;
		case Key::downArrow: return ImGuiKey_DownArrow;
		case Key::upArrow: return ImGuiKey_UpArrow;
		default: return ImGuiKey_None;
	}
}


namespace poppy {
	
	bool ImGuiContext::handleMouseEvent(bloom::Event const& event) {
		ImGuiIO& io = ImGui::GetIO();

		using namespace bloom;
		
		switch (event.type()) {
			case EventType::leftMouseDown:
				io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
				return io.WantCaptureMouse;
			case EventType::rightMouseDown:
				io.AddMouseButtonEvent(ImGuiMouseButton_Right, true);
				return io.WantCaptureMouse;
			case EventType::otherMouseDown:
				io.AddMouseButtonEvent(ImGuiMouseButton_Middle, true);
				return io.WantCaptureMouse;
				
				
			case EventType::leftMouseUp:
				io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
				return io.WantCaptureMouse;
			case EventType::rightMouseUp:
				io.AddMouseButtonEvent(ImGuiMouseButton_Right, false);
				return io.WantCaptureMouse;
			case EventType::otherMouseUp:
				io.AddMouseButtonEvent(ImGuiMouseButton_Middle, false);
				return io.WantCaptureMouse;
				
			case EventType::mouseMoved: {
				auto const locationInWindow = event.get<MouseMoveEvent>().locationInWindow;
				io.AddMousePosEvent(locationInWindow.x, locationInWindow.y);
				break;
			}
			case EventType::leftMouseDragged:
			case EventType::rightMouseDragged:
			case EventType::otherMouseDragged: {
				auto const locationInWindow = event.get<MouseDragEvent>().locationInWindow;
				io.AddMousePosEvent(locationInWindow.x, locationInWindow.y);
				break;
			}
			case EventType::scrollWheel: {
				auto const offset = event.get<ScrollEvent>().offset / 10;
				if (offset.x != 0.0 || offset.y != 0.0) {
					io.AddMouseWheelEvent(offset.x, offset.y);
				}
				return io.WantCaptureMouse;
			}
			case EventType::keyDown: {
				auto& keyEvent = event.get<KeyEvent>();
				if (keyEvent.repeat) {
					return io.WantCaptureKeyboard;
				}
				auto const imguiKey = KeyCodeToImGuiKey(keyEvent.key);
				io.AddKeyEvent(imguiKey, true);
				return io.WantCaptureKeyboard;
			}
			case EventType::keyUp: {
				auto& keyEvent = event.get<KeyEvent>();
				if (keyEvent.repeat) {
					return io.WantCaptureKeyboard;
				}
				auto const imguiKey = KeyCodeToImGuiKey(keyEvent.key);
				io.AddKeyEvent(imguiKey, false);
				return io.WantCaptureKeyboard;
			}
				
			default:
				break;
		}
		
#if 0
		if (event.type == NSEventTypeFlagsChanged)
		{
			unsigned short key_code = [event keyCode];
			NSEventModifierFlags modifier_flags = [event modifierFlags];

			io.AddKeyEvent(ImGuiKey_ModShift, (modifier_flags & NSEventModifierFlagShift)   != 0);
			io.AddKeyEvent(ImGuiKey_ModCtrl,  (modifier_flags & NSEventModifierFlagControl) != 0);
			io.AddKeyEvent(ImGuiKey_ModAlt,   (modifier_flags & NSEventModifierFlagOption)  != 0);
			io.AddKeyEvent(ImGuiKey_ModSuper, (modifier_flags & NSEventModifierFlagCommand) != 0);

			ImGuiKey key = ImGui_ImplOSX_KeyCodeToImGuiKey(key_code);
			if (key != ImGuiKey_None)
			{
				// macOS does not generate down/up event for modifiers. We're trying
				// to use hardware dependent masks to extract that information.
				// 'imgui_mask' is left as a fallback.
				NSEventModifierFlags mask = 0;
				switch (key)
				{
					case ImGuiKey_LeftCtrl:   mask = 0x0001; break;
					case ImGuiKey_RightCtrl:  mask = 0x2000; break;
					case ImGuiKey_LeftShift:  mask = 0x0002; break;
					case ImGuiKey_RightShift: mask = 0x0004; break;
					case ImGuiKey_LeftSuper:  mask = 0x0008; break;
					case ImGuiKey_RightSuper: mask = 0x0010; break;
					case ImGuiKey_LeftAlt:    mask = 0x0020; break;
					case ImGuiKey_RightAlt:   mask = 0x0040; break;
					default:
						return io.WantCaptureKeyboard;
				}

				NSEventModifierFlags modifier_flags = [event modifierFlags];
				io.AddKeyEvent(key, (modifier_flags & mask) != 0);
				io.SetKeyEventNativeData(key, key_code, -1); // To support legacy indexing (<1.87 user code)
			}

			return io.WantCaptureKeyboard;
		}
#endif
		return false;
	}
	
	static bool ViewportButtonBehavior(const ImRect& bb,
									   ImGuiID id,
									   bool* out_hovered,
									   bool* out_held,
									   ImGuiButtonFlags flags);
	
	ViewportInput detectViewportInput(ImGuiButtonFlags buttonFlags) {
		using namespace ImGui;

		buttonFlags |= ImGuiButtonFlags_NoNavFocus;
		
		// save to restore later
		ImVec2 const cursorPos = GetCursorPos();

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return ViewportInput{};

		const ImGuiID id = window->GetID("__Invisible_Button__");
		
		ImVec2 size = CalcItemSize(window->Size, 0.0f, 0.0f);
		const ImRect bb(window->DC.CursorPos, mtl::float2(window->DC.CursorPos) + mtl::float2(size));
		ItemSize(size);
		if (!ItemAdd(bb, id))
			return {};
		
		ViewportInput result{};
		result.pressed = ViewportButtonBehavior(bb, id, &result.hovered, &result.held, buttonFlags);

		SetCursorPos(cursorPos);

		return result;
	}

	static bool ViewportButtonBehavior(const ImRect& bb,
									   ImGuiID id,
									   bool* out_hovered,
									   bool* out_held,
									   ImGuiButtonFlags flags)
	{
		using namespace ImGui;
		::ImGuiContext& g = *GImGui;
		ImGuiWindow* window = GetCurrentWindow();

		// Default only reacts to left mouse button
		if ((flags & ImGuiButtonFlags_MouseButtonMask_) == 0)
			flags |= ImGuiButtonFlags_MouseButtonDefault_;

		// Default behavior requires click + release inside bounding box
		if ((flags & ImGuiButtonFlags_PressedOnMask_) == 0)
			flags |= ImGuiButtonFlags_PressedOnDefault_;

		ImGuiWindow* backup_hovered_window = g.HoveredWindow;
		const bool flatten_hovered_children = (flags & ImGuiButtonFlags_FlattenChildren) && g.HoveredWindow && g.HoveredWindow->RootWindowDockTree == window->RootWindowDockTree;
		if (flatten_hovered_children)
			g.HoveredWindow = window;

	#ifdef IMGUI_ENABLE_TEST_ENGINE
		if (id != 0 && g.LastItemData.ID != id)
			IMGUI_TEST_ENGINE_ITEM_ADD(bb, id);
	#endif

		bool pressed = false;
		bool hovered = ItemHoverable(bb, /* id */ 0); // this needs to be zero to not interfere with global hover stuff

		// Drag source doesn't report as hovered
		if (hovered && g.DragDropActive && g.DragDropPayload.SourceId == id && !(g.DragDropSourceFlags & ImGuiDragDropFlags_SourceNoDisableHover))
			hovered = false;

		if (flatten_hovered_children)
			g.HoveredWindow = backup_hovered_window;

		// AllowOverlap mode (rarely used) requires previous frame HoveredId to be null or to match. This allows using patterns where a later submitted widget overlaps a previous one.
		if (hovered && (flags & ImGuiButtonFlags_AllowItemOverlap) && (g.HoveredIdPreviousFrame != id && g.HoveredIdPreviousFrame != 0))
			hovered = false;

		// Mouse handling
		if (hovered)
		{
			if (!(flags & ImGuiButtonFlags_NoKeyModifiers) || (!g.IO.KeyCtrl && !g.IO.KeyShift && !g.IO.KeyAlt))
			{
				// Poll buttons
				int mouse_button_clicked = -1;
				int mouse_button_released = -1;
				if ((flags & ImGuiButtonFlags_MouseButtonLeft) && g.IO.MouseClicked[0])         { mouse_button_clicked = 0; }
				else if ((flags & ImGuiButtonFlags_MouseButtonRight) && g.IO.MouseClicked[1])   { mouse_button_clicked = 1; }
				else if ((flags & ImGuiButtonFlags_MouseButtonMiddle) && g.IO.MouseClicked[2])  { mouse_button_clicked = 2; }
				if ((flags & ImGuiButtonFlags_MouseButtonLeft) && g.IO.MouseReleased[0])        { mouse_button_released = 0; }
				else if ((flags & ImGuiButtonFlags_MouseButtonRight) && g.IO.MouseReleased[1])  { mouse_button_released = 1; }
				else if ((flags & ImGuiButtonFlags_MouseButtonMiddle) && g.IO.MouseReleased[2]) { mouse_button_released = 2; }

				if (mouse_button_clicked != -1 && g.ActiveId != id)
				{
					if (flags & (ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnClickReleaseAnywhere))
					{
						SetActiveID(id, window);
						g.ActiveIdMouseButton = mouse_button_clicked;
						if (!(flags & ImGuiButtonFlags_NoNavFocus))
							SetFocusID(id, window);
						FocusWindow(window);
					}
					if ((flags & ImGuiButtonFlags_PressedOnClick) || ((flags & ImGuiButtonFlags_PressedOnDoubleClick) && g.IO.MouseClickedCount[mouse_button_clicked] == 2))
					{
						pressed = true;
						if (flags & ImGuiButtonFlags_NoHoldingActiveId)
							ClearActiveID();
						else
							SetActiveID(id, window); // Hold on ID
						if (!(flags & ImGuiButtonFlags_NoNavFocus))
							SetFocusID(id, window);
						g.ActiveIdMouseButton = mouse_button_clicked;
						FocusWindow(window);
					}
				}
				if ((flags & ImGuiButtonFlags_PressedOnRelease) && mouse_button_released != -1)
				{
					// Repeat mode trumps on release behavior
					const bool has_repeated_at_least_once = (flags & ImGuiButtonFlags_Repeat) && g.IO.MouseDownDurationPrev[mouse_button_released] >= g.IO.KeyRepeatDelay;
					if (!has_repeated_at_least_once)
						pressed = true;
					if (!(flags & ImGuiButtonFlags_NoNavFocus))
						SetFocusID(id, window);
					ClearActiveID();
				}

				// 'Repeat' mode acts when held regardless of _PressedOn flags (see table above).
				// Relies on repeat logic of IsMouseClicked() but we may as well do it ourselves if we end up exposing finer RepeatDelay/RepeatRate settings.
				if (g.ActiveId == id && (flags & ImGuiButtonFlags_Repeat))
					if (g.IO.MouseDownDuration[g.ActiveIdMouseButton] > 0.0f && IsMouseClicked(g.ActiveIdMouseButton, true))
						pressed = true;
			}

			if (pressed)
				g.NavDisableHighlight = true;
		}

		// Gamepad/Keyboard navigation
		// We report navigated item as hovered but we don't set g.HoveredId to not interfere with mouse.
		if (g.NavId == id && !g.NavDisableHighlight && g.NavDisableMouseHover && (g.ActiveId == 0 || g.ActiveId == id || g.ActiveId == window->MoveId))
			if (!(flags & ImGuiButtonFlags_NoHoveredOnFocus))
				hovered = true;
		if (g.NavActivateDownId == id)
		{
			bool nav_activated_by_code = (g.NavActivateId == id);
			bool nav_activated_by_inputs = IsNavInputTest(ImGuiNavInput_Activate, (flags & ImGuiButtonFlags_Repeat) ? ImGuiInputReadMode_Repeat : ImGuiInputReadMode_Pressed);
			if (nav_activated_by_code || nav_activated_by_inputs)
			{
				// Set active id so it can be queried by user via IsItemActive(), equivalent of holding the mouse button.
				pressed = true;
				SetActiveID(id, window);
				g.ActiveIdSource = ImGuiInputSource_Nav;
				if (!(flags & ImGuiButtonFlags_NoNavFocus))
					SetFocusID(id, window);
			}
		}

		// Process while held
		bool held = false;
		if (g.ActiveId == id)
		{
			if (g.ActiveIdSource == ImGuiInputSource_Mouse)
			{
				if (g.ActiveIdIsJustActivated)
					g.ActiveIdClickOffset = g.IO.MousePos - bb.Min;

				const int mouse_button = g.ActiveIdMouseButton;
				IM_ASSERT(mouse_button >= 0 && mouse_button < ImGuiMouseButton_COUNT);
				if (g.IO.MouseDown[mouse_button])
				{
					held = true;
				}
				else
				{
					bool release_in = hovered && (flags & ImGuiButtonFlags_PressedOnClickRelease) != 0;
					bool release_anywhere = (flags & ImGuiButtonFlags_PressedOnClickReleaseAnywhere) != 0;
					if ((release_in || release_anywhere) && !g.DragDropActive)
					{
						// Report as pressed when releasing the mouse (this is the most common path)
						bool is_double_click_release = (flags & ImGuiButtonFlags_PressedOnDoubleClick) && g.IO.MouseReleased[mouse_button] && g.IO.MouseClickedLastCount[mouse_button] == 2;
						bool is_repeating_already = (flags & ImGuiButtonFlags_Repeat) && g.IO.MouseDownDurationPrev[mouse_button] >= g.IO.KeyRepeatDelay; // Repeat mode trumps <on release>
						if (!is_double_click_release && !is_repeating_already)
							pressed = true;
					}
					ClearActiveID();
				}
				if (!(flags & ImGuiButtonFlags_NoNavFocus))
					g.NavDisableHighlight = true;
			}
			else if (g.ActiveIdSource == ImGuiInputSource_Nav)
			{
				// When activated using Nav, we hold on the ActiveID until activation button is released
				if (g.NavActivateDownId != id)
					ClearActiveID();
			}
			if (pressed)
				g.ActiveIdHasBeenPressedBefore = true;
		}

		if (out_hovered) *out_hovered = hovered;
		if (out_held) *out_held = held;

		return pressed;
	}

	
	
	
	static bool float3PrettyElement(int index, float* element, float width, char const* userLabelID, float speed) {
		char label[] = "X";
		label[0] += index;
		
		auto& style = GImGui->Style;
		
		auto const labelTextSize = ImGui::CalcTextSize(label, NULL, true);
		float const labelSize = ImGui::CalcItemSize({ 0, 0 },
													labelTextSize.x + style.FramePadding.x * 2.0f,
													labelTextSize.y + style.FramePadding.y * 2.0f).y;
		
		using Color = mtl::colors<mtl::float4>;
		
		auto const color = std::array {
			Color::red, Color::green / std::sqrt(2.0f), Color::blue
		}[index];
		
		auto oldCursorPos = ImGui::GetCursorPos();
		
		ImGui::PushFont((ImFont*)poppy::ImGuiContext::instance().getFont(FontWeight::heavy, FontStyle::roman));
		ImGui::PushStyleColor(ImGuiCol_Button, color);
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		char labelID[64] = "X##x";
		labelID[0] += index;
		labelID[3] += index;
		std::strncpy(labelID + 4, userLabelID, 59);
		ImGui::Button(labelID, { labelSize, labelSize });
		ImGui::PopItemFlag();
		ImGui::PopStyleColor();
		ImGui::PopFont();
		
		oldCursorPos.x += labelSize;
		ImGui::SetCursorPos(oldCursorPos);
		
		ImGui::SetNextItemWidth(width - labelSize);
		
		char dragFloatID[64] = "##x";
		dragFloatID[2] += index;
		std::strncpy(dragFloatID + 3, userLabelID, 60);
		return ImGui::DragFloat(dragFloatID, element, speed);
	}
	
	bool dragFloat3Pretty(float* v, char const* labelID, float speed) {
		float const totalWidth = ImGui::GetContentRegionAvail().x;
		float const itemWidth = totalWidth / 3;
		auto cursorPos = ImGui::GetCursorPos();
		bool result = false;
		for (int i = 0; i < 3; ++i) {
			ImGui::SetCursorPos({ cursorPos.x + i * itemWidth, cursorPos.y });
			result |= float3PrettyElement(i, v + i, itemWidth, labelID, speed);
		}
		return result;
	}
	
	std::array<char, 64> generateUniqueID(std::string_view name, int id, bool const prepentDoubleHash) {
		std::array<char, 64> buffer{};
		char* bufferPtr = buffer.data();
		/* -0000000 */
		std::size_t availSize = 56;
		if (prepentDoubleHash) {
			bufferPtr[0] = '#';
			bufferPtr[1] = '#';
			bufferPtr += 2;
			availSize -= 2;
		}
		
		std::size_t const nameSize = std::min(name.size(), availSize - 2 * !prepentDoubleHash);
		
		std::memcpy(bufferPtr, name.data(), nameSize);
		bufferPtr += nameSize;
		availSize -= nameSize;
		if (!prepentDoubleHash) {
			bufferPtr[0] = '#';
			bufferPtr[1] = '#';
			bufferPtr += 2;
			availSize -= 2;
		}
		poppyAssert(availSize < 56, "Overflow has occured");
		std::strncpy(bufferPtr, utl::format("-{}", id).data(), 8);
		
		return buffer;
	}
	
	
	
}
