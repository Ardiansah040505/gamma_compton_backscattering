#include "EventAction.hh"
#include "RunAction.hh"

#include "G4Event.hh"
#include "G4RunManager.hh"

// =========================================
EventAction::EventAction()
{}

// =========================================
EventAction::~EventAction()
{}

// =========================================
void EventAction::BeginOfEventAction(
    const G4Event*)
{
    fDetectorCounts.clear();
    fDetectorEdep.clear();
    fDetectorFlux.clear();
}

// =========================================
void EventAction::AddHit(
    G4int detectorID,
    G4double edep)
{
    fDetectorCounts[detectorID]++;

    fDetectorEdep[detectorID] += edep;
}

// =========================================
void EventAction::AddFlux(G4int detectorID)
{
    fDetectorFlux[detectorID]++;
}

// =========================================
void EventAction::EndOfEventAction(
    const G4Event* event)
{
    auto runAction =
        const_cast<RunAction*>(
            static_cast<const RunAction*>(
                G4RunManager::GetRunManager()
                ->GetUserRunAction()));

    long eventID =
        event->GetEventID();

    const G4int nDetectors = 6;

    // =====================================
    // SIMPAN SEMUA DETECTOR
    // TERMASUK COUNTS = 0
    // =====================================

    for(G4int detectorID = 0;
        detectorID < nDetectors;
        detectorID++)
    {
        G4int counts = 0;
        G4double edep = 0.0;
        G4int flux = 0;

        if(fDetectorCounts.count(detectorID))
        {
            counts =
                fDetectorCounts[detectorID];

            edep =
                fDetectorEdep[detectorID];
        }

        if(fDetectorFlux.count(detectorID))
        {
            flux =
                fDetectorFlux[detectorID];
        }

        runAction->AddDetectorData(
            eventID,
            detectorID,
            counts,
            edep,
            flux
        );
    }
}
