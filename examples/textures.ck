"struct FrameUniforms {
    projectionMat: mat4x4f,
    viewMat: mat4x4f,
    projViewMat: mat4x4f,
    camPos: vec3f, // camera
    dirLight: vec3f, // lighting
    time: f32,
};

@group(0) @binding(0) var<uniform> u_Frame: FrameUniforms;

// our custom material uniforms
@group(1) @binding(0) var test_texture: texture_2d<f32>;

struct DrawUniforms {
    modelMat: mat4x4f,
};

@group(2) @binding(0) var<storage> drawInstances: array<DrawUniforms>;

struct VertexOutput {
    @builtin(position) position : vec4f,
    @location(0) v_uv : vec2f,
};

struct VertexInput {
    @location(0) position : vec3f,
    @location(1) normal : vec3f,
    @location(2) uv : vec2f,
    @location(3) tangent : vec4f,
    @builtin(instance_index) instance : u32,
};

@vertex 
fn vs_main(in : VertexInput) -> VertexOutput
{
    var out : VertexOutput;
    var u_Draw : DrawUniforms = drawInstances[in.instance];

    var worldPos : vec4f = u_Frame.projViewMat * u_Draw.modelMat * vec4f(in.position, 1.0f);
    out.v_uv     = in.uv;
    out.position = worldPos;

    return out;
}

@fragment 
fn fs_main(in : VertexOutput, @builtin(front_facing) is_front: bool) -> @location(0) vec4f
{
    return textureLoad(test_texture, vec2i(0, 0), 0);
}" => string lambert_shader_string;

Material material;
PlaneGeometry plane_geo;

ShaderDesc shader_desc;
lambert_shader_string => shader_desc.vertexString;
lambert_shader_string => shader_desc.fragmentString;
plane_geo.vertexAttributeNumComponents() @=> shader_desc.vertexLayout; // TODO make this default to builtin geometry layout

Shader custom_shader(shader_desc); // create shader from shader_desc
custom_shader => material.shader; // connect shader to material

GMesh mesh(plane_geo, material) --> GG.scene();

Texture tex;
tex.data([255,0,255,255], 1, 1);
material.texture(0, tex);

while (true) {
    GG.nextFrame() => now;
}