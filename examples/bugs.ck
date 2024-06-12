// TODO: test that update() is only called once per main loop

class Test extends GMesh {
    fun void update(float dt) {
        <<< "test update. should be called once per main loop" >>>;
    }
}

fun void connect() {
    Test test --> GG.scene();
} spork ~ connect();

while (1) {
    GG.nextFrame() => now;
    <<< "main loop" >>>;
}