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

        G4String filename = argv[1];
        std::ifstream macroFile(filename);
        if(!macroFile.is_open())
        {
            G4cout << "Error: Cannot open macro file: " << filename << G4endl;
            return 1;
        }

        std::string line;
        while(std::getline(macroFile, line))
        {
            if(line.empty()) continue;
            size_t firstNonSpace = line.find_first_not_of(" \t\r\n");
            if(firstNonSpace == std::string::npos || line[firstNonSpace] == '#') continue;

            // Skip /run/beamOn, we will control it in our C++ loop
            if(line.find("/run/beamOn") != std::string::npos)
            {
                continue;
            }

            uiManager->ApplyCommand(line);
        }
        macroFile.close();

        // =====================================
        // SCAN LOOP IN C++
        // =====================================
        auto detConstruction = static_cast<DetectorConstruction*>(
            const_cast<G4VUserDetectorConstruction*>(runManager->GetUserDetectorConstruction())
        );

        G4cout << "====================================\n";
        G4cout << "STARTING SCAN LOOP: " << gStartPoint << " -> " << gEndPoint << G4endl;
        G4cout << "====================================\n";

        for(int i = gStartPoint; i <= gEndPoint; i++)
        {
            if(i >= (int)scanPoints.size()) break;

            gCurrentPoint = i;
            ScanPoint p = scanPoints[i];

            // 1. Update position in DetectorConstruction
            detConstruction->SetScanPosition(p.x, p.y);

            // 2. Set the coordinates in ScanConfig for generator/run actions
            ScanConfig::scanX = p.x;
            ScanConfig::scanY = p.y;

            // 3. Notify run manager that geometry has changed (crucial: after shift, right before beamOn)
            G4RunManager::GetRunManager()->GeometryHasBeenModified();

            // 4. Run the beam for this point
            runManager->BeamOn(EVENTS_PER_POINT);
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