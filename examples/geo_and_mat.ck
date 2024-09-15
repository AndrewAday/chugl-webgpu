
[
    null,
    new PlaneGeometry,
    new SuzanneGeometry,
    new SphereGeometry,
] @=> Geometry geometries[];

[
    null,
    new FlatMaterial,
    new DiffuseMaterial,
    new PBRMaterial,
] @=> Material materials[];

UI_Int material_index;
[ 
    "None",
    "FlatMaterial",
    "DiffuseMaterial",
    "PBRMaterial",
] @=> string builtin_materials[];

UI_Int geometry_index;
[
    "None",
    "PlaneGeometry",
    "SuzanneGeometry",
    "SphereGeometry",
] @=> string builtin_geometries[];

GMesh mesh --> GG.scene();

fun void ui() {
    while (true) {
        GG.nextFrame() => now; 
        if (UI.begin("Geometry and Material Example")) {

            if (UI.listBox("Builtin Geometries", geometry_index, builtin_geometries)) {
                mesh.geometry(geometries[geometry_index.val()]);
            }

            if (UI.listBox("Builtin Materials", material_index, builtin_materials)) {
                mesh.material(materials[material_index.val()]);
            }

        }
        UI.end();
    }
}
spork ~ ui();



while (true) {
    GG.nextFrame() => now;
}