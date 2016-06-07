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
rootin.cd("Residuals1D")
h1 = ROOT.gDirectory.Get("FEI4TelPlane3X")

rootin2 = ROOT.TFile.Open("/Users/javierb/Desktop/multipleScat/trackResults_C19_newAlignment.root")
rootin2.cd("Residuals1D")
h2 = ROOT.gDirectory.Get("FEI4TelPlane3X")


h1.Scale(1/h1.Integral())

h2.Scale(1/h2.Integral())

c1 = ROOT.TCanvas("c","c")
h1.SetFillColor(4)
h1.SetLineColor(1)
h1.SetFillStyle(3004)

h2.SetFillColor(8)
h2.SetLineColor(1)
h2.SetFillStyle(3005)

#h1.SetMarkerColor(4)
#h1.GetYaxis().SetTitle("Number of jets")
h1.SetTitle("")
h1.GetYaxis().SetTitleOffset(1.2)
h1.GetYaxis().SetTitleSize(0.04)
h1.GetYaxis().SetLabelSize(0.04)
h1.GetXaxis().SetTitleOffset(1.2)
h1.GetXaxis().SetTitleSize(0.04)
h1.GetXaxis().SetLabelSize(0.04)
h1.GetXaxis().SetNdivisions(4)
h1.GetYaxis().SetRangeUser(0,0.22)

tex = ROOT.TLatex(0.13,0.8,"FE-I4 UniGE telescope")
tex.SetNDC()
tex.SetTextFont(72)
tex.SetTextSize(0.05)
tex.SetLineWidth(2)

tex2 = ROOT.TLatex(0.13,0.73,"SPS data 2015 (#pi, 180 GeV)")
tex2.SetNDC()
tex2.SetTextFont(42)
tex2.SetTextSize(0.05)
tex2.SetLineWidth(2)

tex3 = ROOT.TLatex(0.13,0.73,"Telescope plane #4")
tex3.SetNDC()
tex3.SetTextFont(42)
tex3.SetTextSize(0.05)
tex3.SetLineWidth(2)

leg = ROOT.TLegend(0.6, 0.65, 0.85, 0.85)
leg.SetTextFont(42)
leg.SetTextSize(0.04)
leg.SetFillColor(0)

leg.AddEntry(h2,"PS residuals", "f")
leg.AddEntry(0, "RMS: 95.45 #mum", "")
leg.AddEntry(h1,"SPS residuals", "f")
leg.AddEntry(0, "RMS: 83.63 #mum", "")

#ROOT.gPad.SetLogy()
ROOT.gStyle.SetOptStat(0)
h1.Draw("")
h2.Draw("same")
tex.Draw("Same")
#tex2.Draw("Same")
tex3.Draw("Same")
leg.Draw("Same")
c1.Print("/Users/javierb/Desktop/multipleScat/1DResidualsX.pdf","pdf")