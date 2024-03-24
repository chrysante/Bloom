#include "Poppy/UI/ImGuiContext.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/strcat.hpp>

#include "Bloom/Application.h"
#include "Bloom/Core.h"
#include "Bloom/GPU.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/UI/Font.h"

using namespace bloom;
using namespace poppy;

static ImGuiKey toImGuiKeyCode(Key key) {
    switch (key) {
    case Key::Num0:
        return ImGuiKey_0;
    case Key::Num1:
        return ImGuiKey_1;
    case Key::Num2:
        return ImGuiKey_2;
    case Key::Num3:
        return ImGuiKey_3;
    case Key::Num4:
        return ImGuiKey_4;
    case Key::Num5:
        return ImGuiKey_5;
    case Key::Num6:
        return ImGuiKey_6;
    case Key::Num7:
        return ImGuiKey_7;
    case Key::Num8:
        return ImGuiKey_8;
    case Key::Num9:
        return ImGuiKey_9;
    case Key::A:
        return ImGuiKey_A;
    case Key::S:
        return ImGuiKey_S;
    case Key::D:
        return ImGuiKey_D;
    case Key::F:
        return ImGuiKey_F;
    case Key::H:
        return ImGuiKey_H;
    case Key::G:
        return ImGuiKey_G;
    case Key::Z:
        return ImGuiKey_Z;
    case Key::X:
        return ImGuiKey_X;
    case Key::C:
        return ImGuiKey_C;
    case Key::V:
        return ImGuiKey_V;
    case Key::B:
        return ImGuiKey_B;
    case Key::Q:
        return ImGuiKey_Q;
    case Key::W:
        return ImGuiKey_W;
    case Key::E:
        return ImGuiKey_E;
    case Key::R:
        return ImGuiKey_R;
    case Key::Y:
        return ImGuiKey_Y;
    case Key::T:
        return ImGuiKey_T;
    case Key::O:
        return ImGuiKey_O;
    case Key::U:
        return ImGuiKey_U;
    case Key::I:
        return ImGuiKey_I;
    case Key::P:
        return ImGuiKey_P;
    case Key::L:
        return ImGuiKey_L;
    case Key::J:
        return ImGuiKey_J;
    case Key::N:
        return ImGuiKey_N;
    case Key::M:
        return ImGuiKey_M;
    case Key::K:
        return ImGuiKey_K;
    case Key::Apostrophe:
        return ImGuiKey_Apostrophe;
    case Key::Equal:
        return ImGuiKey_Equal;
    case Key::Minus:
        return ImGuiKey_Minus;
    case Key::LeftBracket:
        return ImGuiKey_LeftBracket;
    case Key::RightBracket:
        return ImGuiKey_RightBracket;
    case Key::Semicolon:
        return ImGuiKey_Semicolon;
    case Key::Backslash:
        return ImGuiKey_Backslash;
    case Key::Comma:
        return ImGuiKey_Comma;
    case Key::Slash:
        return ImGuiKey_Slash;
    case Key::Period:
        return ImGuiKey_Period;
    case Key::GraveAccent:
        return ImGuiKey_GraveAccent;
    case Key::KeypadDecimal:
        return ImGuiKey_KeypadDecimal;
    case Key::KeypadMultiply:
        return ImGuiKey_KeypadMultiply;
    case Key::KeypadAdd:
        return ImGuiKey_KeypadAdd;
    case Key::NumLock:
        return ImGuiKey_NumLock;
    case Key::KeypadDivide:
        return ImGuiKey_KeypadDivide;
    case Key::KeypadEnter:
        return ImGuiKey_KeypadEnter;
    case Key::KeypadSubtract:
        return ImGuiKey_KeypadSubtract;
    case Key::KeypadEqual:
        return ImGuiKey_KeypadEqual;
    case Key::Keypad0:
        return ImGuiKey_Keypad0;
    case Key::Keypad1:
        return ImGuiKey_Keypad1;
    case Key::Keypad2:
        return ImGuiKey_Keypad2;
    case Key::Keypad3:
        return ImGuiKey_Keypad3;
    case Key::Keypad4:
        return ImGuiKey_Keypad4;
    case Key::Keypad5:
        return ImGuiKey_Keypad5;
    case Key::Keypad6:
        return ImGuiKey_Keypad6;
    case Key::Keypad7:
        return ImGuiKey_Keypad7;
    case Key::Keypad8:
        return ImGuiKey_Keypad8;
    case Key::Keypad9:
        return ImGuiKey_Keypad9;
    case Key::Enter:
        return ImGuiKey_Enter;
    case Key::Tab:
        return ImGuiKey_Tab;
    case Key::Space:
        return ImGuiKey_Space;
    case Key::Backspace:
        return ImGuiKey_Backspace;
    case Key::Escape:
        return ImGuiKey_Escape;
    case Key::CapsLock:
        return ImGuiKey_CapsLock;
    case Key::LeftCtrl:
        return ImGuiKey_LeftCtrl;
    case Key::LeftShift:
        return ImGuiKey_LeftShift;
    case Key::LeftAlt:
        return ImGuiKey_LeftAlt;
    case Key::LeftSuper:
        return ImGuiKey_LeftSuper;
    case Key::RightCtrl:
        return ImGuiKey_RightCtrl;
    case Key::RightShift:
        return ImGuiKey_RightShift;
    case Key::RightAlt:
        return ImGuiKey_RightAlt;
    case Key::RightSuper:
        return ImGuiKey_RightSuper;
    case Key::F1:
        return ImGuiKey_F1;
    case Key::F2:
        return ImGuiKey_F2;
    case Key::F3:
        return ImGuiKey_F3;
    case Key::F4:
        return ImGuiKey_F4;
    case Key::F5:
        return ImGuiKey_F5;
    case Key::F6:
        return ImGuiKey_F6;
    case Key::F7:
        return ImGuiKey_F7;
    case Key::F8:
        return ImGuiKey_F8;
    case Key::F9:
        return ImGuiKey_F9;
    case Key::F11:
        return ImGuiKey_F11;
    case Key::F10:
        return ImGuiKey_F10;
    case Key::F12:
        return ImGuiKey_F12;
    case Key::PrintScreen:
        return ImGuiKey_PrintScreen;
    case Key::Menu:
        return ImGuiKey_Menu;
    case Key::Insert:
        return ImGuiKey_Insert;
    case Key::Home:
        return ImGuiKey_Home;
    case Key::End:
        return ImGuiKey_End;
    case Key::PageUp:
        return ImGuiKey_PageUp;
    case Key::PageDown:
        return ImGuiKey_PageDown;
    case Key::Delete:
        return ImGuiKey_Delete;
    case Key::LeftArrow:
        return ImGuiKey_LeftArrow;
    case Key::RightArrow:
        return ImGuiKey_RightArrow;
    case Key::DownArrow:
        return ImGuiKey_DownArrow;
    case Key::UpArrow:
        return ImGuiKey_UpArrow;
    default:
        return ImGuiKey_None;
    }
}

