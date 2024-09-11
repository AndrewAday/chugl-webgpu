#pragma once

#include <glm/glm.hpp>

#include "core/macros.h"

#include <unordered_map>

struct ShaderEntry {
    const char* name;
    const char* code;
};

#define PER_FRAME_GROUP 0
#define PER_MATERIAL_GROUP 1
#define PER_DRAW_GROUP 2
#define VERTEX_PULL_GROUP 3

#define VS_ENTRY_POINT "vs_main"
#define FS_ENTRY_POINT "fs_main"
#define COMPUTE_ENTRY_POINT "main"

// #define STRINGIFY(s) #s
// #define INTERPOLATE(var) STRINGIFY(${##var##})

struct FrameUniforms {
    glm::mat4x4 projection;  // at byte offset 0
    glm::mat4x4 view;        // at byte offset 64
    glm::vec3 camera_pos;    // at byte offset 128
    float time;              // at byte offset 140
    glm::vec3 ambient_light; // at byte offset 144
    int32_t num_lights;      // at byte offset 156

    float _pad[256]; // padding to reach webgpu minimum buffer size requirement
};

struct LightUniforms {
    glm::vec3 color;    // at byte offset 0
    int32_t light_type; // at byte offset 12
    glm::vec3 position; // at byte offset 16
    float _pad0;
    glm::vec3 direction;  // at byte offset 32
    float point_radius;   // at byte offset 44
    float point_falloff;  // at byte offset 48
    float spot_cos_angle; // at byte offset 52
    float _pad1[2];
};

struct MaterialUniforms {     // PBR
    glm::vec4 baseColor;      // at byte offset 0
    glm::vec3 emissiveFactor; // at byte offset 16
    f32 metallic;             // at byte offset 28
    f32 roughness;            // at byte offset 32
    f32 normalFactor;         // at byte offset 36
    f32 aoFactor;             // at byte offset 40
    f32 _pad0;
};

// struct DrawUniforms {
//     glm::mat4x4 modelMat; // at byte offset 0
// };

struct DrawUniforms {
    glm::mat4x4 model; // at byte offset 0
    int32_t id;        // at byte offset 128
    float _pad0[3];
};

// clang-format off

static std::unordered_map<std::string, std::string> shader_table = {
    {
        "FRAME_UNIFORMS", 
        R"glsl(

        struct FrameUniforms {
            projection: mat4x4f,
            view: mat4x4f,
            camera_pos: vec3f,
            time: f32,
            ambient_light: vec3f,
            num_lights: i32,
        };

        @group(0) @binding(0) var<uniform> u_Frame: FrameUniforms;

        )glsl"
    },
    {
        "LIGHTING_UNIFORMS",
        R"glsl(

        // light types
        const LightType_None = 0;
        const LightType_Directional = 1;
        const LightType_Point = 2;
        const LightType_Spot = 3;

        struct LightUniforms {
            color : vec3f,
            light_type: i32,
            position: vec3f,
            direction: vec3f, 

            // point light
            point_radius: f32,
            point_falloff: f32,

            // spot light
            spot_cos_angle: f32,
        };

        @group(0) @binding(1) var<storage, read> u_lights: array<LightUniforms>;

        )glsl"
    },
    {
        "DRAW_UNIFORMS", 
        R"glsl(

        struct DrawUniforms {
            model: mat4x4f,
            id: u32
        };

        @group(2) @binding(0) var<storage> drawInstances: array<DrawUniforms>;

        )glsl"
    },

    {
        "STANDARD_VERTEX_INPUT", // vertex input for standard 3D objects (pos, normal, uv, tangent)
        R"glsl(

        struct VertexInput {
            @location(0) position : vec3f,
            @location(1) normal : vec3f,
            @location(2) uv : vec2f,
            @location(3) tangent : vec4f,
            @builtin(instance_index) instance : u32,
        };

        )glsl"
    },

    {
        "STANDARD_VERTEX_OUTPUT", // vertex output for standard 3D objects (pos, normal, uv, tangent)
        R"glsl(

        struct VertexOutput {
            @builtin(position) position : vec4f,
            @location(0) v_worldPos : vec3f,
            @location(1) v_normal : vec3f,
            @location(2) v_uv : vec2f,
            @location(3) v_tangent : vec4f,
        };

        )glsl"
    },

    {
        "STANDARD_VERTEX_SHADER",
        R"glsl(
        @vertex 
        fn vs_main(in : VertexInput) -> VertexOutput
        {
            var out : VertexOutput;
            var u_Draw : DrawUniforms = drawInstances[in.instance];

            let modelMat3 : mat3x3<f32> = mat3x3(
                u_Draw.model[0].xyz,
                u_Draw.model[1].xyz,
                u_Draw.model[2].xyz
            );

            var worldPos : vec4f = (u_Frame.projection * u_Frame.view) * u_Draw.model * vec4f(in.position, 1.0f);
            out.v_worldPos = worldPos.xyz;
            // TODO handle non-uniform scaling

            // TODO: after wgsl adds matrix inverse, calculate normal matrix here
            // out.v_normal = (transpose(inverse(u_Frame.viewMat * u_Draw.model)) * vec4f(in.normal, 0.0)).xyz;
            out.v_normal = (u_Draw.model * vec4f(in.normal, 0.0)).xyz;

            // tangent vectors aren't impacted by non-uniform scaling or translation
            out.v_tangent = vec4f(modelMat3 * in.tangent.xyz, in.tangent.w);
            out.v_uv     = in.uv;
            out.position = worldPos;

            return out;
        }
        )glsl"
    },

    {
        "SCREEN_PASS_VERTEX_SHADER",
        R"glsl(
        struct VertexOutput {
            @builtin(position) position : vec4<f32>,
            @location(0) v_uv : vec2<f32>,
        };

        @vertex 
        fn vs_main(@builtin(vertex_index) vertexIndex : u32) -> VertexOutput {
            var output : VertexOutput;
            
            // triangle which covers the screen
            if (vertexIndex == 0u) {
                output.position = vec4f(-1.0, -1.0, 0.0, 1.0);
                output.v_uv = vec2f(0.0, 0.0);
            } else if (vertexIndex == 1u) {
                output.position = vec4f(3.0, -1.0, 0.0, 1.0);
                output.v_uv = vec2f(2.0, 0.0);
            } else {
                output.position = vec4f(-1.0, 3.0, 0.0, 1.0);
                output.v_uv = vec2f(0.0, 2.0);
            }
            // flip y (webgpu render textures are flipped)
            output.v_uv.y = 1.0 - output.v_uv.y;
            return output;
        }
        )glsl"
    }

    // TODO lighting
    // TODO normal matrix
    // TODO helper fns (srgb to linear, linear to srgb, etc)
};

