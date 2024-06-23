b2_World world;

b2_BodyDef ground_body_def;
ground_body_def.position(@(0.0, -10.0));
world.createBody(ground_body_def) @=> b2_Body ground;

b2_ShapeDef ground_shape_def;

b2_Polygon.makeBox(50.0, 10.0) @=> b2_Polygon ground_box;
ground.createPolygonShape(ground_shape_def, ground_box);


// create dynamic box
b2_BodyDef dynamic_body_def;
dynamic_body_def.type(b2_BodyType.dynamicBody);
dynamic_body_def.position(@(0.0f, 4.0f));
world.createBody(dynamic_body_def) @=> b2_Body dynamic_body;
<<< "dynamic body pos", dynamic_body.position() >>>;

// then shape
b2_Polygon.makeBox(1.0f, 1.0f) @=> b2_Polygon dynamic_box;
b2_ShapeDef dynamic_shape_def;
// shapeDef.density     = 1.0f;
// shapeDef.friction    = 0.3f;
dynamic_body.createPolygonShape(dynamic_shape_def, dynamic_box) @=> b2_Shape dynamic_shape;

<<< dynamic_shape_def.density(), dynamic_shape_def.friction() >>>;


GG.b2world(world);

// dynamic_body.massData() @=> b2_MassData mass_data;
// <<< mass_data.mass(), mass_data.center(), mass_data.I() >>>;
// b2_MassData default_mass_data;
// <<< "defulat mass", default_mass_data.mass(), default_mass_data.center(), default_mass_data.I() >>>;

// dynamic_body.massData(default_mass_data);
// dynamic_body.massData() @=> b2_MassData new_mass_data;
// <<< "after setting", new_mass_data.mass(), new_mass_data.center(), new_mass_data.I() >>>;

// <<< "-----------------------" >>>;
// b2_MassData default_mass_data;

while (1) {
    GG.nextFrame() => now;
    <<< dynamic_body.position(), dynamic_body.angle() >>>;
}