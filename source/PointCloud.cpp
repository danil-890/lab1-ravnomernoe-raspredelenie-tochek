#include "PointCloud.h"
#include <cmath>
#include <stdexcept>
namespace lab1 {
bool Particle::alive(double elapsedSeconds) const {
    return elapsedSeconds >= 0.0 && elapsedSeconds < lifetimeSeconds;
}
const char* containerName(Container container) {
    switch (container) {
    case Container::Cube: return "Cube";
    case Container::Sphere: return "Sphere";
    case Container::Cylinder: return "Cylinder";
    case Container::Cone: return "Cone";
    }
    throw std::invalid_argument("Unknown container");
}
bool contains(Container container, const Vec3& p) {
    if (!(p.x >= 1 && p.x <= 1023 && p.y >= 1 && p.y <= 1023 && p.z >= 1 && p.z <= 1023)) return false;
    const double x = p.x-512.0, y = p.y-512.0, z = p.z-512.0;
    switch (container) {
    case Container::Cube: return true;
    case Container::Sphere: return x*x+y*y+z*z <= 511.0*511.0;
    case Container::Cylinder: return x*x+z*z <= 511.0*511.0;
    case Container::Cone: {
        const double radius = (1023.0-p.y)*0.5;
        return x*x+z*z <= radius*radius;
    }
    }
    return false;
}
Color pointColor(unsigned index) {
    if (index < 1 || index > 255) throw std::out_of_range("Palette index");
    // 255 distinct bright colours; each RGB component stays in [1/255,1].
    const double hue = (index-1)*6.0/255.0;
    const int sector = static_cast<int>(hue);
    const float f = static_cast<float>(hue-sector), low = 51.0f/255.0f;
    const float up = low+(1-low)*f, down = 1-(1-low)*f;
    switch (sector) {
    case 0: return Color{1,up,low,1};
    case 1: return Color{down,1,low,1};
    case 2: return Color{low,1,up,1};
    case 3: return Color{low,down,1,1};
    case 4: return Color{up,low,1,1};
    default: return Color{1,low,down,1};
    }
}
Color rainbowColor(unsigned index) {
    static const Color colors[7] = {
        {1,0,0,1}, {1,0.5f,0,1}, {1,1,0,1}, {0,1,0,1},
        {0,0.75f,1,1}, {0,0,1,1}, {0.58f,0,0.83f,1}
    };
    if (index < 1 || index > 7) throw std::out_of_range("Rainbow index");
    return colors[index-1];
}
Generator::Generator(std::uint32_t seed) : engine_(seed) {}
double Generator::unit() {
    // Explicit conversion gives the same sequence with different C++ standard libraries.
    return (static_cast<double>(engine_())+0.5)/4294967296.0;
}
unsigned Generator::integer(unsigned minimum, unsigned maximum) {
    const std::uint64_t range = static_cast<std::uint64_t>(maximum)-minimum+1;
    const std::uint64_t limit = (4294967296ULL/range)*range;
    std::uint32_t value;
    do { value = engine_(); } while (value >= limit);
    return minimum+static_cast<unsigned>(value%range);
}
Vec3 Generator::position(Container container) {
    Vec3 p;
    do {
        p.x = static_cast<float>(1+1022*unit());
        p.y = static_cast<float>(1+1022*unit());
        p.z = static_cast<float>(1+1022*unit());
    } while (!contains(container,p));
    return p;
}
std::vector<Particle> Generator::generate(std::size_t count, Container container, Display display) {
    containerName(container);
    if (count > 1000000) throw std::out_of_range("Maximum count is 1000000");
    std::vector<Particle> result;
    result.reserve(count);
    for (std::size_t i=0; i<count; ++i) {
        Particle p;
        p.position = position(container);
        p.radius = display == Display::Spheres ? static_cast<float>(unit()) : 0;
        p.colorIndex = integer(1,display == Display::Spheres ? 7 : 255);
        p.color = display == Display::Spheres ? rainbowColor(p.colorIndex) : pointColor(p.colorIndex);
        p.lifetimeSeconds = integer(1,100);
        result.push_back(p);
    }
    return result;
}
}
