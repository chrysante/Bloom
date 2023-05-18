// #pragma once
//
// #include "Bloom/Core/Base.hpp"
// #include "Bloom/Application/CoreSystem.hpp"
//
// #include <memory>
// #include <concepts>
// #include <chaiscript/chaiscript.hpp>
//
// namespace bloom {
//
//	using ScriptObject = chaiscript::dispatch::Dynamic_Object;
//
//     class BLOOM_API ScriptEngine: public CoreSystem {
//     public:
//		ScriptEngine();
//
//		void registerFunction(auto&&, std::string);
//		template <typename>
//		void registerType(std::string);
//		template <typename Base, std::derived_from<Base> Derived>
//		void registerRelation();
//		template <typename>
//		void registerConstructor(std::string name);
//
//		template <typename T = void>
//		T eval(std::string const&);
//
//		std::shared_ptr<ScriptObject> instanciateObject(std::string_view
// className);
//
//		struct State {
//			friend class ScriptEngine;
//			State(void* = nullptr);
//			State(State const&);
//			State(State&&) = default;
//
//			State& operator=(State const&);
//			State& operator=(State&&) = default;
//		private:
//			struct Deleter { void operator()(void*) const; };
//			std::unique_ptr<void, Deleter> _state;
//		};
//
//		State getState();
//		void setState(State);
//
//		void rememberBaseState();
//		void restoreBaseState();
//
//	private:
//		void registerFunction(chaiscript::Proxy_Function&&,
// std::string&&); 		void registerType(chaiscript::Type_Info&&,
// std::string&&); 		void registerConversion(chaiscript::Type_Conversion&&);
//		chaiscript::Boxed_Value doEval(std::string const&);
//
//     private:
//		struct Deleter { void operator()(void*) const; };
//		std::unique_ptr<void, Deleter> _engine;
//		State _beginState;
//     };
//
// }
//
//
// void bloom::ScriptEngine::registerFunction(auto&& function, std::string name)
// { 	registerFunction(chaiscript::fun(function), std::move(name));
// }
//
// template <typename T>
// void bloom::ScriptEngine::registerType(std::string name) {
//	registerType(chaiscript::user_type<T>(), std::move(name));
// }
//
// template <typename Base, std::derived_from<Base> Derived>
// void bloom::ScriptEngine::registerRelation() {
//	registerConversion(chaiscript::base_class<Base, Derived>());
// }
//
// template <typename Ctor>
// void bloom::ScriptEngine::registerConstructor(std::string name) {
//	registerFunction(chaiscript::constructor<Ctor>(), std::move(name));
// }
//
// template <typename T>
// T bloom::ScriptEngine::eval(std::string const& text) {
//	return chaiscript::boxed_cast<T>(doEval(text));
// }
//
// template <>
// inline void bloom::ScriptEngine::eval<void>(std::string const& text) {
//	doEval(text);
// }
