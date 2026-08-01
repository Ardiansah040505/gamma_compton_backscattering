#include "DetectorConstruction.hh"

#include "G4Material.hh"
#include "G4NistManager.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SystemOfUnits.hh"

#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"

#include "G4SDManager.hh"
#include "SensitiveDetector.hh"

#include "G4VisAttributes.hh"
#include "G4Colour.hh"

#include "G4RotationMatrix.hh"
#include "G4SubtractionSolid.hh"
#include <cmath>

// =====================
// Constructor
// =====================
DetectorConstruction::DetectorConstruction()
: G4VUserDetectorConstruction(),
  fScanX(0),
  fScanY(0),
  fSourcePV(nullptr)
{}

// =====================
// Destructor
// =====================
DetectorConstruction::~DetectorConstruction()
{}

// =====================
// SetScanPosition
// =====================
void DetectorConstruction::SetScanPosition(
    double x,
    double y)
{
    fScanX = x;
    fScanY = y;

    // Update detectors positions (detector ring moves together with the source)
    G4double r = 5.0 * cm;
    for(size_t i = 0; i < fDetectorPVs.size(); i++)
    {
        G4double phi = i * 360.0*deg / fDetectorPVs.size();
        G4double dx = r * std::cos(phi);
        G4double dy = r * std::sin(phi);
        fDetectorPVs[i]->SetTranslation(G4ThreeVector(fScanX * cm + dx, fScanY * cm + dy, 0));
    }

    // Update source visual position
    if (fSourcePV) {
        fSourcePV->SetTranslation(G4ThreeVector(fScanX * cm, fScanY * cm, 0));
    }
}

// =====================
// Construct
// =====================
G4VPhysicalVolume* DetectorConstruction::Construct()
{
    defineMaterials();

    return ConstructWorld();
}

// =====================
// Materials
// =====================
void DetectorConstruction::defineMaterials()
{
    auto nist = G4NistManager::Instance();

    fAir    = nist->FindOrBuildMaterial("G4_AIR");

    fCesium = nist->FindOrBuildMaterial("G4_Cs");

    fNaiTI  = nist->FindOrBuildMaterial("G4_SODIUM_IODIDE");

    fAl     = nist->FindOrBuildMaterial("G4_Al");
}

