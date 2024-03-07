#include "Poppy/UI/ImGuiContext.hpp"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <utl/format.hpp>

#include "Bloom/Application.hpp"
#include "Bloom/Core.hpp"
#include "Bloom/GPU.hpp"
#include "Poppy/Core/Debug.hpp"
#include "Poppy/UI/Font.hpp"
#include "Poppy/UI/Icons.hpp"

using namespace bloom;
using namespace poppy;

static ImGuiKey toImGuiKeyCode(Key key) {
    switch (key) {
    case Key::_0:
        return ImGuiKey_0;
    case Key::_1:
        return ImGuiKey_1;
    case Key::_2:
        return ImGuiKey_2;
    case Key::_3:
        return ImGuiKey_3;
    case Key::_4:
        return ImGuiKey_4;
    case Key::_5:
        return ImGuiKey_5;
    case Key::_6:
        return ImGuiKey_6;
    case Key::_7:
        return ImGuiKey_7;
    case Key::_8:
        return ImGuiKey_8;
    case Key::_9:
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

ImFontAtlas* poppy::ImGuiContext::sFontAtlas = nullptr;

poppy::ImGuiContext::~ImGuiContext() = default;

void poppy::ImGuiContext::init(bloom::Application& application,
                               ImGuiContextDescription const& ds) {
    mApplication = &application;
    auto& device = application.device();
    desc = ds;
    this->bloom::Receiver::operator=(application.makeReciever());

    IMGUI_CHECKVERSION();

    float const scaleFactor = 2;

    fonts.init(application);
    loadFonts(device, scaleFactor);
    context = ImGui::CreateContext(sFontAtlas);
    context->IO.FontGlobalScale = 1.0 / scaleFactor;
    context->IO.FontDefault = fonts.get(Font::UIDefault());

    ImGui::SetCurrentContext(context);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
    //		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    io.IniFilename = desc.iniFilePath.c_str();

    doInitPlatform(device);
    createFontAtlasPlatform(sFontAtlas, device);

    listen([this, scaleFactor](ReloadFontAtlasCommand) {
        if (fontAtlasReloaded) {
            return;
        }
        fontAtlasReloaded = true;
        loadFonts(mApplication->device(), scaleFactor);
        createFontAtlasPlatform(sFontAtlas, mApplication->device());
        context->IO.FontDefault = fonts.get(Font::UIDefault());
    });
}

void poppy::ImGuiContext::loadFonts(bloom::HardwareDevice& device,
                                    float scaleFactor) {
    if (!sFontAtlas) {
        sFontAtlas = IM_NEW(ImFontAtlas);
    }

    sFontAtlas->Clear();

    icons.load(*sFontAtlas, scaleFactor,
               resourceDir() / "Icons/IconsConfig.json",
               resourceDir() / "Icons/Icons.ttf");
    fonts.loadFonts(*sFontAtlas, scaleFactor);

    sFontAtlas->Build();
    {
        auto* testFont = fonts.get(Font::UIDefault());
        assert(testFont->IsLoaded());
    }

    auto* const iconFont16 = icons.font(IconSize::_16);
    assert(iconFont16);
    assert(iconFont16->IsLoaded());

    assert(icons.font(IconSize::_16)->IsLoaded());
}

void poppy::ImGuiContext::shutdown() {
    ImGui::SetCurrentContext(context);
    doShutdownPlatform();
    ImGui::DestroyContext(context);
    context = nullptr;
}

void poppy::ImGuiContext::newFrame(bloom::Window& window) {
    ImGui::SetCurrentContext(context);
    ImGuiIO& io = ImGui::GetIO();

    io.DisplaySize = (mtl::float2)window.size();
    io.DisplayFramebufferScale = window.contentScaleFactor();

    doNewFramePlatform(window);
    ImGui::NewFrame();

    fontAtlasReloaded = false;
}

void poppy::ImGuiContext::drawFrame(bloom::HardwareDevice& device,
                                    bloom::Window& window) {
    ImGui::SetCurrentContext(context);
    doDrawFramePlatform(device, window);
}

void poppy::ImGuiContext::onInput(bloom::InputEvent e) {
    ImGui::SetCurrentContext(context);

    ImGuiIO& io = ImGui::GetIO();

    io.AddMouseViewportEvent(ImGuiID{});

    e.dispatch<InputEventType::mouseDown>([&](MouseDownEvent const& event) {
        int const button = (int)event.button;
        if (button >= 0 && button < ImGuiMouseButton_COUNT)
            io.AddMouseButtonEvent(button, true);

        return true;
    });
    e.dispatch<InputEventType::mouseUp>([&](MouseUpEvent const& event) {
        int const button = (int)event.button;
        if (button >= 0 && button < ImGuiMouseButton_COUNT)
            io.AddMouseButtonEvent(button, false);

        return true;
    });
    e.dispatch<InputEventType::mouseMoved>([&](MouseMoveEvent const& event) {
        io.AddMousePosEvent(event.locationInWindow.x, event.locationInWindow.y);
        return true;
    });
    e.dispatch<InputEventType::mouseDragged>([&](MouseDragEvent const& event) {
        io.AddMousePosEvent(event.locationInWindow.x, event.locationInWindow.y);
        return true;
    });

    e.dispatch<InputEventType::scrollWheel>([&](ScrollEvent const& event) {
        if (event.offset.x != 0.0 || event.offset.y != 0.0) {
            io.AddMouseWheelEvent((float)event.offset.x * 0.1f,
                                  (float)event.offset.y * 0.1f);
        }
        return true;
    });
    e.dispatch<InputEventType::key>([&](KeyEvent const& event) {
        //			if (event.repeat > 1)
        //				return true;

        ImGuiKey const key = toImGuiKeyCode(event.key);
        bool const down = e.type() == InputEventType::keyDown;

        // Set Modifiers
        switch (event.key) {
        case Key::LeftCtrl:
        case Key::RightCtrl:
            io.KeyCtrl = down;
            break;

        case Key::LeftShift:
        case Key::RightShift:
            io.KeyShift = down;
            break;

        case Key::LeftAlt:
        case Key::RightAlt:
            io.KeyAlt = down;
            break;

        case Key::LeftSuper:
        case Key::RightSuper:
            io.KeySuper = down;
            break;

        default:
            break;
        }

        io.AddKeyEvent(key, e.type() == InputEventType::keyDown);

        return true;
    });
}

void poppy::ImGuiContext::onCharInput(unsigned int code) {
    ImGui::SetCurrentContext(context);
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(code);
}

#if !defined(BLOOM_PLATFORM_APPLE)
/// On Apple platform these are defined in Platform/MacOS/ImGuiContext.mm

void poppy::ImGuiContext::doInitPlatform(bloom::HardwareDevice& device,
                                         ImGuiContextDescription const& d) {}

void poppy::ImGuiContext::createFontAtlas(bloom::HardwareDevice&) {}

void poppy::ImGuiContext::doNewFramePlatform(bloom::Window& window) {}

void poppy::ImGuiContext::doDrawFramePlatform(bloom::HardwareDevice& device,
                                              bloom::Window& window) {}

void poppy::ImGuiContext::doShutdownPlatform() {}

void poppy::ImGuiContext::createFontAtlasPlatform(ImFontAtlas* atlas,
                                                  bloom::HardwareDevice&) {}

#endif
