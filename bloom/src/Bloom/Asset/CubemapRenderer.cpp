#include "Bloom/Asset/CubemapRenderer.h"

#include <vml/vml.hpp>

#include "Bloom/Core/Debug.h"
#include "Bloom/GPU/CommandBuffer.h"
#include "Bloom/GPU/HardwareDevice.h"
#include "Bloom/GPU/HardwarePrimitives.h"
#include "Bloom/GPU/ShaderCompiler.h"

using namespace bloom;
using namespace vml::short_types;

static void renderCube(CommandBuffer& commandBuffer, HardwareDevice& device,
                       RenderPipelineHandle& pipeline, TextureView renderTarget,
                       TextureView source) {
    float4x4 projection = vml::perspective(vml::constants<>::pi / 2,
                                           /* aspectRatio: */ 1.0f,
                                           /* near: */ 0.1f,
                                           /* far: */ 10.0f);
    std::array<float4x4, 6> viewMatrices = {
        // +X
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(1.0f, 0.0f, 0.0f),
                     float3(0.0f, -1.0f, 0.0f)),
        // -X
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(-1.0f, 0.0f, 0.0f),
                     float3(0.0f, -1.0f, 0.0f)),
        // -Y
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(0.0f, -1.0f, 0.0f),
                     float3(0.0f, 0.0f, -1.0f)),
        // +Y
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f),
                     float3(0.0f, 0.0f, 1.0f)),
        // +Z
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 1.0f),
                     float3(0.0f, -1.0f, 0.0f)),
        // -Z
        vml::look_at(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, -1.0f),
                     float3(0.0f, -1.0f, 0.0f)),
    };
    for (auto& m: viewMatrices) {
        m = vml::transpose(projection * m);
    }
    auto cameraBuffer =
        device.createBuffer({ .data = viewMatrices.data(),
                              .size = sizeof viewMatrices,
                              .storageMode = StorageMode::Managed });
    for (size_t slice = 0; slice < 6; ++slice) {
        TextureHandle side =
            createSharedTextureView(RenderAPI::Metal, renderTarget,
                                    TextureType::Texture2D,
                                    renderTarget.description().pixelFormat,
                                    /* firstMipLevel: */ 0,
                                    /* numMipLevels: */ 1,
                                    /* firstSlice: */ slice,
                                    /* numSlices: */ 1);
        auto encoder = commandBuffer.makeRenderEncoder({
            .colorAttachments = { {
                .texture = side,
            } },
            .renderTargetSize = renderTarget.size().xy,
        });
        encoder->setPipeline(pipeline);
        encoder->setVertexBuffer(cameraBuffer, 0, sizeof(float4x4) * slice);
        encoder->setFragmentTexture(source, 0);
        encoder->setTriangleCullMode(TriangleCullMode::None);
        encoder->draw(PrimitiveType::TriangleStrip, 4 * slice, 4);
        encoder->end();
    }
}

void bloom::renderCubemap(HardwareDevice& device, CommandBuffer& commandBuffer,
                          TextureHandle cubemap,
                          TextureHandle equirectangular) {
    BL_ASSERT(cubemap.description().type == TextureType::CubeTexture);
    auto shaderCompiler = device.createShaderCompiler();
    auto functions =
        shaderCompiler->compile(std::filesystem::path(
                                    "Shader/CubemapRenderer.metal"),
                                std::vector<std::string>{
                                    "cubeRendererVS", "cubemapRendererFS" });
    if (functions.empty()) {
        return;
    }
    auto pipeline = device.createRenderPipeline({
        .colorAttachments = { {
            .pixelFormat = cubemap.description().pixelFormat,
        } },
        .vertexFunction = functions[0],
        .fragmentFunction = functions[1],
    });
    renderCube(commandBuffer, device, pipeline, cubemap, equirectangular);
}

void bloom::renderIrradianceMap(HardwareDevice& device,
                                CommandBuffer& commandBuffer,
                                TextureHandle dest, TextureHandle source) {
    BL_ASSERT(source.description().type == TextureType::CubeTexture);
    BL_ASSERT(dest.description().type == TextureType::CubeTexture);
    auto shaderCompiler = device.createShaderCompiler();
    auto functions =
        shaderCompiler->compile(std::filesystem::path(
                                    "Shader/CubemapRenderer.metal"),
                                std::vector<std::string>{
                                    "cubeRendererVS", "irradianceRendererFS" });
    if (functions.empty()) {
        return;
    }
    auto pipeline = device.createRenderPipeline({
        .colorAttachments = { {
            .pixelFormat = dest.description().pixelFormat,
        } },
        .vertexFunction = functions[0],
        .fragmentFunction = functions[1],
    });
    renderCube(commandBuffer, device, pipeline, dest, source);
}

