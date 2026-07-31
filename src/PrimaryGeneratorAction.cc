#include "PrimaryGeneratorAction.hh"

#include "G4GeneralParticleSource.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4Exception.hh"
#include "G4RunManager.hh"

#include "ScanConfig.hh"

// ======================================================
// EVENTS PER SCAN POINT (unused since controlled by loop in main)
// ======================================================

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
    // AMBIL TITIK SCAN
    // ==========================================

    ScanPoint p =
        scanPoints[gCurrentPoint];

    // ==========================================
    // SIMPAN POSISI AKTIF
    // ==========================================

    fScanX = p.x;
    fScanY = p.y;

    // ==========================================
    // POSISI SOURCE
    // ==========================================

    G4ThreeVector pos(
        fScanX * cm,
        fScanY * cm,
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
    // DEBUG OUTPUT
    // ==========================================

    static int lastPoint = -1;
    if(gCurrentPoint != lastPoint)
    {
        G4cout << "====================================\n";
        G4cout << "SCAN POINT = " << gCurrentPoint << "\n";
        G4cout << "X = " << p.x << " cm\n";
        G4cout << "Y = " << p.y << " cm\n";
        G4cout << "====================================\n";
        lastPoint = gCurrentPoint;
    }

    // ==========================================
    // GENERATE PRIMARY VERTEX
    // ==========================================

    fParticleGun
        ->GeneratePrimaryVertex(event);
}
