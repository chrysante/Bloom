#include "StaticMesh.hpp"

namespace bloom {
	
	Reference<StaticMesh> StaticMesh::create() {
		return std::make_shared<StaticMesh>(PrivateTag{});
	}
	
}
