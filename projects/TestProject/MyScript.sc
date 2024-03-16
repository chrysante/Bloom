
public struct MyClass {
    fn new(&mut this) {
        __builtin_putstr("Constructing MyClass\n");
        this.counter = 100000;
    }

    fn update(&mut this) {
        __builtin_putstr("Updating MyClass. Counter = ");
        __builtin_puti64(this.counter++);
        __builtin_putstr("\n");
    }

    var counter: int;
}