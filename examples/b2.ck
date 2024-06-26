/*
Demo ideas:
- get collision data, play "doh" for each collision
- add a "gravity" slider
- on mouse click, create explosion at position
- on collision, tween albedo to red
- orthographic camera

Plumbing
- multithreading physics sim
- fix timestep
- add option for #simulation steps per frame
*/

b2_World world;
GG.b2world(world); // TODO: maybe set this under b2.activeWorld instead?
GWindow.maximize();

b2_BodyDef ground_body_def;
ground_body_def.position(@(0.0, -10.0));
world.createBody(ground_body_def) @=> b2_Body ground;
b2_ShapeDef ground_shape_def;
b2_Polygon.makeBox(50.0, 10.0) @=> b2_Polygon ground_box;
ground.createPolygonShape(ground_shape_def, ground_box);

PlaneGeometry plane_geo;
PBRMaterial mat;
GMesh ground_mesh(plane_geo, mat) --> GG.scene();
ground_mesh.posY(-10.0);
ground_mesh.scaX(100.0);
ground_mesh.scaY(20.0);

GMesh@ dynamic_box_meshes[0];
b2_Body@ dynamic_bodies[0];

fun void addBody()
{
    // create dynamic box
    b2_BodyDef dynamic_body_def;
    dynamic_body_def.type(b2_BodyType.dynamicBody);
    dynamic_body_def.position(@(Math.random2f(-4.0, 4.0), Math.random2f(6.0, 12.0)));
    dynamic_body_def.angle(Math.random2f(0.0,Math.two_pi));
    world.createBody(dynamic_body_def) @=> b2_Body dynamic_body;
    dynamic_bodies << dynamic_body;

    // then shape
    b2_Polygon.makeBox(1.0f, 1.0f) @=> b2_Polygon dynamic_box;
    b2_ShapeDef dynamic_shape_def;
    dynamic_shape_def.density(1.0);
    dynamic_shape_def.friction(.3);
    dynamic_body.createPolygonShape(dynamic_shape_def, dynamic_box) @=> b2_Shape dynamic_shape;
    <<< dynamic_shape_def.density(), dynamic_shape_def.friction() >>>;

    // matching GGen
    GMesh dynamic_box_mesh(plane_geo, mat) --> GG.scene();
    dynamic_box_mesh.scaX(2.0);
    dynamic_box_mesh.scaY(2.0);
    dynamic_box_meshes << dynamic_box_mesh;
}



while (1) {
    GG.nextFrame() => now;

    if (UI.isKeyPressed(UI_Key.Space)) addBody();

    for (int i; i < dynamic_bodies.size(); i++) {
        dynamic_bodies[i].position() => vec2 p;
        dynamic_box_meshes[i].pos(@(p.x, p.y, 0.0));
        dynamic_box_meshes[i].rotZ(dynamic_bodies[i].angle());
    }
}