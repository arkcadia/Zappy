#pragma once

#include <format>

template <typename T>
class Vector2 {
public:
    constexpr Vector2() = default;

    constexpr Vector2(T x, T y): x(x), y(y) {}

    T x;
    T y;
};

template <typename T>
constexpr Vector2<T> operator-(Vector2<T> right)
{
    return Vector2<T>(-right.x, -right.y);
}

template <typename T>
constexpr Vector2<T>& operator+=(Vector2<T>& left, Vector2<T> right)
{
    left.x += right.x;
    left.y += right.y;

    return left;
}

template <typename T>
constexpr Vector2<T>& operator-=(Vector2<T>& left, Vector2<T> right)
{
    left.x -= right.x;
    left.y -= right.y;

    return left;
}

template <typename T>
constexpr Vector2<T> operator+(Vector2<T> left, Vector2<T> right)
{
    return Vector2<T>(left.x + right.x, left.y + right.y);
}

template <typename T>
constexpr Vector2<T> operator-(Vector2<T> left, Vector2<T> right)
{
    return Vector2<T>(left.x - right.x, left.y - right.y);
}

template <typename T>
constexpr Vector2<T> operator*(Vector2<T> left, T right)
{
    return Vector2<T>(left.x * right, left.y * right);
}

template <typename T>
constexpr Vector2<T> operator*(T left, Vector2<T> right)
{
    return Vector2<T>(right.x * left, right.y * left);
}

template <typename T>
constexpr Vector2<T>& operator*=(Vector2<T>& left, T right)
{
    left.x *= right;
    left.y *= right;

    return left;
}

template <typename T>
constexpr Vector2<T> operator/(Vector2<T> left, T right)
{
    assert(right != 0 && "Vector2::operator/ cannot divide by 0");
    return Vector2<T>(left.x / right, left.y / right);
}

template <typename T>
[[nodiscard]] constexpr Vector2<T> operator%(Vector2<T> left, Vector2<T> right)
{
    return Vector2<T>(
        ((left.x % right.x) + right.x) % right.x,
        ((left.y % right.y) + right.y) % right.y
    );
}

template <typename T>
constexpr Vector2<T>& operator/=(Vector2<T>& left, T right)
{
    assert(right != 0 && "Vector2::operator/= cannot divide by 0");
    left.x /= right;
    left.y /= right;

    return left;
}

template <typename T>
constexpr bool operator==(Vector2<T> left, Vector2<T> right)
{
    return (left.x == right.x) && (left.y == right.y);
}

template <typename T>
constexpr bool operator!=(Vector2<T> left, Vector2<T> right)
{
    return !(left == right);
}

template <typename T>
struct std::formatter<Vector2<T>> {
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const Vector2<T>& v, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "({}, {})", v.x, v.y);
    }
};

using Vector2i = Vector2<int>;
using Vector2f = Vector2<float>;

using Point = Vector2i;
