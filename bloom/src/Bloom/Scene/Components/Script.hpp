//#pragma once
//
//#include "ComponentBase.hpp"
////#include "Bloom/ScriptEngine/ScriptEngine.hpp"
//#include <mtl/mtl.hpp>
//#include <optional>
//
//namespace bloom {
//	
//	struct BLOOM_API ScriptComponent {
//		BLOOM_REGISTER_COMPONENT("Script");
//		std::string className;
//		std::shared_ptr<ScriptObject> object;
//		std::optional<bool> hasUpdateFn;
//		std::optional<bool> hasRenderFn;
//	};
//
//	/// MARK: Helpers
//	struct BLOOM_API ScriptLoadEvent{};
//	
//	struct BLOOM_API ScriptHelpers {
//		
//		static bool isReserved(std::string_view name);
//		
//		using SupportedTypes = std::tuple<
//			float, mtl::float2, mtl::float3, mtl::float4,
//			double, mtl::double2, mtl::double3, mtl::double4,
//			int, mtl::int2, mtl::int3, mtl::int4
//		>;
//		
//		template <std::size_t I>
//		using TestType = std::tuple_element_t<I, SupportedTypes>;
//		
//		static constexpr std::size_t SupportedTypesCount = std::tuple_size_v<SupportedTypes>;
//		
//		static constexpr std::array SupportedTypesNames = {
//			"Float",
//			"Float2",
//			"Float3",
//			"Float4",
//			"Double",
//			"Double2",
//			"Double3",
//			"Double4",
//			"Int",
//			"Int2",
//			"Int3",
//			"Int4"
//		};
//		
//	};
//}
//
//#ifdef BLOOM_CPP
//
//#include "Bloom/Core/Serialize.hpp"
//
//template <>
//struct YAML::convert<bloom::ScriptComponent> {
//	static Node encode(bloom::ScriptComponent const&);
//	static bool decode(Node const& node, bloom::ScriptComponent&);
//};
//
//#endif