poppy::ImGuiContext::~ImGuiContext() = default;

void poppy::ImGuiContext::init(Application& application,
                               ImGuiContextDescription const& ds) {
    mApplication = &application;
    auto& device = application.device();
    desc = ds;
    this->assignReceiver(application.makeReceiver());
    IMGUI_CHECKVERSION();
    /// Scale factor values are figured out by trial-and-error and are hardcoded
    /// for now.
    static constexpr float ScaleFactor = 2.0;

    auto fontManager = std::make_unique<FontManager>(ScaleFactor);
    static_cast<Emitter&>(*fontManager) = application.makeEmitter();
    FontManager::setGlobalInstance(std::move(fontManager));

    context =
        ImGui::CreateContext(FontManager::getGlobalInstance()->getAtlas());
    ImGui::SetCurrentContext(context);
    ImGuiIO& io = ImGui::GetIO();
    io.FontGlobalScale = 0.5 / ScaleFactor;
    io.FontDefault = FontManager::get(FontDesc::UIDefault());
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
#if 0
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
    io.IniFilename = desc.iniFilePath.c_str();
    doInitPlatform(device, *application.getWindows().front());
    uploadCurrentFontAtlas(device);

    listen([this](ReloadedFontAtlasCommand cmd) {
        BL_ASSERT(cmd.fontManager == FontManager::getGlobalInstance(),
                  "For now...");
        context->IO.Fonts = cmd.fontManager->getAtlas();
        uploadCurrentFontAtlas(mApplication->device());
        context->IO.FontDefault = FontManager::get(FontDesc::UIDefault());
    });
}

