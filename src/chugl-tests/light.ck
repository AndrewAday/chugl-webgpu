T.assert(GLight.Directional == 1, "Directional constant");
T.assert(GLight.Point == 2, "Point constant");

GLight light;

T.assert(light.mode() == GLight.Directional, "Default light type");
light.mode(GLight.Point);
T.assert(light.mode() == GLight.Point, "Set light type");

GPointLight point_light;
GDirLight dir_light;

T.assert(point_light.mode() == GLight.Point, "Point light type");
T.assert(dir_light.mode() == GLight.Directional, "Directional light type");

GLight light_with_ctor(GLight.Point);

T.assert(light_with_ctor.mode() == GLight.Point, "Constructor light type");