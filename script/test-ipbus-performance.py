#! /usr/bin/env python

# for one client 
import Herakles
import sys, timeit , time , getopt
import os
import ROOT
import array

def main(argv):
  if (len(sys.argv)<2):
    print sys.argv[1], " -h for usage"
    sys.exit()
  try:
    opts, args = getopt.getopt(argv,"hw:r:")
  except getopt.GetoptError:
    print "Test-Performance.py -h for help"
    sys.exit()
  for opt,arg in opts:
    if opt == '-h':
      print "ipAddress--> localhost : ports--> 50002 or 50003 " 
      print "read from SROD address: Data 1 PMT1 HG--> 256 to Data 32 PMT1 HG--> 287"
  if opt == '-r':
    if (len(sys.argv)<5):
      print "To  read: test-ipbus-performance.py -r <ipAddress> <number of times to write> <port> <name of output>\n"
      sys.exit()
    else:
      print "reading...."
      hub = Herakles.Uhal(sys.argv[2],int(sys.argv[4])) #pass port as arg?
      print "connected to hub"
      hub.SetVerbose(True)
      
      # create root tree
      print "Create ntuple"
      tfile = ROOT.TFile(sys.argv[5]+".root","RECREATE")
      ntuple = ROOT.TTree("ipbus","IPBus performance tree")
      psize = array.array('L',(0,))
      ptime = array.array('f',(0,))
      ntuple.Branch("size",psize,"PackageSize/I")
      ntuple.Branch("time",ptime,"ReadoutTime/F")
      
      for x3 in xrange(1,301):
        for m3 in xrange(100):
          t1 = time.time()
          hub.Read(0x00000100,x3)
          psize[0]= x3
          ptime[0]= time.time() - t1
          ntuple.Fill()
      print "Close ntuple"
      ntuple.Write()
      tfile.Close()
if __name__ == "__main__":
  main(sys.argv[1:])
