#ifndef POPPY_EDITOR_VIEWS_DEBUGVIEWS_H
#define POPPY_EDITOR_VIEWS_DEBUGVIEWS_H

namespace poppy {

class DebugViews {
public:
    void display();
    void menubar();

    bool showDemoWindow = false;
    bool showUIDebugger = false;
    bool showStylePanel = false;
};

} // namespace poppy

#endif // POPPY_EDITOR_VIEWS_DEBUGVIEWS_H
