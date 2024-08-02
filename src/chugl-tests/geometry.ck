// PlaneGeometry A;
// PlaneGeometry B(3.0, 4.0, 5, 6);

// Custom geometry test
T.assert(Geometry.MAX_VERTEX_ATTRIBUTES == 8, "MAX_VERTEX_ATTRIBUTES " + Geometry.MAX_VERTEX_ATTRIBUTES);

Geometry custom_geo;

T.assert(custom_geo.vertexAttributeNumComponents(0) == 0, "default vertexAttributeNumComponents");
T.assert(custom_geo.vertexAttributeData(0).size() == 0, "default vertexAttributeData");

// CK_DLL_MFUN(geo_set_vertex_attribute);
[1.0, 2.0, 3.0] @=> float vertex_attrib_data[];
custom_geo.vertexAttribute(0, 3, vertex_attrib_data);

// CK_DLL_MFUN(geo_get_vertex_attribute_num_components);
T.assert(custom_geo.vertexAttributeNumComponents(0) == 3, "vertexAttributeNumComponents");
// CK_DLL_MFUN(geo_get_vertex_attribute_data);
custom_geo.vertexAttributeData(0) @=> float copied_vertex_attrib_data[];
T.assert(vertex_attrib_data.size() == copied_vertex_attrib_data.size(), "vertexAttributeData size");
for (0 => int i; i < vertex_attrib_data.size(); ++i) {
    T.assert(T.feq(vertex_attrib_data[i], copied_vertex_attrib_data[i]), "vertexAttributeData equality");
}

// CK_DLL_MFUN(geo_set_indices);
[0, 1, 2] @=> int indices[];
custom_geo.indices(indices);

// CK_DLL_MFUN(geo_get_indices);
custom_geo.indices() @=> int copied_indices[];
T.assert(indices.size() == copied_indices.size(), "indices size");
for (0 => int i; i < indices.size(); ++i) {
    T.assert(indices[i] == copied_indices[i], "indices equality");
}