static float3 mapUVSToN(float u, float v, size_t face) {
    switch (face) {
    case 0: // +X
        return vml::normalize(float3(1, -v, -u));
    case 1: // -X
        return vml::normalize(float3(-1, -v, u));
    case 2: // +Y
        return vml::normalize(float3(u, 1, v));
    case 3: // -Y
        return vml::normalize(float3(u, -1, -v));
    case 4: // +Z
        return vml::normalize(float3(u, -v, 1));
    case 5: // -Z
        return vml::normalize(float3(-u, -v, -1));
    default:
        BL_UNREACHABLE();
    }
}

static std::array<float, 9> genSHCoefficients(float3 N) {
    return { // Band 0
             0.282095f,
             // Band 1
             0.488603f * N.y, 0.488603f * N.z, 0.488603f * N.x,
             // Band 2
             1.092548f * N.x * N.y, 1.092548f * N.y * N.z,
             0.315392f * (3.0f * N.z * N.z - 1.0f), 1.092548f * N.x * N.z,
             0.546274f * (N.x * N.x - N.y * N.y)
    };
}

/// Auto windowing function copied from here:
/// https://github.com/google/filament/blob/main/libs/ibl/src/CubemapSH.cpp

static constexpr double F_PI = 3.14159265358979323846264338327950288;
static constexpr double F_SQRT1_2 = 0.707106781186547524400844362104849039;

using float5 = vml::vector<float, 5>;
using float5x5 = vml::matrix<float, 5, 5>;

static size_t SHindex(ssize_t m, size_t l) { return l * (l + 1) + m; }

/*
 * utilities to rotate very low order spherical harmonics (up to 3rd band)
 */
static float3 rotateShericalHarmonicBand1(float3 band1, float3x3 const& M) {
    float3x3 invA1TimesK(vml::columns, { 0, -1, 0 }, { 0, 0, 1 }, { -1, 0, 0 });
    float3 MN0 = M[0]; // M * N0;
    float3 MN1 = M[1]; // M * N1;
    float3 MN2 = M[2]; // M * N2;
    float3x3 R1OverK(vml::columns, { -MN0.y, MN0.z, -MN0.x },
                     { -MN1.y, MN1.z, -MN1.x }, { -MN2.y, MN2.z, -MN2.x });
    return R1OverK * (invA1TimesK * band1);
}

static float5 rotateShericalHarmonicBand2(float5 const& band2,
                                          float3x3 const& M) {
    constexpr float sqrt3 = 1.7320508076f;
    constexpr float n = F_SQRT1_2;
    constexpr float5x5 invATimesK{ vml::columns,        { 0, 1, 2, 0, 0 },
                                   { -1, 0, 0, 0, -2 }, { 0, sqrt3, 0, 0, 0 },
                                   { 1, 1, 0, -2, 0 },  { 2, 1, 0, 0, 0 } };
    // This projects a float3 to SH2/k space (i.e. we premultiply by 1/k)
    auto project = [](float3 s) -> float5 {
        return { (s.y * s.x), -(s.y * s.z),
                 1 / (2 * sqrt3) * ((3 * s.z * s.z - 1)), -(s.z * s.x),
                 0.5f * ((s.x * s.x - s.y * s.y)) };
    };
    // this is: invA * k * band2
    // 5x5 matrix by vec5 (this a lot of zeroes and constants, which the
    // compiler should eliminate)
    float5 const invATimesKTimesBand2 = invATimesK * band2;
    // this is: mat5{project(N0), project(N1), project(N2), project(N3),
    // project(N4)} / k (the 1/k comes from project(), see above)
    float5x5 const ROverK = {
        vml::columns,
        project(M[0]),              // M * N0
        project(M[2]),              // M * N1
        project(n * (M[0] + M[1])), // M * N2
        project(n * (M[0] + M[2])), // M * N3
        project(n * (M[1] + M[2]))  // M * N4
    };
    // notice how "k" disappears
    // this is: (R / k) * (invA * k) * band2 == R * invA * band2
    return ROverK * invATimesKTimesBand2;
}

/*
 * SH from environment with high dynamic range (or high frequencies -- high
 * dynamic range creates high frequencies) exhibit "ringing" and negative values
 * when reconstructed. To mitigate this, we need to low-pass the input image --
 * or equivalently window the SH by coefficient that tapper towards zero with
 * the band.
 *
 * We use ideas and techniques from
 *    Stupid Spherical Harmonics (SH)
 *    Deringing Spherical Harmonics
 * by Peter-Pike Sloan
 * https://www.ppsloan.org/publications/shdering.pdf
 *
 */
