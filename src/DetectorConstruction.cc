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
  fPipePV(nullptr)
{}

// =====================
// Destructor
// =====================
DetectorConstruction::~DetectorConstruction()
{}

// =====================
// Set Scan Position
// =====================
void DetectorConstruction::SetScanPosition(
    double x,
    double y)
{
    fScanX = x;
    fScanY = y;
    if(fPipePV)
    {
        fPipePV->SetTranslation(G4ThreeVector(x * mm, y * mm, 37.0 * cm));
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

    fPb     = nist->FindOrBuildMaterial("G4_Pb");
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
            70*cm,
            70*cm,
            70*cm
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

    // SOURCE VISUAL TETAP (DI PUSAT 0,0,0)
    new G4PVPlacement(
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
            G4Colour::Blue()
        );

    pipeVis->SetForceSolid(true);

    logicPipe->SetVisAttributes(
        pipeVis
    );

    auto rotPipe =
        new G4RotationMatrix();

    rotPipe->rotateX(90*deg);

    // Moved closer to the detector plane (Z changed from 65*cm to 37*cm)
    // Distance from detector face (at Z = 10*cm) to pipe outer wall (Z = 37 - 24 = 13*cm) is 3*cm
    fPipePV = new G4PVPlacement(
        rotPipe,
        G4ThreeVector(fScanX*mm, fScanY*mm, 37.0*cm),
        logicPipe,
        "PipePV",
        logicWorld,
        false,
        0,
        true // checkOverlaps
    );

    // =====================
    // DEFECT / VOID
    // Bentuk: garis horizontal (kotak pipih)
    // Panjang X = 40 cm (melintang penuh di dinding pipa)
    // Panjang Y = 40 cm (melintang penuh di dinding pipa)
    // Tebal  Z = 0.5 cm (setengah tebal = 0.5 cm → total 1 cm)
    // =====================
    auto solidVoid =
        new G4Box(
            "VoidSolid",
            0.5*cm,   // half-length X
            3.0*cm,    // half-length Y (tipis → garis horizontal)
            20.0*cm     // half-thickness Z → total tebal 1 cm
        );

    auto logicVoid =
        new G4LogicalVolume(
            solidVoid,
            fAir,
            "VoidLV"
        );

    G4ThreeVector defectPos(
        0,
        21*cm,
        0
    );

    new G4PVPlacement(
        nullptr,
        defectPos,
        logicVoid,
        "VoidPV",
        logicPipe,
        false,
        0,
        true // checkOverlaps
    );

    auto voidVis =
        new G4VisAttributes(
            G4Colour::Red()
        );

    voidVis->SetForceSolid(true);

    logicVoid->SetVisAttributes(
        voidVis
    );

    // =====================
    // NaI DETECTOR
    // =====================
    auto solidNaI =
        new G4Tubs(
            "NaISolid",
            0.0*cm,
            10.0*cm,
            10.0*cm,
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
    // LEAD COLLIMATOR
    // =====================
    G4double collThickness = 1.0 * cm;
    G4double collOuterRadius = 10.0 * cm + collThickness; // 11 cm
    G4double collHalfHeight = 10.0 * cm + collThickness / 2.0; // 10.5 cm

    auto solidCollOuter =
        new G4Tubs(
            "CollOuterSolid",
            0.0 * cm,
            collOuterRadius,
            collHalfHeight,
            0.0,
            360.0 * deg
        );

    // Subtraction solid creates a cup wrapping the NaI detector (radius 10 cm, height 20 cm)
    // with 1 cm thickness on the sides and bottom, leaving the top face (facing the pipe) open.
    auto solidCollimator =
        new G4SubtractionSolid(
            "CollimatorSolid",
            solidCollOuter,
            solidNaI,
            nullptr,
            G4ThreeVector(0.0, 0.0, 0.5 * cm)
        );

    auto logicCollimator =
        new G4LogicalVolume(
            solidCollimator,
            fPb,
            "CollimatorLV"
        );

    auto collVis =
        new G4VisAttributes(
            G4Colour::Red()
        );

    collVis->SetForceSolid(true);

    logicCollimator->SetVisAttributes(
        collVis
    );

    // =====================
    // DETECTOR ARRAY (PLANAR ARRAY / FACING +Z)
    // =====================
    G4int Ndet = 6;
    G4double z_det = -5.0 * cm; // Top face of the array is at Z = 5.0 * cm (below the pipe bottom at Z = 13.0 * cm)
    G4double spacingX = 24.0 * cm;
    G4double spacingY = 24.0 * cm;

    for(G4int i = 0; i < Ndet; i++)
    {
        // Arrange 6 detectors in a 2x3 grid:
        // row 0: Y = -spacingY/2, col 0,1,2: X = -spacingX, 0, spacingX
        // row 1: Y = spacingY/2, col 0,1,2: X = -spacingX, 0, spacingX
        G4int row = i / 3;
        G4int col = i % 3;

        G4double x = (col - 1) * spacingX;
        G4double y = (row - 0.5) * spacingY;
        G4ThreeVector pos(x, y, z_det);

        // Place detector with nullptr rotation (parallel/planar, facing +Z)
        new G4PVPlacement(
            nullptr,
            pos,
            logicNaI,
            "NaIPV",
            logicWorld,
            false,
            i,
            true // checkOverlaps
        );

        // Place Lead Collimator (wrapped around detector, shifted by -0.5 cm in Z so detector sits in the cup)
        G4ThreeVector collPos = pos - G4ThreeVector(0, 0, 0.5 * cm);
        new G4PVPlacement(
            nullptr,
            collPos,
            logicCollimator,
            "CollimatorPV",
            logicWorld,
            false,
            i,
            true // checkOverlaps
        );
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

