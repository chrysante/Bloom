//#include "ScriptSystem.hpp"
//
//#include <utl/strcat.hpp>
//
//#include "Bloom/Application/Application.hpp"
//#include "Bloom/Scene/Scene.hpp"
//#include "Bloom/Scene/Entity.hpp"
//#include "Bloom/Scene/Components/Transform.hpp"
//#include "Bloom/Scene/Components/Script.hpp"
//
//static char const* const baseScript = R"(
//// this is a fallback so we don't fail when getting the methods if no class implements them
//class __BloomFallbackClass {
//	def __BloomFallbackClass() {}
//	def init() {}
//	def update(TimeStep) {}
//	def render() {}
//}
//
//)";
//
//namespace {
//
//	using namespace bloom;
//	using namespace chaiscript;
//	
//	Transform& getTransform(ScriptObject& obj) {
//		auto handle = boxed_cast<EntityHandle>(obj.get_attr("__bloomEntity"));
//		return handle.get<Transform>();
//	}
//	
//	template <typename FloatN, std::size_t Index>
//	decltype(auto) getVectorElement(FloatN x) { return x[Index]; }
//	
//	template <typename T, auto>
//	using expand = T;
//	
//}
//
//namespace bloom {
//	
//	template <std::size_t N>
//	static void registerVector(ScriptEngine& engine) {
//		UTL_WITH_INDEX_SEQUENCE((I, N), {
//			using FloatN = mtl::vector<float, N>;
//			std::string const typeName = utl::strcat("float", N);
//			
//			engine.registerType<FloatN>(typeName);
//			
//			engine.registerConstructor<FloatN()>(typeName);
//			engine.registerConstructor<FloatN(float)>(typeName);
//			engine.registerConstructor<FloatN(expand<float, I>...)>(typeName);
//			
//			([&]{
//				engine.registerFunction(&getVectorElement<FloatN&, I>, std::string("xyzw"[I]));
//				engine.registerFunction(&getVectorElement<FloatN const&, I>, std::string("xyzw"[I]));
//			}(), ...);
//		});
//	}
//	
//	static void registerCommonTypes(ScriptEngine& engine) {
//		registerVector<2>(engine);
//		registerVector<3>(engine);
//		registerVector<4>(engine);
//		
//		engine.registerType<mtl::quaternion_float>("quaternion");
//	}
//	
//	static void registerEnvironment(ScriptEngine& engine) {
//		registerCommonTypes(engine);
//		
//		engine.registerType<Timestep>("TimeStep");
//		engine.registerFunction(&Timestep::delta, "delta");
//		engine.registerFunction(&Timestep::absolute, "absolute");
//		
//		engine.registerType<Transform>("Transform");
//		
//		engine.registerFunction([](Transform& t) -> auto& { return t.position;    }, "position");
//		engine.registerFunction([](Transform& t) -> auto& { return t.orientation; }, "orientation");
//		engine.registerFunction([](Transform& t) -> auto& { return t.scale;       }, "scale");
//		
//		engine.registerFunction(&getTransform, "transform");
//		
//		
//		engine.eval(baseScript);
//		engine.rememberBaseState();
//	}
//	
//	ScriptSystem::ScriptSystem(ScriptEngine& engine):
//		mEngine(&engine)
//	{
//		
//	}
//	
//	void ScriptSystem::init() {
//		registerEnvironment(*mEngine);
//		
//#warning
////		Application::get().addStaticEventListener([this](ScriptLoadEvent){
////			this->onScriptReload();
////		});
//	}
//	
//	static bool executeGuarded(auto&& fn,
//							   std::string_view className,
//							   std::string_view fnName)
//	{
//		try {
//			fn();
//			return true;
//		}
//		catch (chaiscript::exception::eval_error const& e) {
//			bloomLog(error, "Failed to evaluate {}::{}():\n{}\nReason: {}\nDetail: {}\n{}\n",
//					 className, fnName,
//					 e.what(),
//					 e.reason,
//					 e.detail,
//					 e.pretty_print());
//		}
//		catch (chaiscript::exception::dispatch_error const& e) {
//			bloomLog(trace, "class {} has no '{}' method: {}",
//					 className, fnName,
//					 e.what());
//		}
//		catch (std::exception const& e) {
//			bloomLog(error, "Failed to evaluate {}::{}():\n{}",
//					 className, fnName,
//					 e.what());
//		}
//		return false;
//	}
//	
//	static void copyFields(auto& object, auto& oldFields) /* mutable lvalue because we swap */ {
//		for (auto&& [name, value]: object.get_attrs()) {
//			if (!oldFields.contains(name)) {
//				continue;
//			}
//			auto& oldValue = oldFields[name];
//			if (oldValue.get().type() != value.get().type()) {
//				continue;
//			}
//			object.get_attr(name).swap(oldValue);
//		}
//	}
//	
//	void ScriptSystem::onSceneConstruction() {
//		auto& engine = *mEngine;
//		initFn  = engine.eval<std::function<void(ScriptObject&)>>("init");
//		updateFn  = engine.eval<std::function<void(ScriptObject&, Timestep)>>("update");
//		renderFn  = engine.eval<std::function<void(ScriptObject&)>>("render");
//		
//		forEach([&](EntityID id, ScriptComponent& script) {
//			executeGuarded([&]{
//				script.hasUpdateFn = std::nullopt;
//				script.hasRenderFn = std::nullopt;
//				
//				std::map<std::string, chaiscript::Boxed_Value> oldFields = script.object ?
//					script.object->get_attrs() : decltype(script.object->get_attrs()){};
//				
//				script.object = engine.instanciateObject(script.className);
//				copyFields(*script.object, oldFields);
//#warning
////				script.object->get_attr("__bloomEntity") = chaiscript::Boxed_Value(sceneSystem->_scene->getHandle(id));
//			}, script.className, script.className);
//		});
//	}
//	
//	void ScriptSystem::onSceneInit() {
//		forEach([&](ScriptComponent& script) {
//			executeGuarded([&]{
//				initFn(*script.object);
//			}, script.className, "init");
//		});
//	}
//	
//	void ScriptSystem::onSceneUpdate(Timestep t) {
//		forEach([&](ScriptComponent& script) {
//			if UTL_LIKELY (script.hasUpdateFn) {
//				if (*script.hasUpdateFn) {
//					updateFn(*script.object, t);
//				}
//			}
//			else {
//				script.hasUpdateFn = executeGuarded([&]{
//					updateFn(*script.object, t);
//				}, script.className, "update");
//			}
//		});
//	}
//	
//	void ScriptSystem::onSceneRender() {
//		forEach([&](ScriptComponent& script) {
//			if UTL_LIKELY (script.hasRenderFn) {
//				if (*script.hasRenderFn) {
//					renderFn(*script.object);
//				}
//			}
//			else {
//				script.hasRenderFn = executeGuarded([&]{
//					renderFn(*script.object);
//				}, script.className, "render");
//			}
//		});
//	}
//	
//	void ScriptSystem::onScriptReload() {
////		if (sceneSystem && sceneSystem->_scene) {
////			onSceneConstruction();
////		}
//	}
//	
//	void ScriptSystem::forEach(auto&& fn) {
////		bloomExpect(sceneSystem && sceneSystem->_scene);
////
////		sceneSystem->_scene->view<ScriptComponent>().each([&](auto id, ScriptComponent& script) {
////			if constexpr (std::invocable<decltype(fn), ScriptComponent&>) {
////				fn(script);
////			}
////			else if constexpr (std::invocable<decltype(fn), EntityID, ScriptComponent&>) {
////				fn(EntityID{ id }, script);
////			}
////			else {
////				static_assert(utl::template_false<decltype(fn)>);
////			}
////		});
//	}
//	
//
//}
