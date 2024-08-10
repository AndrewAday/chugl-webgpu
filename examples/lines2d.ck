Lines2DMaterial lines2d_material;
Lines2DGeometry lines2d_geo;

[@(0.0, 0.0), @(0.5, 0.5), @(1.0, 0.0)] => lines2d_geo.linePoints;
// [@(0.0, 0.0), @(1.0, 1.0)] => lines2d_geo.linePoints;

GMesh lines2d_mesh(lines2d_geo, lines2d_material) --> GG.scene();
GMesh lines2d_mesh2(lines2d_geo, lines2d_material) --> GG.scene();
lines2d_mesh2.posY(-0.5);

while (true) {
    GG.nextFrame() => now;

    // oscillate thickness
    // Math.sin(0.1 * (now/second)) => lines2d_material.thickness;
}