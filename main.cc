#include "G4RunManager.hh"

#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"

#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "ActionInitialization.hh"
#include "ScanConfig.hh"

#include "G4ScoringManager.hh"

int main(int argc, char** argv)
{
    // =====================================
    // INIT SCAN GRID
    // =====================================

    InitScanPoints();

    if(argc >= 4)
    {
        gStartPoint = atoi(argv[2]);
        gEndPoint = atoi(argv[3]);

    }
    else
    {
        gStartPoint = 0;
        gEndPoint = scanPoints.size()-1;
    }

    G4cout << "====================================\n";
    G4cout << "TOTAL SCAN POINTS = "
           << scanPoints.size()
           << G4endl;
    G4cout << "====================================\n";

    // =====================================
    // UI
    // =====================================

    G4UIExecutive* ui = nullptr;

    if(argc == 1)
    {
        ui = new G4UIExecutive(argc, argv);
    }

    // =====================================
    // SINGLE THREAD RUN MANAGER
    // =====================================

    auto runManager =
        new G4RunManager();

    G4ScoringManager::GetScoringManager();

    // =====================================
    // INITIALIZATION
    // =====================================

    runManager->SetUserInitialization(
        new DetectorConstruction());

    runManager->SetUserInitialization(
        new PhysicsList());

    runManager->SetUserInitialization(
        new ActionInitialization());

    // =====================================
    // UI MANAGER
    // =====================================

    auto uiManager =
        G4UImanager::GetUIpointer();

    // =====================================
    // VISUALIZATION
    // =====================================

    G4VisManager* visManager = nullptr;

    if(ui)
    {
        visManager = new G4VisExecutive();

        visManager->Initialize();
    }

    // =====================================
    // BATCH MODE
    // =====================================

    if(!ui)
    {
        if(argc < 2)
        {
            G4cout << "Usage:\n";
            G4cout << "./crack run.mac [startPoint endPoint]\n";

            return 1;
        }

        G4String command =
            "/control/execute ";

        G4String filename =
            argv[1];

        uiManager->ApplyCommand(
            command + filename
        );
    }
    else
    {
        uiManager->ApplyCommand(
            "/control/execute vis.mac"
        );

        ui->SessionStart();

        delete ui;
    }

    // =====================================
    // CLEANUP
    // =====================================

    delete visManager;

    delete runManager;

    return 0;
}