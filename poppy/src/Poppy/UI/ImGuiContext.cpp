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

    case Key::apostrophe:
        return ImGuiKey_Apostrophe;
    case Key::equal:
        return ImGuiKey_Equal;
    case Key::minus:
        return ImGuiKey_Minus;
    case Key::leftBracket:
        return ImGuiKey_LeftBracket;
    case Key::rightBracket:
        return ImGuiKey_RightBracket;
    case Key::semicolon:
        return ImGuiKey_Semicolon;
    case Key::backslash:
        return ImGuiKey_Backslash;
    case Key::comma:
        return ImGuiKey_Comma;
    case Key::slash:
        return ImGuiKey_Slash;
    case Key::period:
        return ImGuiKey_Period;
    case Key::graveAccent:
        return ImGuiKey_GraveAccent;
    case Key::keypadDecimal:
        return ImGuiKey_KeypadDecimal;
    case Key::keypadMultiply:
        return ImGuiKey_KeypadMultiply;
    case Key::keypadAdd:
        return ImGuiKey_KeypadAdd;
    case Key::numLock:
        return ImGuiKey_NumLock;
    case Key::keypadDivide:
        return ImGuiKey_KeypadDivide;
    case Key::keypadEnter:
        return ImGuiKey_KeypadEnter;
    case Key::keypadSubtract:
        return ImGuiKey_KeypadSubtract;
    case Key::keypadEqual:
        return ImGuiKey_KeypadEqual;
    case Key::keypad0:
        return ImGuiKey_Keypad0;
    case Key::keypad1:
        return ImGuiKey_Keypad1;
    case Key::keypad2:
        return ImGuiKey_Keypad2;
    case Key::keypad3:
        return ImGuiKey_Keypad3;
    case Key::keypad4:
        return ImGuiKey_Keypad4;
    case Key::keypad5:
        return ImGuiKey_Keypad5;
    case Key::keypad6:
        return ImGuiKey_Keypad6;
    case Key::keypad7:
        return ImGuiKey_Keypad7;
    case Key::keypad8:
        return ImGuiKey_Keypad8;
    case Key::keypad9:
        return ImGuiKey_Keypad9;
    case Key::enter:
        return ImGuiKey_Enter;
    case Key::tab:
        return ImGuiKey_Tab;
    case Key::space:
        return ImGuiKey_Space;
    case Key::backspace:
        return ImGuiKey_Backspace;
    case Key::escape:
        return ImGuiKey_Escape;
    case Key::capsLock:
        return ImGuiKey_CapsLock;
    case Key::leftCtrl:
        return ImGuiKey_LeftCtrl;
    case Key::leftShift:
        return ImGuiKey_LeftShift;
    case Key::leftAlt:
        return ImGuiKey_LeftAlt;
    case Key::leftSuper:
        return ImGuiKey_LeftSuper;
    case Key::rightCtrl:
        return ImGuiKey_RightCtrl;
    case Key::rightShift:
        return ImGuiKey_RightShift;
    case Key::rightAlt:
        return ImGuiKey_RightAlt;
    case Key::rightSuper:
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

    case Key::printScreen:
        return ImGuiKey_PrintScreen;
    case Key::menu:
        return ImGuiKey_Menu;
    case Key::insert:
        return ImGuiKey_Insert;
    case Key::home:
        return ImGuiKey_Home;
    case Key::end:
        return ImGuiKey_End;
    case Key::pageUp:
        return ImGuiKey_PageUp;
    case Key::pageDown:
        return ImGuiKey_PageDown;
    case Key::delete_:
        return ImGuiKey_Delete;
    case Key::leftArrow:
        return ImGuiKey_LeftArrow;
    case Key::rightArrow:
        return ImGuiKey_RightArrow;
    case Key::downArrow:
        return ImGuiKey_DownArrow;
    case Key::upArrow:
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
    desc         = ds;
    this->bloom::Reciever::operator=(application.makeReciever());

    IMGUI_CHECKVERSION();

    float const scaleFactor = 2;

    fonts.init(application);
    loadFonts(device, scaleFactor);
    context                     = ImGui::CreateContext(sFontAtlas);
    context->IO.FontGlobalScale = 1.0 / scaleFactor;
    context->IO.FontDefault     = fonts.get(Font::UIDefault());

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

    icons.load(*sFontAtlas,
               scaleFactor,
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

    io.DisplaySize             = (mtl::float2)window.size();
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
        bool const down    = e.type() == InputEventType::keyDown;

        // Set Modifiers
        switch (event.key) {
        case Key::leftCtrl:
        case Key::rightCtrl:
            io.KeyCtrl = down;
            break;

        case Key::leftShift:
        case Key::rightShift:
            io.KeyShift = down;
            break;

        case Key::leftAlt:
        case Key::rightAlt:
            io.KeyAlt = down;
            break;

        case Key::leftSuper:
        case Key::rightSuper:
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
