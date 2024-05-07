#include "Poppy/UI/ImGuiContext.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <utl/strcat.hpp>

#include "Bloom/Application.h"
#include "Bloom/Core.h"
#include "Poppy/Core/Debug.h"
#include "Poppy/Editor/Editor.h"
#include "Poppy/UI/CommandPalette.h"
#include "Poppy/UI/Font.h"

#ifdef IMGUI_ENABLE_TEST_ENGINE
#include <imgui_te_engine.h>
#include <imgui_te_ui.h>

#include "Poppy/UI/TestRegistry.h"
#endif

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

void poppy::ImGuiContext::init(Editor& editor,
                               ImGuiContextDescription const& ds) {
    mEditor = &editor;
    desc = ds;
    assignReceiver(editor.makeReceiver());
    IMGUI_CHECKVERSION();
    /// Scale factor values are figured out by trial-and-error and are hardcoded
    /// for now.
    static constexpr float ScaleFactor = 2.0;

    auto fontManager = std::make_unique<FontManager>(ScaleFactor);
    static_cast<Emitter&>(*fontManager) = editor.makeEmitter();
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
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigViewportsNoAutoMerge = true;
    io.ConfigViewportsNoDecoration = false;
    io.ConfigViewportsNoDefaultParent = true;
    io.IniFilename = desc.iniFilePath.c_str();
    initPlatform();
    initTestEngine();

    listen([this](ReloadedFontAtlasCommand cmd) {
        newFontAtlas = cmd.fontManager->getAtlas();
    });
}

void poppy::ImGuiContext::shutdown() {
    stopTestEngine();
    std::lock_guard lock(drawMutex);
    ImGui::SetCurrentContext(context);
    shutdownPlatform();
    ImGui::DestroyContext(context);
    context = nullptr;
    shutdownTestEngine();
}

void poppy::ImGuiContext::newFrame() {
    // FIXME: This is not pretty
    utl::timed_busy_wait(std::chrono::milliseconds{ 16 },
                         [&] { return numRenderTasks == 0; });
    {
        std::lock_guard lock(drawMutex);
        drawDataValid = false;
        if (newFontAtlas) {
            context->IO.Fonts = newFontAtlas;
            uploadCurrentFontAtlas(mEditor->device());
            context->IO.FontDefault = FontManager::get(FontDesc::UIDefault());
            newFontAtlas = nullptr;
        }
    }
    auto& window = *mEditor->mainWindow();
    ImGui::SetCurrentContext(context);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = (vml::float2)window.size();
    io.DisplayFramebufferScale = window.contentScaleFactor();
    newFramePlatform();
    ImGui::NewFrame();

    if (showTestUI) {
        ImGuiTestEngine_ShowTestEngineWindows(testEngine, &showTestUI);
    }
    if (ImGuiTestEngine_IsUsingSimulatedInputs(testEngine)) {
        mEditor->invalidateUI();
    }
}

void poppy::ImGuiContext::endFrame() {
    ImGui::SetCurrentContext(context);
    ImGui::Render();
    ImGui::UpdatePlatformWindows();
    std::lock_guard lock(drawMutex);
    drawDataValid = true;
}

/// This is called asynchronously on the render thread
void poppy::ImGuiContext::drawFrame() {
    ImGui::SetCurrentContext(context);
    drawFramePlatform();
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
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            io.AddMousePosEvent(event.locationOnScreen.x,
                                event.locationOnScreen.y);
        }
        else {
            io.AddMousePosEvent(event.locationInWindow.x,
                                event.locationInWindow.y);
        }
        return true;
    });
    e.dispatch<InputEventMask::MouseDragged>([&](MouseDragEvent const& event) {
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            io.AddMousePosEvent(event.locationOnScreen.x,
                                event.locationOnScreen.y);
        }
        else {
            io.AddMousePosEvent(event.locationInWindow.x,
                                event.locationInWindow.y);
        }
        return true;
    });
    e.dispatch<InputEventMask::ScrollWheel>([&](ScrollEvent const& event) {
        if (event.offset.x != 0.0 || event.offset.y != 0.0) {
            float factor = event.isTrackpad ? 0.01f : 0.1f;
            io.AddMouseWheelEvent((float)event.offset.x * factor,
                                  (float)event.offset.y * factor);
        }
        return true;
    });
    e.dispatch<InputEventMask::Key>([&](KeyEvent const& event) {
        ImGuiKey const key = toImGuiKeyCode(event.key);
        io.AddKeyEvent(key, /* isDown = */ e.type() == InputEventMask::KeyDown);
        return true;
    });
}