static float sincWindow(size_t l, float w) {
    if (l == 0) {
        return 1.0f;
    }
    else if (l >= w) {
        return 0.0f;
    }
    // we use a sinc window scaled to the desired window size in bands units
    // a sinc window only has zonal harmonics
    float x = (float(F_PI) * l) / w;
    x = std::sin(x) / x;
    // The convolution of a SH function f and a ZH function h is just the
    // product of both scaled by 1 / K(0,l) -- the window coefficients include
    // this scale factor. Taking the window to power N is equivalent to applying
    // the filter N times
    return std::pow(x, 4);
}

static void windowSH(std::span<float3> sh, size_t numBands) {
    using SH3 = std::array<float, 9>;
    auto rotateSh3Bands = [](SH3 const& sh, float3x3 M) -> SH3 {
        float b0 = sh[0];
        float3 band1{ sh[1], sh[2], sh[3] };
        float3 b1 = rotateShericalHarmonicBand1(band1, M);
        float5 band2{ sh[4], sh[5], sh[6], sh[7], sh[8] };
        float5 b2 = rotateShericalHarmonicBand2(band2, M);
        return { b0, b1[0], b1[1], b1[2], b2[0], b2[1], b2[2], b2[3], b2[4] };
    };
    auto shmin = [rotateSh3Bands](SH3 f) -> float {
        // See "Deringing Spherical Harmonics" by Peter-Pike Sloan
        // https://www.ppsloan.org/publications/shdering.pdf
        constexpr float M_SQRT_PI = 1.7724538509f;
        constexpr float M_SQRT_3 = 1.7320508076f;
        constexpr float M_SQRT_5 = 2.2360679775f;
        constexpr float M_SQRT_15 = 3.8729833462f;
        constexpr float A[9] = {
            1.0f / (2.0f * M_SQRT_PI),       // 0: 0  0
            -M_SQRT_3 / (2.0f * M_SQRT_PI),  // 1: 1 -1
            M_SQRT_3 / (2.0f * M_SQRT_PI),   // 2: 1  0
            -M_SQRT_3 / (2.0f * M_SQRT_PI),  // 3: 1  1
            M_SQRT_15 / (2.0f * M_SQRT_PI),  // 4: 2 -2
            -M_SQRT_15 / (2.0f * M_SQRT_PI), // 5: 2 -1
            M_SQRT_5 / (4.0f * M_SQRT_PI),   // 6: 2  0
            -M_SQRT_15 / (2.0f * M_SQRT_PI), // 7: 2  1
            M_SQRT_15 / (4.0f * M_SQRT_PI)   // 8: 2  2
        };
        // first this to do is to rotate the SH to align Z with the optimal
        // linear direction
        float3 dir = normalize(float3{ -f[3], -f[1], f[2] });
        float3 z_axis = -dir;
        float3 x_axis = normalize(cross(z_axis, float3{ 0, 1, 0 }));
        float3 y_axis = cross(x_axis, z_axis);
        float3x3 M = float3x3(vml::rows, x_axis, y_axis, -z_axis);
        f = rotateSh3Bands(f, M);
        // here we're guaranteed to have normalize(float3{ -f[3], -f[1], f[2] })
        // == { 0, 0, 1 }

        // Find the min for |m| = 2
        // ------------------------
        //
        // Peter-Pike Sloan shows that the minimum can be expressed as a
        // function of z such as:  m2min = -m2max * (1 - z^2) =  m2max * z^2 -
        // m2max
        //      with m2max = A[8] * std::sqrt(f[8] * f[8] + f[4] * f[4]);
        // We can therefore include this in the ZH min computation (which is
        // function of z^2 as well)
        float m2max = A[8] * std::sqrt(f[8] * f[8] + f[4] * f[4]);
        // Find the min of the zonal harmonics
        // -----------------------------------
        //
        // This comes from minimizing the function:
        //      ZH(z) = (A[0] * f[0])
        //            + (A[2] * f[2]) * z
        //            + (A[6] * f[6]) * (3 * s.z * s.z - 1)
        //
        // We do that by finding where it's derivative d/dz is zero:
        //      dZH(z)/dz = a * z^2 + b * z + c
        //      which is zero for z = -b / 2 * a
        //
        // We also needs to check that -1 < z < 1, otherwise the min is either
        // in z = -1 or 1
        //
        float a = 3 * A[6] * f[6] + m2max;
        float b = A[2] * f[2];
        float c = A[0] * f[0] - A[6] * f[6] - m2max;
        float zmin = -b / (2.0f * a);
        float m0min_z = a * zmin * zmin + b * zmin + c;
        float m0min_b = std::min(a + b + c, a - b + c);
        float m0min = (a > 0 && zmin >= -1 && zmin <= 1) ? m0min_z : m0min_b;

        // Find the min for l = 2, |m| = 1
        // -------------------------------
        //
        // Note l = 1, |m| = 1 is guaranteed to be 0 because of the rotation
        // step
        //
        // The function considered is:
        //        Y(x, y, z) = A[5] * f[5] * s.y * s.z
        //                   + A[7] * f[7] * s.z * s.x
        float d = A[4] * std::sqrt(f[5] * f[5] + f[7] * f[7]);

        // the |m|=1 function is minimal in -0.5 -- use that to skip the
        // Newton's loop when possible
        float minimum = m0min - 0.5f * d;
        if (minimum < 0) {
            // We could be negative, to find the minimum we will use Newton's
            // method See
            // https://en.wikipedia.org/wiki/Newton%27s_method_in_optimization
            // this is the function we're trying to minimize
            auto func = [=](float x) {
                // first term accounts for ZH + |m| = 2, second terms for |m| =
                // 1
                return (a * x * x + b * x + c) + (d * x * std::sqrt(1 - x * x));
            };

            // This is func' / func'' -- this was computed with Mathematica
            auto increment = [=](float x) {
                return (x * x - 1) *
                       (d - 2 * d * x * x +
                        (b + 2 * a * x) * std::sqrt(1 - x * x)) /
                       (3 * d * x - 2 * d * x * x * x -
                        2 * a * std::pow(1 - x * x, 1.5f));
            };
            float dz;
            float z =
                -F_SQRT1_2; // we start guessing at the min of |m|=1 function
            do {
                minimum = func(z); // evaluate our function
                dz = increment(z); // refine our guess by this amount
                z = z - dz;
                // exit if z goes out of range, or if we have reached enough
                // precision
            } while (std::abs(z) <= 1 && std::abs(dz) > 1e-5f);

            if (std::abs(z) > 1) {
                // z was out of range
                minimum = std::min(func(1), func(-1));
            }
        }
        return minimum;
    };
    auto windowing = [numBands](SH3 f, float cutoff) -> SH3 {
        for (ssize_t l = 0; l < (ssize_t)numBands; l++) {
            float w = sincWindow(l, cutoff);
            f[SHindex(0, l)] *= w;
            for (ssize_t m = 1; m <= l; m++) {
                f[SHindex(-m, l)] *= w;
                f[SHindex(m, l)] *= w;
            }
        }
        return f;
    };
    BL_ASSERT(numBands <= 3, "auto can't work with more than 3 bands");
    float cutoff = numBands * 4 + 1; // start at a large band
    // We need to process each channel separately
    SH3 SH = {};
    for (size_t channel = 0; channel < 3; channel++) {
        for (size_t i = 0; i < numBands * numBands; i++) {
            SH[i] = sh[i][channel];
        }

        // find a cut-off band that works
        float l = numBands;
        float r = cutoff;
        for (size_t i = 0; i < 16 && l + 0.1f < r; i++) {
            float m = 0.5f * (l + r);
            if (shmin(windowing(SH, m)) < 0) {
                r = m;
            }
            else {
                l = m;
            }
        }
        cutoff = std::min(cutoff, l);
    }
    for (ssize_t l = 0; l < (ssize_t)numBands; l++) {
        float w = sincWindow(l, cutoff);
        sh[SHindex(0, l)] *= w;
        for (ssize_t m = 1; m <= l; m++) {
            sh[SHindex(-m, l)] *= w;
            sh[SHindex(m, l)] *= w;
        }
    }
}

