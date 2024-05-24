#pragma once

#include <glm/glm.hpp>

#include "core/macros.h"

#define PER_FRAME_GROUP 0
#define PER_MATERIAL_GROUP 1
#define PER_DRAW_GROUP 2

#define VS_ENTRY_POINT "vs_main"
#define FS_ENTRY_POINT "fs_main"

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

struct MaterialUniforms {
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

static const char* shaderCode = R"glsl(
    struct FrameUniforms {
        projectionMat: mat4x4f,
        viewMat: mat4x4f,
        projViewMat: mat4x4f,

        // camera
        camPos: vec3f,

        // lighting
        dirLight: vec3f,

        time: f32,
    };

    @group(0) @binding(0) var<uniform> u_Frame: FrameUniforms;

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

    struct DrawUniforms {
        modelMat: mat4x4f,
        // normalMat: mat4x4f,  // needed to account for non-uniform scaling
    };

    @group(2) @binding(0) var<storage> drawInstances: array<DrawUniforms>;
    // @group(PER_DRAW_GROUP) @binding(0) var<uniform> u_Draw: DrawUniforms;

    struct VertexInput {
        @location(0) position : vec3f,
        @location(1) normal : vec3f,
        @location(2) uv : vec2f,
        @location(3) tangent : vec4f,
        @builtin(instance_index) instance : u32,
        // TODO add color
    };

    /**
     * A structure with fields labeled with builtins and locations can also be used
     * as *output* of the vertex shader, which is also the input of the fragment
     * shader.
     */
    struct VertexOutput {
        @builtin(position) position : vec4f,
        // The location here does not refer to a vertex attribute, it
        // just means that this field must be handled by the
        // rasterizer. (It can also refer to another field of another
        // struct that would be used as input to the fragment shader.)
        @location(0) v_worldPos : vec3f,
        @location(1) v_normal : vec3f,
        @location(2) v_uv : vec2f,
        @location(3) v_tangent : vec4f,
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
        // handle non-uniform scaling

        // TODO: restore to normal Mat. need to pass normal mat from cpu side
        // out.v_normal = (u_Frame.viewMat * u_Draw.normalMat * vec4f(in.normal, 0.0)).xyz;
        out.v_normal = (u_Draw.modelMat * vec4f(in.normal, 0.0)).xyz;
        // tangent vectors aren't impacted by non-uniform scaling or translation
        out.v_tangent = vec4f(modelMat3 * in.tangent.xyz, in.tangent.w);
        out.v_uv     = in.uv;
        out.position = worldPos;

        return out;
    }

    fn calculateNormal(inNormal: vec3f, inUV : vec2f, inTangent: vec4f, scale: f32) -> vec3f {
        var tangentNormal : vec3f = textureSample(normalMap, normalSampler, inUV).rgb * 2.0 - 1.0;
        // scale normal
        // ref: https://github.com/mrdoob/three.js/blob/dev/src/renderers/shaders/ShaderChunk/normal_fragment_maps.glsl.js
        tangentNormal.x *= scale;
        tangentNormal.y *= scale;

        // TODO: account for side of face (can we calculate facenormal in frag shader?)
        // e.g. tangentNormal *= (sign(dot(normal, faceNormal)))

        // from mikkt:
        // For normal maps it is sufficient to use the following simplified version
        // of the bitangent which is generated at pixel/vertex level. 
        // bitangent = fSign * cross(vN, tangent);

        let N : vec3f = normalize(inNormal);
        let T : vec3f = normalize(inTangent.xyz);
        let B : vec3f = inTangent.w * normalize(cross(N, T));
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
    fn G_SchlicksmithGGX(dotNL : f32, dotNV : f32, roughness : f32) -> f32 {
        let r : f32 = (roughness + 1.0);
        let k : f32 = (r*r) / 8.0;
        let GL : f32 = dotNL / (dotNL * (1.0 - k) + k);
        let GV : f32 = dotNV / (dotNV * (1.0 - k) + k);
        return GL * GV;
    }

    // Fresnel function ----------------------------------------------------------
    // cosTheta assumed to be in range [0, 1]
    fn F_Schlick(cosTheta : f32, F0 : vec3<f32>) -> vec3f {
        return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
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
    fn fs_main(in : VertexOutput)->@location(0) vec4f
    {
        let N : vec3f = calculateNormal(in.v_normal, in.v_uv, in.v_tangent, u_Material.normalFactor);
        let V : vec3f = normalize(u_Frame.camPos - in.v_worldPos);

        // linear-space albedo (normally authored in sRGB space so we have to convert to linear space)
        // transparency not supported
        let albedo : vec3f = u_Material.baseColor.rgb * pow(textureSample(albedoMap, albedoSampler, in.v_uv).rgb, vec3f(2.2));

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
                let G : f32 = G_SchlicksmithGGX(dotNL, dotNV, roughness);
                // F = Fresnel factor (Reflectance depending on angle of incidence)
                let F : vec3<f32> = F_Schlick(dotNV, F0);

                // specular contribution
                let spec : vec3f = D * F * G / (4.0 * dotNL * dotNV + 0.001);
                // diffuse contribution
                let kD : vec3f = (vec3f(1.0) - F) * (1.0 - metallic);
                colorContrib += (kD * albedo / PI + spec) * dotNL * radiance;
            }
            Lo += colorContrib;
        // }  // end light loop

        // ambient occlusion (hardcoded for now)
        let ambient : vec3f = vec3f(0.03) * albedo * textureSample(aoMap, aoSampler, in.v_uv).r * u_Material.aoFactor;
        var finalColor : vec3f = Lo + ambient;  // TODO: can factor albedo out and multiply here instead

        // tone mapping
        let exposure : f32 = 1.0;
        let whiteScale : vec3f = 1.0 / Uncharted2Tonemap(vec3f(11.2f));
        finalColor = Uncharted2Tonemap(finalColor * exposure) * whiteScale;

        // finalColor = finalColor / (finalColor + vec3f(1.0));  // reinhard tone mapping

        // gamma correction
        finalColor = pow(finalColor, vec3f(1.0 / 2.2)); // convert back to sRGB

        // color.g = 0.5 + 0.5 * sin(u_Frame.time);
        // lambertian diffuse
        // let normal = normalize(in.v_normal);
        // var lightContrib : f32 = max(0.0, dot(u_Frame.dirLight, -normal));
        // add global ambient
        // lightContrib = clamp(lightContrib, 0.2, 1.0);

        return vec4f(finalColor, u_Material.baseColor.a);
        // return vec4f(
        //     ambient, u_Material.baseColor.a);
        // return vec4f(in.v_normal, 1.0);
        // return vec4f(in.v_uv, 0.0, 1.0);
        // return vec4f(1.0, 0.0, 0.0, 1.0);
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

// clang-format on