void poppy::ImGuiContext::onTextInput(unsigned int code) {
    ImGui::SetCurrentContext(context);
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(code);
}

#ifdef IMGUI_ENABLE_TEST_ENGINE

struct TestBase::Impl {
    static void populateTestObject(TestBase& object, ImGuiTestContext* ctx) {
        object._testCtx = ctx;
    }

    static void populateTestObject(TestBase& object, Emitter* emitter) {
        object._emitter = emitter;
    }

    static void run(TestBase& object) { object.doRun(); }
};

static void registerTests(ImGuiTestEngine* engine, Emitter& emitter) {
    /// Dummy test. We will remove this later
    ImGuiTest* test = IM_REGISTER_TEST(engine, "MyTests", "Test 1");
    test->GuiFunc = [](ImGuiTestContext*) {
        ImGui::SetNextWindowSize({ 500, 500 });
        ImGui::Begin("Test Window##1", NULL, ImGuiWindowFlags_NoSavedSettings);
        ImGui::BeginChild("Child");
        ImGui::Text("Hello, automation world");
        ImGui::Button("Click Me");
        if (ImGui::TreeNode("Node")) {
            static bool b = false;
            ImGui::Checkbox("Checkbox", &b);
            ImGui::TreePop();
        }
        ImGui::EndChild();
        ImGui::End();
    };
    test->TestFunc = [](ImGuiTestContext* ctx) {
        auto child = ctx->WindowInfo("Test Window##1/Child").ID;
        ctx->SetRef(child);
        ctx->ItemClick("Click Me");
        ctx->ItemCheck("Node/Checkbox");
        ctx->ItemUncheck("Node/Checkbox");
    };
    /// Register all statically registered tests
    for (auto& entry: getRegisteredTests()) {
        ImGuiTest* test = ImGuiTestEngine_RegisterTest(engine,
                                                       entry.category.c_str(),
                                                       entry.name.c_str(),
                                                       entry.file, entry.line);
        test->UserData = entry.testObject;
        TestBase::Impl::populateTestObject(*entry.testObject, &emitter);
        test->TestFunc = [](ImGuiTestContext* ctx) {
            auto* testObject = (TestBase*)ctx->Test->UserData;
            TestBase::Impl::populateTestObject(*testObject, ctx);
            TestBase::Impl::run(*testObject);
        };
    }
}
#endif

void poppy::ImGuiContext::initTestEngine() {
#ifdef IMGUI_ENABLE_TEST_ENGINE
    testEngine = ImGuiTestEngine_CreateContext();
    ImGuiTestEngineIO& test_io = ImGuiTestEngine_GetIO(testEngine);
    test_io.ConfigVerboseLevel = ImGuiTestVerboseLevel_Info;
    test_io.ConfigVerboseLevelOnError = ImGuiTestVerboseLevel_Debug;

    testEmitter = mEditor->makeEmitter();
    registerTests(testEngine, testEmitter);
    ImGuiTestEngine_Start(testEngine, context);
    ImGuiTestEngine_InstallDefaultCrashHandler();

    mEditor->addCommand({ "Show Test UI",
                          { Key::T, ModFlags::Control | ModFlags::Shift },
                          [this] { showTestUI ^= true; } });
#endif
}

void poppy::ImGuiContext::stopTestEngine() {
#ifdef IMGUI_ENABLE_TEST_ENGINE
    ImGuiTestEngine_Stop(testEngine);
#endif
}

void poppy::ImGuiContext::shutdownTestEngine() {
#ifdef IMGUI_ENABLE_TEST_ENGINE
    ImGuiTestEngine_DestroyContext(testEngine);
#endif
}

#if !defined(BLOOM_PLATFORM_APPLE)

/// On Apple platform these are defined in Platform/MacOS/ImGuiContext.mm

void poppy::ImGuiContext::initPlatform(HardwareDevice& device,
                                       ImGuiContextDescription const& d) {}

void poppy::ImGuiContext::createFontAtlas(HardwareDevice&) {}

void poppy::ImGuiContext::newFramePlatform(Window& window) {}

void poppy::ImGuiContext::drawFramePlatform(HardwareDevice& device,
                                            Window& window) {}

void poppy::ImGuiContext::shutdownPlatform() {}

void poppy::ImGuiContext::createFontAtlasPlatform(ImFontAtlas* atlas,
                                                  HardwareDevice&) {}

#endif
