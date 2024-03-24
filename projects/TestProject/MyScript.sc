use Bloom;

public struct RotateBehaviour {
    fn new(&mut this, entity: Entity) {
        this.entity = entity;
    }

    fn update(&mut this, t: Timestep) {
        var transform = this.entity.getTransform();
        let radius = 1000.0;
        let x = radius * __builtin_cos_f64(t.absolute);
        let y = radius * __builtin_sin_f64(t.absolute);
        transform.position.x += x * t.delta;
        transform.position.y += y * t.delta;
        this.entity.setTransform(transform);
    }

    private var entity: Entity;
}

public struct BounceBehaviour {
    fn new(&mut this, entity: Entity) {
        this.entity = entity;
        this.startZ = entity.getTransform().position.z;
    }

    fn update(&mut this, t: Timestep) {
        var transform = this.entity.getTransform();
        let speed = 4.0;
        let height = 100.0;
        let z = height * (__builtin_cos_f64(t.absolute * speed) + 1.0) / 2.0;
        transform.position.z = this.startZ + z;
        this.entity.setTransform(transform);
    }

    private var entity: Entity;
    private var startZ: double;
}

public struct PulseBehaviour {
    fn new(&mut this, entity: Entity) {
        this.entity = entity;
        this.originalScale = entity.getTransform().scale;
    }

    fn update(&mut this, t: Timestep) {
        var transform = this.entity.getTransform();
        let intensity = 0.75;
        let speed = 3.5;
        let s = 1.0 + intensity * (__builtin_sin_f64(t.absolute * speed) + 1.0) / 2.0;
        transform.scale = this.originalScale.scale(s);
        this.entity.setTransform(transform);
    }

    private var entity: Entity;
    private var originalScale: double3;
}
