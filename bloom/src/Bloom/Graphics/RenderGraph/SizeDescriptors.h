#ifndef BLOOM_GRAPHICS_RENDERGRAPH_SIZEDESCRIPTORS_H
#define BLOOM_GRAPHICS_RENDERGRAPH_SIZEDESCRIPTORS_H

#include <variant>

#include <vml/vml.hpp>

namespace bloom::rg {

/// Size descriptor for textures that supports absolute and relative values
struct TextureSize {
    struct Absolute {
        size_t size;
        bool operator==(Absolute const&) const = default;
    };

    struct RelativeToScreen {
        double factor = 1.0;
        bool operator==(RelativeToScreen const&) const = default;
    };

    using Element = std::variant<Absolute, RelativeToScreen>;

    vml::vector<Element, 3> value{ Absolute{}, Absolute{}, Absolute{} };

    /// Size descriptor that describes the screen size
    static TextureSize const ScreenSize;

    /// Static constructor for absolute  2D sizes
    static TextureSize Absolute2D(vml::usize2 size) {
        return { { Absolute{ size.x }, Absolute{ size.y }, Absolute{ 1 } } };
    }

    /// Static constructor for 2D sizes relative to screen size
    static TextureSize Relative2D(vml::double2 factor) {
        return { { RelativeToScreen{ factor.x }, RelativeToScreen{ factor.y },
                   Absolute{ 1 } } };
    }

    bool operator==(TextureSize const&) const = default;

    /// Hash value used for storing elements in hash tables
    size_t hashValue() const {
        size_t seed = 0;
        for (auto& elem: value) {
            // clang-format off
            utl::hash_combine_seed(seed, std::visit(utl::overload{
                [](Absolute a) { return std::hash<size_t>{}(a.size); },
                [](RelativeToScreen r) {
                    return std::hash<double>{}(r.factor);
                },
            }, elem)); // clang-format on
        }
        return seed;
    }

    /// \Returns `true` if all components are absolute values
    bool isAbsolute() const {
        return ranges::all_of(value, [](auto& elem) {
            return std::holds_alternative<Absolute>(elem);
        });
    }

    /// \Returns the actual size described by this value depending of \p
    /// screenSize
    vml::usize3 compute(vml::usize2 screenSize) const {
        return vml::map(value, vml::usize3(screenSize, 1),
                        [&](auto elem, size_t screen) {
            // clang-format off
            return std::visit(utl::overload{
                [&](Absolute a) { return a.size; },
                [&](RelativeToScreen r) -> size_t {
                    return r.factor * screen;
                },
            }, elem); // clang-format on
        });
    }
};

constexpr TextureSize TextureSize::ScreenSize = { {
    RelativeToScreen{ 1.0 },
    RelativeToScreen{ 1.0 },
    Absolute{ 1 },
} };

} // namespace bloom::rg

template <>
struct std::hash<bloom::rg::TextureSize> {
    size_t operator()(bloom::rg::TextureSize const& size) const {
        return size.hashValue();
    }
};

#endif // BLOOM_GRAPHICS_RENDERGRAPH_SIZEDESCRIPTORS_H
