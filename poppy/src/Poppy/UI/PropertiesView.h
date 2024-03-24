#ifndef POPPY_UI_PROPERTIESVIEW_H
#define POPPY_UI_PROPERTIESVIEW_H

#include <string>

#include "Poppy/UI/Font.h"
#include "Poppy/UI/ImGuiHelpers.h"

namespace poppy::propertiesView {

void header(
    std::string_view name,
    FontDesc const& = FontDesc::UIDefault().setWeight(FontWeight::Semibold));
bool beginSection();

// calls header(...) && beginSection()
bool beginSection(
    std::string_view name,
    FontDesc const& = FontDesc::UIDefault().setWeight(FontWeight::Semibold));

void endSection();

void beginProperty(std::string_view label);
void beginProperty(std::string_view label, FontDesc const&);

void fullWidth(float offset = 0);

} // namespace poppy::propertiesView

#endif // POPPY_UI_PROPERTIESVIEW_H