static const char* flat_shader_string  = R"glsl(
#include FRAME_UNIFORMS
#include DRAW_UNIFORMS
#include STANDARD_VERTEX_INPUT
#include STANDARD_VERTEX_OUTPUT
#include STANDARD_VERTEX_SHADER

// our custom material uniforms
@group(1) @binding(0) var<uniform> flat_color: vec4f;

// don't actually need normals/tangents
@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    var ret = flat_color;
    ret.a = clamp(ret.a, 0.0, 1.0);
    return ret;
}
)glsl";


static const char* diffuse_shader_string  = R"glsl(
    #include FRAME_UNIFORMS
    #include LIGHTING_UNIFORMS
    #include DRAW_UNIFORMS
    #include STANDARD_VERTEX_INPUT
    #include STANDARD_VERTEX_OUTPUT
    #include STANDARD_VERTEX_SHADER

    // our custom material uniforms
    @group(1) @binding(0) var<uniform> albedo: vec4f;

    // don't actually need normals/tangents
    @fragment 
    fn fs_main(
        in : VertexOutput,
        @builtin(front_facing) is_front: bool,
    ) -> @location(0) vec4f
    {
        // calculate normal
        var normal : vec3f;
        if (is_front) {
            normal = in.v_normal;
        } else {
            normal = -in.v_normal;
        }
        normal = normalize(normal);

        // ambient lighting
        var ambient = albedo.rgb * u_Frame.ambient_light;
        var diffuse = vec3(0.0);

        // add diffuse contributions
        for (var i = 0; i < u_Frame.num_lights; i++) {
            let light = u_lights[i];
            switch (light.light_type) {
            case 1: { // directional
                diffuse += max(dot(normal, -light.direction), 0.0) * light.color * albedo.rgb;
            }
            case 2: { // point
                let dist = distance(in.v_worldPos, light.position);
                let intensity = pow(
                    clamp(1.0 - dist / light.point_radius, 0.0, 1.0), 
                    light.point_falloff
                );
                let dir = normalize(light.position - in.v_worldPos);
                diffuse += max(dot(normal, dir), 0.0) * light.color * albedo.rgb * intensity;
            }
            default: {}
            } // end switch
        }

        return vec4f(ambient + diffuse, albedo.a);
    }
)glsl";

static const char* lines2d_shader_string  = R"glsl(

#include FRAME_UNIFORMS

// line material uniforms
// TODO add color, extrustion, loop
@group(1) @binding(0) var<uniform> line_width: f32;

#include DRAW_UNIFORMS

@group(3) @binding(0) var<storage, read> positions : array<f32>; // vertex pulling group 

struct VertexInput {
    @builtin(instance_index) instance : u32,
};

#include STANDARD_VERTEX_OUTPUT

fn calculate_line_pos(vertex_id : u32) -> vec2f
{
    // var pos_idx = (vertex_id / 2u) + 1u; // add 1 to skip sentinel start point
    var pos_idx = (vertex_id / 2u);
    var this_pos = vec2f(
        positions[2u * pos_idx + 0u],  // x
        positions[2u * pos_idx + 1u]   // y
    );
    var pos = this_pos;

    let half_width = line_width * 0.5; 

    // get even/odd (odd vertices are expanded down, even vertices are expanded up)
    var orientation : f32 = 0.0;
    if (vertex_id % 2u == 0u) {
        orientation = 1.0;
    } else {
        orientation = -1.0;
    }

    // are we the first endpoint?
    if (vertex_id / 2u == 0u) {
        var next_pos = vec2f(
            positions[2u * (pos_idx + 1u) + 0u],  // x
            positions[2u * (pos_idx + 1u) + 1u],  // y
        );

        var line_seg_dir = normalize(next_pos - this_pos);
        var perp_dir = orientation * vec2f(-line_seg_dir.y, line_seg_dir.x);

        // adjust position
        pos += half_width * perp_dir;
    } 
    else if (pos_idx == arrayLength(&positions) / 2u - 1u) {
        // last endpoint
        var prev_pos = vec2f(
            positions[2u * (pos_idx - 1u) + 0u],  // x
            positions[2u * (pos_idx - 1u) + 1u],  // y
        );

        var line_seg_dir = normalize(this_pos - prev_pos);
        var perp_dir = orientation * vec2f(-line_seg_dir.y, line_seg_dir.x);

        // adjust position
        pos += half_width * perp_dir;
    } else {
        // middle points
        var prev_pos = vec2f(
            positions[2u * (pos_idx - 1u) + 0u],  // x
            positions[2u * (pos_idx - 1u) + 1u],  // y
        );
        var next_pos = vec2f(
            positions[2u * (pos_idx + 1u) + 0u],  // x
            positions[2u * (pos_idx + 1u) + 1u],  // y
        );

        var prev_dir = normalize(this_pos - prev_pos);
        var next_dir = normalize(next_pos - this_pos);
        var prev_dir_perp = orientation * vec2f(-prev_dir.y, prev_dir.x);
        var next_dir_perp = orientation * vec2f(-next_dir.y, next_dir.x);

        var miter_dir = normalize(prev_dir_perp + next_dir_perp);
        var miter_length = half_width / dot(miter_dir, prev_dir_perp);

        // adjust position
        pos += miter_length * miter_dir;
    }

    return pos;
}

@vertex 
fn vs_main(
    in : VertexInput,
    @builtin(vertex_index) vertex_id : u32
) -> VertexOutput
{
    var out : VertexOutput;
    var u_Draw : DrawUniforms = drawInstances[in.instance];

    let modelMat3 : mat3x3<f32> = mat3x3(
        u_Draw.model[0].xyz,
        u_Draw.model[1].xyz,
        u_Draw.model[2].xyz
    );

    var worldPos : vec4f = (u_Frame.projection * u_Frame.view) * u_Draw.model * vec4f(calculate_line_pos(vertex_id), 0.0f, 1.0f);
    out.v_worldPos = worldPos.xyz;
    out.v_normal = (u_Draw.model * vec4f(0.0, 0.0, 1.0, 0.0)).xyz;
    // tangent vectors aren't impacted by non-uniform scaling or translation
    out.v_tangent = vec4f(modelMat3 * vec3f(1.0, 0.0, 0.0), 1.0);

    // map uv to progress along line
    // TODO make this work with line loop / no loop
    // let total_points = arrayLength(&positions) - 2u; // subtract 2 for sentinel points
    let total_points = arrayLength(&positions);
    out.v_uv.x = f32(vertex_id) / (2.0 * f32(total_points));
    if (vertex_id % 2u == 0u) {
        out.v_uv.y = 1.0;
    } else {
        out.v_uv.y = 0.0;
    }

    out.position = worldPos;

    return out;
}

