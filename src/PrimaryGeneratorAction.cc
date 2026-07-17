#include "PrimaryGeneratorAction.hh"

#include "G4GeneralParticleSource.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4Exception.hh"
#include "G4RunManager.hh"

#include "ScanConfig.hh"

// EVENTS_PER_POINT is now declared in ScanConfig.hh

// ======================================================
// CONSTRUCTOR
// ======================================================

PrimaryGeneratorAction::PrimaryGeneratorAction()
: G4VUserPrimaryGeneratorAction(),
  fScanX(0.0),
  fScanY(0.0)
{
    fParticleGun =
        new G4GeneralParticleSource();
}

// ======================================================
// DESTRUCTOR
// ======================================================

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
    delete fParticleGun;
}

// ======================================================
// OPTIONAL MANUAL SETTER
// ======================================================

void PrimaryGeneratorAction::SetScanPosition(
    double x,
    double y)
{
    fScanX = x;
    fScanY = y;
}

// ======================================================
// GENERATE PRIMARY
// ======================================================

void PrimaryGeneratorAction::GeneratePrimaries(
    G4Event* event)
{
    // ==========================================
    // VALIDASI
    // ==========================================

    if(gCurrentPoint < 0 ||
       gCurrentPoint >= (int)scanPoints.size())
    {
        G4Exception(
            "GeneratePrimaries",
            "InvalidPointIndex",
            FatalException,
            "gCurrentPoint melebihi jumlah scanPoints"
        );
    }

    // ==========================================
    // SIMPAN POSISI AKTIF (TETAP DI PUSAT 0,0)
    // ==========================================

    fScanX = 0.0;
    fScanY = 0.0;

    // ==========================================
    // POSISI SOURCE
    // ==========================================

    G4ThreeVector pos(
        0.0 * cm,
        0.0 * cm,
        0.0 * cm
    );

    // ==========================================
    // SET SOURCE POSITION
    // ==========================================

    fParticleGun
        ->GetCurrentSource()
        ->GetPosDist()
        ->SetCentreCoords(pos);

    // ==========================================
    // GENERATE PRIMARY VERTEX
    // ==========================================

    fParticleGun
        ->GeneratePrimaryVertex(event);
}
