#include "ScanConfig.hh"
#include <iostream>

std::vector<ScanPoint> scanPoints;
int gStartPoint = 0;
int gEndPoint = -1;
int gCurrentPoint = 0;
const long EVENTS_PER_POINT = 400000;

double ScanConfig::scanX = 0.0;
double ScanConfig::scanY = 0.0;

void InitScanPoints()
{
    scanPoints.clear();

    const int Nx = 501;
    const int Ny = 501;

    scanPoints.reserve(Nx * Ny);

    double xmin = -50.0;
    double xmax =  50.0;

    double ymin = -50.0;
    double ymax =  50.0;

    double dx = (xmax - xmin) / (Nx - 1);
    double dy = (ymax - ymin) / (Ny - 1);

    for (int ix = 0; ix < Nx; ix++)
    {
        double x = xmin + ix * dx;

        for (int iy = 0; iy < Ny; iy++)
        {
            double y = ymin + iy * dy;

            scanPoints.push_back({x, y});
        }
    }

    std::cout << "Total scan points = " << scanPoints.size() << std::endl;
    std::cout << "First point: x = " << scanPoints.front().x
              << ", y = " << scanPoints.front().y << std::endl;
    std::cout << "Last point : x = " << scanPoints.back().x
              << ", y = " << scanPoints.back().y << std::endl;
}