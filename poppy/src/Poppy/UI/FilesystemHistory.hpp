#pragma once

#include <filesystem>
#include <utl/vector.hpp>

namespace poppy {
	
	class FilesystemHistory {
	public:
		void push(std::filesystem::path p);
		void pop();
		void unpop();
		void clear();
		
		std::filesystem::path current() const;
		bool canPop() const;
		bool canUnpop() const;
		
		void displayDebugger();
		
	private:
		utl::vector<std::filesystem::path> paths;
		std::size_t index = 0;
	};
	
}
