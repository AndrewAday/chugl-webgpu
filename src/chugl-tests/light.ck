T.assert(GLight.Directional == 1, "Directional constant");
T.assert(GLight.Point == 2, "Point constant");

GLight light;

T.assert(light.mode() == GLight.Directional, "Default light type");
light.mode(GLight.Point);
T.assert(light.mode() == GLight.Point, "Set light type");