static std::array<float3, 9> genLightingCoefficientsForNormal(float3 N,
                                                              float3 L) {
    auto coeff = genSHCoefficients(N);
    vml::vector<float3, 9> result;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = L * coeff[i];
    }
    return result;
}

std::array<float3, 9> bloom::genLightingCoefficients(float4 const* imageData,
                                                     usize2 resolution,
                                                     bool applyWindowing) {
    vml::vector<float3, 9> result{};
    float weightSum = 0;
    for (size_t s = 0; s < 6; ++s) {
        float4 const* sideData = imageData + s * resolution.x * resolution.y;
        for (size_t y = 0; y < resolution.y; ++y) {
            for (size_t x = 0; x < resolution.x; ++x) {
                float3 L = sideData[y * resolution.x + x].xyz;
                float2 coords = 2 * (float2(x, y) + 0.5f) / resolution - 1;
                float temp = 1.0f + vml::length_squared(coords);
                float weight = 4.0f / (std::sqrt(temp) * temp);
                float3 N = mapUVSToN(coords.x, coords.y, s);
                auto coeff = genLightingCoefficientsForNormal(N, L);
                for (size_t i = 0; i < result.size(); ++i) {
                    result[i] += coeff[i] * weight;
                }
                weightSum += weight;
            }
        }
    }
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] *= 4.0f * vml::constants<float>::pi / weightSum;
    }
    if (applyWindowing) {
        windowSH(result, 3);
    }
    return result;
}
