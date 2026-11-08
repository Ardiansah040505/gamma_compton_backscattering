#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include "TCanvas.h"
#include "TGraphErrors.h"
#include "TLegend.h"
#include "TMultiGraph.h"
#include "TStyle.h"
#include "TSystem.h"
#include "TROOT.h"
#include "TAxis.h"

using namespace std;

// Struktur data titik data
struct DataPoint {
    int    det;
    int    scanPoint;
    double yPos;       // mm
    double mean;       // keV
    double meanErr;    // keV
};

void PlotRegion(const string& regionName,
                const vector<DataPoint>& points,
                const char* outDir,
                const int* detColors,
                const int* detMarkers,
                const char* const* detLabels)
{
    // Filter dan pisahkan data per detektor (0-5)
    vector<vector<DataPoint>> detPoints(6);
    for (const auto& pt : points) {
        if (pt.det >= 0 && pt.det < 6) {
            detPoints[pt.det].push_back(pt);
        }
    }

    // Sort tiap detektor berdasarkan scanPoint / yPos
    for (int d = 0; d < 6; ++d) {
        sort(detPoints[d].begin(), detPoints[d].end(), [](const DataPoint& a, const DataPoint& b) {
            return a.yPos < b.yPos;
        });
    }

    TCanvas* c = new TCanvas(Form("c_%s", regionName.c_str()),
                             Form("Mean vs Position - %s", regionName.c_str()),
                             1000, 700);
    c->SetGrid();

    TMultiGraph* mg = new TMultiGraph();
    TString title = Form("Mean Energy vs Simulation Position y (%s);Mean Energy (keV);Position y (mm)",
                         regionName.c_str());
    mg->SetTitle(title.Data());

    TLegend* leg = new TLegend(0.72, 0.65, 0.90, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillStyle(1001);
    leg->SetFillColor(kWhite);
    leg->SetTextSize(0.028);

    vector<TGraphErrors*> graphs;

    for (int d = 0; d < 6; ++d) {
        int n = detPoints[d].size();
        if (n == 0) continue;

        vector<double> x(n), y(n), ex(n), ey(n);
        for (int i = 0; i < n; ++i) {
            x[i]  = detPoints[d][i].mean;
            y[i]  = detPoints[d][i].yPos;
            ex[i] = detPoints[d][i].meanErr;
            ey[i] = 0.0;
        }

        TGraphErrors* gr = new TGraphErrors(n, x.data(), y.data(), ex.data(), ey.data());
        gr->SetName(Form("gr_det%d_%s", d, regionName.c_str()));
        gr->SetLineColor(detColors[d]);
        gr->SetLineWidth(2);
        gr->SetMarkerColor(detColors[d]);
        gr->SetMarkerStyle(detMarkers[d]);
        gr->SetMarkerSize(1.1);

        mg->Add(gr, "LP");
        leg->AddEntry(gr, detLabels[d], "lp");
        graphs.push_back(gr);
    }

    mg->Draw("A");
    leg->Draw();

    c->Modified();
    c->Update();

    TString outPng = Form("%s/MeanVsPosition_%s.png", outDir, regionName.c_str());
    c->Print(outPng.Data());
    cout << "[INFO] Saved: " << outPng << endl;

    delete c;
}

void PlotMeanVsPosition(const char* csvPath = "result_all_fitting/fitting_summary.csv")
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);

    const char* outDir = "result_all_fitting/mean_vs_position";
    gSystem->mkdir(outDir, kTRUE);

    ifstream file(csvPath);
    if (!file.is_open()) {
        cerr << "[ERROR] Cannot open CSV file: " << csvPath << endl;
        return;
    }

    string line;
    // Baca baris header
    if (!getline(file, line)) {
        cerr << "[ERROR] Empty CSV file: " << csvPath << endl;
        file.close();
        return;
    }

    map<string, vector<DataPoint>> regionData;

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string token;
        vector<string> cols;

        while (getline(ss, token, ',')) {
            cols.push_back(token);
        }

        // Format CSV:
        // 0: File (e.g. spectrum_det0_5090_5092.root)
        // 1: Histogram (e.g. h_scan_5090)
        // 2: Region (Photopeak, Backscatter, ComptonEdge)
        // 7: Mean_keV
        // 8: Mean_Err
        // 18: Status
        if (cols.size() < 19) continue;

        string fname   = cols[0];
        string hname   = cols[1];
        string region  = cols[2];
        double mean    = stod(cols[7]);
        double meanErr = stod(cols[8]);
        string status  = cols[18];

        if (status != "SUCCESS") continue;

        // Parse detektor dari filename, e.g. spectrum_det3_...
        int det = -1;
        size_t detPos = fname.find("spectrum_det");
        if (detPos != string::npos) {
            det = fname[detPos + 12] - '0';
        }

        // Parse scan point dari hname, e.g. h_scan_5090
        int scanPoint = 0;
        size_t scanIdx = hname.find("h_scan_");
        if (scanIdx != string::npos) {
            scanPoint = stoi(hname.substr(scanIdx + 7));
        }

        double yPos = scanPoint - 5100; // mm

        DataPoint pt;
        pt.det = det;
        pt.scanPoint = scanPoint;
        pt.yPos = yPos;
        pt.mean = mean;
        pt.meanErr = meanErr;

        regionData[region].push_back(pt);
    }

    file.close();

    const int detColors[6] = {kRed, kBlue, kGreen+2, kOrange+7, kViolet+1, kCyan+2};
    const int detMarkers[6] = {20, 21, 22, 23, 33, 34}; // Circle, Square, TriangleUp, TriangleDown, Diamond, Cross
    const char* detLabels[6] = {
        "Detector 0", "Detector 1", "Detector 2",
        "Detector 3", "Detector 4", "Detector 5"
    };

    cout << "\n=============================================================" << endl;
    cout << "          PLOT MEAN ENERGY VS POSITION Y                     " << endl;
    cout << "=============================================================" << endl;

    for (const auto& kv : regionData) {
        cout << "[INFO] Processing Region: " << kv.first << " (" << kv.second.size() << " data points)" << endl;
        PlotRegion(kv.first, kv.second, outDir, detColors, detMarkers, detLabels);
    }

    cout << "=============================================================\n" << endl;
}
