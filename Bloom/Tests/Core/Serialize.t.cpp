#include <Catch2/Catch2.hpp>

#include "Bloom/Core/Serialize.hpp"

namespace {
	
	struct TestStruct {
		int value;
		float f;
		long i;
		
		friend bool operator==(TestStruct const&, TestStruct const&) = default;
	};
	
}

BLOOM_MAKE_TEXT_SERIALIZER(TestStruct, value, f, i);

TEST_CASE("Text Serializer") {
	TestStruct const out{ -1, 2, 3 };
	
	std::string text;
	
	{
		YAML::Node node;
		node["test"] = out;
		
		YAML::Emitter e;
		e << node;
		text = e.c_str();
	}
	{
		YAML::Node node = YAML::Load(text);
		auto const in = node["test"].as<TestStruct>();
		
		CHECK(in == out);
	}
}
