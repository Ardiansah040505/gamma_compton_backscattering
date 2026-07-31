#ifndef SENSITIVE_DETECTOR_HH
#define SENSITIVE_DETECTOR_HH

#include "G4VSensitiveDetector.hh"
#include "globals.hh"

class G4Step;
class EventAction;

class SensitiveDetector : public G4VSensitiveDetector
{
public:
    SensitiveDetector(const G4String& name);
    virtual ~SensitiveDetector();

    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory*) override;
};

#endif