@fragment 
fn fs_main(in : VertexOutput, @builtin(front_facing) is_front: bool) -> @location(0) vec4f
{
    // TODO impl

    // var normal : vec3f;
    // if (is_front) {
    //     normal = in.v_normal;
    // } else {
    //     normal = -in.v_normal;
    // }
    // var diffuse : f32 = 0.5 * dot(normal, -u_Frame.dirLight) + 0.5;
    // diffuse = diffuse * diffuse;
    // return vec4f(vec3f(diffuse), 1.0);
    return vec4f(1.0);
}
)glsl";

// ----------------------------------------------------------------------------

static const char* shaderCode_ = R"glsl(

    #include FRAME_UNIFORMS

    struct MaterialUniforms {
        baseColor: vec4f,
        emissiveFactor: vec3f,
        metallic: f32,
        roughness: f32,
        normalFactor: f32,
        aoFactor: f32,
    };

    @group(1) @binding(0) var<uniform> u_Material: MaterialUniforms;

    // object textures
    // Idea: store binding# in the cpp material struct, dynamically create shader from material data?
    @group(1) @binding(1) var albedoMap: texture_2d<f32>;
    @group(1) @binding(2) var albedoSampler: sampler;
    @group(1) @binding(3) var normalMap: texture_2d<f32>;
    @group(1) @binding(4) var normalSampler: sampler;
    @group(1) @binding(5) var aoMap: texture_2d<f32>;
    @group(1) @binding(6) var aoSampler: sampler;
    @group(1) @binding(7) var mrMap: texture_2d<f32>;
    @group(1) @binding(8) var mrSampler: sampler;
    @group(1) @binding(9) var emissiveMap: texture_2d<f32>;
    @group(1) @binding(10) var emissiveSampler: sampler;

    #include DRAW_UNIFORMS

    #include STANDARD_VERTEX_INPUT

    #include STANDARD_VERTEX_OUTPUT

    @vertex 
    fn vs_main(in : VertexInput) -> VertexOutput
    {
        var out : VertexOutput;
        var u_Draw : DrawUniforms = drawInstances[in.instance];

        let modelMat3 : mat3x3<f32> = mat3x3(
            u_Draw.model[0].xyz,
            u_Draw.model[1].xyz,
            u_Draw.model[2].xyz
        );

        var worldPos : vec4f = (u_Frame.projection * u_Frame.view) * u_Draw.model * vec4f(in.position, 1.0f);
        out.v_worldPos = worldPos.xyz;
        // TODO handle non-uniform scaling
        // TODO: restore to normal Mat. need to pass normal mat from cpu side
        // out.v_normal = (u_Frame.viewMat * u_Draw.normalMat * vec4f(in.normal, 0.0)).xyz;
        out.v_normal = (u_Draw.model * vec4f(in.normal, 0.0)).xyz;
        // tangent vectors aren't impacted by non-uniform scaling or translation
        out.v_tangent = vec4f(modelMat3 * in.tangent.xyz, in.tangent.w);
        out.v_uv     = in.uv;
        out.position = worldPos;

        return out;
    }

    fn srgbToLinear(srgb_in : vec3f) -> vec3f {
        return pow(srgb_in.rgb,vec3f(2.2));
    }

    fn calculateNormal(inNormal: vec3f, inUV : vec2f, inTangent: vec4f, scale: f32) -> vec3f {
        var tangentNormal : vec3f = textureSample(normalMap, normalSampler, inUV).rgb * 2.0 - 1.0;
        // scale normal
        // ref: https://github.com/mrdoob/three.js/blob/dev/src/renderers/shaders/ShaderChunk/normal_fragment_maps.glsl.js
        tangentNormal.x *= scale;
        tangentNormal.y *= scale;

        // TODO: account for side of face (can we calculate facenormal in frag shader?)
        // TODO: do we need to adjust tangent normal based on face direction (backface or frontface)?
        // e.g. tangentNormal *= (sign(dot(normal, faceNormal)))

        // from mikkt:
        // For normal maps it is sufficient to use the following simplified version
        // of the bitangent which is generated at pixel/vertex level. 
        // bitangent = fSign * cross(vN, tangent);

        let N : vec3f = normalize(inNormal);
        let T : vec3f = normalize(inTangent.xyz);
        let B : vec3f = inTangent.w * normalize(cross(N, T));  // mikkt method
        let TBN : mat3x3f = mat3x3(T, B, N);

        // return inTangent.xyz;
        return normalize(TBN * tangentNormal);
    }


    const PI = 3.1415926535897932384626433832795;
    const reflectivity = 0.04;  // heuristic, assume F0 of 0.04 for all dielectrics

    // Normal Distribution function ----------------------------------------------
    fn D_GGX(dotNH : f32, roughness : f32) -> f32 {
        let alpha : f32 = roughness * roughness;
        let alpha2 : f32 = alpha * alpha;
        let denom : f32 = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
        return (alpha2)/(PI * denom*denom);
    }

    // Geometric Shadowing function ----------------------------------------------
    fn G_SchlickSmithGGX(dotNL : f32, dotNV : f32, roughness : f32) -> f32 {
        let r : f32 = (roughness + 1.0);
        let k : f32 = (r*r) / 8.0;

        let GL : f32 = dotNL / (dotNL * (1.0 - k) + k);
        let GV : f32 = dotNV / (dotNV * (1.0 - k) + k);
        return GL * GV;

    }

    // Fresnel function ----------------------------------------------------------
    // cosTheta assumed to be in range [0, 1]
    fn F_Schlick(cosTheta : f32, F0 : vec3<f32>) -> vec3f {
        return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    }

    // From http://filmicworlds.com/blog/filmic-tonemapping-operators/
    fn Uncharted2Tonemap(color : vec3<f32>) -> vec3f {
        let A : f32 = 0.15;
        let B : f32 = 0.50;
        let C : f32 = 0.10;
        let D : f32 = 0.20;
        let E : f32 = 0.02;
        let F : f32 = 0.30;
        let W : f32 = 11.2;
        return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
    }

    @fragment 
    fn fs_main(
        in : VertexOutput,
        @builtin(front_facing) is_front: bool
    ) -> @location(0) vec4f
    {
        var normal : vec3f;
        if (is_front) {
            normal = in.v_normal;
        } else {
            normal = -in.v_normal;
        }

        let N : vec3f = calculateNormal( normal, in.v_uv, in.v_tangent, u_Material.normalFactor);
        let V : vec3f = normalize(u_Frame.camPos - in.v_worldPos);

        // linear-space albedo (normally authored in sRGB space so we have to convert to linear space)
        // transparency not supported
        let albedo: vec3f = u_Material.baseColor.rgb * srgbToLinear(textureSample(albedoMap, albedoSampler, in.v_uv).rgb);

        let metallic : f32 = textureSample(mrMap, mrSampler, in.v_uv).b * u_Material.metallic;
        let roughness : f32 = textureSample(mrMap, mrSampler, in.v_uv).g * u_Material.roughness;

        var F0 : vec3f = vec3f(reflectivity);
        F0 = mix(F0, albedo.rgb, metallic); // reflectivity for metals
        

        var Lo : vec3f = vec3(0.0);
        // TODO: loop over all lights
        // for (var i : u32 = 0; i < LIGHTS_ARRAY_LENGTH; i++) {
            // calculate direction to light L
            let L : vec3f = normalize(-u_Frame.dirLight);
            // half vector
            let H : vec3f = normalize(V + L);
            let dotNH : f32 = clamp(dot(N, H), 0.0, 1.0);
            let dotNV : f32 = clamp(dot(N, V), 0.0, 1.0);
            let dotNL : f32 = clamp(dot(N, L), 0.0, 1.0);

            // color contrib of this light
            var colorContrib : vec3f = vec3f(0.0);

            if (dotNL > 0.0) {
                // TODO calculate light color/attenutation
                // float distance    = length(lightPositions[i] - WorldPos);
                // float attenuation = 1.0 / (distance * distance);
                // let radiance : vec3     = lightColors[i] * attenuation;    
                let radiance : vec3f = vec3f(1.0);  // hardcoded for now

                // D = Normal distribution (Distribution of the microfacets)
                let D : f32 = D_GGX(dotNH, roughness);
                // G = Geometric shadowing term (Microfacets shadowing)
                let G : f32 = G_SchlickSmithGGX(dotNL, dotNV, roughness);
                // F = Fresnel factor (Reflectance depending on angle of incidence)
                let F : vec3<f32> = F_Schlick(max(dot(H, V), 0.0), F0);

                // specular contribution
                let spec : vec3f = D * F * G / (4.0 * dotNL * dotNV + 0.0001);
                // diffuse contribution
                let kD : vec3f = (vec3f(1.0) - F) * (1.0 - metallic);
                colorContrib += (kD * albedo / PI + spec) * dotNL * radiance;
            }
            Lo += colorContrib;
        // }  // end light loop

        // // ambient occlusion (hardcoded for now) (ambient should only be applied to direct lighting, not indirect lighting)
        // const float u_OcclusionStrength = 1.0f;
        // // Apply optional PBR terms for additional (optional) shading
        // if (material.occlusionTextureSet > -1) {
        //     float ao = texture(aoMap, (material.occlusionTextureSet == 0 ? inUV0 : inUV1)).r;
        //     color = mix(color, color * ao, u_OcclusionStrength);
        // }
        // let ao: f32 = textureSample(aoMap, aoSampler, in.v_uv).r;
        // var finalColor : vec3f = mix(Lo, Lo * ao, u_Material.aoFactor);

        let ambient : vec3f = vec3f(0.03) * albedo * textureSample(aoMap, aoSampler, in.v_uv).r * u_Material.aoFactor;
        var finalColor : vec3f = Lo + ambient;  // TODO: can factor albedo out and multiply here instead

        // add emission
        let emissiveColor : vec3f = srgbToLinear(textureSample(emissiveMap, emissiveSampler, in.v_uv).rgb);
        finalColor += emissiveColor * u_Material.emissiveFactor;
        // finalColor += emissiveColor;

        // tone mapping
        let exposure : f32 = 1.0;
        let whiteScale : vec3f = 1.0 / Uncharted2Tonemap(vec3f(11.2f));
        finalColor = Uncharted2Tonemap(finalColor * exposure) * whiteScale;
        // finalColor = finalColor / (finalColor + vec3f(1.0));  // reinhard tone mapping

        // gamma correction
        finalColor = pow(finalColor, vec3f(1.0 / 2.2)); // convert back to sRGB

        // lambertian diffuse
        // let normal = normalize(in.v_normal);
        // var lightContrib : f32 = max(0.0, dot(u_Frame.dirLight, -normal));
        // return vec4f(lightContrib * albedo, 1.0);
        // add global ambient
        // lightContrib = clamp(lightContrib, 0.2, 1.0);

        return vec4f(finalColor, u_Material.baseColor.a);
        // return vec4f(Lo, u_Material.baseColor.a);
        // return vec4f(kD, u_Material.baseColor.a);
        // return vec4f(vec3f(ao), u_Material.baseColor.a);
        // return vec4f(kD, u_Material.baseColor.a);
        // return vec4f(
        //     ambient, u_Material.baseColor.a);
        // return vec4f(in.v_normal, 1.0);
        // return vec4f(in.v_uv, 0.0, 1.0);
    }
)glsl";

