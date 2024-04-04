#ifndef BLOOM_SHADER_POSTPROCESSING_H
#define BLOOM_SHADER_POSTPROCESSING_H

namespace bloom {

/// Different tone mapping functions
enum class ToneMappingFunction { ACES, reinhard };

/// Parameter structure for the post processing shader
struct PostProcessParameters {
    ToneMappingFunction toneMappingFunction;
};

} // namespace bloom

#endif // BLOOM_SHADER_POSTPROCESSING_H
