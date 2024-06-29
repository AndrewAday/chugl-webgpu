b2_WorldDef world_def;
// world_def.help();
T.assert(world_def.enableSleep == true, "enableSleep default value");
T.assert(world_def.enableContinuous == true, "enableContinuous default value");
T.assert(T.feq(world_def.contactHertz, 30.0), "contactHertz default value");
false => world_def.enableSleep;
T.assert(world_def.enableSleep == false, "enableSleep set to false");

b2.createWorld(world_def) => int world_id;
T.assert(b2_World.isValid(world_id), "world created");
T.assert(!b2_World.isValid(0), "invalid world");
b2.destroyWorld(world_id);
T.assert(!b2_World.isValid(world_id), "destroyed world");

b2_Filter filter;
T.assert(filter.categoryBits == 0x0001, "categoryBits default value");
T.assert(filter.maskBits & 0xFFFFFFFF == 0xFFFFFFFF, "maskBits default value");
T.assert(filter.groupIndex == 0, "groupIndex default value");

b2_ShapeDef shape_def;
T.assert(T.feq(shape_def.density, 1.0), "density default value");
T.assert(shape_def.filter.categoryBits == 0x0001, "filter.categoryBits default value");
T.assert(shape_def.filter.maskBits & 0xFFFFFFFF == 0xFFFFFFFF, "filter.maskBits default value");
T.assert(shape_def.filter.groupIndex == 0, "filter.groupIndex default value");

b2_BodyDef body_def;
@(1337, 2.3) => body_def.position;
T.assert(body_def.type == b2_BodyType.staticBody, "type default value");
T.assert(T.feq(body_def.gravityScale, 1.0), "gravityScale default value");
T.assert(body_def.isAwake, "body is awake by default");

b2.createWorld(world_def) => world_id;
b2.createBody(world_id, body_def) => int body_id;
T.assert(b2_Body.isValid(body_id), "body created");
T.assert(T.feq(1337, b2_Body.position(body_id).x), "position.x set");

b2_Polygon.makeBox(.5, .5) @=> b2_Polygon@ box_poly;
b2.createPolygonShape(body_id, shape_def, box_poly) => int shape_id;

b2.destroyBody(body_id);
T.assert(!b2_Body.isValid(body_id), "destroy body");
