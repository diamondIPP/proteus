import sys
import os.path
import ROOT

# set ATLAS style
#import RootStyles
#rs = RootStyles.RootStyles()
#rs.setAtlasStyle2()
#rs.style.cd()
#ROOT.gROOT.ForceStyle()

path1= "/Users/javierb/Desktop/CMOS/TBresults/SPS_Nov_HVCMOS402/"
opened=0
directory=0
file=1

c1 = ROOT.TCanvas("c","c")

color = [42,46,8,38,1]
legend = ["Threshold: 0.84 V","Threshold: 0.85 V","Threshold: 0.90 V","Threshold: 1.00 V"]

leg = ROOT.TLegend(0.48, 0.52, 0.89, 0.66)
leg.SetTextFont(42)
leg.SetTextSize(0.04)
leg.SetFillColor(0)
leg.SetBorderSize(0)

tex = ROOT.TLatex(0.49,0.82,"FE-I4 UniGE telescope")
tex.SetNDC()
tex.SetTextFont(72)
tex.SetTextSize(0.05)
tex.SetLineWidth(2)

tex2 = ROOT.TLatex(0.49,0.75,"SPS data 2014 (#pi, 180 GeV)")
tex2.SetNDC()
tex2.SetTextFont(42)
tex2.SetTextSize(0.05)
tex2.SetLineWidth(2)

tex3 = ROOT.TLatex(0.49,0.68,"402 v4 DUT")
tex3.SetNDC()
tex3.SetTextFont(42)
tex3.SetTextSize(0.05)
tex3.SetLineWidth(2)


for filename in os.listdir (path1):
    print filename
    if "effresults" not in filename:
        #print "REJECTED"
        continue
    else:
        if ".root" not in filename:
            print "NO PDFs Thanks"
            continue
        if "_12V_" not in filename:
            print "NO bias voltage"
            continue

        if "th083" in filename:
            print "NO Bias voltage"
            continue
        if "086" in filename:
            #print "NO Bias voltage"
            continue
        if "088" in filename:
            #print "NO Bias voltage"
            continue
        if "093" in filename:
            #print "NO Bias voltage"
            continue
        
        data1 = filename[:-5]
        print data1
        
        rootin1 = ROOT.TFile.Open(path1 + filename, "OLD")
        dirList1 = rootin1.GetListOfKeys()



        def GetKeyNames(self, dir="" ):
            self.cd(dir)
            return [key.GetName() for key in ROOT.gDirectory.GetListOfKeys()]

        for k1 in dirList1:
            directory+=1
            if k1.IsFolder():
                #print "IN DIRECTORY: " + k1.GetName()
                rootin1.cd(k1.GetName())
                dirList1_dir =  ROOT.gDirectory.GetListOfKeys()
                for k1_dir in dirList1_dir:
                    name = k1_dir.GetName()
                    print "    Found: " + name + "  with class: " + k1_dir.GetClassName()
                    h1 = ROOT.gDirectory.Get(name)
                        #temp= h1.Clone(filename+"_"+name)
                    if "MatchedTiming" not in name:
                        continue
                    else:
                        h1.SetTitle("")
                        ROOT.gStyle.SetOptStat(0)
                        h1.GetYaxis().SetTitleOffset(1.5)
                        h1.GetXaxis().SetTitleOffset(1.2)
                        h1.GetXaxis().SetTitle("Matched Tracks Timing [BC]")
                        if h1.Integral()!=0:
                            h1.Scale(1/h1.Integral())
                        ROOT.gPad.SetLogy()
                        ROOT.gStyle.SetOptStat(0)
                        
                        #h1.SetMarkerStyle(file)
                        #h1.SetMarkerColor(file)
                        h1.SetLineColor(1)
                        h1.SetFillColor(color[file-1])
                        data2 = data1[-20:]
                        leg.AddEntry(h1,legend[file-1], "f")

                        print "READY TO PRINT"


                        file=file+1
                        if opened==0:
                            h1.Draw("PH")
                            print "DRAWING FIRST"
                            opened=1

                        else:
                            h1.Draw("PHsame")
                            print "DRAWING IN SAME"

leg.Draw()
tex.Draw()
tex2.Draw()
tex3.Draw()

c1.Print(path1+"TimingVSthr_log.pdf","pdf")
ROOT.gPad.SetLogy(0)
c1.Print(path1+"TimingVSthr.pdf","pdf")

#print "ADDING PAGE"





