#include "RunAction.hh"

#include "G4Run.hh"

#include "ScanConfig.hh"

#include <sstream>

RunAction::RunAction()
{}

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

    if(!fOutFile.is_open())
    {
        std::stringstream fname;

        fname << "scan_results_"
              << gStartPoint
              << "_"
              << gEndPoint
              << ".csv";

        fOutFile.open(fname.str());

        fOutFile
            << "x,y,detectorID,nPrimary,roiCounts,normalizedCounts,flux,normalizedFlux\n";
    }

    G4cout << "====================================\n";
    G4cout << "RUN FOR POINT " << gCurrentPoint << " STARTED\n";
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
    const long EVENTS_PER_POINT = 100000;

    int pointIndex = gCurrentPoint;

    if(pointIndex >= 0 && pointIndex < (int)scanPoints.size())
    {
        ScanPoint p =
            scanPoints[pointIndex];

        for(int detectorID = 0;
            detectorID < nDetectors;
            detectorID++)
        {
            auto key =
                std::make_pair(
                    pointIndex,
                    detectorID
                );

            long counts = 0;
            long flux = 0;

            if(fResults.count(key))
            {
                counts =
                    fResults[key].counts;
                flux =
                    fResults[key].flux;
            }

            double normalizedCounts =
                (double)counts /
                EVENTS_PER_POINT;

            double normalizedFlux =
                (double)flux /
                EVENTS_PER_POINT;

            fOutFile
                << p.x << ","
                << p.y << ","
                << detectorID << ","
                << EVENTS_PER_POINT << ","
                << counts << ","
                << normalizedCounts << ","
                << flux << ","
                << normalizedFlux
                << "\n";
        }
        fOutFile.flush();
    }

    if(gCurrentPoint == gEndPoint)
    {
        fOutFile.close();

        G4cout << "====================================\n";
        G4cout << "CSV SAVED\n";
        G4cout << "====================================\n";
    }
}
