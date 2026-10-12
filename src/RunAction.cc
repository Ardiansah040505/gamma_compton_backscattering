#include "RunAction.hh"

#include "G4Run.hh"
#include "ScanConfig.hh"
#include "G4SystemOfUnits.hh"

#include <sstream>

RunAction::RunAction()
{}

RunAction::~RunAction()
{
    if(fOutFile.is_open())
    {
        fOutFile.close();
    }
    if(fSpectrumFile.is_open())
    {
        fSpectrumFile.close();
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
            << "x,y,nPrimary,"
            << "det0_counts,det1_counts,det2_counts,det3_counts,det4_counts,det5_counts,"
            << "det0_flux,det1_flux,det2_flux,det3_flux,det4_flux,det5_flux\n";
    }

    if(!fSpectrumFile.is_open())
    {
        std::stringstream sfname;

        sfname << "spectrum_"
               << gStartPoint
               << "_"
               << gEndPoint
               << ".csv";

        fSpectrumFile.open(sfname.str());

        fSpectrumFile << "x,y,detectorID";
        for(int bin = 0; bin < 1000; bin++)
        {
            fSpectrumFile << ",bin" << bin;
        }
        fSpectrumFile << "\n";
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
void RunAction::AddSpectrum(
    int detectorID,
    double edep)
{
    int pointIndex = gCurrentPoint;

    if(pointIndex >= (int)scanPoints.size())
        return;

    int bin = static_cast<int>(edep / (1.0 * keV));

    if(bin >= 0 && bin < 1000)
    {
        auto key = std::make_pair(pointIndex, detectorID);
        if(fSpectra.find(key) == fSpectra.end())
        {
            fSpectra[key].assign(1000, 0);
        }
        fSpectra[key][bin]++;
    }
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

        fOutFile
            << p.x << ","
            << p.y << ","
            << EVENTS_PER_POINT;

        for(int detectorID = 0; detectorID < nDetectors; detectorID++)
        {
            auto key = std::make_pair(pointIndex, detectorID);
            long counts = fResults.count(key) ? fResults[key].counts : 0;
            fOutFile << "," << counts;
        }

        for(int detectorID = 0; detectorID < nDetectors; detectorID++)
        {
            auto key = std::make_pair(pointIndex, detectorID);
            long flux = fResults.count(key) ? fResults[key].flux : 0;
            fOutFile << "," << flux;
        }

        fOutFile << "\n";
        fOutFile.flush();

        for(int detectorID = 0; detectorID < nDetectors; detectorID++)
        {
            auto key = std::make_pair(pointIndex, detectorID);

            fSpectrumFile << p.x << "," << p.y << "," << detectorID;

            if(fSpectra.count(key))
            {
                const auto& bins = fSpectra[key];
                for(int bin = 0; bin < 1000; bin++)
                {
                    fSpectrumFile << "," << bins[bin];
                }
            }
            else
            {
                for(int bin = 0; bin < 1000; bin++)
                {
                    fSpectrumFile << ",0";
                }
            }
            fSpectrumFile << "\n";
        }
        fSpectrumFile.flush();
        fSpectra.clear();
    }

    if(gCurrentPoint == gEndPoint)
    {
        fOutFile.close();
        fSpectrumFile.close();

        G4cout << "====================================\n";
        G4cout << "CSV SAVED\n";
        G4cout << "====================================\n";
    }
}
