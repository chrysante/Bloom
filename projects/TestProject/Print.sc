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
