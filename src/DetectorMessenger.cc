#include "DetectorMessenger.hh"
#include "DetectorConstruction.hh"
#include "ScanConfig.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4RunManager.hh"
#include "G4UImanager.hh"

DetectorMessenger::DetectorMessenger(DetectorConstruction* det)
: G4UImessenger(),
  fDetector(det)
{
    fDirectory = new G4UIdirectory("/detector/");
    fDirectory->SetGuidance("Detector control commands");

    fScanPointCmd = new G4UIcmdWithAnInteger("/detector/setScanPoint", this);
    fScanPointCmd->SetGuidance("Set the active scan point index and move source and detectors");
    fScanPointCmd->SetParameterName("index", false);
    fScanPointCmd->SetRange("index>=0");
    fScanPointCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

DetectorMessenger::~DetectorMessenger()
{
    delete fScanPointCmd;
    delete fDirectory;
}

void DetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
    if (command == fScanPointCmd)
    {
        int index = fScanPointCmd->GetNewIntValue(newValue);
        if (index >= 0 && index < (int)scanPoints.size())
        {
            gCurrentPoint = index;
            ScanPoint p = scanPoints[index];
            fDetector->SetScanPosition(p.x, p.y);
            
            // Notify RunManager that geometry changed
            G4RunManager::GetRunManager()->GeometryHasBeenModified();
            
            // Force the visualization viewer to rebuild and redraw with the new positions
            G4UImanager* ui = G4UImanager::GetUIpointer();
            if (ui) {
                ui->ApplyCommand("/vis/viewer/rebuild");
            }
            
            G4cout << ">>> Moved detector & source to Scan Point " << index 
                   << " (X: " << p.x << " cm, Y: " << p.y << " cm) <<<" << G4endl;
        }
        else
        {
            G4cerr << "Error: Scan point index " << index << " out of range (0 to " 
                   << scanPoints.size() - 1 << ")" << G4endl;
        }
    }
}
