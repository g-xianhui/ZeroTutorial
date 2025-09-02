#pragma once

#include <cmath>

const float DEG2RAD = 3.14159265358979323846f / 180.0f;

struct Vector3f {
    float x = 0;
    float y = 0;
    float z = 0;

    Vector3f operator+(const Vector3f& rhs) const {
        return Vector3f{ x + rhs.x, y + rhs.y, z + rhs.z };
    }

    Vector3f operator-(const Vector3f& rhs) const {
        return Vector3f{ x - rhs.x, y - rhs.y, z - rhs.z };
    }

    float lenght() {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vector3f normalized() const {
        float len = std::sqrt(x * x + y * y + z * z);
        return Vector3f(x / len, y / len, z / len);
    }

    static Vector3f cross(const Vector3f& a, const Vector3f& b) {
        return Vector3f(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

    static float dot(const Vector3f& a, const Vector3f& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
};

inline Vector3f operator*(const Vector3f& v, float rate) {
    return Vector3f{ v.x * rate, v.y * rate, v.z * rate };
}

inline Vector3f operator*(float rate, const Vector3f& v) {
    return Vector3f{ v.x * rate, v.y * rate, v.z * rate };
}

struct Rotation {
    float pitch = 0;
    float yaw = 0;
    float roll = 0;
};

struct Quaternion {
    float w, x, y, z;

    static Quaternion from_rotation_matrix(float m[3][3]) {
        float trace = m[0][0] + m[1][1] + m[2][2];
        if (trace > 0) {
            float s = 0.5f / std::sqrt(trace + 1.0f);
            return Quaternion(
                0.25f / s,
                (m[2][1] - m[1][2]) * s,
                (m[0][2] - m[2][0]) * s,
                (m[1][0] - m[0][1]) * s
            );
        }
        else if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
            float s = 2.0f * std::sqrt(1.0f + m[0][0] - m[1][1] - m[2][2]);
            return Quaternion(
                (m[2][1] - m[1][2]) / s,
                0.25f * s,
                (m[0][1] + m[1][0]) / s,
                (m[0][2] + m[2][0]) / s
            );
        }
        else if (m[1][1] > m[2][2]) {
            float s = 2.0f * std::sqrt(1.0f + m[1][1] - m[0][0] - m[2][2]);
            return Quaternion(
                (m[0][2] - m[2][0]) / s,
                (m[0][1] + m[1][0]) / s,
                0.25f * s,
                (m[1][2] + m[2][1]) / s
            );
        }
        else {
            float s = 2.0f * std::sqrt(1.0f + m[2][2] - m[0][0] - m[1][1]);
            return Quaternion(
                (m[1][0] - m[0][1]) / s,
                (m[0][2] + m[2][0]) / s,
                (m[1][2] + m[2][1]) / s,
                0.25f * s
            );
        }
    }
};

float random_01();

bool is_point_in_circle(float cx, float cy, float r, float px, float py);
bool is_point_in_sector(float cx, float cy, float ux, float uy, float r, float theta, float px, float py);

Rotation quaternion_to_euler(const Quaternion& q);