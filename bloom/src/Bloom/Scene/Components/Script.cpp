//#include "Script.hpp"
//
//#include "Bloom/Application/Application.hpp"
//
//using namespace bloom;
//
//namespace {
//	template <typename T, std::size_t Index>
//	bool tryEncodeAs(YAML::Node& node, chaiscript::Boxed_Value const& v) {
//		if (v.get().type() == typeid(std::shared_ptr<T>)) {
//			node["Type"] = ScriptHelpers::SupportedTypesNames[Index];
//			node["Value"] = chaiscript::boxed_cast<T>(v);
//			return true;
//		}
//		return false;
//	}
//	
//	template <typename T, std::size_t Index>
//	bool tryDecodeAs(YAML::Node const& node, chaiscript::Boxed_Value& v) {
//		if (node["Type"].as<std::string>() == ScriptHelpers::SupportedTypesNames[Index]) {
//			v = chaiscript::Boxed_Value(node["Value"].as<T>());
//			return true;
//		}
//		return false;
//	}
//	
//}
//
//namespace bloom {
//	
//	bool ScriptHelpers::isReserved(std::string_view name) {
//		return name.size() >= 2 && name[0] == '_' && name[1] == '_';
//	}
//	
//}
//
//template <>
//struct YAML::convert<chaiscript::Boxed_Value> {
//	static Node encode(chaiscript::Boxed_Value const& v) {
//		Node node;
//		UTL_WITH_INDEX_SEQUENCE((I, ScriptHelpers::SupportedTypesCount), {
//			(... || tryEncodeAs<ScriptHelpers::TestType<I>, I>(node, v));
//		});
//		return node;
//	}
//	
//	static bool decode(Node const& node, chaiscript::Boxed_Value& v) {
//		try {
//			return UTL_WITH_INDEX_SEQUENCE((I, ScriptHelpers::SupportedTypesCount), {
//				return (... || tryDecodeAs<ScriptHelpers::TestType<I>, I>(node, v));
//			});
//		}
//		catch(std::exception const& e) {
//			bloomDebugbreak("Whats going on here?");
//			return false;
//		}
//	}
//};
//
//YAML::Node YAML::convert<bloom::ScriptComponent>::encode(bloom::ScriptComponent const& s) {
//	Node root;
//	root["ClassName"] = s.className;
//	if (!s.object) {
//		return root;
//	}
//	YAML::Node attribs;
//	for (auto&& [name, value]: s.object->get_attrs()) {
//		if (ScriptHelpers::isReserved(name)) {
//			continue;
//		}
//		attribs[name] = value;
//	}
//	
//	root["Attributes"] = attribs;
//	return root;
//}
//
//bool YAML::convert<bloom::ScriptComponent>::decode(Node const& root, bloom::ScriptComponent& s) {
//	s.className = root["ClassName"].as<std::string>();
//	try {
//#warning
//		throw std::runtime_error("Failed to decode script component");
////		s.object = bloom::Application::get().scriptEngine().instanciateObject(s.className);
////		YAML::Node attribs = root["Attributes"];
////		for (auto&& [name, value]: s.object->get_attrs()) {
////			if (attribs[name].IsDefined()) {
////				auto rhs = attribs[name].as<chaiscript::Boxed_Value>();
////				s.object->get_attr(name).swap(rhs);
////			}
////		}
//	}
//	catch (std::exception const& e) {
//		bloomLog(debug, "{}", e.what());
//	}
//	return true;
//}
//
//
