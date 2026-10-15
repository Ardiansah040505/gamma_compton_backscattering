#include "RunAction.hh"

#include "G4Run.hh"
#include "ScanConfig.hh"
#include "G4SystemOfUnits.hh"

#include <TFile.h>
#include <TH1D.h>
#include <sstream>

RunAction::RunAction()
    : fRootFiles(kNumDetectors, nullptr),
      fHistCurrent(kNumDetectors, nullptr)
{}

RunAction::~RunAction()
{
    if(fOutFile.is_open())
    {
        fOutFile.close();
    }
    for(int det = 0; det < kNumDetectors; det++)
    {
        if(fRootFiles[det] && fRootFiles[det]->IsOpen())
        {
            fRootFiles[det]->Close();
            delete fRootFiles[det];
            fRootFiles[det] = nullptr;
        }
    }
}

// ======================================
void RunAction::BeginOfRunAction(
    const G4Run*)
{
    fResults.clear();

    // Summary CSV file
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

    // Open one ROOT file per detector if not yet open
    for(int det = 0; det < kNumDetectors; det++)
    {
        if(!fRootFiles[det])
        {
            std::stringstream rfname;
            rfname << "spectrum_det" << det
                   << "_" << gStartPoint
                   << "_" << gEndPoint
                   << ".root";

            fRootFiles[det] = new TFile(rfname.str().c_str(), "RECREATE");
        }
    }

    // Create a TH1D for each detector for the current scan point
    int pointIndex = gCurrentPoint;
    double px = 0.0, py = 0.0;
    if(pointIndex >= 0 && pointIndex < (int)scanPoints.size())
    {
        px = scanPoints[pointIndex].x;
        py = scanPoints[pointIndex].y;
    }

    for(int det = 0; det < kNumDetectors; det++)
    {
        fRootFiles[det]->cd();
        std::stringstream hname, htitle;
        hname << "h_scan_" << pointIndex;
        htitle << "Detector " << det << " - Scan Point " << pointIndex
               << " (x=" << px << " mm, y=" << py << " mm);Energy (keV);Counts";

        fHistCurrent[det] = new TH1D(hname.str().c_str(), htitle.str().c_str(), 1000, 0.0, 1000.0);
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
    if(detectorID >= 0 && detectorID < kNumDetectors && fHistCurrent[detectorID])
    {
        double edep_keV = edep / keV;
        fHistCurrent[detectorID]->Fill(edep_keV);
    }
}

// ======================================
void RunAction::EndOfRunAction(
    const G4Run*)
{
    const int nDetectors = kNumDetectors;
    const long EVENTS_PER_POINT = 100000;

    int pointIndex = gCurrentPoint;

    if(pointIndex >= 0 && pointIndex < (int)scanPoints.size())
    {
        ScanPoint p =
            scanPoints[pointIndex];

        // 1. Write summary to CSV
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

        // 2. Write histograms to respective ROOT files
        for(int det = 0; det < nDetectors; det++)
        {
            if(fRootFiles[det] && fHistCurrent[det])
            {
                fRootFiles[det]->cd();
                fHistCurrent[det]->Write();
                delete fHistCurrent[det];
                fHistCurrent[det] = nullptr;
            }
        }
    }

    if(gCurrentPoint == gEndPoint)
    {
        fOutFile.close();

        for(int det = 0; det < nDetectors; det++)
        {
            if(fRootFiles[det] && fRootFiles[det]->IsOpen())
            {
                fRootFiles[det]->Close();
                delete fRootFiles[det];
                fRootFiles[det] = nullptr;
            }
        }

        G4cout << "====================================\n";
        G4cout << "CSV AND ROOT SPECTRA FILES SAVED\n";
        G4cout << "====================================\n";
    }
}
