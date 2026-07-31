#ifndef DETECTOR_CONSTRUCTION_HH
#define DETECTOR_CONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Material.hh"
#include <vector>

class G4LogicalVolume;
class G4VPhysicalVolume;
class G4Material;

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();
    virtual ~DetectorConstruction();

    virtual G4VPhysicalVolume* Construct();
    virtual void ConstructSDandField() override;

    // ======================
    // SCAN POSITION
    // ======================
    void SetScanPosition(double x, double y);

private:
    void defineMaterials();
    G4VPhysicalVolume* ConstructWorld();

    // scan offset
    double fScanX;
    double fScanY;

    G4Material* fAir;
    G4Material* fCesium;
    G4Material* fNaiTI;
    G4Material* fAl;

    std::vector<G4VPhysicalVolume*> fDetectorPVs;
    G4VPhysicalVolume* fSourcePV;
};

#endif