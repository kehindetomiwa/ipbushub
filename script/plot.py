#! /usr/bin/env python
from __future__ import division
from ROOT import TCanvas, TGraph,TProfile
from ROOT import TH2F, Double
from array import array
import ROOT

# read file to array

c1 = TCanvas ('c1','Read size vs time',700,500)
c2 = TCanvas ('c2','Read size vs time',700,500)
c3 = TCanvas ('c3','Read size vs mean time taken',700,500)
c4 = TCanvas ('c4','Read size vs mean time taken',700,500)
c5 = TCanvas ('c5','Read size vs mean time taken',700,500)
c6 = TCanvas ('c6','Read size vs mean time taken',700,500)
c7 = TCanvas ('c7','Read size vs mean time taken',700,500)


htxsy = TH2F('htxsy','2-D Histo of size vs time;Packet size(32-bit words); Time(s)',310,0,310,100,0,.001)
htxsy2 = TH2F('htxsy2','2-D Histo of size vs time;Packet size(32-bit words); Time(s)',310,0,310,100,0,.001)

file = ROOT.TFile.Open('withhub2.root', 'read') # a file
for event in file.ipbus:   # loading the tree
    htxsy.Fill(event.size,event.time)
    #print nyear

file2 = ROOT.TFile.Open('withouthub2.root', 'read') # a file
for event2 in file2.ipbus:   # loading the tree
    htxsy2.Fill(event2.size,event2.time)
    #print nyear2

c1.cd()
htxsy.SetStats(0)
htxsy.SetLineColor(2)
htxsy.Draw()
c1.Update()
c1.SaveAs("plot1.pdf")

c2.cd()
htxsy2.SetStats(0)
htxsy2.SetLineColor(4)
htxsy2.Draw()
c2.Update()
c2.SaveAs("plot2.pdf")

c3.Divide(2,1)
c3.cd(1)
htxsy.Draw('COLZ')
c3.cd(2)
htxsy.Draw('CONT1Z')
c3.Update()
c3.SaveAs("plot3.pdf")

c4.Divide(2,1)
c4.cd(1)
htxsy2.Draw('COLZ')
c4.cd(2)
htxsy2.Draw('CONT1Z')
c4.Update()
c4.SaveAs("plot4.pdf")

c7.cd()
ht=ROOT.THStack("sTimeVsSize",";Packet size(32-bit words); Time(s)")

px = htxsy.ProfileX()
px.SetMarkerStyle(ROOT.kFullCircle)
px.SetMarkerSize(1.04)
px.SetMarkerColor(2)
px.SetStats(0)
ht.Add(px)

px2 = htxsy2.ProfileX()
px2.SetMarkerStyle(ROOT.kOpenCircle)
px2.SetMarkerSize(1.04)
px2.SetMarkerColor(4)
px2.SetStats(0)
ht.Add(px2)

ht.Draw("NOSTATS")
tl1 = ROOT.TLegend(0.4,0.3,0.6,0.45)
tl1.AddEntry(px,"withhub","pl")
tl1.AddEntry(px2,"withouthub","pl")
tl1.Draw()
c7.SaveAs("plot7.pdf")

raw_input("wait")
