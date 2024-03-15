#include "Bloom/Application/ResourceUtil.h"

#include <cstdlib>
#include <vector>

#import <Appkit/Appkit.h>

using namespace bloom;
	
std::filesystem::path bloom::resourceDir() {
    if (auto* dir = std::getenv("POPPY_RESOURCE_DIR")) {
        return dir;
    }
    return std::filesystem::path(
        [NSBundle mainBundle].resourceURL.path.UTF8String);
}

std::filesystem::path bloom::libraryDir() {
    NSError* error;
    NSURL *fileURL =
        [[NSFileManager defaultManager] URLForDirectory: NSLibraryDirectory
                                               inDomain: NSUserDomainMask
                                      appropriateForURL: nil
                                                 create: true
                                                  error: &error];
    return std::filesystem::path(fileURL.path.UTF8String) / "Preferences";
}

void bloom::showSavePanel(
    std::function<void(std::filesystem::path const&)> completion) {
    NSSavePanel* panel = [NSSavePanel savePanel];
    [panel beginWithCompletionHandler:^(NSInteger result){
        if (result == NSModalResponseOK) {
            NSURL*  url = [panel URL];
            completion(url.path.UTF8String);
        }
    }];
}

void bloom::showOpenPanel(
    OpenPanelDescription const& desc,
    std::function<void(std::span<std::filesystem::path const>)> completion) {
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setResolvesAliases: desc.resolvesAliases];
    [panel setCanChooseDirectories: desc.canChooseDirectories];
    [panel setAllowsMultipleSelection: desc.allowsMultipleSelection];
    [panel setCanChooseFiles: desc.canChooseFiles];
    [panel beginWithCompletionHandler:^(NSInteger result){
        if (result == NSModalResponseOK) {
            NSArray* urls = [panel URLs];
            std::vector<std::filesystem::path> paths;
            paths.resize(urls.count);
            for (size_t index = 0; auto& path: paths) {
                path = ((NSURL*)[urls objectAtIndex: index]).path.UTF8String;
                ++index;
            }
            completion(paths);
        }
    }];
}
