#pragma once

#include <string>

#include "Poppy/UI/Font.hpp"
#include "Poppy/UI/ImGuiHelpers.hpp"

namespace poppy::propertiesView {

void header(std::string_view name,
            Font const& = Font::UIDefault().setWeight(FontWeight::semibold));
bool beginSection();

// calls header(...) && beginSection()
bool beginSection(
    std::string_view name,
    Font const& = Font::UIDefault().setWeight(FontWeight::semibold));

void endSection();

void beginProperty(std::string_view label);
void beginProperty(std::string_view label, Font const&);

void fullWidth(float offset = 0);

} // namespace poppy::propertiesView
