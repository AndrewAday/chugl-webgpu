#include <chuck/chugin.h>

#include "sg_component.h"

static t_CKUINT sampler_offset_wrapU     = 0;
static t_CKUINT sampler_offset_wrapV     = 0;
static t_CKUINT sampler_offset_wrapW     = 0;
static t_CKUINT sampler_offset_filterMin = 0;
static t_CKUINT sampler_offset_filterMag = 0;
static t_CKUINT sampler_offset_filterMip = 0;

CK_DLL_CTOR(sampler_ctor);

static void ulib_texture_query(Chuck_DL_Query* QUERY)
{
    { // Sampler (only passed by value)
        QUERY->begin_class(QUERY, "TextureSampler", "Object");

        // static vars
        static t_CKINT WRAP_REPEAT    = SG_SAMPLER_WRAP_REPEAT;
        static t_CKINT WRAP_MIRROR    = SG_SAMPLER_WRAP_MIRROR_REPEAT;
        static t_CKINT WRAP_CLAMP     = SG_SAMPLER_WRAP_CLAMP_TO_EDGE;
        static t_CKINT FILTER_NEAREST = SG_SAMPLER_FILTER_NEAREST;
        static t_CKINT FILTER_LINEAR  = SG_SAMPLER_FILTER_LINEAR;
        QUERY->add_svar(QUERY, "int", "WRAP_REPEAT", true, &WRAP_REPEAT);
        QUERY->add_svar(QUERY, "int", "WRAP_MIRROR", true, &WRAP_MIRROR);
        QUERY->add_svar(QUERY, "int", "WRAP_CLAMP", true, &WRAP_CLAMP);
        QUERY->add_svar(QUERY, "int", "FILTER_NEAREST", true, &FILTER_NEAREST);
        QUERY->add_svar(QUERY, "int", "FILTER_LINEAR", true, &FILTER_LINEAR);

        // member vars
        sampler_offset_wrapU = QUERY->add_mvar(QUERY, "int", "wrapU", false);
        sampler_offset_wrapV = QUERY->add_mvar(QUERY, "int", "wrapV", false);
        sampler_offset_wrapW = QUERY->add_mvar(QUERY, "int", "wrapW", false);
        sampler_offset_filterMin
          = QUERY->add_mvar(QUERY, "int", "filterMin", false);
        sampler_offset_filterMag
          = QUERY->add_mvar(QUERY, "int", "filterMag", false);
        sampler_offset_filterMip
          = QUERY->add_mvar(QUERY, "int", "filterMip", false);

        // constructor
        QUERY->add_ctor(QUERY, sampler_ctor); // default constructor

        QUERY->end_class(QUERY); // Sampler
    }
}

CK_DLL_CTOR(sampler_ctor)
{
    // default to repeat wrapping and linear filtering
    OBJ_MEMBER_INT(SELF, sampler_offset_wrapU)     = SG_SAMPLER_WRAP_REPEAT;
    OBJ_MEMBER_INT(SELF, sampler_offset_wrapV)     = SG_SAMPLER_WRAP_REPEAT;
    OBJ_MEMBER_INT(SELF, sampler_offset_wrapW)     = SG_SAMPLER_WRAP_REPEAT;
    OBJ_MEMBER_INT(SELF, sampler_offset_filterMin) = SG_SAMPLER_FILTER_LINEAR;
    OBJ_MEMBER_INT(SELF, sampler_offset_filterMag) = SG_SAMPLER_FILTER_LINEAR;
    OBJ_MEMBER_INT(SELF, sampler_offset_filterMip) = SG_SAMPLER_FILTER_LINEAR;
}