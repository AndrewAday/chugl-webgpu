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
    glm::vec3 dirLight;        // at byte offset 192
    float time;                // at byte offset 204
};

struct MaterialUniforms {
    glm::vec4 color; // at byte offset 0
};
struct DrawUniforms {
    glm::mat4x4 modelMat; // at byte offset 0
};

// clang-format off

static const char* shaderCode = CODE(
    struct FrameUniforms {
        projectionMat: mat4x4f,
        viewMat: mat4x4f,
        projViewMat: mat4x4f,

        // lighting
        dirLight: vec3f,

        time: f32,
    };

    @group(PER_FRAME_GROUP) @binding(0) var<uniform> u_Frame: FrameUniforms;

    struct MaterialUniforms {
        color: vec4f,
    };

    @group(PER_MATERIAL_GROUP) @binding(0) var<uniform> u_Material: MaterialUniforms;
    @group(PER_MATERIAL_GROUP) @binding(1) var u_Texture: texture_2d<f32>;
    @group(PER_MATERIAL_GROUP) @binding(2) var u_Sampler: sampler;


    struct DrawUniforms {
        modelMat: mat4x4f,
    };

    @group(PER_DRAW_GROUP) @binding(0) var<uniform> u_Draw: DrawUniforms;

    struct VertexInput {
        @location(0) position : vec3f,
        @location(1) normal : vec3f,
        @location(2) uv : vec2f,
        // TODO add color, tangent
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
        @location(2) v_uv : vec2f
    };

    @vertex fn vs_main(in : VertexInput) -> VertexOutput
    {
        var out : VertexOutput;

        var worldPos : vec4f = u_Frame.projViewMat * u_Draw.modelMat * vec4f(in.position, 1.0f);
        out.v_worldPos = worldPos.xyz;
        out.v_normal = (u_Draw.modelMat * vec4f(in.normal, 0.0)).xyz;
        out.v_uv     = in.uv;

        // debug clamp z to range [0, 1]
        // worldPos.z = worldPos.z * 0.5 + 0.5;

        out.position = worldPos;
        // out.position = vec4f(in.position, 0.5 + 0.5 * sin(u_Frame.time));
        return out;
    }

    @fragment fn fs_main(in : VertexOutput)->@location(0) vec4f
    {
        // base color
        var color : vec4f = u_Material.color;
        // color.g = 0.5 + 0.5 * sin(u_Frame.time);
        // lambertian diffuse
        let normal = normalize(in.v_normal);
        var lightContrib : f32 = max(0.0, dot(u_Frame.dirLight, -normal));
        // add global ambient
        lightContrib = clamp(lightContrib, 0.2, 1.0);
        
        // texture map
        // let texelCoords = vec2i(in.v_uv * vec2f(textureDimensions(u_Texture)));
        // color *= textureLoad(u_Texture, texelCoords, 0);
        color *= textureSample(u_Texture, u_Sampler, in.v_uv);


        return vec4f(color.rgb * lightContrib, color.a);
        // return vec4f(in.v_normal, 1.0);
        // return vec4f(in.v_uv, 0.0, 1.0);
    }
);

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
        // remap uvs to [0, 1] and flip y
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