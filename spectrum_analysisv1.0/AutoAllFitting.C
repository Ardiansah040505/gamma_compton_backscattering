#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include "TFile.h"
#include "TH1D.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TRandom3.h"
#include "TFitResult.h"
#include "TKey.h"
#include "TSystem.h"
#include "TSystemDirectory.h"
#include "TList.h"
#include "TROOT.h"

using namespace std;

// =============================================================================
// Struktur Data untuk Menyimpan Hasil Fit Tiap Region
// =============================================================================
struct FitResultRecord {
    TString fileName;
    TString histName;
    TString regionType;
    double  cut_min;
    double  cut_max;
    double  constant;
    double  constant_err;
    double  mean;
    double  mean_err;
    double  sigma;
    double  sigma_err;
    double  fwhm;
    double  fwhm_err;
    double  res_calc;
    double  chi2;
    int     ndf;
    double  chi2_ndf;
    double  prob;
    bool    isValid;
};

// =============================================================================
// Fungsi Helper: Proses Smearing, Gaussian Fit, Plotting, dan Ekstraksi Parameter
// =============================================================================
FitResultRecord ProcessFitRegion(TH1D*       h_raw,
                                 const char* fileName,
                                 const char* regionType,
                                 double      cut_min,
                                 double      cut_max,
                                 double      resolution,
                                 double      E_ref,
                                 const char* outPngPath)
{
    FitResultRecord rec;
    rec.fileName   = gSystem->BaseName(fileName);
    rec.histName   = h_raw->GetName();
    rec.regionType = regionType;
    rec.cut_min    = cut_min;
    rec.cut_max    = cut_max;
    rec.constant = rec.constant_err = 0.0;
    rec.mean     = rec.mean_err     = 0.0;
    rec.sigma    = rec.sigma_err    = 0.0;
    rec.fwhm     = rec.fwhm_err     = 0.0;
    rec.res_calc = 0.0;
    rec.chi2     = 0.0;
    rec.ndf      = 0;
    rec.chi2_ndf = 0.0;
    rec.prob     = 0.0;
    rec.isValid  = false;

    // 1. Buat histogram smeared
    TString smearedHistName = Form("%s_smeared_%s", h_raw->GetName(), regionType);
    TH1D* h_smeared = (TH1D*)h_raw->Clone(smearedHistName.Data());
    h_smeared->Reset();
    h_smeared->Sumw2();
    h_smeared->SetDirectory(0);
    h_smeared->SetTitle(Form("%s - %s (%.0f - %.0f keV);Energy (keV);Counts",
                             rec.histName.Data(), regionType, cut_min, cut_max));
    h_smeared->SetLineColor(kRed);
    h_smeared->SetLineWidth(2);

    double sigma_ref = (resolution * E_ref) / 2.3548;
    TRandom3 rng(0);

    // Margin ekstra untuk Compton Edge agar tidak terpotong di tepi
    double margin = (TString(regionType) == "ComptonEdge") ? 3.0 * sigma_ref : 0.0;

    int nbins = h_raw->GetNbinsX();
    for (int i = 1; i <= nbins; ++i) {
        double E_center  = h_raw->GetBinCenter(i);
        double bin_width = h_raw->GetBinWidth(i);
        int    n_events  = (int)std::round(h_raw->GetBinContent(i));

        if (n_events <= 0 || E_center <= 0) continue;
        if (cut_max > cut_min && (E_center < (cut_min - margin) || E_center > (cut_max + margin))) continue;

        for (int ev = 0; ev < n_events; ++ev) {
            double E_awal = E_center + rng.Uniform(-bin_width / 2.0, bin_width / 2.0);
            if (E_awal <= 0) continue;

            double sigma_E = sigma_ref * std::sqrt(E_awal / E_ref);
            double E_baru  = rng.Gaus(E_awal, sigma_E);

            if (cut_max > cut_min && (E_baru < cut_min || E_baru > cut_max)) continue;
            h_smeared->Fill(E_baru);
        }
    }

    // 2. Fitting Gaussian
    gStyle->SetOptFit(0);
    TF1* gausFit = new TF1(Form("gausFit_%s_%s", h_raw->GetName(), regionType), "gaus", cut_min, cut_max);
    gausFit->SetLineColor(kBlue);
    gausFit->SetLineWidth(2);

    // Inisialisasi parameter awal spesifik per region
    double amp_init   = h_smeared->GetMaximum();
    double mean_init  = (cut_min + cut_max) / 2.0;
    double sigma_init = sigma_ref;

    if (TString(regionType) == "Photopeak") {
        mean_init  = (E_ref >= cut_min && E_ref <= cut_max) ? E_ref : (cut_min + cut_max) / 2.0;
        sigma_init = (resolution * E_ref) / 2.3548;
    } else if (TString(regionType) == "Backscatter") {
        int bin_max = h_smeared->GetMaximumBin();
        if (bin_max > 0) {
            mean_init = h_smeared->GetBinCenter(bin_max);
            amp_init  = h_smeared->GetBinContent(bin_max);
        }
        sigma_init = sigma_ref * std::sqrt(mean_init / E_ref);
    } else if (TString(regionType) == "ComptonEdge") {
        int bin_max = h_smeared->GetMaximumBin();
        if (bin_max > 0) {
            mean_init = h_smeared->GetBinCenter(bin_max);
            amp_init  = h_smeared->GetBinContent(bin_max);
        }
        sigma_init = sigma_ref * std::sqrt(477.3 / E_ref);
    }

    gausFit->SetParameters(amp_init, mean_init, sigma_init);

    // Eksekusi fit
    TFitResultPtr fitResult = h_smeared->Fit(gausFit, "R Q S");

    rec.isValid      = (fitResult.Get() != nullptr && fitResult->IsValid());
    rec.constant     = gausFit->GetParameter(0);
    rec.constant_err = gausFit->GetParError(0);
    rec.mean         = gausFit->GetParameter(1);
    rec.mean_err     = gausFit->GetParError(1);
    rec.sigma        = gausFit->GetParameter(2);
    rec.sigma_err    = gausFit->GetParError(2);
    rec.fwhm         = 2.35482 * rec.sigma;
    rec.fwhm_err     = 2.35482 * rec.sigma_err;
    rec.res_calc     = (rec.mean > 0) ? (rec.fwhm / rec.mean) * 100.0 : 0.0;
    rec.chi2         = gausFit->GetChisquare();
    rec.ndf          = gausFit->GetNDF();
    rec.chi2_ndf     = (rec.ndf > 0) ? (rec.chi2 / rec.ndf) : 0.0;
    rec.prob         = gausFit->GetProb();

    // 3. Gambar Canvas & Legend
    TCanvas* c1 = new TCanvas("c_batch", "Batch Fit Canvas", 900, 600);
    c1->SetGrid();

    h_smeared->SetStats(0);
    h_smeared->Draw("E1");
    gausFit->Draw("SAME");

    if (cut_max > cut_min) {
        h_smeared->GetXaxis()->SetRangeUser(cut_min, cut_max);
    }

    TLegend* leg = new TLegend(0.55, 0.60, 0.88, 0.88);
    leg->SetBorderSize(0);
    leg->SetFillStyle(0);
    leg->SetTextSize(0.028);
    leg->AddEntry(h_smeared, "Smeared Data", "l");
    leg->AddEntry(gausFit, "Gaussian Fit", "l");
    leg->AddEntry((TObject*)0, Form("Mean = %.2f #pm %.2f keV", rec.mean, rec.mean_err), "");
    leg->AddEntry((TObject*)0, Form("#sigma = %.2f #pm %.2f keV", rec.sigma, rec.sigma_err), "");
    leg->AddEntry((TObject*)0, Form("#chi^{2}/ndf = %.2f", rec.chi2_ndf), "");
    leg->AddEntry((TObject*)0, Form("Prob = %.4f", rec.prob), "");
    leg->Draw();

    c1->Modified();
    c1->Update();
    c1->Print(outPngPath);

    // Bersihkan memori canvas & fungsi lokal
    delete leg;
    delete c1;
    delete gausFit;
    delete h_smeared;

    return rec;
}

