// TODO
/*
Add builtin textures to apply to materials
Add builtin skybox + IBL lighting
*/

GMesh mesh --> GG.scene();

[
    null,
    new PlaneGeometry,
    new SuzanneGeometry,
    new SphereGeometry,
    new BoxGeometry,
    new CircleGeometry,
    new TorusGeometry,
    new CylinderGeometry,
] @=> Geometry geometries[];

UI_Int geometry_index;
[
    "None",
    "PlaneGeometry",
    "SuzanneGeometry",
    "SphereGeometry",
    "BoxGeometry",
    "CircleGeometry",
    "TorusGeometry",
    "CylinderGeometry",
] @=> string builtin_geometries[];

[
    null,
    new UVMaterial,
    new NormalMaterial,
    new TangentMaterial,
    new FlatMaterial,
    new DiffuseMaterial,
    new PBRMaterial,
] @=> Material materials[];

UI_Int material_index;
[ 
    "None",
    "UVMaterial",
    "NormalMaterial",
    "TangentMaterial",
    "FlatMaterial",
    "DiffuseMaterial",
    "PBRMaterial",
] @=> string builtin_materials[];


// Material params
UI_Int material_topology_index(3); // default to triangle list
[
    "PointList",
    "LineList",
    "LineStrip",
    "TriangleList",
    "TriangleStrip",
] @=> string material_topologies[];

// Normal material params
materials[2] $ NormalMaterial @=> NormalMaterial@ normal_material;
UI_Bool normal_material_worldspace(normal_material.worldspaceNormals());

// Plane geometry params
geometries[1] $ PlaneGeometry @=> PlaneGeometry@ plane_geo;
UI_Float plane_width(plane_geo.width());
UI_Float plane_height(plane_geo.height());
UI_Int plane_width_segments(plane_geo.widthSegments());
UI_Int plane_height_segments(plane_geo.heightSegments());
fun void buildPlane() {
    plane_geo.build(
        plane_width.val(),
        plane_height.val(),
        plane_width_segments.val(),
        plane_height_segments.val()
    );
}

// sphere geometry params
geometries[3] $ SphereGeometry @=> SphereGeometry@ sphere_geo;
UI_Float sphere_radius(sphere_geo.radius());
UI_Int sphere_width(sphere_geo.widthSegments());
UI_Int sphere_height(sphere_geo.heightSegments());
UI_Float sphere_phi_start(sphere_geo.phiStart());
UI_Float sphere_phi_length(sphere_geo.phiLength());
UI_Float sphere_theta_start(sphere_geo.thetaStart());
UI_Float sphere_theta_length(sphere_geo.thetaLength());
fun void buildSphere() {
    sphere_geo.build(
        sphere_radius.val(),
        sphere_width.val(),
        sphere_height.val(),
        sphere_phi_start.val(),
        sphere_phi_length.val(),
        sphere_theta_start.val(),
        sphere_theta_length.val()
    );
}

// box geometry params
geometries[4] $ BoxGeometry @=> BoxGeometry@ box_geo;
UI_Float box_width(box_geo.width());
UI_Float box_height(box_geo.height());
UI_Float box_depth(box_geo.depth());
UI_Int box_width_segments(box_geo.widthSegments());
UI_Int box_height_segments(box_geo.heightSegments());
UI_Int box_depth_segments(box_geo.depthSegments());
fun void buildBox() {
    box_geo.build(
        box_width.val(),
        box_height.val(),
        box_depth.val(),
        box_width_segments.val(),
        box_height_segments.val(),
        box_depth_segments.val()
    );
}

// circle geometry params
geometries[5] $ CircleGeometry @=> CircleGeometry@ circle_geo;
UI_Float circle_radius(circle_geo.radius());
UI_Int circle_segments(circle_geo.segments());
UI_Float circle_theta_start(circle_geo.thetaStart());
UI_Float circle_theta_length(circle_geo.thetaLength());
fun void buildCircle() {
    circle_geo.build(
        circle_radius.val(),
        circle_segments.val(),
        circle_theta_start.val(),
        circle_theta_length.val()
    );
}

// torus geometry params
geometries[6] $ TorusGeometry @=> TorusGeometry@ torus_geo;
UI_Float torus_radius(torus_geo.radius());
UI_Float torus_tube_radius(torus_geo.tubeRadius());
UI_Int torus_radial_segments(torus_geo.radialSegments());
UI_Int torus_tubular_segments(torus_geo.tubularSegments());
UI_Float torus_arc_length(torus_geo.arcLength());
fun void buildTorus() {
    torus_geo.build(
        torus_radius.val(),
        torus_tube_radius.val(),
        torus_radial_segments.val(),
        torus_tubular_segments.val(),
        torus_arc_length.val()
    );
}

// cylinder geometry params
geometries[7] $ CylinderGeometry @=> CylinderGeometry@ cylinder_geo;
UI_Float cylinder_radius_top(cylinder_geo.radiusTop());
UI_Float cylinder_radius_bottom(cylinder_geo.radiusBottom());
UI_Float cylinder_height(cylinder_geo.height());
UI_Int cylinder_radial_segments(cylinder_geo.radialSegments());
UI_Int cylinder_height_segments(cylinder_geo.heightSegments());
UI_Bool cylinder_open_ended(cylinder_geo.openEnded());
UI_Float cylinder_theta_start(cylinder_geo.thetaStart());
UI_Float cylinder_theta_length(cylinder_geo.thetaLength());
fun void buildCylinder() {
    cylinder_geo.build(
        cylinder_radius_top.val(),
        cylinder_radius_bottom.val(),
        cylinder_height.val(),
        cylinder_radial_segments.val(),
        cylinder_height_segments.val(),
        cylinder_open_ended.val(),
        cylinder_theta_start.val(),
        cylinder_theta_length.val()
    );
}


