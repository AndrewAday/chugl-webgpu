ShaderDesc shader_desc;
// test defaults
T.assert(shader_desc.vertexString == "", "shader desc vertexString");
T.assert(shader_desc.fragmentString == "", "shader desc fragmentString");
T.assert(shader_desc.vertexFilepath == "", "shader desc vertexFilepath");
T.assert(shader_desc.fragmentFilepath == "", "shader desc fragmentFilepath");
T.assert(shader_desc.vertexLayout == null, "shader desc vertexLayout");

"vertex_string" => shader_desc.vertexString;
"fragment_string" => shader_desc.fragmentString;
"vertex_filepath" => shader_desc.vertexFilepath;
"fragment_filepath" => shader_desc.fragmentFilepath;
[1,2,3,4,5,6,7,8] @=> shader_desc.vertexLayout;

// Shader shader; // default constructor not allowed
Shader shader(shader_desc);
T.assert(T.strEquals(shader.fragmentString(), shader_desc.fragmentString), "shader fragmentString");
T.assert(T.strEquals(shader.vertexString(), shader_desc.vertexString), "shader vertexString");
T.assert(T.strEquals(shader.fragmentFilepath(), shader_desc.fragmentFilepath), "shader fragmentFilepath");
T.assert(T.strEquals(shader.vertexFilepath(), shader_desc.vertexFilepath), "shader vertexFilepath");
T.assert(T.arrayEquals(shader.vertexLayout(), shader_desc.vertexLayout), "shader vertexLayout");

ShaderDesc shader_desc2;
Shader shader2(shader_desc2);
T.assert(T.strEquals(shader2.fragmentString(), shader_desc2.fragmentString), "shader2 fragmentString");
T.assert(T.strEquals(shader2.vertexString(), shader_desc2.vertexString), "shader2 vertexString");
T.assert(T.strEquals(shader2.fragmentFilepath(), shader_desc2.fragmentFilepath), "shader2 fragmentFilepath");
T.assert(T.strEquals(shader2.vertexFilepath(), shader_desc2.vertexFilepath), "shader2 vertexFilepath");
T.assert(T.arrayEquals(shader2.vertexLayout(), [0,0,0,0,0,0,0,0]), "shader2 vertexLayout");

// PBRMaterial material;

Material material;

T.assert(material.CULL_NONE == 0, "material CULL_NONE");
T.assert(material.CULL_FRONT == 1, "material CULL_FRONT");
T.assert(material.CULL_BACK == 2, "material CULL_BACK");

T.assert(material.cullMode() == material.CULL_NONE, "material cullMode default"); 
material.cullMode(material.CULL_BACK);
T.assert(material.cullMode() == material.CULL_BACK, "material cullMode");

T.assert(material.shader() == null, "material shader default");
shader => material.shader;
T.assert(material.shader() == shader, "material shader");

material.uniformFloat(0, 1.2);
T.assert(T.feq(material.uniformFloat(0), 1.2), "material uniformFloat");
T.assert(T.arrayEquals(material.activeUniformLocations(), [0]), "material activeUniformLocations");
material.uniformFloat(1, 3.4);
T.assert(T.feq(material.uniformFloat(1), 3.4), "material uniformFloat");
material.removeUniform(0);
T.assert(T.arrayEquals(material.activeUniformLocations(), [1]), "material activeUniformLocations");
material.removeUniform(1);

int empty_int_arr[0];
T.assert(T.arrayEquals(material.activeUniformLocations(), empty_int_arr), "material activeUniformLocations");



