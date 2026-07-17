#ifndef SCAN_CONFIG_HH
#define SCAN_CONFIG_HH

#pragma once
#include <vector>

class ScanConfig
{
public:
    static double scanX;
    static double scanY;
};

struct ScanPoint{
    double x;
    double y;
};

extern std::vector<ScanPoint> scanPoints;

extern int gStartPoint;
extern int gEndPoint;

extern int gCurrentPoint;
extern const long EVENTS_PER_POINT;

void InitScanPoints();

#endif