UI_Bool rotate;

fun void ui() {
    while (true) {
        GG.nextFrame() => now; 
        if (UI.begin("Geometry and Material Example")) {

            UI.checkbox("rotate", rotate);

            if (UI.listBox("builtin geometries", geometry_index, builtin_geometries)) {
                mesh.geometry(geometries[geometry_index.val()]);
            }

            if (UI.listBox("builtin materials", material_index, builtin_materials)) {
                mesh.material(materials[material_index.val()]);

                // update material params
                if (mesh.material() != null) {
                    material_topology_index.val() => mesh.material().topology;
                }
            }

            UI.separatorText("Material Params");

            if (mesh.material() != null) {

                // material topology
                if (UI.listBox("topology", material_topology_index, material_topologies)) {
                    <<< "setting topology to", material_topology_index.val() >>>;
                    mesh.material().topology(material_topology_index.val());
                }
            }

            if (mesh.material() == normal_material) {
                if (UI.checkbox("worldspace normals", normal_material_worldspace)) {
                    normal_material.worldspaceNormals(normal_material_worldspace.val());
                }
            }


            UI.separatorText("Geometry Params");

            // plane geometry params
            if (mesh.geometry() == plane_geo) {
                if (UI.slider("width", plane_width, 0.1, 10)) buildPlane();
                if (UI.slider("height", plane_height, 0.1, 10)) buildPlane();
                if (UI.slider("width segments", plane_width_segments, 1, 64)) buildPlane();
                if (UI.slider("height segments", plane_height_segments, 1, 64)) buildPlane();
            }
            
            // sphere geometry params
            if (mesh.geometry() == sphere_geo) {
                if (UI.slider("radius", sphere_radius, 0.1, 10)) buildSphere();
                if (UI.slider("width segments", sphere_width, 3, 64)) buildSphere();
                if (UI.slider("height segments", sphere_height, 2, 64)) buildSphere();
                if (UI.slider("phi start", sphere_phi_start, 0, 2 * Math.PI)) buildSphere();
                if (UI.slider("phi length", sphere_phi_length, 0, 2 * Math.PI)) buildSphere();
                if (UI.slider("theta start", sphere_theta_start, 0, Math.PI)) buildSphere();
                if (UI.slider("theta length", sphere_theta_length, 0, Math.PI)) buildSphere();
            }

            // box geometry params
            if (mesh.geometry() == box_geo) {
                if (UI.slider("width", box_width, 0.1, 10)) buildBox();
                if (UI.slider("height", box_height, 0.1, 10)) buildBox();
                if (UI.slider("depth", box_depth, 0.1, 10)) buildBox();
                if (UI.slider("width segments", box_width_segments, 1, 64)) buildBox();
                if (UI.slider("height segments", box_height_segments, 1, 64)) buildBox();
                if (UI.slider("depth segments", box_depth_segments, 1, 64)) buildBox();
            }

            // circle geometry params
            if (mesh.geometry() == circle_geo) {
                if (UI.slider("radius", circle_radius, 0.1, 10)) buildCircle();
                if (UI.slider("segments", circle_segments, 1, 64)) buildCircle();
                if (UI.slider("theta start", circle_theta_start, 0, 2 * Math.PI)) buildCircle();
                if (UI.slider("theta length", circle_theta_length, 0, 2 * Math.PI)) buildCircle();
            }

            // torus geometry params
            if (mesh.geometry() == torus_geo) {
                if (UI.slider("radius", torus_radius, 0.1, 10)) buildTorus();
                if (UI.slider("tube radius", torus_tube_radius, 0.1, 10)) buildTorus();
                if (UI.slider("radial segments", torus_radial_segments, 3, 64)) buildTorus();
                if (UI.slider("tubular segments", torus_tubular_segments, 3, 64)) buildTorus();
                if (UI.slider("arc length", torus_arc_length, 0, 2 * Math.PI)) buildTorus();
            }

            // cylinder geometry params
            if (mesh.geometry() == cylinder_geo) {
                if (UI.slider("radius top", cylinder_radius_top, 0.1, 10)) buildCylinder();
                if (UI.slider("radius bottom", cylinder_radius_bottom, 0.1, 10)) buildCylinder();
                if (UI.slider("height", cylinder_height, 0.1, 10)) buildCylinder();
                if (UI.slider("radial segments", cylinder_radial_segments, 3, 64)) buildCylinder();
                if (UI.slider("height segments", cylinder_height_segments, 1, 64)) buildCylinder();
                if (UI.checkbox("open ended", cylinder_open_ended)) buildCylinder();
                if (UI.slider("theta start", cylinder_theta_start, 0, 2 * Math.PI)) buildCylinder();
                if (UI.slider("theta length", cylinder_theta_length, 0, 2 * Math.PI)) buildCylinder();
            }
        }
        UI.end();
    }
}
spork ~ ui();



while (true) {
    GG.nextFrame() => now;
    if (rotate.val()) {
        GG.dt() * .3 => mesh.rotateY;
    }
}