static const char* mipMapShader = CODE(
    var<private> pos : array<vec2<f32>, 3> = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0), 
        vec2<f32>(-1.0, 3.0), 
        vec2<f32>(3.0, -1.0)
    );

    struct VertexOutput {
        @builtin(position) position : vec4<f32>,
        @location(0) texCoord : vec2<f32>,
    }

    @vertex
    fn vs_main(@builtin(vertex_index) vertexIndex : u32) -> VertexOutput {
        var output : VertexOutput;
        // remap uvs to [0, 2] and flip y
        // rasterizer will clip uvs to [0, 1]
        output.texCoord = pos[vertexIndex] * vec2<f32>(0.5, -0.5) + vec2<f32>(0.5);
        // positions in ndc space
        output.position = vec4<f32>(pos[vertexIndex], 0.0, 1.0);
        return output;
    }

    @group(0) @binding(0) var imgSampler : sampler;
    @group(0) @binding(1) var img : texture_2d<f32>;  // image to mip

    @fragment
    fn fs_main(@location(0) texCoord : vec2<f32>) -> @location(0) vec4<f32> {
        return textureSample(img, imgSampler, texCoord);
    }
);

const char* gtext_shader_string = R"glsl(

    // Based on: http://wdobbie.com/post/gpu-text-rendering-with-vector-textures/

    #include FRAME_UNIFORMS
    #include DRAW_UNIFORMS

    // custom material uniforms
    @group(1) @binding(0) var<storage, read> u_Glyphs: array<i32>;
    @group(1) @binding(1) var<storage, read> u_Curves: array<f32>;
    @group(1) @binding(2) var<uniform> u_Color: vec4f;

    // Controls for debugging and exploring:

    // Size of the window (in pixels) used for 1-dimensional anti-aliasing along each rays.
    //   0 - no anti-aliasing
    //   1 - normal anti-aliasing
    // >=2 - exaggerated effect 
    @group(1) @binding(3) var<uniform> antiAliasingWindowSize: f32 = 1.0;

    // Enable a second ray along the y-axis to achieve 2-dimensional anti-aliasing.
    @group(1) @binding(4) var<uniform> enableSuperSamplingAntiAliasing: i32 = 1;

    @group(1) @binding(5) var<uniform> bb : vec4f; // x = minx, y = miny, z = maxx, w = maxy
    @group(1) @binding(6) var texture_map: texture_2d<f32>;
    @group(1) @binding(7) var texture_sampler: sampler;


    struct VertexInput {
        @location(0) position : vec2f,
        @location(1) uv : vec2f,
        @location(2) glyph_index : i32, // index into glyphs array (which itself is slice into curves array)
        @builtin(instance_index) instance : u32,
    };

    struct VertexOutput {
        @builtin(position) position : vec4f,
        @location(0) v_uv : vec2f, // per-glyph uv
        @location(1) @interpolate(flat) v_buffer_index: i32,
        @location(2) v_uv_textbox : vec2f, // entire GText uv (e.g. if you want to texture your text)
    };

    @vertex 
    fn vs_main(in : VertexInput) -> VertexOutput
    {
        var out : VertexOutput;
        var u_Draw : DrawUniforms = drawInstances[in.instance];
        out.position = (u_Frame.projection * u_Frame.view) * u_Draw.model * vec4f(in.position, 0.0f, 1.0f);
        out.v_uv     = in.uv;
        out.v_buffer_index = in.glyph_index;

        let bb_w = bb.z - bb.x;
        let bb_h = bb.w - bb.y;
        out.v_uv_textbox = (in.position - vec2f(bb.x, bb.y)) / vec2f(bb_w, bb_h);

        return out;
    }


    struct Glyph {
        start : i32,
        count : i32,
    };

    struct Curve {
        p0 : vec2f,
        p1 : vec2f,
        p2 : vec2f,
    };

    fn loadGlyph(index : i32) -> Glyph {
        var result : Glyph;
        // let data = u_Glyphs[index].xy;
        // result.start = u32(data.x);
        // result.count = u32(data.y);
        result.start = u_Glyphs[2 * index + 0];
        result.count = u_Glyphs[2 * index + 1];
        return result;
    }

    fn loadCurve(index : i32) -> Curve {
        var result : Curve;
        // result.p0 = u_Curves[3u * index + 0u].xy;
        // result.p1 = u_Curves[3u * index + 1u].xy;
        // result.p2 = u_Curves[3u * index + 2u].xy;
        result.p0 = vec2f(u_Curves[6 * index + 0], u_Curves[6 * index + 1]);
        result.p1 = vec2f(u_Curves[6 * index + 2], u_Curves[6 * index + 3]);
        result.p2 = vec2f(u_Curves[6 * index + 4], u_Curves[6 * index + 5]);
        return result;
    }

    fn computeCoverage(inverseDiameter : f32, p0 : vec2f, p1 : vec2f, p2 : vec2f) -> f32 {
        if (p0.y > 0.0 && p1.y > 0.0 && p2.y > 0.0) { return 0.0; }
        if (p0.y < 0.0 && p1.y < 0.0 && p2.y < 0.0) { return 0.0; }

        // Note: Simplified from abc formula by extracting a factor of (-2) from b.
        let a = p0 - 2.0*p1 + p2;
        let b = p0 - p1;
        let c = p0;

        var t0 : f32;
        var t1 : f32;
        if (abs(a.y) >= 1e-5) {
            // Quadratic segment, solve abc formula to find roots.
            let radicand : f32 = b.y*b.y - a.y*c.y;
            if (radicand <= 0.0) { return 0.0; }
        
            let s : f32 = sqrt(radicand);
            t0 = (b.y - s) / a.y;
            t1 = (b.y + s) / a.y;
        } else {
            // Linear segment, avoid division by a.y, which is near zero.
            // There is only one root, so we have to decide which variable to
            // assign it to based on the direction of the segment, to ensure that
            // the ray always exits the shape at t0 and enters at t1. For a
            // quadratic segment this works 'automatically', see readme.
            let t : f32 = p0.y / (p0.y - p2.y);
            if (p0.y < p2.y) {
                t0 = -1.0;
                t1 = t;
            } else {
                t0 = t;
                t1 = -1.0;
            }
        }

        var alpha : f32 = 0.0;
        
        if (t0 >= 0.0 && t0 < 1.0) {
            let x : f32 = (a.x*t0 - 2.0*b.x)*t0 + c.x;
            alpha += clamp(x * inverseDiameter + 0.5, 0.0, 1.0);
        }

        if (t1 >= 0.0 && t1 < 1.0) {
            let x = (a.x*t1 - 2.0*b.x)*t1 + c.x;
            alpha -= clamp(x * inverseDiameter + 0.5, 0.0, 1.0);
        }

        return alpha;
    }

    fn rotate(v : vec2f) -> vec2f {
        return vec2f(v.y, -v.x);
    }

    @fragment
    fn fs_main(in : VertexOutput) -> @location(0) vec4f {
        var alpha : f32 = 0.0;

        // Inverse of the diameter of a pixel in uv units for anti-aliasing.
        let inverseDiameter = 1.0 / (antiAliasingWindowSize * fwidth(in.v_uv));

        let glyph = loadGlyph(in.v_buffer_index);
        for (var i : i32 = 0; i < glyph.count; i++) {
            let curve = loadCurve(glyph.start + i);

            let p0 = curve.p0 - in.v_uv;
            let p1 = curve.p1 - in.v_uv;
            let p2 = curve.p2 - in.v_uv;

            alpha += computeCoverage(inverseDiameter.x, p0, p1, p2);
            if (bool(enableSuperSamplingAntiAliasing)) {
                alpha += computeCoverage(inverseDiameter.y, rotate(p0), rotate(p1), rotate(p2));
            }
        }

        if (bool(enableSuperSamplingAntiAliasing)) {
            alpha *= 0.5;
        }

        alpha = clamp(alpha, 0.0, 1.0);
        let result = u_Color * alpha;
        let sample = textureSample(texture_map, texture_sampler, in.v_uv_textbox);

        // alpha test
        if (result.a < 0.001) {
            discard;
        }

        return result * sample;
        // return vec4f(in.v_uv_textbox, 0.0, 1.0);
    }
)glsl";


