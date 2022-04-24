#include "StaticRenderMesh.hpp"

namespace bloom {
	
	Reference<StaticRenderMesh> StaticRenderMesh::create() {
		return std::make_shared<StaticRenderMesh>(PrivateTag{});
	}
	
}
