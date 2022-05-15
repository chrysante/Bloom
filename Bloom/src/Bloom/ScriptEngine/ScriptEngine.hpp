#pragma once

#include "Bloom/Core/Base.hpp"

#include <memory>
#include <chaiscript/chaiscript.hpp>
#include <chaiscript/dispatchkit/register_function.hpp>
#include <chaiscript/dispatchkit/boxed_value.hpp>
#include <chaiscript/dispatchkit/function_call.hpp>

namespace bloom {

	using ScriptObject = chaiscript::dispatch::Dynamic_Object;
	
    class BLOOM_API ScriptEngine {
    public:
		ScriptEngine();
		
		void registerFunction(auto&&, std::string);
		template <typename>
		void registerType(std::string);
		
		template <typename T = void>
		T eval(std::string const&);

		std::shared_ptr<ScriptObject> instanciateObject(std::string_view className);
		
		struct State {
			friend class ScriptEngine;
			State(void*);
			State(State const&);
			State(State&&) = default;
		private:
			struct Deleter { void operator()(void*) const; };
			std::unique_ptr<void, Deleter> _state;
		};
		
		State getState();
		void setState(State);
		
		void restoreBeginState();
		
	private:
		void registerFunction(chaiscript::Proxy_Function&&, std::string&&);
		void registerType(chaiscript::Type_Info&&, std::string&&);
		chaiscript::Boxed_Value doEval(std::string const&);
		
    private:
		struct Deleter { void operator()(void*) const; };
		std::unique_ptr<void, Deleter> _engine;
		State _beginState;
    };

}


void bloom::ScriptEngine::registerFunction(auto&& function, std::string name) {
	registerFunction(chaiscript::fun(function), std::move(name));
}

template <typename T>
void bloom::ScriptEngine::registerType(std::string name) {
	registerType(chaiscript::user_type<T>(), std::move(name));
}

template <typename T>
T bloom::ScriptEngine::eval(std::string const& text) {
	return chaiscript::boxed_cast<T>(doEval(text));
}

template <>
inline void bloom::ScriptEngine::eval<void>(std::string const& text) {
	doEval(text);
}