const char* default_postprocess_shader_string = R"glsl(
    #include SCREEN_PASS_VERTEX_SHADER

    @fragment 
    fn fs_main(in : VertexOutput) -> @location(0) vec4f {
        return vec4f(in.v_uv, 0.0, 1.0);
    }
)glsl";


const char* output_pass_shader_string = R"glsl(
    #include SCREEN_PASS_VERTEX_SHADER

    const TONEMAP_NONE = 0;
    const TONEMAP_LINEAR = 1;
    const TONEMAP_REINHARD = 2;
    const TONEMAP_CINEON = 3;
    const TONEMAP_ACES = 4;
    const TONEMAP_UNCHARTED = 5;

    @group(0) @binding(0) var texture: texture_2d<f32>;
    @group(0) @binding(1) var texture_sampler: sampler;
    @group(0) @binding(2) var<uniform> u_Gamma: f32;
    @group(0) @binding(3) var<uniform> u_Exposure: f32;
    @group(0) @binding(4) var<uniform> u_Tonemap: i32 = TONEMAP_NONE;

    // Helpers ==================================================================
    fn Uncharted2Tonemap(x: vec3<f32>) -> vec3<f32> {
        let A: f32 = 0.15;
        let B: f32 = 0.5;
        let C: f32 = 0.1;
        let D: f32 = 0.2;
        let E: f32 = 0.02;
        let F: f32 = 0.3;
        return (x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F) - E / F;
    } 

    // source: https://github.com/selfshadow/ltc_code/blob/master/webgl/shaders/ltc/ltc_blit.fs
    fn rrt_odt_fit(v: vec3<f32>) -> vec3<f32> {
        let a: vec3<f32> = v * (v + 0.0245786) - 0.000090537;
        let b: vec3<f32> = v * (0.983729 * v + 0.432951) + 0.238081;
        return a / b;
    } 

    fn mat3_from_rows(c0: vec3<f32>, c1: vec3<f32>, c2: vec3<f32>) -> mat3x3<f32> {
        var m: mat3x3<f32> = mat3x3<f32>(c0, c1, c2);
        m = transpose(m);
        return m;
    } 

    // main =====================================================================
    @fragment 
    fn fs_main(in : VertexOutput) -> @location(0) vec4f {
        let hdrColor: vec4<f32> = textureSample(texture, texture_sampler, in.v_uv);
        var color: vec3<f32> = hdrColor.rgb;
        if (u_Tonemap != TONEMAP_NONE) {
            color = color * (u_Exposure);
        }
        switch (u_Tonemap) {
        case 1: { // linear
            color = clamp(color, vec3f(0.), vec3f(1.));
        }
        case 2: { // reinhard
            color = hdrColor.rgb / (hdrColor.rgb + vec3<f32>(1.));
        }
        case 3: { // cineon
            let x: vec3<f32> = max(vec3<f32>(0.), color - 0.004);
            color = x * (6.2 * x + 0.5) / (x * (6.2 * x + 1.7) + 0.06);
            // color = pow(color, vec3<f32>(u_Gamma));
            color = pow(color, vec3<f32>(2.2));  // invert gamma correction (assumes final output to srgb texture)
            // Note: will need to change this if we want to output to linear texture and do gamma correction ourselves
        } 
        case 4: { // aces
            var ACES_INPUT_MAT: mat3x3<f32> = mat3_from_rows(vec3<f32>(0.59719, 0.35458, 0.04823), vec3<f32>(0.076, 0.90834, 0.01566), vec3<f32>(0.0284, 0.13383, 0.83777));
            var ACES_OUTPUT_MAT: mat3x3<f32> = mat3_from_rows(vec3<f32>(1.60475, -0.53108, -0.07367), vec3<f32>(-0.10208, 1.10813, -0.00605), vec3<f32>(-0.00327, -0.07276, 1.07602));
            color = color / 0.6;
            color = ACES_INPUT_MAT * color;
            color = rrt_odt_fit(color);
            color = ACES_OUTPUT_MAT * color;
            color = clamp(color, vec3f(0.), vec3f(1.));
        }
        case 5: { // uncharted
            let ExposureBias: f32 = 2.;
            let curr: vec3<f32> = Uncharted2Tonemap(ExposureBias * color);
            let W: f32 = 11.2;
            let whiteScale: vec3<f32> = vec3<f32>(1. / Uncharted2Tonemap(vec3<f32>(W)));
            color = curr * whiteScale;
        }
        default: {}
        }

        // gamma correction
        // 9/3/2024: assuming swapchain texture is always in srgb format so we DON'T gamma correct,
        // let the final canvas/backbuffer gamma correct for us
        color = pow(color, vec3<f32>(1. / u_Gamma));


        return vec4<f32>(color, 1.0); // how does alpha work?
        // return vec4<f32>(color, clamp(hdrColor.a, 0.0, 1.0));

        // return textureSample(texture, texture_sampler, in.v_uv); // passthrough
    } 



)glsl";


