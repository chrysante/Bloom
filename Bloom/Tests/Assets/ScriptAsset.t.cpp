#include <Catch2/Catch2.hpp>

#include "Bloom/Assets/ScriptAsset.hpp"

static std::string const testScript = R"(class MyClass{
	var data;
	def MyClass()	{
		this.data = -1;
	}
	def update() {
		
	}
}

class classMyClass {
	var data;
	def classMyClass()	{
		this.data = -1;
	}
	def update() {
		
	}
}


class AnotherClass{
	var value;
	def AnotherClass() {
		this.value = -1;
	}
	def update() {
		print("Update...");
	}
}

)";



TEST_CASE("findClassNames") {

	auto const names = bloom::findClassNames(testScript);
	REQUIRE(names.size() == 3);
	for (int i = 0; i < 3; ++i) {
		CHECK(names[i] == std::array{ "MyClass", "classMyClass", "AnotherClass" }[i]);
	}
	
	
}
