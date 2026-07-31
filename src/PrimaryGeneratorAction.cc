#include "PrimaryGeneratorAction.hh"

#include "G4GeneralParticleSource.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4Exception.hh"
#include "G4RunManager.hh"

#include "ScanConfig.hh"

// ======================================================
// EVENTS PER SCAN POINT
// ======================================================

const long EVENTS_PER_POINT = 400000;

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
    static long globalEventCounter = 0;

    // ==========================================
    // KONVERSI EVENT -> SCAN POINT LOKAL
    // ==========================================

    long localPoint =
        globalEventCounter / EVENTS_PER_POINT;

    long pointIndex =
        gStartPoint + localPoint;

    globalEventCounter++;

    // ==========================================
    // HENTIKAN JIKA SUDAH LEWAT RANGE
    // ==========================================

    if(gEndPoint >= 0 &&
       pointIndex > gEndPoint)
    {
        G4cout
            << "====================================\n"
            << "SCAN RANGE SELESAI\n"
            << "STOP AT POINT "
            << pointIndex
            << "\n"
            << "====================================\n";

        G4RunManager::GetRunManager()
            ->AbortRun(true);

        return;
    }

    // ==========================================
    // VALIDASI
    // ==========================================

    if(pointIndex < 0 ||
       pointIndex >= (long)scanPoints.size())
    {
        G4Exception(
            "GeneratePrimaries",
            "InvalidPointIndex",
            FatalException,
            "pointIndex melebihi jumlah scanPoints"
        );
    }

    // ==========================================
    // AMBIL TITIK SCAN
    // ==========================================

    ScanPoint p =
        scanPoints[pointIndex];

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

    if(globalEventCounter % EVENTS_PER_POINT == 0)
    {
        G4cout << "====================================\n";

        G4cout << "SCAN POINT = "
               << pointIndex
               << "\n";

        G4cout << "X = "
               << p.x
               << " cm\n";

        G4cout << "Y = "
               << p.y
               << " cm\n";

        G4cout << "EVENT START = "
               << globalEventCounter
               << "\n";

        G4cout << "====================================\n";
    }

    // ==========================================
    // GENERATE PRIMARY VERTEX
    // ==========================================

    fParticleGun
        ->GeneratePrimaryVertex(event);
}