void poppy::ImGuiContext::shutdown() {
    ImGui::SetCurrentContext(context);
    doShutdownPlatform();
    ImGui::DestroyContext(context);
    context = nullptr;
}

void poppy::ImGuiContext::newFrame(Window& window) {
    ImGui::SetCurrentContext(context);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = (mtl::float2)window.size();
    io.DisplayFramebufferScale = window.contentScaleFactor();
    doNewFramePlatform(window);
    ImGui::NewFrame();
}

void poppy::ImGuiContext::drawFrame(HardwareDevice& device, Window& window) {
    ImGui::SetCurrentContext(context);
    doDrawFramePlatform(device, window);
}

void poppy::ImGuiContext::onInput(InputEvent e) {
    ImGui::SetCurrentContext(context);
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseViewportEvent(ImGuiID{});
    e.dispatch<InputEventMask::MouseDown>([&](MouseDownEvent const& event) {
        int const button = (int)event.button;
        if (button >= 0 && button < ImGuiMouseButton_COUNT)
            io.AddMouseButtonEvent(button, true);
        return true;
    });
    e.dispatch<InputEventMask::MouseUp>([&](MouseUpEvent const& event) {
        int const button = (int)event.button;
        if (button >= 0 && button < ImGuiMouseButton_COUNT)
            io.AddMouseButtonEvent(button, false);
        return true;
    });
    e.dispatch<InputEventMask::MouseMoved>([&](MouseMoveEvent const& event) {
        io.AddMousePosEvent(event.locationInWindow.x, event.locationInWindow.y);
        return true;
    });
    e.dispatch<InputEventMask::MouseDragged>([&](MouseDragEvent const& event) {
        io.AddMousePosEvent(event.locationInWindow.x, event.locationInWindow.y);
        return true;
    });
    e.dispatch<InputEventMask::ScrollWheel>([&](ScrollEvent const& event) {
        if (event.offset.x != 0.0 || event.offset.y != 0.0) {
            io.AddMouseWheelEvent((float)event.offset.x * 0.1f,
                                  (float)event.offset.y * 0.1f);
        }
        return true;
    });
    e.dispatch<InputEventMask::Key>([&](KeyEvent const& event) {
        ImGuiKey const key = toImGuiKeyCode(event.key);
        bool const down = e.type() == InputEventMask::KeyDown;
        // Set Modifiers
        switch (event.key) {
        case Key::LeftCtrl:
            [[fallthrough]];
        case Key::RightCtrl:
            io.KeyCtrl = down;
            break;
        case Key::LeftShift:
            [[fallthrough]];
        case Key::RightShift:
            io.KeyShift = down;
            break;
        case Key::LeftAlt:
            [[fallthrough]];
        case Key::RightAlt:
            io.KeyAlt = down;
            break;

        case Key::LeftSuper:
            [[fallthrough]];
        case Key::RightSuper:
            io.KeySuper = down;
            break;
        default:
            break;
        }
        io.AddKeyEvent(key, e.type() == InputEventMask::KeyDown);
        return true;
    });
}

void poppy::ImGuiContext::onTextInput(unsigned int code) {
    ImGui::SetCurrentContext(context);
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(code);
}

#if !defined(BLOOM_PLATFORM_APPLE)

/// On Apple platform these are defined in Platform/MacOS/ImGuiContext.mm

void poppy::ImGuiContext::doInitPlatform(HardwareDevice& device,
                                         ImGuiContextDescription const& d) {}

void poppy::ImGuiContext::createFontAtlas(HardwareDevice&) {}

void poppy::ImGuiContext::doNewFramePlatform(Window& window) {}

void poppy::ImGuiContext::doDrawFramePlatform(HardwareDevice& device,
                                              Window& window) {}

void poppy::ImGuiContext::doShutdownPlatform() {}

void poppy::ImGuiContext::createFontAtlasPlatform(ImFontAtlas* atlas,
                                                  HardwareDevice&) {}

#endif
