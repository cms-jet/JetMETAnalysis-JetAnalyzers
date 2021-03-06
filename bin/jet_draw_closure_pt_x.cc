///////////////////////////////////////////////////////////////////
//
// jet_draw_closure_pt_x
// ---------------------
//
//            12/09/2011 Alexx Perloff  <aperloff@physics.tamu.edu>
///////////////////////////////////////////////////////////////////

#include "JetMETAnalysis/JetAnalyzers/interface/Settings.h"
#include "JetMETAnalysis/JetAnalyzers/interface/Style.h"
#include "JetMETAnalysis/JetUtilities/interface/CommandLine.h"

#include "TROOT.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TH1F.h"
#include "TH1D.h"
#include "TH2.h"
#include "TH2F.h"
#include "TF1.h"
#include "TString.h"
#include "TPaveText.h"
#include "TLatex.h"
#include "TLegend.h"

#include <fstream>
#include <string>
#include <stdio.h>
#include <stdarg.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////
// define local functions
////////////////////////////////////////////////////////////////////////////////

///CMS Preliminary label;
void cmsPrelim(double intLUMI = 0);

/// get the uppercase version of the algorithm name
TString getAlias(TString s);

////////////////////////////////////////////////////////////////////////////////
// main
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
int main(int argc,char**argv)
{
  gROOT->SetStyle("Plain");
  gStyle->SetOptStat(0);

  gSystem->Load("libFWCoreFWLite.so");
  
  //
  // evaluate command-line / configuration file options
  // 
  CommandLine cl;
  if (!cl.parse(argc,argv)) return 0;
  
  vector<TString> algs         = cl.getVector<TString> ("algs");
  TString         flavor       = cl.getValue<TString>  ("flavor",           "");
  TString         path         = cl.getValue<TString>  ("path",             "");
  bool            mpv          = cl.getValue<bool>     ("mpv",            true);
  TString         outputDir    = cl.getValue<TString>  ("outputDir",  "images");
  vector<TString> outputFormat = cl.getVector<TString> ("outputFormat", ".png:::.eps:::.pdf");
  bool            tdr          = cl.getValue<bool>     ("tdr",           false);

  if (!cl.check()) return 0;
  cl.print();

  if (tdr) {
     setStyle();
  }

  for(unsigned int a=0; a<algs.size(); a++)
    {
      //
      // Open the file containing the original, unformated closure plots
      //
      TFile *inf;
      if(path.IsNull()) path = string (gSystem->pwd())+"/";
      if(!path.EndsWith("/")) path+="/";

      if (!flavor.IsNull()) inf = new TFile(path+"Closure_"+algs[a]+"_"+flavor+".root");
      else inf = new TFile(path+"Closure_"+algs[a]+".root");

      //
      // book histograms and functions
      //
      char name[1024];
      TH2F *RespVsPt[4];
      RespVsPt[0] = (TH2F*)inf->Get("RespVsPt_Bar");
      RespVsPt[1] = (TH2F*)inf->Get("RespVsPt_IEnd");
      RespVsPt[2] = (TH2F*)inf->Get("RespVsPt_OEnd");
      RespVsPt[3] = (TH2F*)inf->Get("RespVsPt_Fwd");
  
      TF1 *func[4][NPtBins];
      TH1D *h[4][NPtBins];
      TH1F *hClosure[4];
      hClosure[0] = new TH1F("ClosureVsPt_Bar","ClosureVsPt_Bar",NPtBins,vpt);
      hClosure[1] = new TH1F("ClosureVsPt_IEnd","ClosureVsPt_IEnd",NPtBins,vpt);
      hClosure[2] = new TH1F("ClosureVsPt_OEnd","ClosureVsPt_OEnd",NPtBins,vpt);
      hClosure[3] = new TH1F("ClosureVsPt_Fwd","ClosureVsPt_Fwd",NPtBins,vpt);
      double XminCalo[4] = {15,15,15,15};
      double XminPF[4] = {5,5,5,5};
      double Xmax[4] = {3000,3000,3000,190};

      //
      // Find the mean peak of the gaussian fit or the mean of the histogram and 
      // fill the final histograms
      //
      for(int i=0;i<NPtBins;i++)
        { 
          for(int j=0;j<4;j++)
            {
              sprintf(name,"CorResponse_%d_RefPt%sto%s",j,Pt[i],Pt[i+1]);
              h[j][i] = RespVsPt[j]->ProjectionY(name,i+1,i+1);
              sprintf(name,"FitResponse_%d_RefPt%sto%s",j,Pt[i],Pt[i+1]);
              func[j][i] = new TF1(name,"gaus",h[j][i]->GetMean()-1.5*h[j][i]->GetRMS(),
                                   h[j][i]->GetMean()+1.5*h[j][i]->GetRMS());
              func[j][i]->SetLineColor(4);
              func[j][i]->SetLineWidth(2);
              func[j][i]->SetParNames("N","#mu","#sigma");

              if (h[j][i]->GetEntries()>5)
                {
                  h[j][i]->Fit(func[j][i],"RQ");
                  if(mpv)
                    {
                      hClosure[j]->SetBinContent(i+1,func[j][i]->GetParameter(1));
                      hClosure[j]->SetBinError(i+1,func[j][i]->GetParError(1));
                    }
                  else
                    {
                      hClosure[j]->SetBinContent(i+1,h[j][i]->GetMean());
                      hClosure[j]->SetBinError(i+1,h[j][i]->GetMeanError());
                    }
                }
              else
                {
                  hClosure[j]->SetBinContent(i+1,h[j][i]->GetMean());
                  hClosure[j]->SetBinError(i+1,h[j][i]->GetMeanError());
                }  
              sprintf(name,"can_%d_RefPt%sto%s",j,Pt[i],Pt[i+1]);
              TString ss(name);
              ss+="_"+algs[a];
              if (tdr && j==0 && i>=NPtBins-6) {
                 hClosure[j]->SetBinContent(i+1,0.0);
                 hClosure[j]->SetBinError(i+1,0.0);
              }
            }
        }

      //
      // Create guides (lines) for the output histograms
      //
      TF1 *line = new TF1("line","0*x+1",0,5000);
      line->SetLineColor(1);
      line->SetLineWidth(1);
      line->SetLineStyle(2);
      TF1 *linePlus;
      if (tdr)
         linePlus = new TF1("linePlus","0*x+1.01",0,5000);
      else
         linePlus = new TF1("linePlus","0*x+1.02",0,5000);
      linePlus->SetLineColor(1);
      linePlus->SetLineWidth(1);
      linePlus->SetLineStyle(2);
      TF1 *lineMinus;
      if (tdr)
         lineMinus = new TF1("lineMinus","0*x+0.99",0,5000);
      else
         lineMinus = new TF1("lineMinus","0*x+0.98",0,5000);
      lineMinus->SetLineColor(1);
      lineMinus->SetLineWidth(1);
      lineMinus->SetLineStyle(2);

      TCanvas *can[4];
      //TString Text[4] = {"|#eta| < 1.3","1.3 < |#eta| < 3","3 < |#eta| < 5"};
      TString Text[4] = {"|#eta| < 1.3","1.3 < |#eta| < 2.5","2.5 < |#eta| < 3.0","3.0 < |#eta| < 5.0"};
      TPaveText *pave[4];

      //
      // Open/create the output directory and file
      //
      if(!outputDir.EndsWith("/")) outputDir+="/";
      if(!gSystem->OpenDirectory(outputDir)) gSystem->mkdir(outputDir);
      TString ofname = outputDir+"ClosureVsPt_"+algs[a]+".root";
      if(!flavor.IsNull()) ofname = outputDir+"ClosureVsPt_"+algs[a]+"_"+flavor+".root";
      TFile* outf = new TFile(ofname,"RECREATE");

      //
      // Format and save the output
      //
      for(int j=0;j<4;j++)
        {
          pave[j] = new TPaveText(0.3,0.75,0.8,0.9,"NDC");
          if (tdr) {
             pave[j]->AddText("QCD Monte Carlo");

             TString algNameLong;
             if(TString(algs[a]).Contains("ak"))        algNameLong += "Anti-kT";
             if(TString(algs[a]).Contains("1")&&
                !TString(algs[a]).Contains("10")&&
                !TString(algs[a]).Contains("l1"))       algNameLong += " R=0.1";
             else if(TString(algs[a]).Contains("2"))    algNameLong += " R=0.2";
             else if(TString(algs[a]).Contains("3"))    algNameLong += " R=0.3";
             else if(TString(algs[a]).Contains("4"))    algNameLong += " R=0.4";
             else if(TString(algs[a]).Contains("5"))    algNameLong += " R=0.5";
             else if(TString(algs[a]).Contains("6"))    algNameLong += " R=0.6";
             else if(TString(algs[a]).Contains("7"))    algNameLong += " R=0.7";
             else if(TString(algs[a]).Contains("8"))    algNameLong += " R=0.8";
             else if(TString(algs[a]).Contains("9"))    algNameLong += " R=0.9";
             else if(TString(algs[a]).Contains("10"))   algNameLong += " R=1.0";
             if(TString(algs[a]).Contains("pfchs"))     algNameLong += ", PFlow+CHS";
             else if(TString(algs[a]).Contains("pf"))   algNameLong += ", PFlow";
             else if(TString(algs[a]).Contains("calo")) algNameLong += ", Calo";
             else if(TString(algs[a]).Contains("jpt"))  algNameLong += ", JPT";

             pave[j]->AddText(algNameLong);
          }
          else {
             pave[j]->AddText(algs[a]);
          }
          pave[j]->AddText(Text[j]);
          sprintf(name,"ClosureVsPt_%d",j);
          TString ss(name);
          if(!flavor.IsNull()) ss+="_"+algs[a]+"_"+flavor;
          else ss+="_"+algs[a];
          can[j] = new TCanvas(ss,ss,800,800);
          if (j<3)
            gPad->SetLogx();
          if (ss.Contains("pf"))	
            hClosure[j]->GetXaxis()->SetRangeUser(XminPF[j],Xmax[j]);
          else
            hClosure[j]->GetXaxis()->SetRangeUser(XminCalo[j],Xmax[j]);
          hClosure[j]->GetXaxis()->SetTitle("GenJet p_{T} (GeV)"); 
          hClosure[j]->GetYaxis()->SetTitle("Corrected Response");
          if (tdr) {
             hClosure[j]->GetXaxis()->SetTitleSize(0.058);
             hClosure[j]->GetXaxis()->SetTitleOffset(0.95);
             hClosure[j]->SetMarkerStyle(20);
             hClosure[j]->SetMarkerSize(0.5);
          }
          else {
             hClosure[j]->GetXaxis()->SetLabelSize(0.04);
             hClosure[j]->GetYaxis()->SetLabelSize(0.04);             
             hClosure[j]->SetMarkerSize(2.0);
             hClosure[j]->GetXaxis()->SetNoExponent();
             hClosure[j]->GetXaxis()->SetMoreLogLabels();
          }
          hClosure[j]->SetMarkerColor(kBlue);
          hClosure[j]->SetLineColor(kBlue);
          hClosure[j]->SetMaximum(1.1);
          hClosure[j]->SetMinimum(0.9);
          hClosure[j]->Draw();
          line->Draw("same");
          linePlus->Draw("same");
          lineMinus->Draw("same");
          pave[j]->SetFillColor(0);
          pave[j]->SetBorderSize(0);
          pave[j]->SetTextFont(42);
          pave[j]->SetTextSize(0.05);
          pave[j]->Draw("EP");
          if (tdr) cmsPrelim();
          for(unsigned int iformat=0; iformat<outputFormat.size(); iformat++) {
            can[j]->SaveAs(outputDir+ss+outputFormat[iformat]);
          }
          hClosure[j]->Write();
          can[j]->Write();
        }//for(int j=0;j<3;j++)

      //
      // create overview canvas
      //
      TString ss("ClosureVsPt_Overview");
      if(!flavor.IsNull()) ss+="_"+algs[a]+"_"+flavor;
      else ss+="_"+algs[a];

      TCanvas *ove = new TCanvas(ss,ss,1200,300);
      ove->Divide(4,1);
      for (int c=0;c<4;c++) {
        ove->cd(c+1);
        if (c<3) 
          gPad->SetLogx();
        if (!tdr) {
           hClosure[c]->GetXaxis()->SetMoreLogLabels();
           hClosure[c]->GetXaxis()->SetNoExponent();
        }
        hClosure[c]->Draw("EP");
        line->Draw("same");
        linePlus->Draw("same");
        lineMinus->Draw("same");
        pave[c]->Draw();
        if (tdr) cmsPrelim();
      }
      for(unsigned int iformat=0; iformat<outputFormat.size(); iformat++) {
        ove->SaveAs(outputDir+ss+outputFormat[iformat]);
      }
      ove->Write();

      //
      // create shared overview canvas
      //
      ss = "ClosureVsPt_Overview2";
      if(!flavor.IsNull()) ss+="_"+algs[a]+"_"+flavor;
      else ss+="_"+algs[a];
      //TLegend* leg = new TLegend(0.70,0.65,0.90,0.90);
      TLegend* leg = new TLegend(0.40,0.20,0.80,0.40);
      leg->SetTextSize(0.04);//0.03);
      leg->SetBorderSize(0);
      leg->SetFillColor(0);
      TCanvas *ove2 = new TCanvas(ss,ss,800,800);//600);
      ove2->cd();
      gPad->SetLogx();
      for (int c=0;c<4;c++) {
         hClosure[c]->SetMaximum(1.05);
         hClosure[c]->SetMinimum(0.95);
         //hClosure[c]->SetMaximum(1.60);
         //hClosure[c]->SetMinimum(0.40);
         //if (!tdr) {
            hClosure[c]->GetXaxis()->SetMoreLogLabels();
            hClosure[c]->GetXaxis()->SetNoExponent();
            hClosure[c]->GetXaxis()->SetLabelSize(0.045);
            hClosure[c]->GetYaxis()->SetLabelSize(0.045);
            hClosure[c]->SetMarkerSize(0.8);
            //}
         if (c==0) {
            hClosure[c]->SetMarkerColor(kBlack);
            hClosure[c]->SetLineColor(kBlack);
         }
         else if (c==1) {
            hClosure[c]->SetMarkerColor(kBlue);
            hClosure[c]->SetLineColor(kBlue);
         }
         else if (c==2) {
            hClosure[c]->SetMarkerColor(kRed);
            hClosure[c]->SetLineColor(kRed);
         }
         else {
            hClosure[c]->SetMarkerColor(kMagenta);
            hClosure[c]->SetLineColor(kMagenta);
         }
         if (c==0)
            hClosure[c]->Draw("EP");
         else
            hClosure[c]->Draw("EPsame");
         leg->AddEntry(hClosure[c],Text[c],"lep");
         line->SetLineWidth(2);
         line->Draw("same");
         linePlus->SetLineWidth(2);
         linePlus->Draw("same");
         lineMinus->SetLineWidth(2);
         lineMinus->Draw("same");
         delete pave[c];
         pave[c] = new TPaveText(0.35,0.8,0.75,0.9,"NDC");
         if (tdr) {
            pave[c]->AddText("QCD Monte Carlo");
            TString algNameLong;
            if(TString(algs[a]).Contains("ak"))        algNameLong += "Anti-kT";
            if(TString(algs[a]).Contains("3"))         algNameLong += " R=0.3";
            else if(TString(algs[a]).Contains("4"))    algNameLong += " R=0.4";
            else if(TString(algs[a]).Contains("5"))    algNameLong += " R=0.5";
            else if(TString(algs[a]).Contains("6"))    algNameLong += " R=0.6";
            else if(TString(algs[a]).Contains("7"))    algNameLong += " R=0.7";
            else if(TString(algs[a]).Contains("8"))    algNameLong += " R=0.8";
            else if(TString(algs[a]).Contains("9"))    algNameLong += " R=0.9";
            else if(TString(algs[a]).Contains("10"))   algNameLong += " R=1.0";
            if(TString(algs[a]).Contains("pfchs"))     algNameLong += ", PFlow+CHS";
            else if(TString(algs[a]).Contains("pf"))   algNameLong += ", PFlow";
            else if(TString(algs[a]).Contains("calo")) algNameLong += ", Calo";
            else if(TString(algs[a]).Contains("jpt"))  algNameLong += ", JPT";
            pave[c]->AddText(algNameLong);
         }
         else {
            pave[c]->AddText(algs[a]);
         }
         pave[c]->SetFillColor(0);
         pave[c]->SetBorderSize(0);
         pave[c]->SetTextFont(42);
         pave[c]->SetTextSize(0.035);//0.05);
         pave[c]->Draw();
         leg->Draw("same");
      }
      if (tdr) cmsPrelim();
      for(unsigned int iformat=0; iformat<outputFormat.size(); iformat++) {
        ove2->SaveAs(outputDir+ss+outputFormat[iformat]);
      }
      ove2->Write();
      
      outf->Close();
    }//for(unsigned int a=0; a<algs.size(); a++)
}

