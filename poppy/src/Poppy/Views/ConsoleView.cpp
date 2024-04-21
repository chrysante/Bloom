#include <imgui.h>
#include <imgui_internal.h>
#include <utl/format_time.hpp>
#include <utl/uuid.hpp>
#include <vml/vml.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/Core/Serialize.h"
#include "Poppy/UI/Font.h"
#include "Poppy/UI/View.h"
#include "Poppy/UI/ViewRegistry.h"

using namespace poppy;
using namespace bloom;
using namespace vml::short_types;

using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;

namespace {

struct Message {
    int ID;
    Logger::Level level;
    TimePoint timestamp;
    std::string text = {};
};

class UILogger: public Logger::Impl {
public:
    std::vector<Message> messages() const {
        std::vector<Message> result;
        Logger::doLocked([&] { result = _messages; });
        return result;
    }

    void clear() {
        Logger::doLocked([&] { _messages.clear(); });
    }

private:
    void beginLog(Logger::Level level) final {
        _messages.push_back({ IDCounter++, level, Clock::now() });
    }

    void endLog() final { _messages.back().text = std::move(sstr).str(); }

    std::ostream& getOstream() final { return sstr; }

    std::stringstream sstr;
    std::vector<Message> _messages;
    int IDCounter = 0;
};

static UILogger* gLogger = nullptr;

/// Data to communicate between frames
struct ActiveData {
    bool isMaxScroll = true;
    float timestampWidth = 0;
};

static int const TextSizeMin = 5;
static int const TextSizeMax = 20;

/// Data that persists between program runs
struct PersistentData {
    bool showTimeStamps = true;
    bool showStyleEditor = false;
    int verbosity = EnumCount<Logger::Level>;
    int textSize = 14;
};

/// Console that displays logger messages
struct ConsoleView: View {
    ConsoleView();

    void frame() override;

    void displayMessage(float& tsWidthAcc, Message const& msg);

    void backgroundContextMenu();

    void showDebugMenuItems();

    YAML::Node serialize() const override;

    void deserialize(YAML::Node node) override;

    ActiveData active;
    PersistentData persistent;
};

} // namespace

POPPY_REGISTER_VIEW(ConsoleView, "Console");

static void modifyTextSize(int& textSize, int offset) {
    textSize = std::clamp(textSize + offset, TextSizeMin, TextSizeMax);
}

ConsoleView::ConsoleView(): View({ .title = "Console" }) {
    auto& cmd = commandPaletteWrite();
    cmd.add("Increase Text Size", { Key::Plus, ModFlags::Control },
            [this] { modifyTextSize(persistent.textSize, 1); });
    cmd.add("Decrease Text Size", { Key::Minus, ModFlags::Control },
            [this] { modifyTextSize(persistent.textSize, -1); });
}

/// # Style

struct ConsoleStyleColors {
    float4 background;
    float4 timestamp;
    float4 level[EnumCount<Logger::Level>];
};

struct ConsoleStyle {
    ConsoleStyleColors color;
    FontDesc baseFont;
    float YSpacing;
    float InnerSpacingX;
};

static ConsoleStyle style{
    .color = { .background = vml::hsva_to_rgba(0.0, 0.0, 0.05, 1.0),
               .timestamp = { 1, 1, 1, 0.5 },
               .level = { /* Trace:   */ { 1, 1, 1, 0.75 },
                          /* Info:    */ { 0.6, 0.9, 0.1, 1 },
                          /* Debug:   */ { 1, 0.2, 0.7, 1 },
                          /* Warning: */ { 0.95, 0.8, 0.15, 1 },
                          /* Error:   */ { 1, 0.08, 0.08, 1 },
                          /* Fatal:   */ { 1, 0.08, 0.08, 1 } } },
    .baseFont = { FontSize{}, FontWeight::Regular, FontStyle::Monospaced }
};

static void colorEdit(char const* label, float4& color) {
    ImGui::ColorEdit4(label, color.data(),
                      ImGuiColorEditFlags_Float |
                          ImGuiColorEditFlags_PickerHueWheel);
}

