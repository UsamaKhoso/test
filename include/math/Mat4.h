#pragma once

#include <array>
#include <cmath>

#include "Vec3.h"

struct Mat4 {
    std::array<float, 16> m{};

    Mat4() {
        for (int i = 0; i < 16; ++i) {
            m[i] = 0.0f;
        }
    }

    static Mat4 identity() {
        Mat4 mat;
        mat.m[0] = mat.m[5] = mat.m[10] = mat.m[15] = 1.0f;
        return mat;
    }

    const float* data() const {
        return m.data();
    }

    float* data() {
        return m.data();
    }

    float& operator()(int row, int col) {
        return m[col * 4 + row];
    }

    const float& operator()(int row, int col) const {
        return m[col * 4 + row];
    }
};

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 result;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a(row, k) * b(k, col);
            }
            result(row, col) = sum;
        }
    }
    return result;
}

inline Mat4 translate(const Mat4& mat, float x, float y, float z) {
    Mat4 result = mat;
    result(0, 3) += mat(0, 0) * x + mat(0, 1) * y + mat(0, 2) * z;
    result(1, 3) += mat(1, 0) * x + mat(1, 1) * y + mat(1, 2) * z;
    result(2, 3) += mat(2, 0) * x + mat(2, 1) * y + mat(2, 2) * z;
    result(3, 3) += mat(3, 0) * x + mat(3, 1) * y + mat(3, 2) * z;
    return result;
}

inline Mat4 rotateY(float angleRadians) {
    Mat4 mat = Mat4::identity();
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    mat(0, 0) = c;
    mat(0, 2) = s;
    mat(2, 0) = -s;
    mat(2, 2) = c;
    return mat;
}

inline Mat4 rotateX(float angleRadians) {
    Mat4 mat = Mat4::identity();
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);
    mat(1, 1) = c;
    mat(1, 2) = -s;
    mat(2, 1) = s;
    mat(2, 2) = c;
    return mat;
}

inline Mat4 perspective(float fovRadians, float aspect, float nearPlane, float farPlane) {
    Mat4 mat;
    float f = 1.0f / std::tan(fovRadians / 2.0f);
    mat(0, 0) = f / aspect;
    mat(1, 1) = f;
    mat(2, 2) = (farPlane + nearPlane) / (nearPlane - farPlane);
    mat(2, 3) = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    mat(3, 2) = -1.0f;
    return mat;
}

inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = normalize(center - eye);
    Vec3 s = normalize(cross(f, up));
    Vec3 u = cross(s, f);

    Mat4 mat = Mat4::identity();
    mat(0, 0) = s.x;
    mat(1, 0) = s.y;
    mat(2, 0) = s.z;

    mat(0, 1) = u.x;
    mat(1, 1) = u.y;
    mat(2, 1) = u.z;

    mat(0, 2) = -f.x;
    mat(1, 2) = -f.y;
    mat(2, 2) = -f.z;

    mat(3, 0) = -dot(s, eye);
    mat(3, 1) = -dot(u, eye);
    mat(3, 2) = dot(f, eye);
    mat(3, 3) = 1.0f;

    return mat;
}