const char* bloom_downsample_screen_shader = R"glsl(

#include SCREEN_PASS_VERTEX_SHADER
@group(0) @binding(0) var u_texture: texture_2d<f32>; // texture at previous mip level
@group(0) @binding(1) var u_sampler: sampler;
@group(0) @binding(2) var<uniform> u_threshold: f32;
@group(0) @binding(3) var<uniform> u_full_res: vec2u; // full resolution of input texture

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    let input_dim = textureDimensions(u_texture).xy;
    let dx = 1.0 / f32(input_dim.x); // change in uv.x that corresponds to 1 pixel in input texture x direction
    let dy = 1.0 / f32(input_dim.y); // change in uv.y that corresponds to 1 pixel in input texture y direction
    let uv = in.v_uv;

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    let a = textureSample(u_texture, u_sampler, vec2f(uv.x - 2.0 * dx, uv.y + 2.0 * dy)).rgb;
    let b = textureSample(u_texture, u_sampler, vec2f(uv.x, uv.y + 2.0 * dy)).rgb;
    let c = textureSample(u_texture, u_sampler, vec2f(uv.x + 2.0 * dx, uv.y + 2.0 * dy)).rgb;

    let d = textureSample(u_texture, u_sampler, vec2f(uv.x - 2.0 * dx, uv.y)).rgb;
    let e = textureSample(u_texture, u_sampler, vec2f(uv.x, uv.y)).rgb;
    let f = textureSample(u_texture, u_sampler, vec2f(uv.x + 2.0 * dx, uv.y)).rgb;

    let g = textureSample(u_texture, u_sampler, vec2f(uv.x - 2.0 * dx, uv.y - 2.0 * dy)).rgb;
    let h = textureSample(u_texture, u_sampler, vec2f(uv.x, uv.y - 2.0 * dy)).rgb;
    let i = textureSample(u_texture, u_sampler, vec2f(uv.x + 2.0 * dx, uv.y - 2.0 * dy)).rgb;

    let j = textureSample(u_texture, u_sampler, vec2f(uv.x - dx, uv.y + dy)).rgb;
    let k = textureSample(u_texture, u_sampler, vec2f(uv.x + dx, uv.y + dy)).rgb;
    let l = textureSample(u_texture, u_sampler, vec2f(uv.x - dx, uv.y - dy)).rgb;
    let m = textureSample(u_texture, u_sampler, vec2f(uv.x + dx, uv.y - dy)).rgb;

    // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a,b,d,e * 0.125
    // b,c,e,f * 0.125
    // d,e,g,h * 0.125
    // e,f,h,i * 0.125
    // j,k,l,m * 0.5
    // This shows 5 square areas that are being sampled. But some of them overlap,
    // so to have an energy preserving downsample we need to make some adjustments.
    // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
    // contribute 0.5 to the final color output. The code below is written
    // to effectively yield this sum. We get:
    // 0.125*5 + 0.03125*4 + 0.0625*4 = 1
    var downsample = vec3f(0.0);
    downsample = e*0.125;
    downsample += (a+c+g+i)*0.03125;
    downsample += (b+d+f+h)*0.0625;
    downsample += (j+k+l+m)*0.125;

    if (all(input_dim == u_full_res)) {
        let brightness = max(max(downsample.r, downsample.g), downsample.b);
        let contribution = max(0.0, brightness - u_threshold) / max(brightness, 0.00001);
        downsample *= contribution;
    }

    return vec4f(downsample, 1.0);
}
)glsl";