void displayStyleEditor(bool* open = nullptr) {
    using enum Logger::Level;
    ImGui::Begin("Console Style Editor", open);
    colorEdit("Background", style.color.background);
    colorEdit("Timestamps", style.color.timestamp);
    colorEdit("Trace", style.color.level[(size_t)Trace]);
    colorEdit("Info", style.color.level[(size_t)Info]);
    colorEdit("Debug", style.color.level[(size_t)Debug]);
    colorEdit("Warning", style.color.level[(size_t)Warning]);
    colorEdit("Error", style.color.level[(size_t)Error]);
    colorEdit("Fatal", style.color.level[(size_t)Fatal]);
    ImGui::SliderFloat("Y Spacing", &style.YSpacing, -10, 20);
    ImGui::SliderFloat("Inner X Spacing", &style.InnerSpacingX, 0, 50);
    ImGui::End();
}

static ImFont* getTimestampFont(int textSize) {
    return FontManager::get(style.baseFont.setSize(FontSize{ textSize }));
}

static ImFont* selectMsgFont(Logger::Level level, int textSize) {
    auto base = style.baseFont.setSize(FontSize{ textSize });
    using enum Logger::Level;
    switch (level) {
    case Trace:
        return FontManager::get(base);
    case Info:
        [[fallthrough]];
    case Debug:
        [[fallthrough]];
    case Warning:
        [[fallthrough]];
    case Error:
        [[fallthrough]];
    case Fatal:
        return FontManager::get(base.setWeight(FontWeight::Semibold));
    }
}

float4 selectMsgColor(Logger::Level level) {
    return style.color.level[(size_t)level];
}

// Copied from Bloom/Core/Debug.cpp
static std::string formatTime(TimePoint tp) {
    using namespace std::chrono;
    auto dur = tp.time_since_epoch();
    dur -= duration_cast<days>(dur);
    auto h = duration_cast<hours>(dur);
    dur -= h;
    auto min = duration_cast<minutes>(dur);
    dur -= min;
    auto sec = duration_cast<seconds>(dur);
    dur -= sec;
    auto mil = duration_cast<milliseconds>(dur);
    dur -= mil;
    std::stringstream str;
    str << std::setfill(' ') << std::setw(2) << h.count()
        << "h:" << std::setfill(' ') << std::setw(2) << min.count()
        << "m:" << std::setfill(' ') << std::setw(2) << sec.count()
        << "s:" << std::setfill(' ') << std::setw(3) << mil.count() << "ms ";
    return std::move(str).str();
}

static void scrollBehaviour(bool& isMaxScroll) {
    auto& io = GImGui->IO;
    if (isMaxScroll) {
        if (io.MouseWheel <= 0.0f) {
            ImGui::SetScrollY(ImGui::GetScrollMaxY());
        }
        else {
            isMaxScroll = false;
        }
    }
    else {
        isMaxScroll = ImGui::GetScrollY() == ImGui::GetScrollMaxY();
    }
}

static void displayTimestamp(float& tsWidthAcc, TimePoint tp, int ID,
                             int textSize) {
    ImGui::PushFont(getTimestampFont(textSize));
    std::string timestamp = formatTime(tp);
    float2 size = ImGui::CalcTextSize(timestamp.data(),
                                      timestamp.data() + timestamp.size());
    tsWidthAcc = std::max(tsWidthAcc, size.x);
    auto* window = GImGui->CurrentWindow;
    ImRect bb(window->DC.CursorPos, (float2)window->DC.CursorPos + size);
    ImGui::ItemAdd(bb, window->GetID(ID));
    ImGui::ItemSize(bb);
    auto* DL = window->DrawList;
    DL->AddText(bb.Min, ImGui::ColorConvertFloat4ToU32(style.color.timestamp),
                timestamp.c_str());
    ImGui::PopFont();
}

static void displayMessageText(Message const& msg, int textSize) {
    ImGui::PushFont(selectMsgFont(msg.level, textSize));
    ImGui::PushStyleColor(ImGuiCol_Text, selectMsgColor(msg.level));
    ImGui::PushTextWrapPos(0.0f);
    ImGui::TextEx(msg.text.data(), msg.text.data() + msg.text.size());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    ImGui::PopFont();
}

