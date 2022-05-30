#include "Renderer.hpp"

#include "RendererSanitizer.hpp"
#include "BloomRenderer.hpp"

#include "Bloom/Graphics/Renderer/ShaderParameters.hpp"
#include "Bloom/Core/Core.hpp"
#include "Bloom/GPU/HardwarePrimitives.hpp"


#include <mtl/mtl.hpp>
#include <utl/structure_of_arrays.hpp>

namespace bloom {

	struct BLOOM_API ForwardRendererFramebuffer: Framebuffer {
		friend class ForwardRenderer;
		
		TextureHandle depth;
		TextureHandle rawColor;
		TextureHandle postProcessed;
		
		BloomFramebuffer bloom;
	};
	
	struct BLOOM_API ForwardRendererDebugFramebuffer: ForwardRendererFramebuffer {
		friend class ForwardRenderer;
		TextureHandle shadowCascade;
	};
	
	namespace {
		UTL_SOA_TYPE(SceneRenderObject,
					 (mtl::float4x4, transform),
					 (Reference<MaterialInstance>, materialInstance),
					 (Reference<StaticMeshRenderer>, mesh));
		
		struct FWCPUSceneData {
			utl::structure_of_arrays<SceneRenderObject> objects;
			utl::vector<PointLight> pointLights;
			utl::vector<SpotLight> spotLights;
			utl::vector<DirectionalLight> dirLights;
			utl::vector<SkyLight> skyLights;
			
			Camera camera;
			
			struct ShadowData {
				utl::small_vector<int> numCascades;
				utl::vector<mtl::float4x4> lightSpaceTransforms;
				
				int numShadowCasters;
				int shadowMapArrayLength = 0;
				mtl::uint2 shadowMapResolution = 512;
				bool needsNewShadowMaps = true;
			} shadows;
			
			void clear() {
				objects.clear();
				pointLights.clear();
				spotLights.clear();
				dirLights.clear();
				skyLights.clear();
				shadows.numShadowCasters = 0;
				shadows.numCascades.clear();
				shadows.lightSpaceTransforms.clear();
			}
		};
		
		struct FWRenderData {
			BufferHandle transformBuffer;
			BufferHandle parameterBuffer;
			
			DepthStencilHandle depthStencil;
			ComputePipelineHandle postprocessPipeline;
			SamplerHandle postprocessSampler;
			
			struct ShadowData {
				RenderPipelineHandle pipeline;
				SamplerHandle sampler;
				BufferHandle lightSpaceTransforms;
				TextureHandle shadowMaps;
			} shadows;
		};
		
		struct FWDebugRenderData {
			
		};
	}
	
	class BLOOM_API ForwardRenderer: public Renderer, private RendererSanitizer {
	public:
		ForwardRenderer(bloom::Reciever);
		
		/// MARK: Framebuffer Creation
		std::unique_ptr<Framebuffer> createFramebuffer(mtl::int2 size) const override;
		std::unique_ptr<Framebuffer> createDebugFramebuffer(mtl::int2 size) const override;
		
		void populateFramebuffer(HardwareDevice& device,
								 ForwardRendererFramebuffer&,
								 mtl::usize2 size) const;
		
		/// MARK: Settings
		
		
		/// MARK: Initialization
		void init(HardwareDevice&) override;
		
		void createGPUState(bloom::HardwareDevice&);
		
		/// MARK: Scene Construction
		void beginScene(Camera const&) override;
		void endScene() override;
		
		void submit(Reference<StaticMeshRenderer>, Reference<MaterialInstance>, mtl::float4x4 const& transform) override;
		void submit(PointLight const&) override;
		void submit(SpotLight const&) override;
		void submit(DirectionalLight const&) override;
		void submit(SkyLight const&) override;
		
		/// MARK: Draw
		void draw(Framebuffer&, CommandQueue&) override;
		
		RendererParameters makeParameters(mtl::usize2 framebufferSize);
		
//	private: // this class is private anyways
		void mainPass(ForwardRendererFramebuffer&, CommandQueue&) const;
		void shadowMapPass(CommandQueue&);
		void postprocessPass(ForwardRendererFramebuffer&, CommandQueue&) const;
		
//	private:
		BloomRenderer bloomRenderer;
		FWCPUSceneData scene;
		FWRenderData renderObjects;
		
		ToneMapping mToneMapping = ToneMapping::ACES;
	};
	
}

