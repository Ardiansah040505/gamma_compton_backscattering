#include "PhysicsList.hh"

#include "G4EmStandardPhysics.hh"
#include "G4SystemOfUnits.hh"
#include "G4DecayPhysics.hh"
#include "G4RadioactiveDecayPhysics.hh"

PhysicsList::PhysicsList()
: G4VModularPhysicsList()
{
    defaultCutValue = 0.01*cm;

    RegisterPhysics(new G4EmStandardPhysics());
    RegisterPhysics(new G4DecayPhysics());
    RegisterPhysics(new G4RadioactiveDecayPhysics());

    SetVerboseLevel(1);

}

PhysicsList::~PhysicsList()
{}

void PhysicsList::SetCuts()
{
    SetCutsWithDefault();
}