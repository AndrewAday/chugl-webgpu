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

Texture.load(me.dir() + "../assets/brickwall_albedo.png") @=> Texture albedo_tex;
Texture.load(me.dir() + "../assets/brickwall_normal.png") @=> Texture normal_tex;

7 => int NUM_ROWS;

PBRMaterial material[NUM_ROWS][NUM_ROWS];
FlatMaterial light_material;
SphereGeometry geo;
// SuzanneGeometry geo;
SuzanneGeometry suzanne_geo;

// init pbr meshes
for (int i; i < NUM_ROWS; i++) {
    for (int j; j < NUM_ROWS; j++) {
        material[i][j].albedo(Color.WHITE); 
        material[i][j].metallic( (i $ float) / NUM_ROWS );
        material[i][j].roughness( Math.clampf((j $ float) / NUM_ROWS, 0.05, 1) );
        material[i][j].albedoMap(albedo_tex);
        material[i][j].normalMap(normal_tex);
        GMesh mesh(geo, material[i][j]) --> GG.scene();
        mesh.pos(@(
            (j - NUM_ROWS / 2) * 1.5,
            (i - NUM_ROWS / 2) * 1.5,
            0
        ));
    }
}

GGen point_light_axis --> GG.scene();
GPointLight point_lights[1];
GMesh point_light_meshes[0];
point_light_meshes << new GMesh(geo, light_material);
point_light_meshes[0].sca(0.1);
point_lights[0].pos(@(.8, 0, 0));
// connect mesh to light position
point_light_meshes[0] --> point_lights[0] --> point_light_axis --> GG.scene();

UI_Int num_point_lights(point_lights.size());
UI_Float dir_light_rotation;
UI_Float3 bg_color(GG.scene().backgroundColor());
UI_Float3 ambient_light(GG.scene().ambient());
UI_Float3 dir_light_color(GG.scene().light().color());
UI_Float dir_light_intensity(GG.scene().light().intensity());

// material properties
UI_Float3 albedo(material[0][0].albedo());
// UI_Float metallic(material.metallic());
// UI_Float roughness(material.roughness());
UI_Float normal_factor(material[0][0].normalFactor());


GG.scene().camera() $ GOrbitCamera @=> GOrbitCamera camera;
UI_Float3 camera_target(camera.target());

fun void ui() {
    while (true) {
        GG.nextFrame() => now;
        if (UI.begin("Lighting Example", null, 0)) {

            if (UI.colorEdit("material albedo", albedo, 0)) {
                for (int i; i < NUM_ROWS; i++) {
                    for (int j; j < NUM_ROWS; j++) {
                        albedo.val() => material[i][j].albedo;
                    }
                }
            }

            // if (UI.slider("material metallic", metallic, 0, 1)) {
            //     metallic.val() => material.metallic;
            // }

            // if (UI.slider("material roughness", roughness, 0, 1)) {
            //     roughness.val() => material.roughness;
            // }

            if (UI.slider("material normal factor", normal_factor, 0, 1)) {
                for (int i; i < NUM_ROWS; i++) {
                    for (int j; j < NUM_ROWS; j++) {
                        normal_factor.val() => material[i][j].normalFactor;
                    }
                }
            }

            if (UI.colorEdit("background color", bg_color, 0)) {
                bg_color.val() => GG.scene().backgroundColor;
            }

            if (UI.colorEdit("ambient light", ambient_light, 0)) {
                ambient_light.val() => GG.scene().ambient;
            }

            if (UI.colorEdit("dir light color", dir_light_color, 0)) {
                dir_light_color.val() => GG.scene().light().color;
            }

            if (UI.slider("dir light intensity", dir_light_intensity, 0, 10)) {
                dir_light_intensity.val() => GG.scene().light().intensity;
            }

            if (UI.slider("dir light rotation", dir_light_rotation, -Math.PI, Math.PI)) {
                dir_light_rotation.val() => GG.scene().light().rotX;
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
                    point_lights[i].pos(.8 * @(
                        Math.cos(i * Math.TWO_PI / point_lights.size()),
                        0,
                        Math.sin(i * Math.TWO_PI / point_lights.size())
                    ));
                }
            }

            UI.scenegraph(GG.scene());

            if (UI.drag("camera target", camera_target, 0.01, 0, 0, "%0.2f", 0)) {
                camera_target.val() => camera.target;
            }
        }
        UI.end();
    }
} spork ~ ui();


while (true) {
    GG.nextFrame() => now;
    GG.dt() => point_light_axis.rotateY;
}