// =============================================================================
// line_profiles_plot.C
// Makro ROOT C untuk visualisasi Profil Garis Horizontal Defek vs Normal
// Proyek Deteksi Keretakan Pipa GCB (Gamma Compton Backscattering)
// Dibaca dari: hasil_batch_cnr/line_profiles_data.csv
// Output    : hasil_batch_cnr/line_profiles_comparison.png
// Jalankan  : root -l -b -q line_profiles_plot.C
// =============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include "TCanvas.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TAxis.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TSystem.h"

using namespace std;

struct ProfilePoint {
    double x;
    double counts;
};

struct SampleData {
    string label;
    double size;
    double cnr;
    vector<ProfilePoint> points;
};

void line_profiles_plot(
    const char* csvPath = "hasil_batch_cnr/line_profiles_data.csv",
    const char* outDir  = "hasil_batch_cnr"
)
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    gStyle->SetPadGridX(kTRUE);
    gStyle->SetPadGridY(kTRUE);
    gStyle->SetGridColor(kGray);
    gStyle->SetGridStyle(3);
    gStyle->SetGridWidth(1);
    gStyle->SetPadLeftMargin(0.13);
    gStyle->SetPadBottomMargin(0.13);
    gStyle->SetPadRightMargin(0.06);
    gStyle->SetPadTopMargin(0.09);

    gSystem->mkdir(outDir, kTRUE);

    ifstream file(csvPath);
    if (!file.is_open()) {
        cerr << "[ERROR] Tidak dapat membuka CSV: " << csvPath << endl;
        return;
    }

    string line;
    if (!getline(file, line)) {
        cerr << "[ERROR] CSV kosong: " << csvPath << endl;
        file.close();
        return;
    }

    // Map ordered list of samples
    vector<string> sampleOrder;
    map<string, SampleData> sampleMap;

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string sample, szStr, cnrStr, xStr, cntStr;

        if (!getline(ss, sample, ',')) continue;
        if (!getline(ss, szStr, ',')) continue;
        if (!getline(ss, cnrStr, ',')) continue;
        if (!getline(ss, xStr, ',')) continue;
        if (!getline(ss, cntStr, ',')) continue;

        double size = stod(szStr);
        double cnr  = stod(cnrStr);
        double x    = stod(xStr);
        double cnt  = stod(cntStr);

        if (sampleMap.find(sample) == sampleMap.end()) {
            sampleOrder.push_back(sample);
            sampleMap[sample] = {sample, size, cnr, {}};
        }
        sampleMap[sample].points.push_back({x, cnt});
    }
    file.close();

    if (sampleOrder.empty()) {
        cerr << "[ERROR] Tidak ada data di: " << csvPath << endl;
        return;
    }

    cout << "[INFO] Terbaca " << sampleOrder.size() << " kurva profil garis." << endl;

    // Warna dan marker untuk tiap sampel retakan
    const int crackColors[] = {
        kBlue + 1, kCyan + 2, kTeal + 2, kGreen + 2,
        kOrange + 7, kRed + 1, kViolet + 1, kMagenta + 2
    };

    TMultiGraph* mg = new TMultiGraph();
    TLegend* leg = new TLegend(0.16, 0.16, 0.56, 0.48);
    leg->SetBorderSize(1);
    leg->SetFillStyle(1001);
    leg->SetFillColor(kWhite);
    leg->SetTextSize(0.026);

    int colorIdx = 0;
    for (const auto& sName : sampleOrder) {
        const auto& sData = sampleMap[sName];
        int nPts = sData.points.size();
        if (nPts == 0) continue;

        TGraph* gr = new TGraph(nPts);
        for (int i = 0; i < nPts; ++i) {
            gr->SetPoint(i, sData.points[i].x, sData.points[i].counts);
        }

        if (sName == "Normal") {
            gr->SetLineColor(kBlack);
            gr->SetLineWidth(2);
            gr->SetLineStyle(7); // dashed
            mg->Add(gr, "L");
            leg->AddEntry(gr, "Normal (Baseline)", "l");
        } else {
            int col = crackColors[colorIdx % 8];
            colorIdx++;
            gr->SetLineColor(col);
            gr->SetLineWidth(2);
            mg->Add(gr, "L");
            TString legText = Form("%s (CNR: %.1f)", sName.c_str(), sData.cnr);
            leg->AddEntry(gr, legText.Data(), "l");
        }
    }

    TCanvas* c = new TCanvas("c_profiles", "Line Profiles Comparison", 820, 580);
    c->SetGrid();

    mg->Draw("A");
    mg->GetXaxis()->SetTitle("X Position (cm)");
    mg->GetYaxis()->SetTitle("Total ROI Counts");
    mg->GetXaxis()->SetTitleSize(0.048);
    mg->GetYaxis()->SetTitleSize(0.048);
    mg->GetXaxis()->SetLabelSize(0.040);
    mg->GetYaxis()->SetLabelSize(0.040);
    mg->GetXaxis()->SetTitleOffset(1.1);
    mg->GetYaxis()->SetTitleOffset(1.2);
    mg->GetXaxis()->SetLimits(-5.0, 5.0);

    leg->Draw();

    // Judul dengan TLatex
    TLatex title;
    title.SetNDC();
    title.SetTextSize(0.040);
    title.SetTextFont(62);
    title.SetTextAlign(22);
    title.DrawLatex(0.52, 0.95, "Horizontal Line Profiles across Defect Widths (Y = 0 cm)");

    c->Modified();
    c->Update();

    TString outPng = Form("%s/line_profiles_comparison.png", outDir);
    c->Print(outPng.Data());
    cout << "[INFO] Grafik profil garis disimpan ke: " << outPng << endl;

    delete c;
}

