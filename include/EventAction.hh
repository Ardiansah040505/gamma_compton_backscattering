#ifndef EventAction_h
#define EventAction_h

#include "G4UserEventAction.hh"
#include "globals.hh"

#include <map>

class G4Event;

class EventAction : public G4UserEventAction
{
public:

    EventAction();
    virtual ~EventAction();

    virtual void BeginOfEventAction(
        const G4Event*);

    virtual void EndOfEventAction(
        const G4Event*);

    // ===============================
    // ADD HIT
    // ===============================
    void AddHit(
        G4int detectorID,
        G4double edep
    );

    //Flux
    void AddFlux(G4int detectorID);

private:

    // ===============================
    // EVENT-LEVEL STORAGE
    // ===============================
    std::map<G4int, G4int> fDetectorCounts;

    std::map<G4int, G4double> fDetectorEdep;

    std::map<G4int, G4int> fDetectorFlux;
};

#endif