const char* bloom_downsample_shader_string = R"glsl(

@group(0) @binding(0) var u_input_texture: texture_2d<f32>;
@group(0) @binding(1) var u_input_tex_sampler: sampler;
@group(0) @binding(2) var u_out_texture: texture_storage_2d<rgba16float, write>; // hdr
@group(0) @binding(3) var<uniform> u_mip_level: i32; 

@compute @workgroup_size(8, 8, 1)
fn main(
    @builtin(global_invocation_id) GlobalInvocationID : vec3<u32>,
    @builtin(workgroup_id) WorkGroupID : vec3<u32>,
    @builtin(local_invocation_index) LocalInvocationIndex : u32, // ranges from 0 - 63
    @builtin(local_invocation_id) LocalInvocationID : vec3<u32>
)
{
    // TODO use same texel size logic in upsample shader
    let output_size = textureDimensions(u_out_texture);
    let input_size = textureDimensions(u_input_texture);
    let pixel_coords = vec2i(GlobalInvocationID.xy);
    let x = 1.0 / f32(input_size.x);
    let y = 1.0 / f32(input_size.y);

    let uv        = (vec2f(GlobalInvocationID.xy) + vec2f(0.5)) / vec2f(output_size.xy);

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    let a = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x - 2.0*x, uv.y + 2.0*y), 0.0).rgb;
    let b = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x,       uv.y + 2.0*y), 0.0).rgb;
    let c = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x + 2.0*x, uv.y + 2.0*y), 0.0).rgb;
    let d = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x - 2.0*x, uv.y), 0.0).rgb;
    let e = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x,       uv.y), 0.0).rgb;
    let f = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x + 2.0*x, uv.y), 0.0).rgb;
    let g = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x - 2.0*x, uv.y - 2.0*y), 0.0).rgb;
    let h = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x,       uv.y - 2.0*y), 0.0).rgb;
    let i = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x + 2.0*x, uv.y - 2.0*y), 0.0).rgb;
    let j = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x - x,   uv.y + y), 0.0).rgb;
    let k = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x + x,   uv.y + y), 0.0).rgb;
    let l = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x - x,   uv.y - y), 0.0).rgb;
    let m = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x + x,   uv.y - y), 0.0).rgb;

    // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a,b,d,e * 0.125
    // b,c,e,f * 0.125
    // d,e,g,h * 0.125
    // e,f,h,i * 0.125
    // j,k,l,m * 0.5
    // This shows 5 square areas that are being sampled. But some of them overlap,
    // so to have an energy preserving downsample we need to make some adjustments.
    // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
    // contribute 0.5 to the final color output. The code below is written
    // to effectively yield this sum. We get:
    // 0.125*5 + 0.03125*4 + 0.0625*4 = 1
    var downsample = vec3f(0.0);
    downsample = e*0.125;
    downsample += (a+c+g+i)*0.03125;
    downsample += (b+d+f+h)*0.0625;
    downsample += (j+k+l+m)*0.125;

    // apply thresholding
    if (u_mip_level == 0) {
    //     // thresholding
    //     float brightness = max3(e);
    //     float contribution = max(0, brightness - u_Threshold) / max (brightness, 0.00001);
    //     downsample *= contribution;
    //     break;
    }

    let alpha = textureSampleLevel(u_input_texture, u_input_tex_sampler, uv, 0.0).a;
	textureStore(u_out_texture, pixel_coords, vec4f(downsample, alpha));
}
)glsl";

