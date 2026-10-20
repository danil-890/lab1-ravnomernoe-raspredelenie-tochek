#include "../source/PointCloud.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
double ks(std::vector<double> values) {
    std::sort(values.begin(),values.end());
    double result = 0;
    for (std::size_t i=0; i<values.size(); ++i)
        result = std::max(result,std::max((i+1.0)/values.size()-values[i],values[i]-double(i)/values.size()));
    return result;
}
}
int main(int argc, char** argv) {
    try {
        const std::string output = argc > 1 ? argv[1] : ".";
        std::ofstream summary(output+"/geometry_checks.csv"), binsFile(output+"/uniformity_bins.csv"),
                      samples(output+"/cloud_samples.csv");
        require(bool(summary) && bool(binsFile) && bool(samples),"Output directory");
        summary << "container,count,seed,mean_x,mean_y,mean_z,ks_first,ks_second,ks_third,min_lifetime,max_lifetime\n";
        binsFile << "container,transform,bin,count\n";
        samples << "container,x,y,z,color_index,lifetime_seconds\n";
        summary << std::setprecision(10);
        for (int kind=0; kind<4; ++kind) {
            const auto shape = static_cast<lab1::Container>(kind);
            lab1::Generator generator(23);
            const auto points = generator.generate(100000,shape,lab1::Display::Points);
            require(points.size() == 100000,"Exactly 100000 points");
            std::vector<double> cdf[3];
            double sums[3] = {};
            unsigned minLife=100,maxLife=0,lifetimeBins[100]={},colorBins[255]={};
            for (std::size_t i=0; i<points.size(); ++i) {
                const auto& p = points[i];
                require(lab1::contains(shape,p.position),"Container bounds");
                require(p.colorIndex >= 1 && p.colorIndex <= 255,"255-colour palette");
                require(p.color.r >= 1.0/255 && p.color.r <= 1 && p.color.g >= 1.0/255 &&
                        p.color.g <= 1 && p.color.b >= 1.0/255 && p.color.b <= 1 && p.color.a == 1,"RGBA range");
                require(p.lifetimeSeconds >= 1 && p.lifetimeSeconds <= 100,"Lifetime range");
                require(p.alive(0) && p.alive(p.lifetimeSeconds-0.001) &&
                        !p.alive(p.lifetimeSeconds) && !p.alive(100),"Expiration boundary");
                ++lifetimeBins[p.lifetimeSeconds-1]; ++colorBins[p.colorIndex-1];
                minLife=std::min(minLife,p.lifetimeSeconds); maxLife=std::max(maxLife,p.lifetimeSeconds);
                sums[0]+=p.position.x; sums[1]+=p.position.y; sums[2]+=p.position.z;
                const double x=p.position.x-512.0,y=p.position.y-512.0,z=p.position.z-512.0;
                const double h=(p.position.y-1.0)/1022.0;
                const double azimuth=(std::atan2(z,x)+3.14159265358979323846)/(2*3.14159265358979323846);
                if (shape == lab1::Container::Cube) {
                    cdf[0].push_back((x+511)/1022); cdf[1].push_back(h); cdf[2].push_back((z+511)/1022);
                } else if (shape == lab1::Container::Sphere) {
                    const double r=std::sqrt(x*x+y*y+z*z);
                    cdf[0].push_back(std::pow(r/511,3)); cdf[1].push_back((y/r+1)*0.5); cdf[2].push_back(azimuth);
                } else {
                    const double radius=shape == lab1::Container::Cone ? 511*(1-h) : 511;
                    cdf[0].push_back((x*x+z*z)/(radius*radius));
                    cdf[1].push_back(shape == lab1::Container::Cone ? 1-std::pow(1-h,3) : h);
                    cdf[2].push_back(azimuth);
                }
                if (i<1500) samples << lab1::containerName(shape) << ',' << p.position.x << ',' << p.position.y
                    << ',' << p.position.z << ',' << p.colorIndex << ',' << p.lifetimeSeconds << '\n';
            }
            double statistics[3];
            for (int j=0; j<3; ++j) {
                statistics[j]=ks(cdf[j]);
                require(statistics[j] < 0.012,"Uniformity KS diagnostic");
                unsigned bins[20]={};
                for (double value:cdf[j]) ++bins[std::min(19,int(value*20))];
                for (int b=0; b<20; ++b) binsFile << lab1::containerName(shape) << ',' << j << ',' << b << ',' << bins[b] << '\n';
            }
            for (unsigned count:lifetimeBins) require(count>750 && count<1250,"Lifetime frequencies");
            for (unsigned count:colorBins) require(count>270 && count<520,"Colour frequencies");
            summary << lab1::containerName(shape) << ',' << points.size() << ",23,"
                << sums[0]/points.size() << ',' << sums[1]/points.size() << ',' << sums[2]/points.size()
                << ',' << statistics[0] << ',' << statistics[1] << ',' << statistics[2] << ',' << minLife << ',' << maxLife << '\n';
            lab1::Generator sphereGenerator(23);
            const auto spheres=sphereGenerator.generate(100000,shape,lab1::Display::Spheres);
            unsigned rainbow[7]={};
            for (const auto& p:spheres) {
                require(lab1::contains(shape,p.position),"Sphere centre bounds");
                require(p.radius>=0 && p.radius<=1,"Radius range");
                require(p.position.x-p.radius>=0 && p.position.x+p.radius<=1024 &&
                        p.position.y-p.radius>=0 && p.position.y+p.radius<=1024 &&
                        p.position.z-p.radius>=0 && p.position.z+p.radius<=1024,"Whole sphere in modelling cube");
                require(p.colorIndex>=1 && p.colorIndex<=7,"Seven rainbow colours");
                ++rainbow[p.colorIndex-1];
            }
            for (unsigned count:rainbow) require(count>13500 && count<15000,"Rainbow frequencies");
            lab1::Generator repeat(23);
            const auto same=repeat.generate(100000,shape,lab1::Display::Points);
            for (std::size_t i=0; i<same.size(); ++i)
                require(same[i].position.x == points[i].position.x && same[i].position.y == points[i].position.y &&
                        same[i].position.z == points[i].position.z && same[i].colorIndex == points[i].colorIndex &&
                        same[i].lifetimeSeconds == points[i].lifetimeSeconds,"Reproducible seed");
            std::cout << lab1::containerName(shape) << ": 100000 points + 100000 spheres passed\n";
        }
        lab1::Generator edge(23);
        require(edge.generate(0,lab1::Container::Cube,lab1::Display::Points).empty(),"Empty set");
        bool rejected=false;
        try { edge.generate(1000001,lab1::Container::Cube,lab1::Display::Points); }
        catch (const std::out_of_range&) { rejected=true; }
        require(rejected,"Count limit");
        std::cout << "PASS: bounds, RGBA, radius, lifetime, reproducibility, volume uniformity\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n'; return 1;
    }
}
