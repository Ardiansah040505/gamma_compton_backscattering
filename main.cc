#include "G4RunManager.hh"

#include "G4UImanager.hh"
#include "G4UIExecutive.hh"
#include "G4VisExecutive.hh"

#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "ActionInitialization.hh"
#include "ScanConfig.hh"

#include "G4ScoringManager.hh"
#include <fstream>
#include <string>

void ExecuteMacroWithoutBeamOn(const G4String& filename, G4UImanager* uiManager)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        G4cerr << "Error opening macro file: " << filename << G4endl;
        return;
    }
    std::string line;
    while (std::getline(file, line))
    {
        // Skip comment lines and beamOn command
        if (line.rfind("/run/beamOn", 0) == 0)
        {
            continue;
        }
        if (!line.empty() && line[0] != '#')
        {
            uiManager->ApplyCommand(line);
        }
    }
}

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

    auto detectorConstruction = new DetectorConstruction();
    runManager->SetUserInitialization(detectorConstruction);

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

        G4String filename = argv[1];
        ExecuteMacroWithoutBeamOn(filename, uiManager);

        // Run the scan loop in C++
        for (int pointIndex = gStartPoint; pointIndex <= gEndPoint; pointIndex++)
        {
            if (pointIndex >= (int)scanPoints.size()) break;

            gCurrentPoint = pointIndex;

            // 1. Move detectors and source visual in DetectorConstruction
            detectorConstruction->SetScanPosition(scanPoints[pointIndex].x, scanPoints[pointIndex].y);

            // 2. Notify Geant4 geometry changed
            runManager->GeometryHasBeenModified();

            // 3. Run 100,000 events for this position
            runManager->BeamOn(100000);
        }
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