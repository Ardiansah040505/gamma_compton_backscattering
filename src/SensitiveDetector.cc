#include "SensitiveDetector.hh"
#include "EventAction.hh"

#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4EventManager.hh"
#include "G4TouchableHandle.hh"

// ===============================
// Constructor
// ===============================
SensitiveDetector::SensitiveDetector(const G4String& name)
: G4VSensitiveDetector(name)
{}

// ===============================
// Destructor
// ===============================
SensitiveDetector::~SensitiveDetector()
{}

// ===============================
// ProcessHits
// ===============================
G4bool SensitiveDetector::ProcessHits(
    G4Step* step,
    G4TouchableHistory*)
{
    // ===============================
    // DETECTOR ID
    // ===============================
    auto touchable =
        step->GetPreStepPoint()
        ->GetTouchable();

    G4int detectorID =
        touchable->GetCopyNumber();

    // ===============================
    // FLUX MEASUREMENT (Boundary crossing)
    // ===============================
    if (step->GetPreStepPoint()->GetStepStatus() == fGeomBoundary)
    {
        auto eventAction =
            static_cast<EventAction*>(
                G4EventManager::GetEventManager()
                ->GetUserEventAction());
        if (eventAction)
        {
            eventAction->AddFlux(detectorID);
        }
    }

    // ===============================
    // ENERGY DEPOSIT
    // ===============================
    G4double edep =
        step->GetTotalEnergyDeposit();

    if(edep <= 0)
        return false;

    // ===============================
    // PARTICLE ENERGY
    // ===============================
    G4double energy =
        step->GetPreStepPoint()
        ->GetKineticEnergy();

    // ===============================
    // ROI ENERGY WINDOW
    // Cs-137 Backscatter Region
    // ===============================
    G4double Emin = 0.18 * MeV;

    G4double Emax = 0.23 * MeV;

    // hanya hit dalam ROI
    if(energy < Emin || energy > Emax)
        return false;

    // ===============================
    // AMBIL EVENT ACTION
    // ===============================
    auto eventAction =
        static_cast<EventAction*>(
            G4EventManager::GetEventManager()
            ->GetUserEventAction());

    // ===============================
    // TAMBAH ROI COUNT
    // ===============================
    eventAction->AddHit(
        detectorID,
        edep
    );

    return true;
}