////////////////////////////////////////////////////////////////////////////////
// implement local functions
////////////////////////////////////////////////////////////////////////////////

//______________________________________________________________________________
void cmsPrelim(double intLUMI)
{
   const float LUMINOSITY = intLUMI;
  TLatex latex;
  latex.SetNDC();
  latex.SetTextSize(0.045);

  latex.SetTextAlign(31); // align right
  latex.DrawLatex(0.93,0.96,"#sqrt{s} = 8 TeV");
  if (LUMINOSITY > 0.) {
    latex.SetTextAlign(31); // align right
    //latex.DrawLatex(0.82,0.7,Form("#int #font[12]{L} dt = %d pb^{-1}", (int) LUMINOSITY)); //Original
    latex.DrawLatex(0.65,0.85,Form("#int #font[12]{L} dt = %d pb^{-1}", (int) LUMINOSITY)); //29/07/2011
  }
  latex.SetTextAlign(11); // align left
  latex.DrawLatex(0.16,0.96,"CMS Simulation");// 2012");
}

//______________________________________________________________________________
TString getAlias(TString s)
{
   if (s=="ic5calo")
      return "IC5Calo";
   else if (s=="ic5pf")
      return "IC5PF";
   else if (s=="ak5calo")
      return "AK5Calo";  
   else if (s=="ak5calol1")
      return "AK5Calol1";
   else if (s=="ak5calol1off")
      return "AK5Calol1off";
   else if (s=="ak5calol1offl2l3")
      return "AK5Calol1off";
   else if (s=="ak7calo")
      return "AK7Calo";
   else if (s=="ak7calol1")
      return "AK7Calol1";
   else if (s=="ak7calol1off")
      return "AK7Calol1off";
   else if (s=="ak5caloHLT")
      return "AK5CaloHLT";
   else if (s=="ak5caloHLTl1")
      return "AK5CaloHLTl1";
   else if (s=="ak1pf")
      return "AK1PF";
   else if (s=="ak1pfl1")
      return "AK1PFl1";
   else if (s=="ak2pf")
      return "AK2PF";
   else if (s=="ak2pfl1")
      return "AK2PFl1";
   else if (s=="ak3pf")
      return "AK3PF";
   else if (s=="ak3pfl1")
      return "AK3PFl1";
   else if (s=="ak4pf")
      return "AK4PF";
   else if (s=="ak4pfl1")
      return "AK4PFl1";
   else if (s=="ak5pf")
      return "AK5PF";
   else if (s=="ak5pfl1")
      return "AK5PFl1";
   else if (s=="ak5pfl1l2l3")
      return "AK5PFl1";
   else if (s=="ak5pfl1off")
      return "AK5PFl1off";
   else if (s=="ak6pf")
      return "AK6PF";
   else if (s=="ak6pfl1")
      return "AK6PFl1";
   else if (s=="ak7pf")
      return "AK7PF";
   else if (s=="ak7pfl1")
      return "AK7PFl1";
   else if (s=="ak7pfl1off")
      return "AK7PFl1off";
   else if (s=="ak8pf")
      return "AK8PF";
   else if (s=="ak8pfl1")
      return "AK8PFl1";
   else if (s=="ak9pf")
      return "AK9PF";
   else if (s=="ak9pfl1")
      return "AK9PFl1";
   else if (s=="ak10pf")
      return "AK10PF";
   else if (s=="ak10pfl1")
      return "AK10PFl1";
   else if (s=="ak1pfchs")
      return "AK1PFchs";
   else if (s=="ak1pfchsl1")
      return "AK1PFchsl1";
   else if (s=="ak2pfchs")
      return "AK2PFchs";
   else if (s=="ak2pfchsl1")
      return "AK2PFchsl1";
   else if (s=="ak3pfchs")
      return "AK3PFchs";
   else if (s=="ak3pfchsl1")
      return "AK3PFchsl1";
   else if (s=="ak4pfchs")
      return "AK4PFchs";
   else if (s=="ak4pfchsl1")
      return "AK4PFchsl1";
   else if (s=="ak5pfchs")
      return "AK5PFchs";
   else if (s=="ak5pfchsl1")
      return "AK5PFchsl1";
   else if (s=="ak5pfchsl1l2l3")
      return "AK5PFchsl1";
   else if (s=="ak5pfchsl1off")
      return "AK5PFchsl1off";
   else if (s=="ak6pfchs")
      return "AK6PFchs";
   else if (s=="ak6pfchsl1")
      return "AK6PFchsl1";
   else if (s=="ak7pfchs")
      return "AK7PFchs";
   else if (s=="ak7pfchsl1")
      return "AK7PFchsl1";
   else if (s=="ak7pfchsl1off")
      return "AK7PFchsl1off";
   else if (s=="ak8pfchs")
      return "AK8PFchs";
   else if (s=="ak8pfchsl1")
      return "AK8PFchsl1";
   else if (s=="ak9pfchs")
      return "AK9PFchs";
   else if (s=="ak9pfchsl1")
      return "AK9PFchsl1";
   else if (s=="ak10pfchs")
      return "AK10PFchs";
   else if (s=="ak10pfchsl1")
      return "AK10PFchsl1";
   else if (s=="ak5pfHLT")
      return "AK5PFHLT";
  else if (s=="ak5pfHLTl1")
      return "AK5PFHLTl1";
   else if (s=="ak5pfchsHLT")
      return "AK5PFchsHLT";
   else if (s=="ak5pfchsHLTl1")
      return "AK5PFchsHLTl1";
   else if (s=="ak5jpt")
      return "AK5JPT";
   else if (s=="ak5jptl1")
      return "AK5JPTl1";
   else if (s=="ak5jptl1l2l3")
      return "AK5JPTl1";
   else if (s=="ak7jpt")
      return "AK7JPT";
   else if (s=="ak7jptl1")
      return "AK7JPTl1";
   else if (s=="sc5calo")
      return "SC5Calo";
   else if (s=="sc5pf")
      return "SC5PF";
   else if (s=="sc7calo")
      return "SC5Calo";
   else if (s=="sc7pf")
      return "SC5PF";
   else if (s=="kt4calo")
      return "KT4Calo";
   else if (s=="kt4pf")
      return "KT4PF";
   else if (s=="kt6calo")
      return "KT6Calo";
   else if (s=="kt6pf")
      return "KT6PF";
   else
      return "unknown";
}
