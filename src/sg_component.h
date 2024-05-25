#pragma once

#include "core/macros.h"

#include <iostream> // std::string

typedef u64 SG_ID;

enum SG_ComponentType {
    SG_COMPONENT_INVALID,
    SG_COMPONENT_TRANSFORM,
    SG_COMPONENT_GEOMETRY,
    SG_COMPONENT_MATERIAL,
    SG_COMPONENT_TEXTURE,
    SG_COMPONENT_COUNT
};

struct SG_Component {
    SG_ID id;
    SG_ComponentType type;
    std::string name; // TODO move off std::string
};

// ============================================================================
// SG_Sampler
// ============================================================================

enum SG_Sampler_WrapMode : u8 {
    SG_SAMPLER_WRAP_REPEAT        = 0,
    SG_SAMPLER_WRAP_MIRROR_REPEAT = 1,
    SG_SAMPLER_WRAP_CLAMP_TO_EDGE = 2,
};

enum SG_Sampler_FilterMode : u8 {
    SG_SAMPLER_FILTER_NEAREST = 0,
    SG_SAMPLER_FILTER_LINEAR  = 1,
};

struct SG_Sampler {
    SG_Sampler_WrapMode wrapU, wrapV, wrapW;
    SG_Sampler_FilterMode filterMin, filterMag, filterMip;
};

static SG_Sampler SG_SAMPLER_DEFAULT // make this a #define instead?
  = { SG_SAMPLER_WRAP_REPEAT,   SG_SAMPLER_WRAP_REPEAT,
      SG_SAMPLER_WRAP_REPEAT,   SG_SAMPLER_FILTER_LINEAR,
      SG_SAMPLER_FILTER_LINEAR, SG_SAMPLER_FILTER_LINEAR };