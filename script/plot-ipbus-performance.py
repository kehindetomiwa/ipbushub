#! /usr/bin/env python
import ROOT


hTimeVsSize1 = ROOT.TH2F('hTimeVsSize1',';Packet size (32-bit words); Time (ms)',256,0,256,1000,0,10)
hTimeVsSize2 = ROOT.TH2F('hTimeVsSize2',';Packet size (32-bit words); Time (ms)',256,0,256,1000,0,10)

file1 = ROOT.TFile.Open('withhub2.root')
for event1 in file1.ipbus:
	hTimeVsSize1.Fill(event1.size,event1.time*10**3)
    
file2 = ROOT.TFile.Open('withouthub2.root')
for event2 in file2.ipbus:
    hTimeVsSize2.Fill(event2.size,event2.time*10**3)
    

cTimeVsSize1 = ROOT.TCanvas("cTimeVsSize1","cTimeVsSize1",800,600)
hTimeVsSize1.Draw("COLZ")
cTimeVsSize1.SaveAs("cTimeVsSize1.pdf")

cTimeVsSize2 = ROOT.TCanvas("cTimeVsSize2","cTimeVsSize2",800,600)
hTimeVsSize2.Draw("COLZ")
cTimeVsSize2.SaveAs("cTimeVsSize2.pdf")

cTimeVsSize3 = ROOT.TCanvas("cTimeVsSize3","cTimeVsSize3",800,600)
hTimeVsSize3 = ROOT.THStack("hTimeVsSize3",";Packet size (32-bit words); Time (ms)")

pTimeVsSize1 = hTimeVsSize1.ProfileX()
pTimeVsSize1.SetMarkerStyle(ROOT.kFullCircle)
pTimeVsSize1.SetMarkerSize(1.04)
pTimeVsSize1.SetMarkerColor(ROOT.kRed)
hTimeVsSize3.Add(pTimeVsSize1)

pTimeVsSize2 = hTimeVsSize2.ProfileX()
pTimeVsSize2.SetMarkerStyle(ROOT.kFullCircle)
pTimeVsSize2.SetMarkerSize(1.04)
pTimeVsSize2.SetMarkerColor(ROOT.kBlue)
hTimeVsSize3.Add(pTimeVsSize2)

fTimeVsSize3 = ROOT.TF1("fTimeVsSize3","pol1")
pTimeVsSize1.Fit(fTimeVsSize3,"","",20,200)
pTimeVsSize2.Fit(fTimeVsSize3,"","",20,200)

hTimeVsSize3.Draw("NOSTACK")

tl1 = ROOT.TLegend(0.12,0.7,0.35,0.85)
tl1.AddEntry(pTimeVsSize1,"with hub","pl")
tl1.AddEntry(pTimeVsSize2,"without hub","pl")
tl1.Draw()
cTimeVsSize3.Update()
cTimeVsSize3.SaveAs("cTimeVsSize3.pdf")

def extract(th2):
	th1 = ROOT.TH1F(th2.GetName()+"_ex",th2.GetTitle(),th2.GetNbinsX(),th2.GetXaxis().GetXmin(),th2.GetXaxis().GetXmax())
	th2p=th2.ProfileX()
	for i in xrange(th2.GetNbinsX()):
		th1.SetBinContent(i,th2p.GetBinContent(i))
		th1.SetBinError(i,th2p.GetBinError(i))
		
	return th1

cTimeVsSize4 = ROOT.TCanvas("cTimeVsSize4","cTimeVsSize4",800,600)
pTimeVsSize3 = extract(hTimeVsSize1)
pTimeVsSize3.Sumw2()
pTimeVsSize4 = extract(hTimeVsSize2)
pTimeVsSize4.Sumw2()
pTimeVsSize3.Divide(pTimeVsSize4)
pTimeVsSize3.Draw()
pTimeVsSize3.SetStats(0)
pTimeVsSize3.SetMarkerStyle(ROOT.kFullCircle)
pTimeVsSize3.SetMarkerSize(1.04)
pTimeVsSize3.SetMarkerColor(ROOT.kBlack)
fTimeVsSize4 = ROOT.TF1("fTimeVsSize4","pol0")
pTimeVsSize3.Fit(fTimeVsSize4,"","",20,200)
cTimeVsSize4.SaveAs("cTimeVsSize4.pdf")

raw_input("wait")
