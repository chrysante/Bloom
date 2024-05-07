#ifndef BLOOM_GPU_COMMANDQUEUE_H
#define BLOOM_GPU_COMMANDQUEUE_H

#include <memory>

#include "Bloom/GPU/Fwd.h"

namespace bloom {

///
class BLOOM_API CommandQueue {
public:
    virtual ~CommandQueue() = default;

    /// Creates a command buffer that will be submitted to this queue
    virtual std::unique_ptr<CommandBuffer> createCommandBuffer() = 0;

protected:
    CommandQueue() = default;
};

} // namespace bloom

#endif // BLOOM_GPU_COMMANDQUEUE_H
