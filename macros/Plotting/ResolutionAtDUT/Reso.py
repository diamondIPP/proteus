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



rootin = ROOT.TFile.Open("/Users/javierb/Desktop/multipleScat/trackResults_4063_65_75_SPS_HVCMOS404.root")
rootin.cd("TrackInfo")
h1 = ROOT.gDirectory.Get("ResolutionAtDUTX")
h2 = ROOT.gDirectory.Get("ResolutionAtDUTY")

h1.Scale(1/h1.Integral())
h2.Scale(1/h2.Integral())

c1 = ROOT.TCanvas("c","c")
ROOT.gPad.SetLogy()
h1.SetFillColor(8)
h1.SetLineColor(1)
h1.SetFillStyle(3005)

h2.SetFillColor(4)
h2.SetLineColor(1)
h2.SetFillStyle(3004)


f1 = ROOT.TF1("f1","6.5*((TMath::Exp(-x*4)*(x*8)^(([0]/2)-1))/(2^([0]/2)*24))",0,10);
f1.SetParameter(0,8);
f1.SetParameter(1,6);

#h1.SetMarkerColor(4)
#h1.GetYaxis().SetTitle("Number of jets")
h1.SetTitle("")
h1.GetYaxis().SetTitleOffset(1.2)
h1.GetXaxis().SetRangeUser(0.,70)
h1.SetMaximum(100)
h1.GetYaxis().SetTitleSize(0.04)
h1.GetYaxis().SetLabelSize(0.04)
h1.GetXaxis().SetTitleOffset(1.2)
h1.GetXaxis().SetTitleSize(0.04)
h1.GetXaxis().SetLabelSize(0.04)
h1.GetXaxis().SetNdivisions(10)
h1.GetXaxis().SetTitle("Resolution at DUT [#mum]")

tex = ROOT.TLatex(0.13,0.82,"FE-I4 UniGE telescope")
tex.SetNDC()
tex.SetTextFont(72)
tex.SetTextSize(0.05)
tex.SetLineWidth(2)

tex2 = ROOT.TLatex(0.13,0.75,"SPS data 2015 (#pi, 180 GeV)")
tex2.SetNDC()
tex2.SetTextFont(42)
tex2.SetTextSize(0.05)
tex2.SetLineWidth(2)

tex3 = ROOT.TLatex(0.13,0.68,"Pointing resolution at DUT")
tex3.SetNDC()
tex3.SetTextFont(42)
tex3.SetTextSize(0.05)
tex3.SetLineWidth(2)

leg = ROOT.TLegend(0.6, 0.68, 0.85, 0.87)
leg.SetTextFont(42)
leg.SetTextSize(0.04)
leg.SetFillColor(0)

leg.AddEntry(h1, "X-axis Resolution", "f")
leg.AddEntry(0, "mean: 9.91 #mum", "")
leg.AddEntry(h2, "Y-axis Resolutiom", "f")
leg.AddEntry(0, "mean: 7.15 #mum", "")

#ROOT.gPad.SetLogy()
ROOT.gStyle.SetOptStat(0)
h1.Draw("hist")
h2.Draw("histsame")
#h1.Fit("f1","","",0,10)
tex.Draw("Same")
tex2.Draw("Same")
tex3.Draw("Same")
leg.Draw("Same")
c1.Print("/Users/javierb/Desktop/multipleScat/ResolutionAtDUT.pdf","pdf")
