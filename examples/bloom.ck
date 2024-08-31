PlaneGeometry plane_geo;
FlatMaterial material;

material.color(5 * @(0, 1, 0, 1));

GMesh mesh(plane_geo, material) --> GG.scene();

// render graph
(GG.renderPass().next() $ OutputPass) @=> OutputPass @ output_pass;
GG.renderPass() --> BloomPass bloom_pass --> output_pass;
bloom_pass.input(GG.renderPass().target());

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
UI_Int tonemap;

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
    }
    UI.end();
}