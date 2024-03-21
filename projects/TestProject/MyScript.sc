
public struct Timestep {
    var absolute: double;
    var delta: double;
}

public struct Entity {
    private var ID: u64;
    private var scene: u64;
}

struct double3 {
    var x: double;
    var y: double;
    var z: double;

    fn scale(this, s: double) -> double3 {
        return double3(this.x * s,
                       this.y * s,
                       this.z * s);
    }
}

struct Transform {
    var position: double3;
    var scale: double3;
}

extern "C" fn bloomGetEntityTransform(e: Entity) -> Transform;

extern "C" fn bloomSetEntityTransform(e: Entity, t: Transform) -> void;

public struct RotateBehaviour {
    fn new(&mut this, entity: Entity) {
        this.entity = entity;
    }

    fn update(&mut this, t: Timestep) {
        var transform = bloomGetEntityTransform(this.entity);
        let radius = 1000.0;
        let x = radius * __builtin_cos_f64(t.absolute);
        let y = radius * __builtin_sin_f64(t.absolute);
        transform.position.x += x * t.delta;
        transform.position.y += y * t.delta;
        bloomSetEntityTransform(this.entity, transform);
    }

    private var entity: Entity;
}

public struct BounceBehaviour {
    fn new(&mut this, entity: Entity) {
        this.entity = entity;
        this.startZ = bloomGetEntityTransform(entity).position.z;
    }

    fn update(&mut this, t: Timestep) {
        var transform = bloomGetEntityTransform(this.entity);
        let speed = 4.0;
        let height = 100.0;
        let z = height * (__builtin_cos_f64(t.absolute * speed) + 1.0) / 2.0;
        transform.position.z = this.startZ + z;
        bloomSetEntityTransform(this.entity, transform);
    }

    private var entity: Entity;
    private var startZ: double;
}

public struct PulseBehaviour {
    fn new(&mut this, entity: Entity) {
        this.entity = entity;
        this.originalScale = bloomGetEntityTransform(entity).scale;
    }

    fn update(&mut this, t: Timestep) {
        var transform = bloomGetEntityTransform(this.entity);
        let intensity = 0.75;
        let speed = 3.5;
        let s = 1.0 + intensity * (__builtin_sin_f64(t.absolute * speed) + 1.0) / 2.0;
        transform.scale = this.originalScale.scale(s);
        bloomSetEntityTransform(this.entity, transform);
    }

    private var entity: Entity;
    private var originalScale: double3;
}
