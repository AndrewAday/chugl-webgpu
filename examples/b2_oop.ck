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

------

// workflow WITHOUT shreds / Chuck_Events

b2_World world;

class b2_ContactEvents {
    b2_ContactBeginTouchEvent begin_touch_events[];
    b2_ContactEndTouchEvent end_touch_events[];
    b2_ContactHitEvent hit_events[];
}


class b2_ContactHitEvent {
	b2_Shape shapeIdA;
	b2_Shape shapeIdB;

	// Point where the shapes hit
	vec2 point;

	// Normal vector pointing from shape A to shape B
	vec2 normal;

	// The speed the shapes are approaching. Always positive. Typically in meters per second.
	float approachSpeed;
}

while (1) {
    GG.nextFrame() => now;
    world.contactEvents() @=> b2_ContactEvents @ events; // TODO call something other than event
    for (auto hit : events.hitEvents()) {
        hit.shapIdA().body() @=> b2_Body @ body_a;
        hit.shapIdB().body() @=> b2_Body @ body_b;
        // do something with the bodies
    }
}

B2_API b2ContactEvents b2World_GetContactEvents(b2WorldId worldId);

--------

// workflow WITH shreds / Chuck_Events
// need 1 shred PER shape, also somehow need to group all contact events together to broadcast to a single shred
// less efficient and more complex

--------

b2_Body / b2_World / etc are just IDS, not actual object pointers.
As a result, they don't need constructors / destructors to manage memory.
Furthermore, ChuGL doesn't need to worry about refcount / garbage collection.
Under the hood, box2d already handles memory efficiently and recycles memory pools.

create via b2_XXX.createYYY()
destroy via b2_YYY.destroy()
The ChuGL programmer is therefore responsible for their own physics memory management.

refcounting / destructors wouldn't work anyways, because technically any world can 
always have a reference to any body, and vice versa. So it's a circular reference.

"Box2D allows you to avoid destroying bodies by destroying the world directly using b2DestroyWorld(), 
which does all the cleanup work for you. However, you should be mindful to nullify body ids that you 
keep in your application."
- so box2d already handles memory references for you, just explicitly called b2_World.destroy()


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