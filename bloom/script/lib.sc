
extern "C" fn bloomGetEntityTransform(e: Entity) -> Transform;

extern "C" fn bloomSetEntityTransform(e: Entity, t: Transform) -> void;

public struct Timestep {
    /// The time in seconds that has passed since the program started running
    var absolute: double;

    /// The expected execution duration of this frame
    var delta: double;
}

public struct double3 {
    var x: double;
    var y: double;
    var z: double;

    fn scale(this, s: double) -> double3 {
        return double3(this.x * s,
                       this.y * s,
                       this.z * s);
    }
}

public struct quaternion {
    var r: double;
    var i: double;
    var j: double;
    var k: double;
}

public struct Transform {
    var position: double3;
    var orientation: quaternion;
    var scale: double3;
}

public struct Entity {
    /// \Returns the transform of this entity
    fn getTransform(this) -> Transform {
        // TODO: This causes SROA to crash. Investigate!
        //let t = bloomGetEntityTransform(this); 
        //return t;
        return bloomGetEntityTransform(this); 
    }

    /// Sets the transform of this entity to \p transform
    fn setTransform(this, transform: Transform) {
        bloomSetEntityTransform(this, transform);
    }

    private var ID: u64;
    private var scene: u64;
}
