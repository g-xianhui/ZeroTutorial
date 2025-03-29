#include "math_utils.h"
#include <math.h>
#include <random>

float random_01()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    return dis(gen);
}

bool is_point_in_circle(float cx, float cy, float r, float px, float py)
{
    // D = P - C
    float dx = px - cx;
    float dy = py - cy;

    if (dx * dx + dy * dy > r * r)
        return false;
    return true;
}

bool is_point_in_sector(float cx, float cy, float ux, float uy, float r, float theta, float px, float py)
{
    // D = P - C
    float dx = px - cx;
    float dy = py - cy;

    // |D| = (dx^2 + dy^2)^0.5
    float length = sqrtf(dx * dx + dy * dy);

    // |D| > r
    if (length > r)
        return false;

    // Normalize D
    dx /= length;
    dy /= length;

    // acos(D dot U) < theta
    return acos(dx * ux + dy * uy) < theta;
}
