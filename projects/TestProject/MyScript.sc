
public struct Timestep {
    var absolute: double;
    var delta: double;
}

public struct EntityID {
    var raw: u32;
}

public struct MyClass {
    fn new(&mut this) {
        __builtin_putstr("Constructing MyClass\n");
    }

    fn update(&mut this, t: Timestep) {
        __builtin_putstr("Updating MyClass\n");
        printVar("delta   ", t.delta);
        printVar("absolute", t.absolute);
        __builtin_putstr("\n");
    }

    private var ID: EntityID;
}

fn printVar(name: &str, value: double) {
    __builtin_putstr(name);
    __builtin_putstr(": ");
    __builtin_putf64(value);
    __builtin_putstr("\n");
}