#include "ScriptSystem.hpp"
#include "SceneSystem.hpp"

#include "Bloom/Application/Application.hpp"

static char const* const baseScript = R"(
// this is a fallback so we don't fail when getting the methods if no class implements them
class __BloomFallbackClass {
	def __BloomFallbackClass() {}
	def init() {}
	def update(TimeStep) {}
	def render() {}
}

)";

namespace {

	using namespace bloom;
	using namespace chaiscript;
	
	TransformComponent& getTransform(ScriptObject& obj) {
		auto handle = boxed_cast<EntityHandle>(obj.get_attr("__bloomEntity"));
		return handle.get<TransformComponent>();
	}
	
	template <typename FloatN, std::size_t Index>
	decltype(auto) getVectorElement(FloatN x) { return x[Index]; }
	
	template <typename T, auto>
	using expand = T;
	
}

namespace bloom {
	
	template <std::size_t N>
	static void registerVector(ScriptEngine& engine) {
		UTL_WITH_INDEX_SEQUENCE((I, N), {
			using FloatN = mtl::vector<float, N>;
			std::string const typeName = utl::format("float{}", N);
			
			engine.registerType<FloatN>(typeName);
			
			engine.registerConstructor<FloatN()>(typeName);
			engine.registerConstructor<FloatN(float)>(typeName);
			engine.registerConstructor<FloatN(expand<float, I>...)>(typeName);
			
			([&]{
				engine.registerFunction(&getVectorElement<FloatN&, I>, utl::format("{}", std::string_view("xyzw")[I]));
				engine.registerFunction(&getVectorElement<FloatN const&, I>, utl::format("{}", std::string_view("xyzw")[I]));
			}(), ...);
		});
	}
	
	static void registerCommonTypes(ScriptEngine& engine) {
		registerVector<2>(engine);
		registerVector<3>(engine);
		registerVector<4>(engine);
		
		engine.registerType<mtl::quaternion_float>("quaternion");
	}
	
	static void registerEnvironment(ScriptEngine& engine) {
		registerCommonTypes(engine);
		
		engine.registerType<TimeStep>("TimeStep");
		engine.registerFunction(&TimeStep::delta, "delta");
		engine.registerFunction(&TimeStep::absolute, "absolute");
		
		engine.registerType<TransformComponent>("Transform");
		
		engine.registerFunction([](TransformComponent& t) -> auto& { return t.position;    }, "position");
		engine.registerFunction([](TransformComponent& t) -> auto& { return t.orientation; }, "orientation");
		engine.registerFunction([](TransformComponent& t) -> auto& { return t.scale;       }, "scale");
		
		engine.registerFunction(&getTransform, "transform");
		
		
		engine.eval(baseScript);
		engine.rememberBaseState();
	}
	
	ScriptSystem::ScriptSystem(SceneSystem* s): sceneSystem(s) {
		
	}
	
	void ScriptSystem::init() {
		auto& engine = Application::get().scriptEngine();
		
		registerEnvironment(engine);
		
		Application::get().addStaticEventListener([this](ScriptLoadEvent){
			this->onScriptReload();
		});
	}
	
	static bool executeGuarded(auto&& fn,
							   std::string_view className,
							   std::string_view fnName)
	{
		try {
			fn();
			return true;
		}
		catch (chaiscript::exception::eval_error const& e) {
			bloomLog(error, "Failed to evaluate {}::{}():\n{}\nReason: {}\nDetail: {}\n{}\n",
					 className, fnName,
					 e.what(),
					 e.reason,
					 e.detail,
					 e.pretty_print());
		}
		catch (chaiscript::exception::dispatch_error const& e) {
			bloomLog(trace, "class {} has no '{}' method: {}",
					 className, fnName,
					 e.what());
		}
		catch (std::exception const& e) {
			bloomLog(error, "Failed to evaluate {}::{}():\n{}",
					 className, fnName,
					 e.what());
		}
		return false;
	}
	
	static void copyFields(auto& object, auto& oldFields) /* mutable lvalue because we swap */ {
		for (auto&& [name, value]: object.get_attrs()) {
			if (!oldFields.contains(name)) {
				continue;
			}
			auto& oldValue = oldFields[name];
			if (oldValue.get().type() != value.get().type()) {
				continue;
			}
			object.get_attr(name).swap(oldValue);
		}
	}
	
	void ScriptSystem::onSceneConstruction() {
		auto& engine = Application::get().scriptEngine();
		initFn  = engine.eval<std::function<void(ScriptObject&)>>("init");
		updateFn  = engine.eval<std::function<void(ScriptObject&, TimeStep)>>("update");
		renderFn  = engine.eval<std::function<void(ScriptObject&)>>("render");
		
		forEach([&](EntityID id, ScriptComponent& script) {
			executeGuarded([&]{
				script.hasUpdateFn = std::nullopt;
				script.hasRenderFn = std::nullopt;
				
				std::map<std::string, chaiscript::Boxed_Value> oldFields = script.object ?
					script.object->get_attrs() : decltype(script.object->get_attrs()){};
				
				script.object = engine.instanciateObject(script.className);
				copyFields(*script.object, oldFields);
				script.object->get_attr("__bloomEntity") = chaiscript::Boxed_Value(sceneSystem->_scene->getHandle(id));
			}, script.className, script.className);
		});
	}
	
	void ScriptSystem::onSceneInit() {
		forEach([&](ScriptComponent& script) {
			executeGuarded([&]{
				initFn(*script.object);
			}, script.className, "init");
		});
	}
	
	void ScriptSystem::onSceneUpdate(TimeStep t) {
		forEach([&](ScriptComponent& script) {
			if UTL_LIKELY (script.hasUpdateFn) {
				if (*script.hasUpdateFn) {
					updateFn(*script.object, t);
				}
			}
			else {
				script.hasUpdateFn = executeGuarded([&]{
					updateFn(*script.object, t);
				}, script.className, "update");
			}
		});
	}
	
	void ScriptSystem::onSceneRender() {
		forEach([&](ScriptComponent& script) {
			if UTL_LIKELY (script.hasRenderFn) {
				if (*script.hasRenderFn) {
					renderFn(*script.object);
				}
			}
			else {
				script.hasRenderFn = executeGuarded([&]{
					renderFn(*script.object);
				}, script.className, "render");
			}
		});
	}
	
	void ScriptSystem::onScriptReload() {
		if (sceneSystem && sceneSystem->_scene) {
			onSceneConstruction();
		}
	}
	
	void ScriptSystem::forEach(auto&& fn) {
		bloomExpect(sceneSystem && sceneSystem->_scene);
		
		sceneSystem->_scene->view<ScriptComponent>().each([&](auto id, ScriptComponent& script) {
			if constexpr (std::invocable<decltype(fn), ScriptComponent&>) {
				fn(script);
			}
			else if constexpr (std::invocable<decltype(fn), EntityID, ScriptComponent&>) {
				fn(EntityID{ id }, script);
			}
			else {
				static_assert(utl::template_false<decltype(fn)>);
			}
		});
	}
	

}
