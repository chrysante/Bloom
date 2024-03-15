#ifndef BLOOM_GPU_BLITCONTEXT_H
#define BLOOM_GPU_BLITCONTEXT_H

namespace bloom {

class BlitContext {
public:
    virtual ~BlitContext() = default;

    virtual void commit() = 0;
};

} // namespace bloom

#endif // BLOOM_GPU_BLITCONTEXT_H
