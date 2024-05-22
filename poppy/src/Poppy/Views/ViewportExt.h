#ifndef POPPY_VIEWS_VIEWPORTEXT_H
#define POPPY_VIEWS_VIEWPORTEXT_H

#include <span>

#include <utl/function_view.hpp>
#include <vml/vml.hpp>
#include <yaml-cpp/yaml.h>

#include "Bloom/Application/Input.h"
#include "Bloom/Asset/Fwd.h"
#include "Bloom/Core/Serialize.h"
#include "Bloom/Core/Time.h"
#include "Bloom/Graphics/Camera.h"
#include "Bloom/Scene/Components/Lights.h"
#include "Bloom/Scene/Components/Transform.h"
#include "Bloom/Scene/Entity.h"
#include "Poppy/Editor/EntitySelectionManager.h"

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
    void applyProjection(vml::float2 screenSize);

    /// \Returns the front vector
    vml::float3 front() const;

    /// \Returns the up vector
    vml::float3 up() const { return { 0, 0, 1 }; }

    /// \Returns the current projection mode
    Projection projection() const { return data.projection; }

    /// Sets the projection mode to \p proj
    void setProjection(Projection proj) { data.projection = proj; }

    /// \Returns the current field of view in degrees
    float fieldOfView() const { return data.fieldOfView; }

    /// Sets the field of view to \p fov in degrees
    void setFieldOfView(float fov) { data.fieldOfView = fov; }

    ///
    float nearClipPlane() const { return data.nearClip; }

    ///
    void setNearClipPlane(float clip) { data.nearClip = clip; }

    /// \Returns the current speed
    float speed() const { return data.speed; }

    /// Sets the speed
    void setSpeed(float speed) { data.speed = speed; }

    ///
    bloom::Camera const& camera() const { return cam; }

private:
    void applyTransform();

    YAML::Node serialize() const;

    bool deserialize(YAML::Node const& node);

    struct Data {
        float angleLR = vml::constants<>::pi / 2;
        float angleUD = vml::constants<>::pi / 2;
        float speed = 500;
        vml::float3 position = { 0, 0, 1 };
        Projection projection = Projection::Perspective;
        float fieldOfView = 60;
        float nearClip = 1;
    };

    friend struct YAML::convert<poppy::ViewportCameraController::Data>;
    friend struct YAML::convert<poppy::ViewportCameraController>;

    Data data;
    bloom::Camera cam;
};

/// \Returns the clicked entity if any
bloom::EntityHandle displayLightOverlays(
    std::span<bloom::Scene* const> scenes,
    EntitySelectionManager const& selection,
    utl::function_view<vml::float3(vml::float3)> WStoSS);

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

#endif // POPPY_VIEWS_VIEWPORTEXT_H