const char* bloom_upsample_screen_shader = R"glsl(

#include SCREEN_PASS_VERTEX_SHADER

@group(0) @binding(0) var u_prev_upsample_texture: texture_2d<f32>; // upsample texture at mip i+1 (half-resolution of target)
@group(0) @binding(1) var u_sampler: sampler;
@group(0) @binding(2) var u_curr_downsample_texture: texture_2d<f32>; // downsample texture at same mip/resolution
@group(0) @binding(3) var<uniform> u_full_resolution_size: vec2<u32>;  // used to check if this is last mip level
@group(0) @binding(4) var<uniform> u_internal_blend: f32; // linear blend between mip levels
@group(0) @binding(5) var<uniform> u_final_blend: f32; // linear blend with original image

@fragment
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    let input_dim = textureDimensions(u_curr_downsample_texture);
    let dx = 1.0 / f32(input_dim.x); // change in uv.x that corresponds to 1 pixel in input texture x direction
    let dy = 1.0 / f32(input_dim.y); // change in uv.y that corresponds to 1 pixel in input texture y direction
    let uv = in.v_uv;

    let weights = array(
        0.0625, 0.125, 0.0625,
        0.125,  0.25,  0.125,
        0.0625, 0.125, 0.0625
    );

    var upsampled_color = vec3f(0.0);
    upsampled_color += weights[0] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(-dx, dy)).rgb;
    upsampled_color += weights[1] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(0.0, dy)).rgb;
    upsampled_color += weights[2] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(dx, dy)).rgb;
    upsampled_color += weights[3] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(-dx, 0.0)).rgb;
    upsampled_color += weights[4] * textureSample(u_prev_upsample_texture, u_sampler, uv).rgb;
    upsampled_color += weights[5] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(dx, 0.0)).rgb;
    upsampled_color += weights[6] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(-dx, -dy)).rgb;
    upsampled_color += weights[7] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(0.0, -dy)).rgb;
    upsampled_color += weights[8] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(dx, -dy)).rgb;

    let curr_color = textureSample(u_curr_downsample_texture, u_sampler, uv).rgb;

    if (all(input_dim == u_full_resolution_size)) {
        // upsampled_color = mix(curr_color, upsampled_color, u_final_blend);
        // additive blend on final stage
        upsampled_color = curr_color + upsampled_color * u_final_blend;
    } else {
        upsampled_color = mix(curr_color, upsampled_color, u_internal_blend);
    }

    return vec4f(upsampled_color, 1.0);
}   
)glsl";


const char* bloom_upsample_shader_string = R"glsl(

@group(0) @binding(0) var u_input_texture: texture_2d<f32>; // output_render_texture at mip i
@group(0) @binding(1) var u_sampler: sampler;
@group(0) @binding(2) var u_output_texture: texture_storage_2d<rgba16float, write>; // output_render_texture at mip i - 1
// @group(0) @binding(3) var<uniform> u_mip_level: i32; // doesn't seem mip level affects 1-mip views
@group(0) @binding(3) var<uniform> u_full_resolution_size: vec2<u32>;  // same across all calls
@group(0) @binding(4) var<uniform> u_internal_blend: f32; // linear blend between mip levels
@group(0) @binding(5) var<uniform> u_final_blend: f32; // linear blend with original image
@group(0) @binding(6) var u_downsample_texture: texture_2d<f32>; // mip level i-1 of downsample chain

@compute @workgroup_size(8, 8, 1)
fn main(
    @builtin(global_invocation_id) GlobalInvocationID : vec3<u32>,
    @builtin(workgroup_id) WorkGroupID : vec3<u32>,
    @builtin(local_invocation_index) LocalInvocationIndex : u32, // ranges from 0 - 63
    @builtin(local_invocation_id) LocalInvocationID : vec3<u32>
)
{
    let output_size = textureDimensions(u_output_texture);
	let pixel_coords = vec2<i32>(GlobalInvocationID.xy);

    let u_FilterRadius = 0.001;
    let x = u_FilterRadius;
    let y = u_FilterRadius;

    let uv        = (vec2f(pixel_coords.xy) + vec2f(0.5)) / vec2f(f32(output_size.x), f32(output_size.y));

    let a = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x - x, uv.y + y), 0.0).rgb;
    let b = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x,     uv.y + y), 0.0).rgb;
    let c = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x + x, uv.y + y), 0.0).rgb;
    let d = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x - x, uv.y), 0.0).rgb;
    let e = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x,     uv.y), 0.0).rgb;
    let f = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x + x, uv.y), 0.0).rgb;
    let g = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x - x, uv.y - y), 0.0).rgb;
    let h = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x,     uv.y - y), 0.0).rgb;
    let i = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x + x, uv.y - y), 0.0).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    var bloom = e*4.0;
    bloom += (b+d+f+h)*2.0;
    bloom += (a+c+g+i);
    bloom *= (1.0 / 16.0);

	let curr_pixel = textureLoad(u_downsample_texture, pixel_coords, 0);
    var out_pixel = curr_pixel.rgb;

    // if this is last mip level
    if (all(output_size == u_full_resolution_size)) {
        out_pixel = mix(curr_pixel.rgb, bloom, u_final_blend);
    } else {
        out_pixel = mix(curr_pixel.rgb, bloom, u_internal_blend);
    }

	textureStore(u_output_texture, pixel_coords, vec4f(out_pixel, 1.0));
}

)glsl";

// clang-format on

std::string Shaders_genSource(const char* src)
{
    std::string source(src);
    size_t pos = source.find("#include");
    while (pos != std::string::npos) {
        size_t start            = source.find_first_of(' ', pos);
        size_t end              = source.find_first_of('\n', start + 1);
        std::string includeName = source.substr(start + 1, end - start - 1);
        // strip \r
        includeName.erase(std::remove(includeName.begin(), includeName.end(), '\r'),
                          includeName.end());
        // strip \n
        includeName.erase(std::remove(includeName.begin(), includeName.end(), '\n'),
                          includeName.end());
        // strip ;
        includeName.erase(std::remove(includeName.begin(), includeName.end(), ';'),
                          includeName.end());
        std::string includeSource = shader_table[includeName];
        source.replace(pos, end - pos + 1, includeSource);
        pos = source.find("#include");
    }

    return source;
}
