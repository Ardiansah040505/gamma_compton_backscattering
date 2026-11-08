#include <TFile.h>
#include <TProfile.h>
#include <TF1.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TPaveStats.h>
#include <TPaveText.h>
#include <TStyle.h>
#include <TGraphErrors.h>
#include <TSystem.h>
#include <TMath.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

void GaussianFitting() {
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(0);

    const char* inFileName = "profiles_backscatter_detectors.root";
    TFile *fin = TFile::Open(inFileName, "READ");
    if (!fin || fin->IsZombie()) {
        std::cerr << "Error: cannot open input file " << inFileName << std::endl;
        return;
    }

    gSystem->mkdir("per_detector", kTRUE);

    const char* outRootName = "profiles_backscatter_fitted.root";
    TFile *fout = TFile::Open(outRootName, "RECREATE");

    std::ofstream txtOut("gaussian_fit_results.txt");
    std::ofstream csvOut("gaussian_fit_results.csv");

    csvOut << "Detector,Chi2,NDF,Chi2_NDF,p0,p0_err,p1,p1_err,A1,A1_err,mu1,mu1_err,sigma1,sigma1_err,FWHM1,FWHM1_err,A2,A2_err,mu2,mu2_err,sigma2,sigma2_err,FWHM2,FWHM2_err,Delta_mu,Delta_mu_err\n";

    txtOut << "========================================================================================================\n";
    txtOut << "                    GAUSSIAN FITTING RESULTS - BACKSCATTER DETECTOR PROFILES                           \n";
    txtOut << "  Model: f(x) = p0 + p1*x - A1*exp(-(x-mu1)^2/(2*sigma1^2)) - A2*exp(-(x-mu2)^2/(2*sigma2^2))          \n";
    txtOut << "  Fit Range: [-35.0, 35.0] mm                                                                          \n";
    txtOut << "========================================================================================================\n\n";

    const int nDet = 6;
    Color_t detColors[nDet] = {kRed+1, kBlue+1, kGreen+2, kOrange+7, kViolet+1, kTeal+2};

    TCanvas *cAll = new TCanvas("c_all_detectors", "All Detectors Fit Summary", 1600, 1000);
    cAll->Divide(3, 2);

    TCanvas *cOverlay = new TCanvas("c_overlay", "Fitted Backscatter Profiles Overlay", 1100, 750);
    cOverlay->SetGrid();
    TLegend *legOverlay = new TLegend(0.68, 0.65, 0.88, 0.88);
    legOverlay->SetBorderSize(1);
    legOverlay->SetFillColor(kWhite);

    for (int d = 0; d < nDet; ++d) {
        TString profName = Form("prof_det%d", d);
        TProfile *prof = (TProfile*)fin->Get(profName);
        if (!prof) {
            std::cerr << "Warning: " << profName << " not found in " << inFileName << std::endl;
            continue;
        }

        TProfile *profClone = (TProfile*)prof->Clone(Form("prof_fit_det%d", d));
        profClone->SetDirectory(fout);

        TString fitName = Form("fit_det%d", d);
        // Double Gaussian dip on linear baseline
        TF1 *fitFunc = new TF1(fitName, "[0] + [1]*x - [2]*TMath::Gaus(x, [3], [4]) - [5]*TMath::Gaus(x, [6], [7])", -35.0, 35.0);
        fitFunc->SetParNames("p0", "p1", "A1", "mu1", "sigma1", "A2", "mu2", "sigma2");

        // Initial parameter estimates based on data
        fitFunc->SetParameters(206.0, 0.0, 6.0, -12.0, 7.0, 6.0, 11.0, 7.0);
        fitFunc->SetParLimits(0, 195.0, 220.0);
        fitFunc->SetParLimits(2, 0.5, 20.0);
        fitFunc->SetParLimits(3, -25.0, -4.0);
        fitFunc->SetParLimits(4, 1.5, 20.0);
        fitFunc->SetParLimits(5, 0.5, 20.0);
        fitFunc->SetParLimits(6, 4.0, 25.0);
        fitFunc->SetParLimits(7, 1.5, 20.0);

        fitFunc->SetLineColor(kBlack);
        fitFunc->SetLineWidth(2);

        profClone->Fit(fitFunc, "Q R");

        double chi2 = fitFunc->GetChisquare();
        int ndf = fitFunc->GetNDF();
        double chi2Ndf = (ndf > 0) ? (chi2 / ndf) : 0.0;

        double p0 = fitFunc->GetParameter(0);
        double p0Err = fitFunc->GetParError(0);
        double p1 = fitFunc->GetParameter(1);
        double p1Err = fitFunc->GetParError(1);

        double A1 = fitFunc->GetParameter(2);
        double A1Err = fitFunc->GetParError(2);
        double mu1 = fitFunc->GetParameter(3);
        double mu1Err = fitFunc->GetParError(3);
        double sig1 = fitFunc->GetParameter(4);
        double sig1Err = fitFunc->GetParError(4);
        double fwhm1 = 2.354820 * sig1;
        double fwhm1Err = 2.354820 * sig1Err;

        double A2 = fitFunc->GetParameter(5);
        double A2Err = fitFunc->GetParError(5);
        double mu2 = fitFunc->GetParameter(6);
        double mu2Err = fitFunc->GetParError(6);
        double sig2 = fitFunc->GetParameter(7);
        double sig2Err = fitFunc->GetParError(7);
        double fwhm2 = 2.354820 * sig2;
        double fwhm2Err = 2.354820 * sig2Err;

        double deltaMu = std::abs(mu2 - mu1);
        double deltaMuErr = std::sqrt(mu1Err * mu1Err + mu2Err * mu2Err);

        // Print to stdout
        std::cout << Form(">>> Detector %d: Chi2/NDF = %.3f (%.1f/%d) | Dip1 mu = %.2f +- %.2f mm, FWHM = %.2f mm | Dip2 mu = %.2f +- %.2f mm, FWHM = %.2f mm | Delta_mu = %.2f +- %.2f mm\n",
                          d, chi2Ndf, chi2, ndf, mu1, mu1Err, fwhm1, mu2, mu2Err, fwhm2, deltaMu, deltaMuErr);

        // Write to text output
        txtOut << Form("Detector %d:\n", d);
        txtOut << Form("  Chi2 / NDF           : %.3f / %d = %.3f\n", chi2, ndf, chi2Ndf);
        txtOut << Form("  Baseline (p0 + p1*x) : (%.3f +- %.3f) + (%.4f +- %.4f)*x keV\n", p0, p0Err, p1, p1Err);
        txtOut << Form("  Dip 1 (Left Wall)    : Amplitude = %.3f +- %.3f keV, Centroid (mu1) = %.3f +- %.3f mm, Sigma = %.3f +- %.3f mm, FWHM = %.3f +- %.3f mm\n",
                       A1, A1Err, mu1, mu1Err, sig1, sig1Err, fwhm1, fwhm1Err);
        txtOut << Form("  Dip 2 (Right Wall)   : Amplitude = %.3f +- %.3f keV, Centroid (mu2) = %.3f +- %.3f mm, Sigma = %.3f +- %.3f mm, FWHM = %.3f +- %.3f mm\n",
                       A2, A2Err, mu2, mu2Err, sig2, sig2Err, fwhm2, fwhm2Err);
        txtOut << Form("  Wall Separation      : Delta_mu = |mu2 - mu1| = %.3f +- %.3f mm\n\n", deltaMu, deltaMuErr);

        // Write to CSV
        csvOut << d << "," << chi2 << "," << ndf << "," << chi2Ndf << ","
               << p0 << "," << p0Err << "," << p1 << "," << p1Err << ","
               << A1 << "," << A1Err << "," << mu1 << "," << mu1Err << ","
               << sig1 << "," << sig1Err << "," << fwhm1 << "," << fwhm1Err << ","
               << A2 << "," << A2Err << "," << mu2 << "," << mu2Err << ","
               << sig2 << "," << sig2Err << "," << fwhm2 << "," << fwhm2Err << ","
               << deltaMu << "," << deltaMuErr << "\n";

        // Subcomponents for individual display
        TF1 *fBg = new TF1(Form("bg_det%d", d), "[0] + [1]*x", -35.0, 35.0);
        fBg->SetParameters(p0, p1);
        fBg->SetLineColor(kGray+2);
        fBg->SetLineStyle(kDotted);
        fBg->SetLineWidth(2);

        TF1 *fDip1 = new TF1(Form("dip1_det%d", d), "[0] + [1]*x - [2]*TMath::Gaus(x, [3], [4])", -35.0, 35.0);
        fDip1->SetParameters(p0, p1, A1, mu1, sig1);
        fDip1->SetLineColor(kBlue);
        fDip1->SetLineStyle(kDashed);
        fDip1->SetLineWidth(2);

        TF1 *fDip2 = new TF1(Form("dip2_det%d", d), "[0] + [1]*x - [2]*TMath::Gaus(x, [3], [4])", -35.0, 35.0);
        fDip2->SetParameters(p0, p1, A2, mu2, sig2);
        fDip2->SetLineColor(kGreen+2);
        fDip2->SetLineStyle(kDashed);
        fDip2->SetLineWidth(2);

        // --- Individual Detector Canvas ---
        TCanvas *cDet = new TCanvas(Form("c_det%d", d), Form("Detector %d Fit", d), 1000, 700);
        cDet->SetGrid();

        profClone->SetTitle(Form("Backscatter Profile Fit - Detector %d;Position x (mm);Mean Backscatter Energy (keV)", d));
        profClone->SetMarkerStyle(20);
        profClone->SetMarkerSize(0.8);
        profClone->SetMarkerColor(detColors[d]);
        profClone->SetLineColor(detColors[d]);
        profClone->GetYaxis()->SetRangeUser(198.0, 212.0);
        profClone->Draw("E1");

        fitFunc->Draw("SAME");
        fBg->Draw("SAME");
        fDip1->Draw("SAME");
        fDip2->Draw("SAME");

        TLegend *leg = new TLegend(0.15, 0.70, 0.48, 0.88);
        leg->SetBorderSize(1);
        leg->SetFillColor(kWhite);
        leg->AddEntry(profClone, Form("Detector %d Profile", d), "ep");
        leg->AddEntry(fitFunc, "Total Fit (Double Gaus Dip)", "l");
        leg->AddEntry(fDip1, Form("Dip 1 (#mu_{1}=%.1f mm)", mu1), "l");
        leg->AddEntry(fDip2, Form("Dip 2 (#mu_{2}=%.1f mm)", mu2), "l");
        leg->AddEntry(fBg, "Baseline Background", "l");
        leg->Draw();

        TPaveText *pave = new TPaveText(0.56, 0.67, 0.88, 0.88, "NDC");
        pave->SetBorderSize(1);
        pave->SetFillColor(kWhite);
        pave->SetTextAlign(12);
        pave->SetTextSize(0.026);
        pave->AddText(Form("#chi^{2} / NDF = %.2f / %d = %.3f", chi2, ndf, chi2Ndf));
        pave->AddText(Form("#mu_{1} = %.2f #pm %.2f mm  (FWHM=%.2f mm)", mu1, mu1Err, fwhm1));
        pave->AddText(Form("#mu_{2} = %.2f #pm %.2f mm  (FWHM=%.2f mm)", mu2, mu2Err, fwhm2));
        pave->AddText(Form("#Delta#mu = %.2f #pm %.2f mm", deltaMu, deltaMuErr));
        pave->Draw();

        cDet->SaveAs(Form("per_detector/Fit_Det%d.png", d));
        cDet->Write(Form("canvas_det%d", d));

        // --- Add to 6-panel summary canvas ---
        cAll->cd(d + 1);
        gPad->SetGrid();
        TProfile *pSub = (TProfile*)profClone->DrawClone("E1");
        pSub->SetTitle(Form("Detector %d: #Delta#mu = %.2f #pm %.2f mm (#chi^{2}/ndf = %.2f);Position x (mm);Energy (keV)",
                            d, deltaMu, deltaMuErr, chi2Ndf));
        pSub->GetYaxis()->SetRangeUser(198.0, 212.0);
        fitFunc->Draw("SAME");
        fDip1->Draw("SAME");
        fDip2->Draw("SAME");

        // --- Add to Overlay Canvas ---
        cOverlay->cd();
        if (d == 0) {
            profClone->Draw("E1");
        } else {
            profClone->Draw("E1 SAME");
        }
        fitFunc->SetLineColor(detColors[d]);
        fitFunc->Draw("SAME");
        legOverlay->AddEntry(profClone, Form("Det %d (#mu_{1}=%.1f, #mu_{2}=%.1f mm)", d, mu1, mu2), "lep");

        fout->cd();
        fitFunc->Write(fitName);
    }

    cAll->SaveAs("Fit_All_Detectors_Summary.png");
    cAll->Write("canvas_all_summary");

    cOverlay->cd();
    legOverlay->Draw();
    cOverlay->SaveAs("Fit_Combined_Overlay.png");
    cOverlay->Write("canvas_overlay");

    txtOut.close();
    csvOut.close();
    fout->Close();
    fin->Close();

    std::cout << "\nGaussian fitting completed successfully." << std::endl;
    std::cout << "Outputs saved:" << std::endl;
    std::cout << "  - per_detector/Fit_Det0.png .. Fit_Det5.png" << std::endl;
    std::cout << "  - Fit_All_Detectors_Summary.png" << std::endl;
    std::cout << "  - Fit_Combined_Overlay.png" << std::endl;
    std::cout << "  - gaussian_fit_results.txt" << std::endl;
    std::cout << "  - gaussian_fit_results.csv" << std::endl;
    std::cout << "  - profiles_backscatter_fitted.root" << std::endl;
}

