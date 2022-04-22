#include "Bloom/Application/Resource.hpp"

#include "Bloom/Core/Base.hpp"
#include "Bloom/Core/Debug.hpp"

#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <Appkit/Appkit.h>
#import <Cocoa/Cocoa.h>

namespace bloom {
	
	static NSString* toNSString(char const* cstr) {
		return [NSString stringWithUTF8String: cstr];
	}
	
	static NSString* toNSString(std::string_view str) {
		return toNSString(str.data());
	}
	
	static std::string toSTDString(NSString* nsString) {
		char const* cstr = [nsString UTF8String];
		if (cstr) {
			return cstr;
		}
		return {};
	}

	BLOOM_API void* loadTextureFromFile(void* _device,
							  std::filesystem::path filename)
	{
		NSURL *imgURL = [[NSURL alloc] initFileURLWithPath: toNSString(pathForResource(filename).string())];
		
		id<MTLDevice> device = (__bridge id<MTLDevice>)_device;
		
		MTKTextureLoader* textureLoader = [[MTKTextureLoader alloc] initWithDevice:device];
		id<MTLTexture> texture = [textureLoader newTextureWithContentsOfURL:imgURL options:@{MTKTextureLoaderOptionSRGB: @true} error:NULL];
		
		void* result = (void*)CFBridgingRetain(texture);
		if (!result) {
			bloomLog(error, "Failed to load Texture {}", filename);
		}
		return result;
	}
	
	BLOOM_API std::filesystem::path currentDirectoryPath() {
		return toSTDString(NSFileManager.defaultManager.currentDirectoryPath);
	}
	
	BLOOM_API std::filesystem::path executablePath() {
		const char* result =
			[[[NSBundle mainBundle] executablePath] fileSystemRepresentation];
		return result;
	}
	
	
	
	BLOOM_API std::filesystem::path pathForResource(std::filesystem::path file) {
		auto const ext = toNSString(file.extension().string());
		auto const resource = toNSString(file.replace_extension().string());
		
		NSString* result = [[NSBundle mainBundle] pathForResource: resource
														   ofType: ext];
		
		return toSTDString(result);
	}
	
	BLOOM_API std::filesystem::path resourceDir() {
		return toSTDString([NSBundle mainBundle].resourceURL.path);
	}
	
	BLOOM_API std::filesystem::path getLibraryDir() {
		NSError* error;
		NSURL *fileURL = [[NSFileManager defaultManager] URLForDirectory:NSLibraryDirectory
																inDomain:NSUserDomainMask
													   appropriateForURL:nil
																  create:true
																   error:&error];
		return std::filesystem::path(fileURL.path.UTF8String);
	}
	
	BLOOM_API void showSaveFilePanel(utl::function<void(std::string)> completion) {
		NSSavePanel* panel = [NSSavePanel savePanel];
		
		[panel beginWithCompletionHandler:^(NSInteger result){
			if (result == NSModalResponseOK) {
				NSURL*  url = [panel URL];
				completion(toSTDString(url.path));
			}
		}];
	}
	
	BLOOM_API void showOpenFilePanel(utl::function<void(std::string)> completion) {
		NSOpenPanel* panel = [NSOpenPanel openPanel];
		[panel setCanChooseFiles:YES];
		[panel setCanChooseDirectories:NO];
		[panel setAllowsMultipleSelection:NO];
		
		[panel beginWithCompletionHandler:^(NSInteger result){
			if (result == NSModalResponseOK) {
				NSURL*  url = [panel URL];
				completion(toSTDString(url.path));
			}
		}];
	}
	
	BLOOM_API void showSelectDirectoryPanel(utl::function<void(std::string)> completion) {
		NSOpenPanel* panel = [NSOpenPanel openPanel];
		[panel setCanChooseFiles:NO];
		[panel setCanChooseDirectories:YES];
		[panel setCanCreateDirectories:YES];
		
		[panel setAllowsMultipleSelection:NO];
		
		[panel beginWithCompletionHandler:^(NSInteger result){
			if (result == NSModalResponseOK) {
				NSURL*  url = [panel URL];
				completion(toSTDString(url.path));
			}
		}];
	}
	
}
