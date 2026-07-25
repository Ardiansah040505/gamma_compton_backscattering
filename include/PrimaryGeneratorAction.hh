#ifndef PRIMARY_GENERATOR_ACTION_HH
#define PRIMARY_GENERATOR_ACTION_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4GeneralParticleSource.hh"
#include "G4Event.hh"

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
public:
    PrimaryGeneratorAction();
    virtual ~PrimaryGeneratorAction();

    void GeneratePrimaries(G4Event*) override;

    // ======================
    // SCAN POSITION
    // ======================
    void SetScanPosition(double x, double y);

private:
    G4GeneralParticleSource* fParticleGun;

    double fScanX;
    double fScanY;
};

#endif
