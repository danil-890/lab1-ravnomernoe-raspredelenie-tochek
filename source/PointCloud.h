#ifndef POINT_CLOUD_H
#define POINT_CLOUD_H
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>
namespace lab1 {
enum class Container { Cube, Sphere, Cylinder, Cone };
enum class Display { Points, Spheres };
struct Vec3 { float x, y, z; };
struct Color { float r, g, b, a; };
struct Particle {
    Vec3 position;
    Color color;
    float radius;
    unsigned colorIndex;
    unsigned lifetimeSeconds;
    bool alive(double elapsedSeconds) const;
};
const char* containerName(Container container);
bool contains(Container container, const Vec3& point);
Color pointColor(unsigned index);
Color rainbowColor(unsigned index);
class Generator {
public:
    explicit Generator(std::uint32_t seed);
    std::vector<Particle> generate(std::size_t count, Container container, Display display);
private:
    std::mt19937 engine_;
    double unit();
    unsigned integer(unsigned minimum, unsigned maximum);
    Vec3 position(Container container);
};
}
#endif
