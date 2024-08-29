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

// enum VertexAttributeLocations {
//     POSITION = 0,
//     NORMAL   = 1,
//     UV       = 2,
//     COLOR    = 3,
// };

struct FrameUniforms {
    glm::mat4x4 projectionMat; // at byte offset 0
    glm::mat4x4 viewMat;       // at byte offset 64
    glm::mat4x4 projViewMat;   // at byte offset 128
    glm::vec3 camPos;          // at byte offset 192
    f32 _pad0;
    glm::vec3 dirLight; // at byte offset 208
    f32 time;           // at byte offset 220
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

struct DrawUniforms {
    glm::mat4x4 modelMat; // at byte offset 0
};

// clang-format off

static std::unordered_map<std::string, std::string> shader_table = {
    {
        "FRAME_UNIFORMS", 
        R"glsl(

        struct FrameUniforms {
            projectionMat: mat4x4f,
            viewMat: mat4x4f,
            projViewMat: mat4x4f,
            camPos: vec3f, // camera
            dirLight: vec3f, // lighting
            time: f32,
        };

        @group(0) @binding(0) var<uniform> u_Frame: FrameUniforms;

        )glsl"
    },

    {
        "DRAW_UNIFORMS", 
        R"glsl(

        struct DrawUniforms {
            modelMat: mat4x4f,
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
    return flat_color;
}
)glsl";

static const char* lines2d_shader_string  = R"glsl(

#include FRAME_UNIFORMS

// our custom material uniforms
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
        u_Draw.modelMat[0].xyz,
        u_Draw.modelMat[1].xyz,
        u_Draw.modelMat[2].xyz
    );

    var worldPos : vec4f = u_Frame.projViewMat * u_Draw.modelMat * vec4f(calculate_line_pos(vertex_id), 0.0f, 1.0f);
    out.v_worldPos = worldPos.xyz;
    out.v_normal = (u_Draw.modelMat * vec4f(0.0, 0.0, 1.0, 0.0)).xyz;
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
    var normal : vec3f;
    if (is_front) {
        normal = in.v_normal;
    } else {
        normal = -in.v_normal;
    }

    var diffuse : f32 = 0.5 * dot(normal, -u_Frame.dirLight) + 0.5;
    diffuse = diffuse * diffuse;
    return vec4f(vec3f(diffuse), 1.0);
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
        out.position = u_Frame.projViewMat * u_Draw.modelMat * vec4f(in.position, 0.0f, 1.0f);
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
    @group(0) @binding(2) var<uniform> u_Gamma: f32 = 2.2;
    @group(0) @binding(3) var<uniform> u_Exposure: f32 = 1.;
    @group(0) @binding(4) var<uniform> u_Tonemap: i32 = TONEMAP_REINHARD;

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
        // return textureSample(texture, texture_sampler, in.v_uv);
        // return vec4f(in.v_uv, 0.0, 1.0);

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
            color = pow(color, vec3<f32>(u_Gamma));
        } 
        case 4: { // aces
            let ACES_INPUT_MAT: mat3x3<f32> = mat3_from_rows(vec3<f32>(0.59719, 0.35458, 0.04823), vec3<f32>(0.076, 0.90834, 0.01566), vec3<f32>(0.0284, 0.13383, 0.83777));
            let ACES_OUTPUT_MAT: mat3x3<f32> = mat3_from_rows(vec3<f32>(1.60475, -0.53108, -0.07367), vec3<f32>(-0.10208, 1.10813, -0.00605), vec3<f32>(-0.00327, -0.07276, 1.07602));
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

        color = pow(color, vec3<f32>(1. / u_Gamma));
        // return vec4<f32>(color, 1); // how does alpha work?
        // return vec4<f32>(color, hdrColor.a);
        return hdrColor;
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