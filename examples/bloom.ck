PlaneGeometry plane_geo;
SphereGeometry sphere_geo;
FlatMaterial mat1, mat2, mat3;

// TODO port color class

5 => float intensity;
mat1.color(intensity * @(1, 0, 0, 1));
mat2.color(intensity * @(0, 0, 1, 1));
mat3.color(intensity * @(0, 1, 0, 1));

GMesh plane_l(plane_geo, mat1) --> GG.scene();
GMesh plane_r(plane_geo, mat2) --> GG.scene();
GMesh sphere(sphere_geo, mat3) --> GG.scene();

@(-1.5, 0, 0) => plane_l.translate;
@(0, 0, 0) => sphere.translate;
@(1.5, 0, 0) => plane_r.translate;


// render graph
(GG.renderPass().next() $ OutputPass) @=> OutputPass @ output_pass;
BloomPass bloom_pass;
// GG.renderPass() --> BloomPass bloom_pass --> output_pass;
// bloom_pass.input(GG.renderPass().target());

// output_pass.input(bloom_pass.output());

GG.renderPass() --> output_pass; // bypass bloom


UI_Float blend(bloom_pass.blend());
UI_Float internal_blend(bloom_pass.internalBlend());

[ 
    "NONE",
    "LINEAR",
    "REINHARD",
    "CINEON",
    "ACES",
    "UNCHARTED",
] @=> string builtin_tonemap_algorithms[];
UI_Int tonemap(output_pass.tonemap());
UI_Int levels(bloom_pass.levels());

while (true) {
    GG.nextFrame() => now;

    UI.setNextWindowSize(@(400, 600), UI_Cond.Once);
    if (UI.begin("Bloom Example", null, 0)) {
        if (UI.slider("Blend", blend, 0.0, 1.0)) {
            bloom_pass.blend(blend.val());
        }

        if (UI.slider("Internal Blend", internal_blend, 0.0, 1.0)) {
            bloom_pass.internalBlend(internal_blend.val());
        }

        if (UI.listBox("Tonemap Function", tonemap, builtin_tonemap_algorithms, -1)) {
            output_pass.tonemap(tonemap.val());
        }

        if (UI.slider("Levels", levels, 0, 10)) {
            bloom_pass.levels(levels.val());
        }
    }
    UI.end();
}