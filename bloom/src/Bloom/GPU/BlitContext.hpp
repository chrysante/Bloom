#pragma once

namespace bloom {
	
	class BlitContext {
	public:
		virtual ~BlitContext() = default;
		
		virtual void commit() = 0;
	};
	
}
