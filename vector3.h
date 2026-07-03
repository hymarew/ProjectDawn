#pragma once
#include <cmath>

/**
 * @brief 3次元ベクトルを扱うクラス
 * ゲーム3D P.64より
 */
class Vector3
{
public:
    float x, y, z;

    // --- コンストラクタ ---
    constexpr Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
    constexpr Vector3(const Vector3& a) : x(a.x), y(a.y), z(a.z) {}
    constexpr Vector3(float nx, float ny, float nz) : x(nx), y(ny), z(nz) {}

    // --- オブジェクト操作 ---
    // 代入演算子
    Vector3& operator=(const Vector3& a) {
        x = a.x; y = a.y; z = a.z;
        return *this;
    }

    // 比較演算子
    bool operator == (const Vector3& a) const { return x == a.x && y == a.y && z == a.z; }
    bool operator != (const Vector3& a) const { return x != a.x || y != a.y || z != a.z; }

    // --- ベクトル操作 ---
    void zero() { x = y = z = 0.0f; } // ゼロベクトル化

    // 単項マイナス（ベクトルの反転）
    Vector3 operator-() const { return Vector3(-x, -y, -z); }

    // 加減算
    Vector3 operator+(const Vector3& a) const { return Vector3(x + a.x, y + a.y, z + a.z); }
    Vector3 operator-(const Vector3& a) const { return Vector3(x - a.x, y - a.y, z - a.z); }

    // スカラー乗算（ベクトル * 数値）
    Vector3 operator*(float a) const { return Vector3(x * a, y * a, z * a); }

    // スカラー除算（ベクトル / 数値）
    Vector3 operator/(float a) const {
        float inv = 1.0f / a;
        return Vector3(x * inv, y * inv, z * inv);
    }

    // 複合代入演算子
    Vector3& operator+=(const Vector3& a) { x += a.x; y += a.y; z += a.z; return *this; }
    Vector3& operator-=(const Vector3& a) { x -= a.x; y -= a.y; z -= a.z; return *this; }
    Vector3& operator*=(float a) { x *= a; y *= a; z *= a; return *this; }
    Vector3& operator/=(float a) { float inv = 1.0f / a; x *= inv; y *= inv; z *= inv; return *this; }

    // 正規化（単位ベクトル化）
    void normalize() {
        float magSq = x * x + y * y + z * z;
        if (magSq > 0.0f) {
            float invMag = 1.0f / std::sqrt(magSq);
            x *= invMag; y *= invMag; z *= invMag;
        }
    }

    // 内積演算（ベクトル * ベクトル）
    float operator *(const Vector3& a) const {
        return x * a.x + y * a.y + z * a.z;
    }

    /**
     * @brief 外積（クロス積）を計算する
     * * 自身(this)と引数(a)の両方に垂直なベクトルを返します。
     * 右手系座標系において、結果のベクトルは面に対して「法線」となる方向を指します。
     *
     * @param a 演算対象のベクトル
     * @return 自身と a の外積結果としての新しい Vector3
     */
    //Vector3 cross(const Vector3& a) const {
    //    return Vector3(
    //        y * a.z - z * a.y, // x成分 = y*az - z*ay
    //        z * a.x - x * a.z, // y成分 = z*ax - x*az
    //        x * a.y - y * a.x  // z成分 = x*ay - y*ax
    //    );
    //}

    // 内積
    static float dot(Vector3& a, Vector3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vector3 corss(Vector3& a, Vector3& b)
    {
        return Vector3(
            a.y * b.z - a.z * b.y, // x成分
            a.z * b.x - a.x * b.z, // y成分
            a.x * b.y - a.y * b.x  // z成分
        );
    }
    
    float Length() const   { return sqrtf(x * x + y * y + z * z); }
    float LengthSq() const { return x * x + y * y + z * z; }  // sqrt なし高速版

    // Y軸まわりに angle ラジアン回転したベクトルを返す（Y成分は不変）
    Vector3 RotatedAroundY(float angle) const
    {
        float s = sinf(angle);
        float c = cosf(angle);
        return { x * c + z * s, y, -x * s + z * c };
    }
};