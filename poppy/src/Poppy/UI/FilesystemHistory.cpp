#include "FilesystemHistory.hpp"


#include "Poppy/UI/ImGuiHelpers.hpp"
#include "Poppy/Core/Debug.hpp"

#include <imgui/imgui.h>
#include <utl/utility.hpp>

namespace poppy {
	
	void FilesystemHistory::push(std::filesystem::path p) {
		if (index + 1 < paths.size()) {
			paths.resize(index + 1);
		}
		paths.push_back(std::move(p));
		if (paths.size() != 1) {
			++index;
		}
		poppyAssert(paths.size() == index + 1);
	}
	
	
	void FilesystemHistory::pop() {
		poppyExpect(canPop());
		--index;
	}
	
	void FilesystemHistory::unpop() {
		poppyExpect(canUnpop());
		++index;
	}
	
	void FilesystemHistory::clear() {
		paths.clear();
		index = 0;
	}
	
	std::filesystem::path FilesystemHistory::current() const {
		poppyExpect(!paths.empty());
		return paths[index];
	}
	
	bool FilesystemHistory::canPop() const {
		if (paths.empty()) { return false; }
		return index > 0;
	}
	
	bool FilesystemHistory::canUnpop() const {
		if (paths.empty()) { return false; }
		return index < paths.size() - 1;
	}

	void FilesystemHistory::displayDebugger() {
		ImGui::Begin("Filesystem History Debugger");
		
		for (auto&& [i, path]: utl::enumerate(paths)) {
			auto& p = path;
			withFont(Font::UIDefault().setWeight(i == index ? FontWeight::semibold : FontWeight::regular), [&]{
				ImGui::Text("%s", p.string().data());
			});
		}
		
		ImGui::End();
	}
	
}
