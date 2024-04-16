#ifndef SizeDescriptors_h
#define SizeDescriptors_h

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

    vml::vector<Element, 3> value;

    /// Size descriptor that describes the screen size
    static TextureSize const ScreenSize;

    ///
    static TextureSize Relative2D(vml::double2 factor) {
        return { { RelativeToScreen{ factor.x }, RelativeToScreen{ factor.y },
                   Absolute{ 1 } } };
    }

    bool operator==(TextureSize const&) const = default;

    ///
    bool isAbsolute() const {
        return ranges::all_of(value, [](auto& elem) {
            return std::holds_alternative<Absolute>(elem);
        });
    }

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

    struct Hash {
        size_t operator()(TextureSize const& size) const {
            size_t seed = 0;
            for (auto& elem: size.value) {
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
    };
};

constexpr TextureSize TextureSize::ScreenSize = { {
    RelativeToScreen{ 1.0 },
    RelativeToScreen{ 1.0 },
    Absolute{ 1 },
} };

} // namespace bloom::rg

#endif /* SizeDescriptors_h */
