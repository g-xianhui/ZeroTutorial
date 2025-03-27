#pragma once

const float DEG2RAD = 3.14159265358979323846f / 180.0f;

bool is_point_in_circle(float cx, float cy, float r, float px, float py);
bool is_point_in_sector(float cx, float cy, float ux, float uy, float r, float theta, float px, float py);