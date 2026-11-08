// =============================================================================
// generate_error_plots.C
// Makro ROOT C untuk Visualisasi Analisis Error Mutlak Keretakan Pipa GCB
// Output grafis:
// 1. grafik_error_dual_panel.png
// 2. grafik_error_dual_axis.png
// 3. grafik_komparasi_metode_dekonvolusi.png
// 4. grafik_reduksi_error_kalibrasi.png
// Jalankan: root -l -b -q generate_error_plots.C
// =============================================================================

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include "TCanvas.h"
#include "TPad.h"
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

// Helper untuk menemukan lokasi file CSV (fleksibel di folder sendiri atau parent)
string FindPath(const string& filename) {
    if (gSystem->AccessPathName(filename.c_str()) == 0) return filename;
    string parent = "../" + filename;
    if (gSystem->AccessPathName(parent.c_str()) == 0) return parent;
    string sub = "grafik_analisis_error/" + filename;
    if (gSystem->AccessPathName(sub.c_str()) == 0) return sub;
    return filename;
}

// Struktur data Summary
struct SummaryRow {
    double actual_w;
    double abs_err_roi;
    double abs_rel_err_roi;
    double abs_err_flux;
    double abs_rel_err_flux;
};

// Struktur data Deconvolution
struct DeconvRow {
    double actual_w;
    double abs_err_raw;
    double abs_err_dec;
    double abs_rel_raw;
    double abs_rel_dec;
    double cal_raw;
    double cal_dec;
};

// Struktur data Richardson-Lucy
struct RLRow {
    double actual_w;
    double abs_err_rl;
    double abs_rel_rl;
};

