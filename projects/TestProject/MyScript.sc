use Bloom;

fn print(v: double3) {
    __builtin_putstr("(");
    __builtin_putf64(v.x);
    __builtin_putstr(", ");
    __builtin_putf64(v.y);
    __builtin_putstr(", ");
    __builtin_putf64(v.z);
    __builtin_putstr(")\n");
}

fn print(q: quaternion) {
    __builtin_putstr("(");
    __builtin_putf64(q.r);
    __builtin_putstr(", ");
    __builtin_putf64(q.i);
    __builtin_putstr(", ");
    __builtin_putf64(q.j);
    __builtin_putstr(", ");
    __builtin_putf64(q.k);
    __builtin_putstr(")\n");
}

fn print(t: Transform) {
    __builtin_putstr("Position:    ");
    print(t.position);
    __builtin_putstr("Orientation: ");
    print(t.orientation);
    __builtin_putstr("Scale:       ");
    print(t.scale);
}

fn normalize(v: double3) -> double3 {
    return v.scale(1.0 / __builtin_sqrt_f64(v.x * v.x + v.y * v.y + v.z * v.z));
}

fn makeRotation(angle: double, axis: double3) -> quaternion {
    let s = __builtin_sin_f64(angle);
    let c = __builtin_cos_f64(angle);
    let a = normalize(axis);
    return quaternion(c, 
                      s * a.x,
                      s * a.y,
                      s * a.z);
}

public struct RotateBehaviour {
    fn new(&mut this, entity: Entity) {
        this.entity = entity;
    }

    fn update(&mut this, t: Timestep) {
        var transform = this.entity.getTransform();
        let t2 = t.absolute * 0.5;
        transform.orientation = makeRotation(t.absolute, double3(__builtin_sin_f64(t2), __builtin_cos_f64(t2), 0.0));
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
