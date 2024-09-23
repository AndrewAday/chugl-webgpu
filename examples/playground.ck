// Audio graph
adc => Gain g => OnePole p => blackhole;
// square the input
adc => g;
// multiply
3 => g.op;
0.999 => p.pole;

SinOsc osc => dac;
osc.gain(.3);

// GGens <---> GameObject
// Transform data (pos, rot, scale)

// GGens : GMesh
// geometry (vertex data, where are teh triangles in space?), material (shader, what color are the triangles?)
// geometry + mesh 

// builtin geometries
KnotGeometry geometry;

// builtin materials
PhongMaterial material;

GMesh mesh(geometry, material) --> GG.scene();


UI_Float time_interval(.5);
UI_Float2 mesh_range(@(-1, 1));

fun void doComputerMusic() {
    while (true) {
        time_interval.val()::second => now;
        Math.random2f(100, 1000) => osc.freq;
        Math.remap(osc.freq(), 100, 1000, mesh_range.val().x, mesh_range.val().y) => mesh.posY;
    }
} spork ~ doComputerMusic();


fun void ui() {
    while (true) {
        GG.nextFrame() => now;

        if (UI.begin("Hello, world!")) {
            UI.scenegraph(GG.scene());

            UI.slider("time interval", time_interval, 0.01, 1.0);
            UI.drag("mesh range", mesh_range);
        }
        UI.end();
    }
}
spork ~ ui();

while (true) {
    GG.nextFrame() => now;
    1.0 + Math.pow(p.last(), .24) => mesh.sca;
}