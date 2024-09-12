/*
- make lighting test
    - point light
    - spot light
    - scene ambient light
    - scene bg color
    - material albedo
    - both material and lights on parent-child transforms

controllable dir-light

N point lights on rotating around central cube. able to inc/dec number of lights
- each inc/dec is random color
*/

// scenegraph setup

DiffuseMaterial material;
FlatMaterial light_material;
SphereGeometry geo;
SuzanneGeometry suzanne_geo;

GMesh mesh(geo, material) --> GG.scene();
GMesh mesh2(suzanne_geo, material);

GDirLight dir_light --> GG.scene();

GGen point_light_axis --> GG.scene();
GPointLight point_lights[1];
GMesh point_light_meshes[0];
point_light_meshes << new GMesh(geo, light_material);
point_light_meshes[0].sca(0.1);
point_lights[0].pos(@(1, 0, 0));
// connect mesh to light position
point_light_meshes[0] --> point_lights[0] --> point_light_axis --> GG.scene();

UI_Int num_point_lights(point_lights.size());
UI_Float dir_light_rotation;
UI_Float4 bg_color(GG.scene().backgroundColor());
UI_Float3 ambient_light(GG.scene().ambient());

fun void ui() {
    while (true) {
        GG.nextFrame() => now;
        if (UI.begin("Lighting Example", null, 0)) {

            if (UI.colorEdit("background color", bg_color, 0)) {
                bg_color.val() => GG.scene().backgroundColor;
            }

            if (UI.colorEdit("ambient light", ambient_light, 0)) {
                ambient_light.val() => GG.scene().ambient;
            }

            if (UI.slider("dir light rotation", dir_light_rotation, -Math.PI, Math.PI)) {
                dir_light_rotation.val() => dir_light.rotX;
                dir_light_rotation.val() => mesh2.rotX;
                <<< dir_light.rot(), dir_light.forward(), dir_light.right(), dir_light.up() >>>;
            }   

            if (UI.inputInt("num point lights", num_point_lights)) {
                if (num_point_lights.val() < 0) {
                    num_point_lights.val(0);
                }
                if (num_point_lights.val() > point_lights.size()) {
                    for (point_lights.size() => int i; i < num_point_lights.val(); i++) {
                        point_lights << new GPointLight;
                        GMesh new_point_light_mesh(geo, light_material);
                        new_point_light_mesh.sca(0.1);
                        point_light_meshes << new_point_light_mesh;
                        point_light_meshes[i] --> point_lights[i] --> point_light_axis --> GG.scene();
                    }
                }
                else if (num_point_lights.val() < point_lights.size()) {
                    for (num_point_lights.val() => int i; i < point_lights.size(); i++) {
                        point_lights[i].detach(); // detach from scenegraph
                    }
                    // remove from arrays
                    point_lights.erase(num_point_lights.val(), point_lights.size());
                    point_light_meshes.erase(num_point_lights.val(), point_light_meshes.size());
                }
                // update light positions to form equally spaced circle 
                for (int i; i < point_lights.size(); i++) {
                    point_lights[i].pos(@(
                        Math.cos(i * Math.TWO_PI / point_lights.size()),
                        0,
                        Math.sin(i * Math.TWO_PI / point_lights.size())
                    ));
                }
            }

            UI.scenegraph(GG.scene());

        }
        UI.end();
    }
} spork ~ ui();


while (true) {
    GG.nextFrame() => now;
    GG.dt() => point_light_axis.rotateY;
}