// -----------------------------------------------------------------------------
// FUNGSI LOAD DATA CSV
// -----------------------------------------------------------------------------
bool LoadSummaryCSV(const string& path, vector<SummaryRow>& rows) {
    ifstream f(path);
    if (!f.is_open()) return false;
    string line;
    if (!getline(f, line)) return false; // skip header

    while (getline(f, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string token;
        vector<string> cols;
        while (getline(ss, token, ',')) cols.push_back(token);
        if (cols.size() < 14) continue;

        SummaryRow r;
        r.actual_w         = stod(cols[9]);
        r.abs_err_roi      = fabs(stod(cols[10]));
        r.abs_rel_err_roi  = fabs(stod(cols[11]));
        r.abs_err_flux     = fabs(stod(cols[12]));
        r.abs_rel_err_flux = fabs(stod(cols[13]));
        rows.push_back(r);
    }
    sort(rows.begin(), rows.end(), [](const SummaryRow& a, const SummaryRow& b) {
        return a.actual_w < b.actual_w;
    });
    return true;
}

static double SafeStod(const string& str, double defVal = 0.0) {
    try {
        if (str.empty() || str == "nan" || str == "NaN" || str == "+nan" || str == "-nan") return defVal;
        return stod(str);
    } catch (...) {
        return defVal;
    }
}

static bool IsValidNum(const string& str) {
    if (str.empty() || str == "nan" || str == "NaN" || str == "+nan" || str == "-nan") return false;
    try {
        stod(str);
        return true;
    } catch (...) {
        return false;
    }
}

bool LoadDeconvCSV(const string& path, vector<DeconvRow>& rows) {
    ifstream f(path);
    if (!f.is_open()) return false;
    string line;
    if (!getline(f, line)) return false;

    while (getline(f, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string token;
        vector<string> cols;
        while (getline(ss, token, ',')) cols.push_back(token);
        if (cols.size() < 12) continue;
        if (!IsValidNum(cols[1]) || !IsValidNum(cols[4]) || !IsValidNum(cols[5]) ||
            !IsValidNum(cols[6]) || !IsValidNum(cols[7]) || !IsValidNum(cols[10]) || !IsValidNum(cols[11])) {
            continue;
        }

        DeconvRow r;
        r.actual_w     = SafeStod(cols[1]);
        r.abs_err_raw  = fabs(SafeStod(cols[4]));
        r.abs_err_dec  = fabs(SafeStod(cols[5]));
        r.abs_rel_raw  = fabs(SafeStod(cols[6]));
        r.abs_rel_dec  = fabs(SafeStod(cols[7]));
        r.cal_raw      = SafeStod(cols[10]);
        r.cal_dec      = SafeStod(cols[11]);
        rows.push_back(r);
    }
    sort(rows.begin(), rows.end(), [](const DeconvRow& a, const DeconvRow& b) {
        return a.actual_w < b.actual_w;
    });
    return true;
}

bool LoadRLCSV(const string& path, vector<RLRow>& rows) {
    ifstream f(path);
    if (!f.is_open()) return false;
    string line;
    if (!getline(f, line)) return false;

    while (getline(f, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string token;
        vector<string> cols;
        while (getline(ss, token, ',')) cols.push_back(token);
        if (cols.size() < 8) continue;
        if (!IsValidNum(cols[1]) || !IsValidNum(cols[5]) || !IsValidNum(cols[7])) {
            continue;
        }

        RLRow r;
        r.actual_w   = SafeStod(cols[1]);
        r.abs_err_rl = fabs(SafeStod(cols[5]));
        r.abs_rel_rl = fabs(SafeStod(cols[7]));
        rows.push_back(r);
    }
    sort(rows.begin(), rows.end(), [](const RLRow& a, const RLRow& b) {
        return a.actual_w < b.actual_w;
    });
    return true;
}

// -----------------------------------------------------------------------------
// 1. PLOT DUAL PANEL (SUBPLOT 1x2): Error Absolut (cm) & Relatif Absolut (%)
// -----------------------------------------------------------------------------
void PlotDualPanel(const vector<SummaryRow>& sumData, const string& outDir) {
    int n = sumData.size();
    if (n == 0) return;

    TGraph* grAbsRoi  = new TGraph(n);
    TGraph* grAbsFlux = new TGraph(n);
    TGraph* grRelRoi  = new TGraph(n);
    TGraph* grRelFlux = new TGraph(n);

    for (int i = 0; i < n; ++i) {
        grAbsRoi->SetPoint(i, sumData[i].actual_w, sumData[i].abs_err_roi);
        grAbsFlux->SetPoint(i, sumData[i].actual_w, sumData[i].abs_err_flux);
        grRelRoi->SetPoint(i, sumData[i].actual_w, sumData[i].abs_rel_err_roi);
        grRelFlux->SetPoint(i, sumData[i].actual_w, sumData[i].abs_rel_err_flux);
    }

    // Styling ROI
    grAbsRoi->SetLineColor(kBlue + 1);
    grAbsRoi->SetLineWidth(2);
    grAbsRoi->SetMarkerColor(kBlue + 1);
    grAbsRoi->SetMarkerStyle(kFullCircle);
    grAbsRoi->SetMarkerSize(1.1);

    grRelRoi->SetLineColor(kBlue + 1);
    grRelRoi->SetLineWidth(2);
    grRelRoi->SetMarkerColor(kBlue + 1);
    grRelRoi->SetMarkerStyle(kFullCircle);
    grRelRoi->SetMarkerSize(1.1);

    // Styling Flux
    grAbsFlux->SetLineColor(kRed + 1);
    grAbsFlux->SetLineWidth(2);
    grAbsFlux->SetLineStyle(7);
    grAbsFlux->SetMarkerColor(kRed + 1);
    grAbsFlux->SetMarkerStyle(kFullSquare);
    grAbsFlux->SetMarkerSize(1.0);

    grRelFlux->SetLineColor(kRed + 1);
    grRelFlux->SetLineWidth(2);
    grRelFlux->SetLineStyle(7);
    grRelFlux->SetMarkerColor(kRed + 1);
    grRelFlux->SetMarkerStyle(kFullSquare);
    grRelFlux->SetMarkerSize(1.0);

    TCanvas* c = new TCanvas("c_dual_panel", "Dual Panel Error Analysis", 1500, 600);
    c->Divide(2, 1, 0.005, 0.005);

    // === Panel 1 (Kiri): Error Absolut (cm) ===
    c->cd(1);
    gPad->SetGrid();
    gPad->SetLeftMargin(0.13);
    gPad->SetBottomMargin(0.13);
    gPad->SetRightMargin(0.04);
    gPad->SetTopMargin(0.10);

    TMultiGraph* mg1 = new TMultiGraph();
    mg1->Add(grAbsRoi, "LP");
    mg1->Add(grAbsFlux, "LP");
    mg1->Draw("A");

    mg1->GetXaxis()->SetTitle("Lebar Keretakan Sebenarnya (cm)");
    mg1->GetYaxis()->SetTitle("Error Absolut |W_{FWHM} - W_{aktual}| (cm)");
    mg1->GetXaxis()->SetTitleSize(0.042);
    mg1->GetYaxis()->SetTitleSize(0.042);
    mg1->GetXaxis()->SetLabelSize(0.038);
    mg1->GetYaxis()->SetLabelSize(0.038);
    mg1->GetXaxis()->SetTitleOffset(1.2);
    mg1->GetYaxis()->SetTitleOffset(1.3);
    mg1->GetXaxis()->SetLimits(0.0, 2.6);
    mg1->SetMinimum(0.0);
    mg1->SetMaximum(0.75);

    // Garis ideal nol
    TLine* lineZero1 = new TLine(0.0, 0.0, 2.6, 0.0);
    lineZero1->SetLineColor(kBlack);
    lineZero1->SetLineStyle(7);
    lineZero1->Draw("SAME");

    // Garis resolusi 1.0 cm
    TLine* lineRes1 = new TLine(1.0, 0.0, 1.0, 0.75);
    lineRes1->SetLineColor(kGray + 1);
    lineRes1->SetLineStyle(3);
    lineRes1->Draw("SAME");

    TLegend* leg1 = new TLegend(0.40, 0.70, 0.94, 0.88);
    leg1->SetBorderSize(1);
    leg1->SetFillColor(kWhite);
    leg1->SetTextSize(0.030);
    leg1->AddEntry(grAbsRoi, "Error Absolut roiCounts (|#DeltaW_{roi}|)", "lp");
    leg1->AddEntry(grAbsFlux, "Error Absolut Flux (|#DeltaW_{flux}|)", "lp");
    leg1->AddEntry(lineRes1, "Batas Resolusi Sinar (~1.0 cm)", "l");
    leg1->Draw();

    TLatex t1;
    t1.SetNDC();
    t1.SetTextSize(0.042);
    t1.SetTextFont(62);
    t1.DrawLatex(0.15, 0.93, "(a) Error Absolut FWHM vs Lebar Aktual");

    // === Panel 2 (Kanan): Error Relatif Absolut (%) ===
    c->cd(2);
    gPad->SetGrid();
    gPad->SetLeftMargin(0.13);
    gPad->SetBottomMargin(0.13);
    gPad->SetRightMargin(0.04);
    gPad->SetTopMargin(0.10);

    TMultiGraph* mg2 = new TMultiGraph();
    mg2->Add(grRelRoi, "LP");
    mg2->Add(grRelFlux, "LP");
    mg2->Draw("A");

    mg2->GetXaxis()->SetTitle("Lebar Keretakan Sebenarnya (cm)");
    mg2->GetYaxis()->SetTitle("Error Relatif Absolut (%)");
    mg2->GetXaxis()->SetTitleSize(0.042);
    mg2->GetYaxis()->SetTitleSize(0.042);
    mg2->GetXaxis()->SetLabelSize(0.038);
    mg2->GetYaxis()->SetLabelSize(0.038);
    mg2->GetXaxis()->SetTitleOffset(1.2);
    mg2->GetYaxis()->SetTitleOffset(1.3);
    mg2->GetXaxis()->SetLimits(0.0, 2.6);
    mg2->SetMinimum(0.0);
    mg2->SetMaximum(600.0);

    TLine* lineZero2 = new TLine(0.0, 0.0, 2.6, 0.0);
    lineZero2->SetLineColor(kBlack);
    lineZero2->SetLineStyle(7);
    lineZero2->Draw("SAME");

    TLine* lineRes2 = new TLine(1.0, 0.0, 1.0, 600.0);
    lineRes2->SetLineColor(kGray + 1);
    lineRes2->SetLineStyle(3);
    lineRes2->Draw("SAME");

    TLegend* leg2 = new TLegend(0.48, 0.72, 0.94, 0.88);
    leg2->SetBorderSize(1);
    leg2->SetFillColor(kWhite);
    leg2->SetTextSize(0.030);
    leg2->AddEntry(grRelRoi, "Error Relatif Absolut (%) roiCounts", "lp");
    leg2->AddEntry(grRelFlux, "Error Relatif Absolut (%) Flux", "lp");
    leg2->Draw();

    TLatex t2;
    t2.SetNDC();
    t2.SetTextSize(0.042);
    t2.SetTextFont(62);
    t2.DrawLatex(0.15, 0.93, "(b) Error Relatif Absolut (%) vs Lebar Aktual");

    // Anotasi 0.1 cm
    TLatex annot;
    annot.SetTextSize(0.032);
    annot.SetTextColor(kBlue + 2);
    annot.SetTextFont(62);
    annot.DrawLatex(0.25, 522.0, "0.1 cm: 522.1%");

    c->Modified();
    c->Update();
    TString outPath = Form("%s/grafik_error_dual_panel.png", outDir.c_str());
    c->Print(outPath.Data());
    cout << "[INFO] Disimpan: " << outPath << endl;

    delete c;
}

// -----------------------------------------------------------------------------
// 2. PLOT DUAL-AXIS (TWIN AXIS): Error Absolut (cm) & Relatif Absolut (%)
// -----------------------------------------------------------------------------
void PlotDualAxis(const vector<SummaryRow>& sumData, const string& outDir) {
    int n = sumData.size();
    if (n == 0) return;

    TGraph* grAbs = new TGraph(n);
    TGraph* grRel = new TGraph(n);

    for (int i = 0; i < n; ++i) {
        grAbs->SetPoint(i, sumData[i].actual_w, sumData[i].abs_err_roi);
        grRel->SetPoint(i, sumData[i].actual_w, sumData[i].abs_rel_err_roi);
    }

    grAbs->SetLineColor(kBlue + 1);
    grAbs->SetLineWidth(2);
    grAbs->SetMarkerColor(kBlue + 1);
    grAbs->SetMarkerStyle(kFullCircle);
    grAbs->SetMarkerSize(1.2);

    grRel->SetLineColor(kOrange + 7);
    grRel->SetLineWidth(2);
    grRel->SetLineStyle(7);
    grRel->SetMarkerColor(kOrange + 7);
    grRel->SetMarkerStyle(kFullSquare);
    grRel->SetMarkerSize(1.1);

    TCanvas* c = new TCanvas("c_dual_axis", "Dual Axis Error", 1100, 600);
    gPad->SetGrid();
    gPad->SetLeftMargin(0.12);
    gPad->SetBottomMargin(0.13);
    gPad->SetRightMargin(0.12);
    gPad->SetTopMargin(0.10);

    grAbs->Draw("ALP");
    grAbs->GetXaxis()->SetTitle("Lebar Keretakan Sebenarnya (cm)");
    grAbs->GetYaxis()->SetTitle("Error Absolut |#DeltaW| (cm)");
    grAbs->GetXaxis()->SetTitleSize(0.042);
    grAbs->GetYaxis()->SetTitleSize(0.042);
    grAbs->GetXaxis()->SetLabelSize(0.038);
    grAbs->GetYaxis()->SetLabelSize(0.038);
    grAbs->GetYaxis()->SetTitleColor(kBlue + 1);
    grAbs->GetYaxis()->SetLabelColor(kBlue + 1);
    grAbs->GetXaxis()->SetTitleOffset(1.2);
    grAbs->GetYaxis()->SetTitleOffset(1.1);
    grAbs->GetXaxis()->SetLimits(0.0, 2.6);
    grAbs->SetMinimum(0.0);
    grAbs->SetMaximum(0.75);

    // Transparan overlay pad untuk sumbu kanan (Error Relatif %)
    TPad* pad2 = new TPad("pad2", "", 0, 0, 1, 1);
    pad2->SetFillStyle(4000);
    pad2->SetFrameFillStyle(4000);
    pad2->SetLeftMargin(0.12);
    pad2->SetBottomMargin(0.13);
    pad2->SetRightMargin(0.12);
    pad2->SetTopMargin(0.10);
    pad2->Draw();
    pad2->cd();

    grRel->Draw("Y+LP");
    grRel->GetXaxis()->SetLimits(0.0, 2.6);
    grRel->GetYaxis()->SetTitle("Error Relatif Absolut (%)");
    grRel->GetYaxis()->SetTitleSize(0.042);
    grRel->GetYaxis()->SetLabelSize(0.038);
    grRel->GetYaxis()->SetTitleOffset(1.1);
    grRel->GetYaxis()->SetTitleColor(kOrange + 7);
    grRel->GetYaxis()->SetLabelColor(kOrange + 7);
    grRel->SetMinimum(0.0);
    grRel->SetMaximum(600.0);

    // Legenda gabungan
    TLegend* leg = new TLegend(0.35, 0.72, 0.75, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillColor(kWhite);
    leg->SetTextSize(0.030);
    leg->AddEntry(grAbs, "Error Absolut |#DeltaW| (cm) - Sumbu Kiri", "lp");
    leg->AddEntry(grRel, "Error Relatif Absolut (%) - Sumbu Kanan", "lp");
    leg->Draw();

    TLatex t;
    t.SetNDC();
    t.SetTextSize(0.042);
    t.SetTextFont(62);
    t.SetTextAlign(22);
    t.DrawLatex(0.50, 0.95, "Grafik Tunggal Terpadu (Dual Y-Axis): Error Absolut (cm) & Relatif (%)");

    c->Modified();
    c->Update();
    TString outPath = Form("%s/grafik_error_dual_axis.png", outDir.c_str());
    c->Print(outPath.Data());
    cout << "[INFO] Disimpan: " << outPath << endl;

    delete c;
}

// -----------------------------------------------------------------------------
// 3. PLOT KOMPARASI METODE DEKONVOLUSI (Raw vs Wiener vs Richardson-Lucy)
// -----------------------------------------------------------------------------
void PlotDeconvComparison(const vector<DeconvRow>& decData,
                          const vector<RLRow>& rlData,
                          const string& outDir)
{
    int nDec = decData.size();
    int nRL  = rlData.size();
    if (nDec == 0) return;

    TGraph* grRawAbs = new TGraph(nDec);
    TGraph* grDecAbs = new TGraph(nDec);
    TGraph* grRLAbs  = new TGraph(nRL);

    TGraph* grRawRel = new TGraph(nDec);
    TGraph* grDecRel = new TGraph(nDec);
    TGraph* grRLRel  = new TGraph(nRL);

    for (int i = 0; i < nDec; ++i) {
        grRawAbs->SetPoint(i, decData[i].actual_w, decData[i].abs_err_raw);
        grDecAbs->SetPoint(i, decData[i].actual_w, decData[i].abs_err_dec);
        grRawRel->SetPoint(i, decData[i].actual_w, decData[i].abs_rel_raw);
        grDecRel->SetPoint(i, decData[i].actual_w, decData[i].abs_rel_dec);
    }
    for (int i = 0; i < nRL; ++i) {
        grRLAbs->SetPoint(i, rlData[i].actual_w, rlData[i].abs_err_rl);
        grRLRel->SetPoint(i, rlData[i].actual_w, rlData[i].abs_rel_rl);
    }

    // Styling
    grRawAbs->SetLineColor(kGray + 2);
    grRawAbs->SetLineWidth(2);
    grRawAbs->SetMarkerColor(kGray + 2);
    grRawAbs->SetMarkerStyle(kFullCircle);

    grDecAbs->SetLineColor(kBlue + 1);
    grDecAbs->SetLineWidth(2);
    grDecAbs->SetMarkerColor(kBlue + 1);
    grDecAbs->SetMarkerStyle(kFullSquare);

    grRLAbs->SetLineColor(kGreen + 2);
    grRLAbs->SetLineWidth(2);
    grRLAbs->SetLineStyle(7);
    grRLAbs->SetMarkerColor(kGreen + 2);
    grRLAbs->SetMarkerStyle(kFullTriangleUp);

    grRawRel->SetLineColor(kGray + 2);
    grRawRel->SetLineWidth(2);
    grRawRel->SetMarkerColor(kGray + 2);
    grRawRel->SetMarkerStyle(kFullCircle);

    grDecRel->SetLineColor(kBlue + 1);
    grDecRel->SetLineWidth(2);
    grDecRel->SetMarkerColor(kBlue + 1);
    grDecRel->SetMarkerStyle(kFullSquare);

    grRLRel->SetLineColor(kGreen + 2);
    grRLRel->SetLineWidth(2);
    grRLRel->SetLineStyle(7);
    grRLRel->SetMarkerColor(kGreen + 2);
    grRLRel->SetMarkerStyle(kFullTriangleUp);

    TCanvas* c = new TCanvas("c_deconv_comp", "Deconvolution Comparison", 1500, 600);
    c->Divide(2, 1, 0.005, 0.005);

    // Panel 1: Absolut
    c->cd(1);
    gPad->SetGrid();
    gPad->SetLeftMargin(0.13);
    gPad->SetBottomMargin(0.13);
    gPad->SetRightMargin(0.04);
    gPad->SetTopMargin(0.10);

    TMultiGraph* mg1 = new TMultiGraph();
    mg1->Add(grRawAbs, "LP");
    mg1->Add(grDecAbs, "LP");
    if (nRL > 0) mg1->Add(grRLAbs, "LP");
    mg1->Draw("A");

    mg1->GetXaxis()->SetTitle("Lebar Keretakan Sebenarnya (cm)");
    mg1->GetYaxis()->SetTitle("Error Absolut: |W_{terukur} - W_{aktual}| (cm)");
    mg1->GetXaxis()->SetTitleSize(0.042);
    mg1->GetYaxis()->SetTitleSize(0.042);
    mg1->GetXaxis()->SetLimits(0.0, 2.6);
    mg1->SetMinimum(0.0);
    mg1->SetMaximum(1.05);

    TLegend* leg1 = new TLegend(0.48, 0.68, 0.94, 0.88);
    leg1->SetBorderSize(1);
    leg1->SetFillColor(kWhite);
    leg1->SetTextSize(0.030);
    leg1->AddEntry(grRawAbs, "FWHM Mentah (Raw)", "lp");
    leg1->AddEntry(grDecAbs, "Dekonvolusi Wiener", "lp");
    if (nRL > 0) leg1->AddEntry(grRLAbs, "Dekonvolusi Richardson-Lucy", "lp");
    leg1->Draw();

    TLatex t1;
    t1.SetNDC();
    t1.SetTextSize(0.042);
    t1.SetTextFont(62);
    t1.DrawLatex(0.15, 0.93, "(a) Perbandingan Error Absolut Antar-Metode (cm)");

    // Panel 2: Relatif
    c->cd(2);
    gPad->SetGrid();
    gPad->SetLeftMargin(0.13);
    gPad->SetBottomMargin(0.13);
    gPad->SetRightMargin(0.04);
    gPad->SetTopMargin(0.10);

    TMultiGraph* mg2 = new TMultiGraph();
    mg2->Add(grRawRel, "LP");
    mg2->Add(grDecRel, "LP");
    if (nRL > 0) mg2->Add(grRLRel, "LP");
    mg2->Draw("A");

    mg2->GetXaxis()->SetTitle("Lebar Keretakan Sebenarnya (cm)");
    mg2->GetYaxis()->SetTitle("Error Relatif Absolut (%)");
    mg2->GetXaxis()->SetTitleSize(0.042);
    mg2->GetYaxis()->SetTitleSize(0.042);
    mg2->GetXaxis()->SetLimits(0.0, 2.6);
    mg2->SetMinimum(0.0);
    mg2->SetMaximum(600.0);

    TLegend* leg2 = new TLegend(0.48, 0.68, 0.94, 0.88);
    leg2->SetBorderSize(1);
    leg2->SetFillColor(kWhite);
    leg2->SetTextSize(0.030);
    leg2->AddEntry(grRawRel, "FWHM Mentah (Raw)", "lp");
    leg2->AddEntry(grDecRel, "Dekonvolusi Wiener", "lp");
    if (nRL > 0) leg2->AddEntry(grRLRel, "Dekonvolusi Richardson-Lucy", "lp");
    leg2->Draw();

    TLatex t2;
    t2.SetNDC();
    t2.SetTextSize(0.042);
    t2.SetTextFont(62);
    t2.DrawLatex(0.15, 0.93, "(b) Perbandingan Error Relatif Absolut (%)");

    c->Modified();
    c->Update();
    TString outPath = Form("%s/grafik_komparasi_metode_dekonvolusi.png", outDir.c_str());
    c->Print(outPath.Data());
    cout << "[INFO] Disimpan: " << outPath << endl;

    delete c;
}

// -----------------------------------------------------------------------------
// 4. PLOT REDUKSI ERROR KALIBRASI POLINOMIAL
// -----------------------------------------------------------------------------
void PlotCalibrationReduction(const vector<DeconvRow>& decData, const string& outDir) {
    int n = decData.size();
    if (n == 0) return;

    TGraph* grRawErr = new TGraph(n);
    TGraph* grCalRaw = new TGraph(n);
    TGraph* grCalDec = new TGraph(n);

    double sumRaw = 0, sumCalRaw = 0, sumCalDec = 0;

    for (int i = 0; i < n; ++i) {
        double errRaw = decData[i].abs_err_raw;
        double errCalRaw = fabs(decData[i].cal_raw - decData[i].actual_w);
        double errCalDec = fabs(decData[i].cal_dec - decData[i].actual_w);

        grRawErr->SetPoint(i, decData[i].actual_w, errRaw);
        grCalRaw->SetPoint(i, decData[i].actual_w, errCalRaw);
        grCalDec->SetPoint(i, decData[i].actual_w, errCalDec);

        sumRaw += errRaw;
        sumCalRaw += errCalRaw;
        sumCalDec += errCalDec;
    }

    double maeRaw = sumRaw / n;
    double maeCalRaw = sumCalRaw / n;
    double maeCalDec = sumCalDec / n;

    grRawErr->SetLineColor(kRed + 1);
    grRawErr->SetLineWidth(2);
    grRawErr->SetMarkerColor(kRed + 1);
    grRawErr->SetMarkerStyle(kFullCircle);
    grRawErr->SetMarkerSize(1.1);

    grCalRaw->SetLineColor(kBlue + 1);
    grCalRaw->SetLineWidth(2);
    grCalRaw->SetMarkerColor(kBlue + 1);
    grCalRaw->SetMarkerStyle(kFullSquare);
    grCalRaw->SetMarkerSize(1.1);

    grCalDec->SetLineColor(kGreen + 2);
    grCalDec->SetLineWidth(2);
    grCalDec->SetMarkerColor(kGreen + 2);
    grCalDec->SetMarkerStyle(kFullTriangleUp);
    grCalDec->SetMarkerSize(1.1);

    TCanvas* c = new TCanvas("c_cal_reduction", "Calibration Reduction", 1000, 600);
    gPad->SetGrid();
    gPad->SetLeftMargin(0.12);
    gPad->SetBottomMargin(0.13);
    gPad->SetRightMargin(0.05);
    gPad->SetTopMargin(0.10);

    TMultiGraph* mg = new TMultiGraph();
    mg->Add(grRawErr, "LP");
    mg->Add(grCalRaw, "LP");
    mg->Add(grCalDec, "LP");
    mg->Draw("A");

    mg->GetXaxis()->SetTitle("Lebar Keretakan Sebenarnya (cm)");
    mg->GetYaxis()->SetTitle("Error Absolut |#DeltaW| (cm)");
    mg->GetXaxis()->SetTitleSize(0.042);
    mg->GetYaxis()->SetTitleSize(0.042);
    mg->GetXaxis()->SetLabelSize(0.038);
    mg->GetYaxis()->SetLabelSize(0.038);
    mg->GetXaxis()->SetTitleOffset(1.2);
    mg->GetYaxis()->SetTitleOffset(1.2);
    mg->GetXaxis()->SetLimits(0.0, 2.6);
    mg->SetMinimum(0.0);
    mg->SetMaximum(0.70);

    TLegend* leg = new TLegend(0.42, 0.68, 0.93, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillColor(kWhite);
    leg->SetTextSize(0.028);
    leg->AddEntry(grRawErr, Form("Error FWHM Mentah (MAE = %.3f cm)", maeRaw), "lp");
    leg->AddEntry(grCalRaw, Form("Setelah Kalibrasi Polinomial (MAE = %.3f cm)", maeCalRaw), "lp");
    leg->AddEntry(grCalDec, Form("Setelah Dekonvolusi + Kalibrasi (MAE = %.3f cm)", maeCalDec), "lp");
    leg->Draw();

    TLatex t;
    t.SetNDC();
    t.SetTextSize(0.040);
    t.SetTextFont(62);
    t.SetTextAlign(22);
    t.DrawLatex(0.50, 0.95, "Penurunan Error Absolut Pengukuran Menggunakan Model Kalibrasi Polinomial");

    c->Modified();
    c->Update();
    TString outPath = Form("%s/grafik_reduksi_error_kalibrasi.png", outDir.c_str());
    c->Print(outPath.Data());
    cout << "[INFO] Disimpan: " << outPath << endl;

    delete c;
}

// -----------------------------------------------------------------------------
// FUNGSI UTAMA (MAIN MACRO ENTRY)
// -----------------------------------------------------------------------------
void generate_error_plots() {
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);
    gStyle->SetPadGridX(kTRUE);
    gStyle->SetPadGridY(kTRUE);
    gStyle->SetGridColor(kGray);
    gStyle->SetGridStyle(3);
    gStyle->SetGridWidth(1);

    string sumPath   = FindPath("fwhm_vs_actual_summary.csv");
    string decPath   = FindPath("deconvolution_fwhm_comparison.csv");
    string rlPath    = FindPath("richardson_lucy_fwhm_comparison.csv");

    // Direktori output: jika file ada di grafik_analisis_error, simpan di situ
    string outDir = ".";
    if (gSystem->AccessPathName("grafik_analisis_error") == 0) {
        outDir = "grafik_analisis_error";
    }

    cout << "=============================================================" << endl;
    cout << "   GENERATOR GRAFIK ANALISIS ERROR MENGGUNAKAN ROOT C       " << endl;
    cout << "=============================================================" << endl;
    cout << "[INFO] Path Summary : " << sumPath << endl;
    cout << "[INFO] Path Wiener  : " << decPath << endl;
    cout << "[INFO] Path RL      : " << rlPath << endl;
    cout << "[INFO] Output Dir   : " << outDir << endl;
    cout << "-------------------------------------------------------------" << endl;

    vector<SummaryRow> sumData;
    if (LoadSummaryCSV(sumPath, sumData)) {
        cout << "[INFO] Data Summary berhasil dibaca (" << sumData.size() << " titik)." << endl;
        PlotDualPanel(sumData, outDir);
        PlotDualAxis(sumData, outDir);
    } else {
        cerr << "[WARN] Gagal membaca summary CSV: " << sumPath << endl;
    }

    vector<DeconvRow> decData;
    vector<RLRow> rlData;
    bool hasDec = LoadDeconvCSV(decPath, decData);
    bool hasRL  = LoadRLCSV(rlPath, rlData);

    if (hasDec) {
        cout << "[INFO] Data Dekonvolusi berhasil dibaca (" << decData.size() << " titik)." << endl;
        PlotDeconvComparison(decData, rlData, outDir);
        PlotCalibrationReduction(decData, outDir);
    }

    cout << "=============================================================" << endl;
    cout << "[SELESAI] Seluruh grafik analisis error sukses dibuat via ROOT C!" << endl;
    cout << "=============================================================" << endl;
}

