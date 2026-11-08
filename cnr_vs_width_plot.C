// =============================================================================
// cnr_vs_width_plot.C
// Makro ROOT C untuk visualisasi CNR vs Lebar Retakan (vs Template Normal & Spasial Latar)
// Proyek Deteksi Keretakan Pipa GCB (Gamma Compton Backscattering)
// Dibaca dari: hasil_cnr/cnr_summary_all_cracks.csv
// Output    : hasil_cnr/cnr_vs_crack_width_root.png
// Jalankan  : root -l -b -q cnr_vs_width_plot.C
// =============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include "TCanvas.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TLine.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TAxis.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TSystem.h"

using namespace std;

void cnr_vs_width_plot(
    const char* csvPath = "hasil_cnr/cnr_summary_all_cracks.csv",
    const char* outDir  = "hasil_cnr"
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

    // === Baca CSV ===
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

    stringstream hss(line);
    string token;
    vector<string> headers;
    while (getline(hss, token, ',')) {
        while (!token.empty() && (token.back() == '\r' || token.back() == ' ')) token.pop_back();
        headers.push_back(token);
    }

    int iWidth = -1, iMetric = -1, iCNR_Norm = -1, iCNR_Spat = -1;
    for (int i = 0; i < (int)headers.size(); ++i) {
        if (headers[i] == "Actual_Width_cm")      iWidth    = i;
        if (headers[i] == "Metrik")                iMetric   = i;
        if (headers[i] == "CNR (vs Normal)")       iCNR_Norm = i;
        if (headers[i] == "CNR (Spasial Latar)")   iCNR_Spat = i;
    }

    if (iWidth < 0 || iMetric < 0 || iCNR_Norm < 0 || iCNR_Spat < 0) {
        cerr << "[ERROR] Kolom yang dibutuhkan tidak lengkap di header CSV." << endl;
        file.close();
        return;
    }

    vector<double> vWidth, vCNR_Norm, vCNR_Spat;

    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        vector<string> cols;
        while (getline(ss, token, ',')) cols.push_back(token);
        if ((int)cols.size() <= max(iCNR_Norm, iCNR_Spat)) continue;

        string metric = cols[iMetric];
        while (!metric.empty() && (metric.back() == '\r' || metric.back() == ' ')) metric.pop_back();
        if (metric != "roiCounts") continue; // fokus ke roiCounts

        double w     = stod(cols[iWidth]);
        double c_norm = stod(cols[iCNR_Norm]);
        double c_spat = stod(cols[iCNR_Spat]);

        vWidth.push_back(w);
        vCNR_Norm.push_back(c_norm);
        vCNR_Spat.push_back(c_spat);
    }
    file.close();

    int n = (int)vWidth.size();
    if (n == 0) {
        cerr << "[ERROR] Tidak ada data roiCounts di CSV: " << csvPath << endl;
        return;
    }

    cout << "[INFO] Data terbaca: " << n << " titik data (roiCounts)." << endl;

    // === Bangun TGraph untuk kedua pendekatan ===
    TGraph* grNorm = new TGraph(n);
    TGraph* grSpat = new TGraph(n);

    for (int i = 0; i < n; ++i) {
        grNorm->SetPoint(i, vWidth[i], vCNR_Norm[i]);
        grSpat->SetPoint(i, vWidth[i], vCNR_Spat[i]);
    }

    // Gaya kurva vs Normal
    grNorm->SetName("grNorm");
    grNorm->SetLineColor(kBlue + 1);
    grNorm->SetLineWidth(2);
    grNorm->SetMarkerColor(kBlue + 1);
    grNorm->SetMarkerStyle(kFullCircle);
    grNorm->SetMarkerSize(1.1);

    // Gaya kurva Spasial Latar
    grSpat->SetName("grSpat");
    grSpat->SetLineColor(kOrange + 7);
    grSpat->SetLineWidth(2);
    grSpat->SetLineStyle(2); // dashed
    grSpat->SetMarkerColor(kOrange + 7);
    grSpat->SetMarkerStyle(kFullSquare);
    grSpat->SetMarkerSize(1.1);

    // Batas sumbu
    double xMin = 0.0;
    double xMax = 2.6;
    double max1 = *max_element(vCNR_Norm.begin(), vCNR_Norm.end());
    double max2 = *max_element(vCNR_Spat.begin(), vCNR_Spat.end());
    double yMax = max({max1, max2, 6.0}) * 1.18;

    // === Canvas ===
    TCanvas* c = new TCanvas("c_cnr_all", "CNR vs Crack Width", 820, 580);
    c->SetGrid();

    TMultiGraph* mg = new TMultiGraph();
    mg->Add(grNorm, "LP");
    mg->Add(grSpat, "LP");
    mg->Draw("A");

    mg->GetXaxis()->SetTitle("Crack Width (cm)");
    mg->GetYaxis()->SetTitle("Contrast-to-Noise Ratio (CNR)");
    mg->GetXaxis()->SetTitleSize(0.048);
    mg->GetYaxis()->SetTitleSize(0.048);
    mg->GetXaxis()->SetLabelSize(0.040);
    mg->GetYaxis()->SetLabelSize(0.040);
    mg->GetXaxis()->SetTitleOffset(1.1);
    mg->GetYaxis()->SetTitleOffset(1.2);
    mg->GetXaxis()->SetLimits(xMin, xMax);
    mg->SetMinimum(0.0);
    mg->SetMaximum(yMax);

    // Garis ambang batas Rose Criterion (CNR = 5)
    TLine* lineRose = new TLine(xMin, 5.0, xMax, 5.0);
    lineRose->SetLineColor(kGreen + 2);
    lineRose->SetLineWidth(2);
    lineRose->SetLineStyle(7);
    lineRose->Draw("SAME");

    // === TLegend ===
    TLegend* leg = new TLegend(0.16, 0.68, 0.62, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillStyle(1001);
    leg->SetFillColor(kWhite);
    leg->SetTextSize(0.030);
    leg->AddEntry(grNorm, "CNR (vs Normal Template)", "lp");
    leg->AddEntry(grSpat, "CNR (vs Spatial Background)", "lp");
    leg->AddEntry(lineRose, "Rose Criterion Limit (CNR = 5)", "l");
    leg->Draw();

    // === Judul manual dengan TLatex ===
    TLatex title;
    title.SetNDC();
    title.SetTextSize(0.040);
    title.SetTextFont(62);
    title.SetTextAlign(22);
    title.DrawLatex(0.52, 0.95, "CNR Comparison across Crack Widths (roiCounts)");

    c->Modified();
    c->Update();

    TString outPng = Form("%s/cnr_vs_crack_width_root.png", outDir);
    c->Print(outPng.Data());
    cout << "[INFO] Grafik komparasi CNR disimpan ke: " << outPng << endl;

    delete c;
}

