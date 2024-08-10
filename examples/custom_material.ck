SphereGeometry sphere_geo;
Material custom_material;


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
    @group(1) @binding(0) var<uniform> u_grayscale: f32;

    // TODO handle normal Mat
    struct DrawUniforms {
        modelMat: mat4x4f,
        // normalMat: mat4x4f,  // needed to account for non-uniform scaling
    };

    @group(2) @binding(0) var<storage> drawInstances: array<DrawUniforms>;


    struct VertexOutput {
        @builtin(position) position : vec4f,
        @location(0) v_worldPos : vec3f,
        @location(1) v_normal : vec3f,
        @location(2) v_uv : vec2f,
        @location(3) v_tangent : vec4f,
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

        let modelMat3 : mat3x3<f32> = mat3x3(
            u_Draw.modelMat[0].xyz,
            u_Draw.modelMat[1].xyz,
            u_Draw.modelMat[2].xyz
        );

        var worldPos : vec4f = u_Frame.projViewMat * u_Draw.modelMat * vec4f(in.position, 1.0f);
        out.v_worldPos = worldPos.xyz;
        // TODO handle non-uniform scaling
        // TODO: restore to normal Mat. need to pass normal mat from cpu side
        // out.v_normal = (u_Frame.viewMat * u_Draw.normalMat * vec4f(in.normal, 0.0)).xyz;
        out.v_normal = (u_Draw.modelMat * vec4f(in.normal, 0.0)).xyz;
        // tangent vectors aren't impacted by non-uniform scaling or translation
        out.v_tangent = vec4f(modelMat3 * in.tangent.xyz, in.tangent.w);
        out.v_uv     = in.uv;
        out.position = worldPos;

        return out;
    }

    @fragment 
    fn fs_main(in : VertexOutput, @builtin(front_facing) is_front: bool) -> @location(0) vec4f
    {
        var normal : vec3f;
        if (is_front) {
            normal = in.v_normal;
        } else {
            normal = -in.v_normal;
        }

        // valve half-lambert
        // https://developer.valvesoftware.com/wiki/Half_Lambert
        var diffuse : f32 = 0.5 * dot(normal, -u_Frame.dirLight) + 0.5;
        diffuse = diffuse * diffuse;
        return vec4f(diffuse * vec3f(u_grayscale), 1.0);
    }" => string shader_string;

    <<< shader_string >>>;

ShaderDesc shader_desc;
shader_string => shader_desc.vertexString;
shader_string => shader_desc.fragmentString;
sphere_geo.vertexAttributeNumComponents() @=> shader_desc.vertexLayout;

Shader custom_shader(shader_desc); // create shader from shader_desc
custom_shader => custom_material.shader; // connect shader to material

GMesh custom_mesh(sphere_geo, custom_material) --> GG.scene();
custom_material.uniformFloat(0, 1.0);

while (true) {
    GG.nextFrame() => now;
    custom_material.uniformFloat(0, .5 * Math.sin(now/second) + 0.5);
}