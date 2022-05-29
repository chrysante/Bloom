#include "ScriptEngine.hpp"

#include <utl/format.hpp>

#include <chaiscript/extras/math.hpp>

namespace bloom {
    
	static chaiscript::ChaiScript& asChai(void* ptr) {
		return *static_cast<chaiscript::ChaiScript*>(ptr);
	}

	static chaiscript::ChaiScript::State const& asState(void const* ptr) {
		return *static_cast<chaiscript::ChaiScript::State const*>(ptr);
	}
	
	static chaiscript::ChaiScript::State& asState(void* ptr) {
		return const_cast<chaiscript::ChaiScript::State&>(asState((void const*)ptr));
	}
	
	ScriptEngine::ScriptEngine():
		_engine(new chaiscript::ChaiScript())
	{
		auto mathlib = chaiscript::extras::math::bootstrap();
		asChai(_engine.get()).add(mathlib);
		
		_beginState = getState();
	}
	
	void ScriptEngine::Deleter::operator()(void* engine) const {
		delete &asChai(engine);
	}
	
	void ScriptEngine::registerFunction(chaiscript::Proxy_Function&& fn, std::string&& name) {
		asChai(_engine.get()).add(std::move(fn), std::move(name));
	}
	
	void ScriptEngine::registerType(chaiscript::Type_Info&& type, std::string&& name) {
		asChai(_engine.get()).add(std::move(type), std::move(name));
	}
	
	void ScriptEngine::registerConversion(chaiscript::Type_Conversion&& conv) {
		asChai(_engine.get()).add(std::move(conv));
	}
	
	chaiscript::Boxed_Value ScriptEngine::doEval(std::string const& text) {
		return asChai(_engine.get()).eval(text);
	}
	
	std::shared_ptr<ScriptObject> ScriptEngine::instanciateObject(std::string_view className) {
		return eval<std::shared_ptr<chaiscript::dispatch::Dynamic_Object>>(utl::format("{}()", className));
	}
	
	ScriptEngine::State ScriptEngine::getState() {
		return State(new chaiscript::ChaiScript::State(asChai(_engine.get()).get_state()));
	}
	
	void ScriptEngine::setState(State state) {
		asChai(_engine.get()).set_state(asState(state._state.get()));
	}
	
	void ScriptEngine::rememberBaseState() {
		_beginState = getState();
	}
	
	void ScriptEngine::restoreBaseState() {
		setState(_beginState);
	}
	
	ScriptEngine::State::State(void* state): _state(state) {
		
	}
	
	ScriptEngine::State::State(State const& rhs):
		_state(new chaiscript::ChaiScript::State(asState(rhs._state.get())))
	{
		
	}
	
	ScriptEngine::State& ScriptEngine::State::operator=(State const& rhs) {
		_state = std::unique_ptr<void, Deleter>(new chaiscript::ChaiScript::State(asState(rhs._state.get())));
		return *this;
	}
	
	void ScriptEngine::State::Deleter::operator()(void* state) const {
		if (state) {
			delete &asState(state);
		}
	}
	
}
