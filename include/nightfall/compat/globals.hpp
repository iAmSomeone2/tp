//
// Created by Brenden Davidson on 10/2/26.
//

#pragma once

#include <cmath>
#include <numbers>


#define DEG_TO_RAD(deg) ((deg) * std::numbers::pi_v<float> / 180.f)
#define RAD_TO_DEG(rad) ((rad) * 180.f / std::numbers::pi_v<float>)

// Support for Disc-derived asset headers
#include <dolphin/gx/GXEnum.h>
#define U32_AS_U8(v) (u8)(((v) >> 24) & 0xFF), (u8)(((v) >> 16) & 0xFF), (u8)(((v) >> 8) & 0xFF), (u8)(((v) >> 0) & 0xFF)
#define U24_AS_U8(v) (u8)(((v) >> 16) & 0xFF), (u8)(((v) >> 8) & 0xFF), (u8)(((v) >> 0) & 0xFF)
#define U16_AS_U8(v) (u8)(((v) >> 8) & 0xFF), (u8)(((v) >> 0) & 0xFF)
#define IMAGE_ADDR(addr) (uintptr_t)(addr) >> 5
#define LOAD_BP_REG(reg, value) GX_CMD_LOAD_BP_REG, reg, U24_AS_U8(value)
#define LOAD_XF_REG(reg, num_args, ...) GX_CMD_LOAD_XF_REG, U16_AS_U8(num_args-1), U16_AS_U8(reg), __VA_ARGS__
#define LOAD_CP_REG(reg, value) GX_CMD_LOAD_CP_REG, reg, U32_AS_U8(value)

