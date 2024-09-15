
GLines2D lines2d;
lines2d.material() $ Lines2DMaterial @=> Lines2DMaterial@ lines2d_material;
lines2d.geometry() $ Lines2DGeometry @=> Lines2DGeometry@ lines2d_geo;

[@(0.0, 0.0), @(0.5, 0.5), @(1.0, 0.0)] => lines2d_geo.linePoints;
// [@(0.0, 0.0), @(1.0, 1.0)] => lines2d_geo.linePoints;

GMesh lines2d_mesh(lines2d_geo, lines2d_material) --> GG.scene();
GMesh lines2d_mesh2(lines2d_geo, lines2d_material) --> GG.scene();
lines2d_mesh2.posY(-0.5);

UI_Float line_width(lines2d_material.width());
UI_Bool line_loop(lines2d_material.loop());
UI_Float3 line_color(lines2d_material.color());
UI_Float line_extrusion(lines2d_material.extrusion());

fun void ui() {
    while (true) {
        GG.nextFrame() => now;

        if (UI.begin("Lines2D Example")) {
            if (UI.slider("line width", line_width, 0.01, 1)) {
                line_width.val() => lines2d_material.width;
            }

            if (UI.colorEdit("line color", line_color, 0)) {
                line_color.val() => lines2d_material.color;
            }

            if (UI.slider("line extrusion", line_extrusion, 0, 1)) {
                line_extrusion.val() => lines2d_material.extrusion;
            }

            if (UI.checkbox("line loop", line_loop)) {
                line_loop.val() => lines2d_material.loop;   
            }
        }
        UI.end();
    }
}
spork ~ ui();

fun void remove() {
    while(true) {
        lines2d_mesh --< GG.scene();
        1::second => now;
        lines2d_mesh --> GG.scene();
        1::second => now;
        [@(0.0, 1.0), @(0.5, 0.5), @(1.0, 1.0)] => lines2d_geo.linePoints;
        1::second => now;
        [@(0.0, 0.0), @(0.5, 0.5), @(1.0, 0.0)] => lines2d_geo.linePoints;
        1::second => now;
    }
} 
spork ~ remove();

while (true) {
    GG.nextFrame() => now;

    // oscillate thickness
    // Math.sin(0.1 * (now/second)) => lines2d_material.thickness;
}