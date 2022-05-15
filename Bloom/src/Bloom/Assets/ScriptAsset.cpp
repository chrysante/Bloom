#include "ScriptAsset.hpp"

namespace bloom {
    
	static bool isDelim(char c) {
		return isspace(c) || [c]{
			for (auto x: R"({}()[]<>,.;:+-*/&"')") {
				if (c == x) { return true; }
			}
			return false;
		}();
	}

	utl::vector<std::string> findClassNames(std::string_view text) {
		std::string const classToken = "class";
		std::size_t pos = 0;

		utl::vector<std::string> result;
		
		auto const advanceUntil = [&](std::size_t& position, auto&& condition) {
			while (!condition(text[position])) {
				++position;
				if (position == text.size()) {
					return;
				}
			}
		};
		
		while (true) {
			pos = text.find(classToken, pos);
			if (pos == std::string::npos) {
				break;
			}
			auto const prevPos = pos - 1;
			if (prevPos < text.size() && !isDelim(text[prevPos])) {
				continue;
			}
			pos += classToken.size();
			if (pos < text.size() && !isDelim(text[pos])) {
				continue;
			}
			advanceUntil(pos, [](char c) { return !isDelim(c); });
			std::size_t endPos = pos;
			advanceUntil(endPos, isDelim);
			if (pos == endPos) {
				continue;
			}
			result.push_back(std::string(text.substr(pos, endPos - pos)));
			pos = endPos;
		}

		return result;
	}
	
	void ScriptAsset::setText(std::string str) {
		text = std::move(str);
		classes = findClassNames(text);
	}


}
