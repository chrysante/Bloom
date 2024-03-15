#ifndef POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORTCAMERACONTROLLER_H
#define POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORTCAMERACONTROLLER_H

#include <mtl/mtl.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Application/Input.h"
#include "Bloom/Core/Serialize.h"
#include "Bloom/Core/Time.h"
#include "Bloom/Graphics/Camera.h"

namespace poppy {

/// Different kinds of projection
enum class Projection { Perspective = 0, Orthogonal, LAST = Orthogonal };

///
std::string toString(Projection proj);

/// Camera controller for the editor viewport window
class ViewportCameraController {
public:
    ///
    ViewportCameraController() { applyTransform(); }

    /// This is excepted to be called on every frame that has inputs
    void update(bloom::Timestep ts, bloom::Input const& input);

    ///
    void applyProjection(mtl::float2 screenSize);

    /// \Returns the front vector
    mtl::float3 front() const;

    /// \Returns the up vector
    mtl::float3 up() const { return { 0, 0, 1 }; }

    /// \Returns the current projection mode
    Projection projection() const { return data.projection; }

    /// Sets the projection mode to \p proj
    void setProjection(Projection proj) { data.projection = proj; }

    ///
    float fieldOfView() const { return data.fieldOfView; }

    ///
    void setFieldOfView(float fov) { data.fieldOfView = fov; }

    ///
    float nearClipPlane() const { return data.nearClip; }

    ///
    void setNearClipPlane(float clip) { data.nearClip = clip; }

    ///
    bloom::Camera const& camera() const { return cam; }

private:
    void applyTransform();

    YAML::Node serialize() const;

    bool deserialize(YAML::Node const& node);

    struct Data {
        float angleLR = mtl::constants<>::pi / 2;
        float angleUD = mtl::constants<>::pi / 2;
        float speed = 500;
        mtl::float3 position = { 0, 0, 1 };
        Projection projection = Projection::Perspective;
        float fieldOfView = 60;
        float nearClip = 1;
    };

    friend struct YAML::convert<poppy::ViewportCameraController::Data>;
    friend struct YAML::convert<poppy::ViewportCameraController>;

    Data data;
    bloom::Camera cam;
};

} // namespace poppy

template <>
struct YAML::convert<poppy::ViewportCameraController> {
    static Node encode(poppy::ViewportCameraController const& c) {
        return c.serialize();
    }

    static bool decode(Node const& node, poppy::ViewportCameraController& c) {
        return c.deserialize(node);
    }
};

#endif // POPPY_EDITOR_VIEWS_VIEWPORT_VIEWPORTCAMERACONTROLLER_H
