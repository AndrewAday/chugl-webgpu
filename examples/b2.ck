b2_World world;

b2_BodyDef ground_body_def;
ground_body_def.position(@(0.0, -10.0));
world.createBody(ground_body_def) @=> b2_Body ground;

b2_ShapeDef ground_shape_def;

b2_Polygon.makeBox(50.0, 10.0) @=> b2_Polygon ground_box;
ground.createPolygonShape(ground_shape_def, ground_box);

