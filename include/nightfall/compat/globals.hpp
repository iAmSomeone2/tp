//
// Created by Brenden Davidson on 10/2/26.
//

#pragma once

#include <cmath>
#include <numbers>


#define DEG_TO_RAD(deg) ((deg) * std::numbers::pi_v<float> / 180.f)
#define RAD_TO_DEG(rad) ((rad) * 180.f / std::numbers::pi_v<float>)