// =====================
// World Construction
// =====================
G4VPhysicalVolume* DetectorConstruction::ConstructWorld()
{
    // =====================
    // WORLD
    // =====================
    auto solidWorld =
        new G4Box(
            "WorldSolid",
            150*cm,
            150*cm,
            150*cm
        );

    auto logicWorld =
        new G4LogicalVolume(
            solidWorld,
            fAir,
            "WorldLV"
        );

    logicWorld->SetVisAttributes(
        G4VisAttributes::GetInvisible()
    );

    auto physWorld =
        new G4PVPlacement(
            nullptr,
            G4ThreeVector(),
            logicWorld,
            "WorldPV",
            nullptr,
            false,
            0
        );

    // =====================
    // SOURCE VISUAL
    // (STATIC ONLY)
    // =====================
    auto solidSource =
        new G4Box(
            "SourceSolid",
            0.5*cm,
            0.5*cm,
            0.1*cm
        );

    auto logicSource =
        new G4LogicalVolume(
            solidSource,
            fCesium,
            "SourceLV"
        );

    auto sourceVis =
        new G4VisAttributes(
            G4Colour::Blue()
        );

    sourceVis->SetForceSolid(true);

    logicSource->SetVisAttributes(
        sourceVis
    );

    // SOURCE VISUAL TETAP
    // Partikel asli bergerak di PrimaryGeneratorAction
    fSourcePV = new G4PVPlacement(
        nullptr,
        G4ThreeVector(0,0,0),
        logicSource,
        "SourcePV",
        logicWorld,
        false,
        0
    );

    // =====================
    // PIPE
    // =====================
    auto solidPipe =
        new G4Tubs(
            "PipeSolid",
            20.0*cm,
            24.0*cm,
            40.0*cm,
            0.0,
            360.0*deg
        );

    auto logicPipe =
        new G4LogicalVolume(
            solidPipe,
            fAl,
            "PipeLV"
        );

    auto pipeVis =
        new G4VisAttributes(
            G4Colour::Gray()
        );

    pipeVis->SetForceSolid(true);

    logicPipe->SetVisAttributes(
        pipeVis
    );

    auto rotPipe =
        new G4RotationMatrix();

    rotPipe->rotateX(90*deg);

    new G4PVPlacement(
        rotPipe,
        G4ThreeVector(0,0,27*cm),
        logicPipe,
        "PipePV",
        logicWorld,
        false,
        0
    );

    // =====================
    // DEFECT / VOID (COMMENTED OUT FOR NORMAL PIPE SIMULATION)
    // Bentuk: garis horizontal (kotak pipih)
    // Panjang X = 40 cm (melintang penuh di dinding pipa)
    // Panjang Y = 40 cm (melintang penuh di dinding pipa)
    // Tebal  Z = 0.5 cm (setengah tebal = 0.5 cm → total 1 cm)
    // =====================
    /*
    auto solidVoid =
        new G4Box(
            "VoidSolid",
            20.0*cm,   // half-length X
            1.0*cm,    // half-length Y (total tebal 2 cm, aman di dinding tebal 4 cm)
            1.5*cm     // half-thickness Z
        );

    auto logicVoid =
        new G4LogicalVolume(
            solidVoid,
            fAir,
            "VoidLV"
        );

    G4ThreeVector defectPos(
        0,
        -22*cm,        // Diposisikan di dinding bawah (yang menghadap detektor)
        0
    );

    new G4PVPlacement(
        nullptr,
        defectPos,
        logicVoid,
        "VoidPV",
        logicPipe,
        false,
        0
    );

    auto voidVis =
        new G4VisAttributes(
            G4Colour::Red()
        );

    voidVis->SetForceSolid(true);

    logicVoid->SetVisAttributes(
        voidVis
    );
    */

    // =====================
    // NaI DETECTOR
    // =====================
    auto solidNaI =
        new G4Tubs(
            "NaISolid",
            0.0*cm,
            2.0*cm,    // Radius dikurangi agar 6 detektor muat di ring radius 5 cm
            2.0*cm,    // Half-length dikurangi agar muat
            0.0,
            360.0*deg
        );

    auto logicNaI =
        new G4LogicalVolume(
            solidNaI,
            fNaiTI,
            "NaILV"
        );

    auto naiVis =
        new G4VisAttributes(
            G4Colour::Green()
        );

    naiVis->SetForceSolid(true);

    logicNaI->SetVisAttributes(
        naiVis
    );

    // =====================
    // DETECTOR ARRAY
    // =====================
    G4int Ndet = 6;

    G4double r = 5.0 * cm;

    for(G4int i = 0; i < Ndet; i++)
    {
        G4double phi =
            i * 360.0*deg / Ndet;

        G4double x =
            r * std::cos(phi);

        G4double y =
            r * std::sin(phi);

        G4ThreeVector pos(x,y,0);

        auto rot =
            new G4RotationMatrix();

        rot->rotateZ(phi);

        auto detPV = new G4PVPlacement(
            rot,
            pos,
            logicNaI,
            "NaIPV",
            logicWorld,
            false,
            i
        );
        fDetectorPVs.push_back(detPV);
    }

    return physWorld;
}

// =====================
// Sensitive Detector
// =====================
void DetectorConstruction::ConstructSDandField()
{
    auto sdManager =
        G4SDManager::GetSDMpointer();

    auto naiSD =
        new SensitiveDetector(
            "NaISD"
        );

    sdManager->AddNewDetector(
        naiSD
    );

    SetSensitiveDetector(
        "NaILV",
        naiSD
    );

    G4cout
        << ">>> SD ATTACHED TO NaILV <<<"
        << G4endl;
}

