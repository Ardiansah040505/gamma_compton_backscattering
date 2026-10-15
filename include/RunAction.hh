#ifndef RUN_ACTION_HH
#define RUN_ACTION_HH

#include "G4UserRunAction.hh"
#include "globals.hh"

#include <map>
#include <vector>
#include <fstream>
#include <utility>

class G4Run;
class TFile;
class TH1D;

class RunAction : public G4UserRunAction
{
public:

    RunAction();
    virtual ~RunAction();

    virtual void BeginOfRunAction(
        const G4Run*) override;

    virtual void EndOfRunAction(
        const G4Run*) override;

    // ======================================
    // ADD DETECTOR DATA
    // ======================================
    void AddDetectorData(
        long eventID,
        int detectorID,
        int counts,
        double edep,
        int flux
    );

    void AddSpectrum(
        int detectorID,
        double edep
    );

private:

    // ======================================
    // ACCUMULATOR STRUCT
    // ======================================
    struct DetectorData
    {
        long counts = 0;
        double edep = 0.0;
        long flux = 0;
    };

    // KEY: (pointIndex, detectorID)
    std::map<
        std::pair<int,int>,
        DetectorData
    > fResults;

    // Current point energy deposits: detectorID -> TH1D*
    static const int kNumDetectors = 6;
    std::vector<TFile*> fRootFiles;
    std::vector<TH1D*> fHistCurrent;

    // ======================================
    // OUTPUT FILES
    // ======================================
    std::ofstream fOutFile;
};

#endif