#include <imgui.h>
#include <imgui_internal.h>
#include <utl/format_time.hpp>
#include <vml/vml.hpp>

#include "Bloom/Core/Debug.h"
#include "Poppy/UI/View.h"

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

struct UILogger: Logger::Impl {
    void beginLog(Logger::Level level) final {
        messages.push_back({ IDCounter++, level, Clock::now() });
    }

    void endLog() final { messages.back().text = std::move(sstr).str(); }

    std::ostream& getOstream() final { return sstr; }

    std::stringstream sstr;
    std::vector<Message> messages;
    int IDCounter = 0;
};

static UILogger* gLogger = nullptr;

struct ConsoleView: View {
    void frame() override;

    void displayMessage(float& tsWidthAcc, Message const& msg);

    bool isMaxScroll = true;
    float timestampWidth = 0;
};

} // namespace

POPPY_REGISTER_VIEW(ConsoleView, "Console", {});

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

static void displayTimestamp(float& tsWidthAcc, TimePoint tp, int ID) {
    std::string timestamp = formatTime(tp);
    float2 size = ImGui::CalcTextSize(timestamp.data(),
                                      timestamp.data() + timestamp.size());
    tsWidthAcc = std::max(tsWidthAcc, size.x);
    auto* window = GImGui->CurrentWindow;
    ImRect bb(window->DC.CursorPos, (float2)window->DC.CursorPos + size);
    ImGui::ItemAdd(bb, window->GetID(ID));
    ImGui::ItemSize(bb);
    auto* DL = window->DrawList;
    DL->AddText(bb.Min, IM_COL32(255, 255, 255, 255), timestamp.c_str());
}

static void displayMessageText(Message const& msg) {
    float2 size =
        ImGui::CalcTextSize(msg.text.data(), msg.text.data() + msg.text.size());
    auto* window = GImGui->CurrentWindow;
    ImRect bb(window->DC.CursorPos, (float2)window->DC.CursorPos + size);
    ImGui::ItemAdd(bb, window->GetID(msg.ID));
    ImGui::ItemSize(bb);
    auto* DL = window->DrawList;
    DL->AddText(bb.Min, IM_COL32(255, 255, 255, 255), msg.text.c_str());
}

void ConsoleView::displayMessage(float& tsWidthAcc, Message const& msg) {
    std::string timestamp = formatTime(msg.timestamp);
    float2 tsSize = ImGui::CalcTextSize(timestamp.data(),
                                        timestamp.data() + timestamp.size());
    tsWidthAcc = std::max(tsWidthAcc, tsSize.x);
    auto* window = GImGui->CurrentWindow;
    float2 cursorBegin = window->DC.CursorPos;
    displayTimestamp(tsWidthAcc, msg.timestamp, msg.ID);
    float2 cursorEnd = window->DC.CursorPos;
    float const InnerSpacing = 10;
    window->DC.CursorPos =
        cursorBegin + float2(timestampWidth + InnerSpacing, 0);
    displayMessageText(msg);
    window->DC.CursorPos = cursorEnd;
    window->DC.IsSetPos = true;
}

void ConsoleView::frame() {
    if (!gLogger) {
        return;
    }
    float tsWidth = 0;
    for (auto& msg: gLogger->messages) {
        displayMessage(tsWidth, msg);
    }
    if (tsWidth > 0) {
        timestampWidth = tsWidth;
    }
    scrollBehaviour(isMaxScroll);
}

std::unique_ptr<Logger::Impl> bloom::makeGlobalLoggerImpl() {
    auto l = std::make_unique<UILogger>();
    gLogger = l.get();
    return l;
}
