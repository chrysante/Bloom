#include "Bloom/Application/ResourceUtil.hpp"

#import <Appkit/Appkit.h>


namespace bloom {
	
	std::filesystem::path resourceDir() {
		return std::filesystem::path([NSBundle mainBundle].resourceURL.path.UTF8String);
	}
	
	std::filesystem::path libraryDir() {
		NSError* error;
		NSURL *fileURL = [[NSFileManager defaultManager] URLForDirectory: NSLibraryDirectory
																inDomain: NSUserDomainMask
													   appropriateForURL: nil
																  create: true
																   error: &error];
		return std::filesystem::path(fileURL.path.UTF8String);
	}
	
	void showSavePanel(utl::function<void(std::filesystem::path const&)> completion) {
		NSSavePanel* panel = [NSSavePanel savePanel];
		
		[panel beginWithCompletionHandler:^(NSInteger result){
			if (result == NSModalResponseOK) {
				NSURL*  url = [panel URL];
				
				completion(url.path.UTF8String);
			}
		}];
	}
	
	void showOpenPanel(OpenPanelDescription const& desc,
					   utl::function<void(utl::vector<std::filesystem::path> const&)> completion) {
		NSOpenPanel* panel = [NSOpenPanel openPanel];
		
		[panel setResolvesAliases: desc.resolvesAliases];
		[panel setCanChooseDirectories: desc.canChooseDirectories];
		[panel setAllowsMultipleSelection: desc.allowsMultipleSelection];
		[panel setCanChooseFiles: desc.canChooseFiles];
		
		[panel beginWithCompletionHandler:^(NSInteger result){
			if (result == NSModalResponseOK) {
				NSArray* urls = [panel URLs];
				utl::vector<std::filesystem::path> paths;
				paths.resize(urls.count);
				for (auto&& [index, path]: utl::enumerate(paths)) {
					path = ((NSURL*)[urls objectAtIndex: index]).path.UTF8String;
				}
				
				completion(paths);
			}
		}];
	}
	
}
