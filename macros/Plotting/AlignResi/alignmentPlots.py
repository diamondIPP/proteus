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

ROOT.gStyle.SetPadRightMargin(0.05)
ROOT.gStyle.SetPadLeftMargin(0.13)
ROOT.gStyle.SetPadBottomMargin(0.13)

rootin = ROOT.TFile.Open("/Users/javierb/Desktop/DUTalign/effResults_402_3918_3919-merged_Stime_ultimo.root")
#rootin = ROOT.TFile.Open("/Users/javierb/Desktop/DUTalign/effResults_004076_SPS_HVCMOS404_30V_th085_MERGED_Stime.root")
rootin.cd("DUTResiduals1D")
h1 = ROOT.gDirectory.Get("DUTPlane0Y")

c1 = ROOT.TCanvas("c","c")
h1.SetLineColor(1)
h1.SetMarkerStyle(20)
h1.Scale(1/h1.Integral())


h1.SetTitle("")
h1.GetYaxis().SetTitle("Clusters")
h1.GetYaxis().SetTitleOffset(1.5)
#h1.GetYaxis().SetRangeUser(0., 0.19)
h1.GetYaxis().SetRangeUser(0., 0.08)
h1.GetYaxis().SetTitleSize(0.04)
h1.GetYaxis().SetLabelSize(0.04)
#h1.GetXaxis().SetTitle("Alignment iteration")
h1.GetXaxis().SetTitleOffset(1.3)
h1.GetXaxis().SetTitleSize(0.04)
h1.GetXaxis().SetLabelSize(0.04)
h1.GetXaxis().SetNdivisions(10)

tex = ROOT.TLatex(0.16,0.82,"FE-I4 UniGE telescope")
tex.SetNDC()
tex.SetTextFont(72)
tex.SetTextSize(0.05)
tex.SetLineWidth(2)

tex2 = ROOT.TLatex(0.16,0.75,"SPS data 2015 (#pi, 180 GeV)")
tex2.SetNDC()
tex2.SetTextFont(42)
tex2.SetTextSize(0.05)
tex2.SetLineWidth(2)

tex3 = ROOT.TLatex(0.16,0.68,"402 v2 DUT")
tex3.SetNDC()
tex3.SetTextFont(42)
tex3.SetTextSize(0.05)
tex3.SetLineWidth(2)

leg = ROOT.TLegend(0.66, 0.7, 0.94, 0.89)
leg.SetTextFont(42)
leg.SetTextSize(0.04)
leg.SetFillColor(0)

leg.AddEntry(h1,"DUT resusiduals", "f")
leg.AddEntry(0, "mean: -3.85 #mum", "")
leg.AddEntry(0, "RMS: 83.58 #mum", "")

#ROOT.gPad.SetLogy()
ROOT.gStyle.SetOptStat(0)
h1.Draw("hist")
#h1.Fit("f1","","",0,10)
tex.Draw("Same")
tex2.Draw("Same")
tex3.Draw("Same")
leg.Draw("Same")
print h1.GetRMS(2)
c1.Print("/Users/javierb/Desktop/DUTalign/Alignment_402_Y.pdf","pdf")
