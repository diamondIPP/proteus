import sys
import os.path
import ROOT

# set ATLAS style
#import RootStyles
#rs = RootStyles.RootStyles()
#rs.setAtlasStyle2()
#rs.style.cd()
#ROOT.gROOT.ForceStyle()

ROOT.gStyle.SetLegendBorderSize(0)

ROOT.gStyle.SetPadTopMargin(0.16)
ROOT.gStyle.SetPadRightMargin(0.1)
ROOT.gStyle.SetPadLeftMargin(0.13)
ROOT.gStyle.SetPadBottomMargin(0.13)

rootin = ROOT.TFile.Open("/Users/javierb/Desktop/GlobalEff/402_eff_3911_nuovi_plot.root")
#rootin = ROOT.TFile.Open("/Users/javierb/Desktop/GlobalEff/3_effresults_402_003911_12V_Th084.root")
#rootin = ROOT.TFile.Open("/Users/javierb/Desktop/GlobalEff/effResults_004074_SPS_HVCMOS404_30V_th083_MERGED_Stime.root")

rootin.cd("Efficiency")
h1 = ROOT.gDirectory.Get("DUTPlane0InPixelEfficiencyC")

c1 = ROOT.TCanvas("c","c")
h1.SetLineColor(1)
h1.SetMarkerStyle(20)

h1.Draw("colztext")
ROOT.gPad.Update()

h1.SetTitle("")
h1.GetPaintedHistogram().GetYaxis().SetTitleOffset(1.5)
#h1.GetPaintedHistogram().GetYaxis().SetRangeUser(2200, 3190)
h1.GetPaintedHistogram().GetYaxis().SetRangeUser(0, 100)

h1.GetPaintedHistogram().GetYaxis().SetTitleSize(0.04)
#h1.GetXaxis().SetTitle("Alignment iteration")
h1.GetPaintedHistogram().GetXaxis().SetTitleOffset(1.3)
h1.GetPaintedHistogram().GetXaxis().SetTitleSize(0.04)
h1.GetPaintedHistogram().GetXaxis().SetLabelSize(0.04)
#h1.GetPaintedHistogram().GetXaxis().SetRangeUser(110, 1600)
h1.GetPaintedHistogram().GetXaxis().SetRangeUser(0, 250)

h1.GetPaintedHistogram().GetZaxis().SetTitle("Efficiency")
#h1.GetPaintedHistogram().GetZaxis().SetRangeUser(0, 1)

#h1.GetXaxis().SetNdivisions(10)

tex = ROOT.TLatex(0.13,0.94,"FE-I4 UniGE telescope")
tex.SetNDC()
tex.SetTextFont(72)
tex.SetTextSize(0.05)
tex.SetLineWidth(2)

tex2 = ROOT.TLatex(0.13,0.87,"SPS data 2014 (#pi, 180 GeV)")
tex2.SetNDC()
tex2.SetTextFont(42)
tex2.SetTextSize(0.05)
tex2.SetLineWidth(2)

tex3 = ROOT.TLatex(0.66,0.94,"402 v2 DUT")
#tex3 = ROOT.TLatex(0.66,0.94,"404 v2 DUT")
tex3.SetNDC()
tex3.SetTextFont(42)
tex3.SetTextSize(0.05)
tex3.SetLineWidth(2)

tex4 = ROOT.TLatex(0.66,0.87,"Bias: 12 V Th: 0.84 V")
#tex4 = ROOT.TLatex(0.66,0.87,"Bias: 30 V Th: 0.84 V")
tex4.SetNDC()
tex4.SetTextFont(42)
tex4.SetTextSize(0.05)
tex4.SetLineWidth(2)


#ROOT.gPad.SetLogy()
ROOT.gStyle.SetOptStat(0)
h1.Draw("colz")
#h1.Fit("f1","","",0,10)
tex.Draw("Same")
tex2.Draw("Same")
tex3.Draw("Same")
tex4.Draw("Same")
c1.Print("/Users/javierb/Desktop/GlobalEff/InEff_402.pdf","pdf")