void ConsoleView::displayMessage(float& tsWidthAcc, Message const& msg) {
    auto* window = GImGui->CurrentWindow;
    float2 cursorBegin = window->DC.CursorPos;
    if (persistent.showTimeStamps) {
        displayTimestamp(tsWidthAcc, msg.timestamp, msg.ID,
                         persistent.textSize);
    }
    float2 cursorEnd = window->DC.CursorPos;
    window->DC.CursorPos =
        cursorBegin + float2(active.timestampWidth + style.InnerSpacingX, 0);
    displayMessageText(msg, persistent.textSize);
    window->DC.CursorPos =
        float2(cursorEnd.x, std::max(cursorEnd.y, window->DC.CursorPos.y));
    window->DC.IsSetPos = true;
}

void ConsoleView::frame() {
    if (!gLogger) {
        return;
    }
    setBGColor(style.color.background);
    auto* DL = ImGui::GetWindowDrawList();
    DL->AddRect(ImGui::GetWindowPos(),
                (float2)ImGui::GetWindowPos() + (float2)ImGui::GetWindowSize(),
                ImGui::ColorConvertFloat4ToU32(style.color.background));

    float tsWidth = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,
                        { ImGui::GetStyle().ItemSpacing.x, style.YSpacing });
    for (auto& msg: gLogger->messages()) {
        if ((int)msg.level >=
            (int)EnumCount<Logger::Level> - persistent.verbosity)
        {
            displayMessage(tsWidth, msg);
        }
    }
    ImGui::PopStyleVar();
    active.timestampWidth = tsWidth;
    scrollBehaviour(active.isMaxScroll);
    backgroundContextMenu();
    if (persistent.showStyleEditor) {
        displayStyleEditor(&persistent.showStyleEditor);
    }
}

void ConsoleView::backgroundContextMenu() {
    auto name = "BGContextMenu";
    if (ImGui::IsWindowHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImGui::OpenPopup(name);
    }
    if (!ImGui::BeginPopup(name)) {
        return;
    }
    ImGui::Checkbox("Show Timestamps", &persistent.showTimeStamps);
    if (ImGui::MenuItem("Clear")) {
        gLogger->clear();
    }
    ImGui::Separator();
    ImGui::SliderInt("Verbosity", (int*)&persistent.verbosity, 1,
                     (int)EnumCount<Logger::Level>);
    ImGui::SliderInt("Text Size", (int*)&persistent.textSize, TextSizeMin,
                     TextSizeMax);
    showDebugMenuItems();
    ImGui::EndPopup();
}

static void logTestMessages() {
    Logger::Trace("Trace test message");
    Logger::Info("Info test message");
    Logger::Debug("Debug test message");
    Logger::Warn("Warning test message");
    Logger::Error("Error test message");
    Logger::Fatal("Fatal test message");
}

void ConsoleView::showDebugMenuItems() {
    ImGui::Separator();
    if (ImGui::MenuItem("Log Test Messages")) {
        logTestMessages();
    }
    if (ImGui::MenuItem("Log Verbose Message")) {
        Logger::Trace(
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.");
    }
    if (ImGui::MenuItem("Spam")) {
        for (int i = 0; i < 10000; ++i) {
            Logger::Debug(utl::uuid::generate());
        }
    }
    if (ImGui::MenuItem("Show Style Editor")) {
        persistent.showStyleEditor = true;
    }
}

BLOOM_MAKE_TEXT_SERIALIZER(PersistentData, showTimeStamps, showStyleEditor,
                           verbosity, textSize);

YAML::Node ConsoleView::serialize() const {
    YAML::Node root;
    root["Persisent"] = persistent;
    return root;
}

void ConsoleView::deserialize(YAML::Node node) {
    persistent = node["Persisent"].as<PersistentData>();
}

std::unique_ptr<Logger::Impl> bloom::makeGlobalLoggerImpl() {
    auto l = std::make_unique<UILogger>();
    gLogger = l.get();
    return l;
}
