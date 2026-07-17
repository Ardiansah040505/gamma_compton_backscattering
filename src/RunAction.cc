#include "RunAction.hh"

#include "G4Run.hh"

#include "ScanConfig.hh"

#include <sstream>

// EVENTS_PER_POINT is now declared in ScanConfig.hh

// ======================================
RunAction::RunAction()
{}

// ======================================
RunAction::~RunAction()
{
    if(fOutFile.is_open())
    {
        fOutFile.close();
    }
}

// ======================================
void RunAction::BeginOfRunAction(
    const G4Run*)
{
    fResults.clear();

    // Hanya buat file baru dan tulis header jika ini titik awal pemindaian
    if(gCurrentPoint == gStartPoint)
    {
        std::stringstream fname;
        fname << "scan_results_"
              << gStartPoint
              << "_"
              << gEndPoint
              << ".csv";

        fOutFile.open(fname.str(), std::ios::out);
        if(fOutFile.is_open())
        {
            fOutFile << "x,y,nPrimary,roiCounts,normalizedCounts,flux,normalizedFlux\n";
            fOutFile.close();
        }
    }

    G4cout << "====================================\n";
    G4cout << "START POINT RUN: " << gCurrentPoint << "\n";
    G4cout << "====================================\n";
}

// ======================================
void RunAction::AddDetectorData(
    long /*eventID*/,
    int detectorID,
    int counts,
    double edep,
    int flux)
{
    int pointIndex = gCurrentPoint;

    if(pointIndex >= (int)scanPoints.size())
        return;

    auto key =
        std::make_pair(
            pointIndex,
            detectorID
        );

    fResults[key].counts += counts;
    fResults[key].edep += edep;
    fResults[key].flux += flux;
}

// ======================================
void RunAction::EndOfRunAction(
    const G4Run*)
{
    const int nDetectors = 6;
    int pointIndex = gCurrentPoint;

    if(pointIndex >= (int)scanPoints.size())
        return;

    ScanPoint p = scanPoints[pointIndex];

    long totalCounts = 0;
    long totalFlux = 0;

    for(int detectorID = 0;
        detectorID < nDetectors;
        detectorID++)
    {
        auto key =
            std::make_pair(
                pointIndex,
                detectorID
            );

        if(fResults.count(key))
        {
            totalCounts += fResults[key].counts;
            totalFlux += fResults[key].flux;
        }
    }

    double normalizedCounts =
        (double)totalCounts /
        EVENTS_PER_POINT;

    double normalizedFlux =
        (double)totalFlux /
        EVENTS_PER_POINT;

    std::stringstream fname;
    fname << "scan_results_"
          << gStartPoint
          << "_"
          << gEndPoint
          << ".csv";

    // Buka file dalam mode append
    fOutFile.open(fname.str(), std::ios::app);
    if(fOutFile.is_open())
    {
        fOutFile
            << p.x << ","
            << p.y << ","
            << EVENTS_PER_POINT << ","
            << totalCounts << ","
            << normalizedCounts << ","
            << totalFlux << ","
            << normalizedFlux
            << "\n";
        fOutFile.close();
    }

    G4cout << "====================================\n";
    G4cout << "POINT " << pointIndex << " (x = " << p.x << ", y = " << p.y << ") SAVED TO CSV\n";
    G4cout << "====================================\n";
}