// =============================================================================
// Fungsi Utama: AutoAllFitting
// =============================================================================
void AutoAllFitting(const char* targetFile = "",
                    double      resolution = 0.07,
                    double      E_ref      = 662.0)
{
    // Aktifkan mode batch agar tidak menampilkan pop-up ratusan window GUI
    gROOT->SetBatch(kTRUE);

    // Buat struktur folder output
    const char* baseOutDir = "result_all_fitting";
    gSystem->mkdir(baseOutDir, kTRUE);
    gSystem->mkdir(Form("%s/photopeak", baseOutDir), kTRUE);
    gSystem->mkdir(Form("%s/backscatter", baseOutDir), kTRUE);
    gSystem->mkdir(Form("%s/compton_edge", baseOutDir), kTRUE);

    // Buka file CSV ringkasan hasil
    TString csvPath = Form("%s/fitting_summary.csv", baseOutDir);
    ofstream csvFile(csvPath.Data(), ios::out);
    if (!csvFile.is_open()) {
        cout << "[ERROR] Gagal membuka file output CSV: " << csvPath << endl;
        return;
    }

    // Header CSV
    csvFile << "File,Histogram,Region,CutMin_keV,CutMax_keV,Constant,Constant_Err,"
            << "Mean_keV,Mean_Err,Sigma_keV,Sigma_Err,FWHM_keV,FWHM_Err,Resolution_Pct,"
            << "Chi2,NDF,Chi2_NDF,Prob,Status\n";

    // Kumpulkan daftar file target
    vector<TString> fileList;

    if (targetFile && strlen(targetFile) > 0) {
        fileList.push_back(targetFile);
    } else {
        TSystemDirectory dir(".", ".");
        TList* files = dir.GetListOfFiles();
        if (files) {
            TIter next(files);
            TSystemFile* file;
            while ((file = (TSystemFile*)next())) {
                TString fname = file->GetName();
                if (!file->IsDirectory() && fname.BeginsWith("spectrum_det") && fname.EndsWith(".root") && !fname.BeginsWith("smeared_")) {
                    fileList.push_back(fname);
                }
            }
        }
    }

    int totalFiles = fileList.size();
    if (totalFiles == 0) {
        cout << "[WARNING] Tidak ditemukan file 'spectrum_det*.root' untuk diproses!" << endl;
        csvFile.close();
        return;
    }

    cout << "\n================================================================================" << endl;
    cout << "                    OTOMATISASI FITTING SPEKTRUM GAMMA                          " << endl;
    cout << "================================================================================" << endl;
    cout << " Total File Target  : " << totalFiles << " file" << endl;
    cout << " Output Folder      : " << baseOutDir << "/" << endl;
    cout << " Rekapitulasi CSV   : " << csvPath << endl;
    cout << " Parameter Resolusi : " << (resolution * 100.0) << "% @ " << E_ref << " keV" << endl;
    cout << "--------------------------------------------------------------------------------" << endl;

    int totalHistograms = 0;
    int totalFits = 0;

    for (size_t fIdx = 0; fIdx < fileList.size(); ++fIdx) {
        TString curFileName = fileList[fIdx];
        TString baseName    = curFileName;
        baseName.ReplaceAll(".root", "");

        cout << Form("[%2zu/%2d] Memproses: %-32s ... ", fIdx + 1, totalFiles, curFileName.Data()) << flush;

        TFile* fin = TFile::Open(curFileName.Data(), "READ");
        if (!fin || fin->IsZombie()) {
            cout << "[GAGAL BUKA FILE]" << endl;
            continue;
        }

        TIter nextKey(fin->GetListOfKeys());
        TKey* key;
        int fileHists = 0;

        while ((key = (TKey*)nextKey())) {
            TObject* obj = key->ReadObj();
            if (obj && obj->InheritsFrom(TH1D::Class())) {
                TH1D* h_raw = (TH1D*)obj;
                TString hName = h_raw->GetName();
                fileHists++;
                totalHistograms++;

                // 1. Photopeak Fit (600 - 730 keV)
                TString pngPhotopeak = Form("%s/photopeak/Photopeak_%s_%s.png",
                                            baseOutDir, baseName.Data(), hName.Data());
                FitResultRecord r1 = ProcessFitRegion(h_raw, curFileName.Data(), "Photopeak",
                                                      600.0, 730.0, resolution, E_ref, pngPhotopeak.Data());

                // 2. Backscatter Fit (130 - 280 keV)
                TString pngBackscatter = Form("%s/backscatter/Backscatter_%s_%s.png",
                                              baseOutDir, baseName.Data(), hName.Data());
                FitResultRecord r2 = ProcessFitRegion(h_raw, curFileName.Data(), "Backscatter",
                                                      130.0, 280.0, resolution, E_ref, pngBackscatter.Data());

                // 3. Compton Edge Fit (400 - 540 keV)
                TString pngCompton = Form("%s/compton_edge/ComptonEdge_%s_%s.png",
                                          baseOutDir, baseName.Data(), hName.Data());
                FitResultRecord r3 = ProcessFitRegion(h_raw, curFileName.Data(), "ComptonEdge",
                                                      400.0, 540.0, resolution, E_ref, pngCompton.Data());

                // Simpan ke CSV
                vector<FitResultRecord> records = {r1, r2, r3};
                for (const auto& r : records) {
                    csvFile << fixed << setprecision(4)
                            << r.fileName << ","
                            << r.histName << ","
                            << r.regionType << ","
                            << r.cut_min << ","
                            << r.cut_max << ","
                            << r.constant << ","
                            << r.constant_err << ","
                            << r.mean << ","
                            << r.mean_err << ","
                            << r.sigma << ","
                            << r.sigma_err << ","
                            << r.fwhm << ","
                            << r.fwhm_err << ","
                            << r.res_calc << ","
                            << r.chi2 << ","
                            << r.ndf << ","
                            << r.chi2_ndf << ","
                            << r.prob << ","
                            << (r.isValid ? "SUCCESS" : "FAILED") << "\n";
                    totalFits++;
                }
            }
            delete obj;
        }

        fin->Close();
        delete fin;

        cout << Form("OK (%d histogram, %d fit)", fileHists, fileHists * 3) << endl;
    }

    csvFile.close();

    cout << "--------------------------------------------------------------------------------" << endl;
    cout << " SELESAI!" << endl;
    cout << " Total File Diproses      : " << totalFiles << endl;
    cout << " Total Histogram Diproses : " << totalHistograms << endl;
    cout << " Total Fit Dilakukan      : " << totalFits << endl;
    cout << " Gambar Hasil Fitting     : " << baseOutDir << "/ (subfolder photopeak, backscatter, compton_edge)" << endl;
    cout << " Rekapitulasi Tabel CSV   : " << csvPath << endl;
    cout << "================================================================================\n" << endl;
}

