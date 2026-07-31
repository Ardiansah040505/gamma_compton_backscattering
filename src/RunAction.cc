#include "RunAction.hh"

#include "G4Run.hh"

#include "ScanConfig.hh"

#include <sstream>

// ======================================
const long EVENTS_PER_POINT = 400000;

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

    std::stringstream fname;

    fname << "scan_results_"
          << gStartPoint
          << "_"
          << gEndPoint
          << ".csv";

    fOutFile.open(fname.str());

    fOutFile
        << "x,y,detectorID,nPrimary,roiCounts,normalizedCounts\n";

    G4cout << "====================================\n";
    G4cout << "RUN STARTED\n";
    G4cout << "SCAN RANGE = "
           << gStartPoint
           << " -> "
           << gEndPoint
           << "\n";
    G4cout << "====================================\n";
}

// ======================================
void RunAction::AddDetectorData(
    long eventID,
    int detectorID,
    int counts,
    double edep)
{
    int localPoint =
        eventID / EVENTS_PER_POINT;

    int pointIndex =
        gStartPoint + localPoint;

    if(pointIndex >= (int)scanPoints.size())
        return;

    auto key =
        std::make_pair(
            pointIndex,
            detectorID
        );

    fResults[key].counts += counts;

    // kalau nanti mau pakai energi
    fResults[key].edep += edep;
}

// ======================================
void RunAction::EndOfRunAction(
    const G4Run*)
{
    const int nDetectors = 6;

    for(int pointIndex = gStartPoint;
        pointIndex <= gEndPoint;
        pointIndex++)
    {
        if(pointIndex >= (int)scanPoints.size())
            break;

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

            if(fResults.count(key))
            {
                counts =
                    fResults[key].counts;
            }

            double normalizedCounts =
                (double)counts /
                EVENTS_PER_POINT;

            fOutFile
                << p.x << ","
                << p.y << ","
                << detectorID << ","
                << EVENTS_PER_POINT << ","
                << counts << ","
                << normalizedCounts
                << "\n";
        }
    }

    fOutFile.close();

    G4cout << "====================================\n";
    G4cout << "CSV SAVED\n";
    G4cout << "====================================\n";
}
