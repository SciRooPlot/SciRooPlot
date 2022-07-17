// PlottingFramework
//
// Copyright (C) 2019-2022  Mario Krüger
// Contact: mario.kruger@cern.ch
// For a full list of contributors please see doc/CONTRIBUTORS.md
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

void generateMWE()
{
  string folder = "InputFiles/";
  gSystem->Exec((string("mkdir -p ") + folder).data());
  TFile file1((folder + "file1.root").data(), "RECREATE");
  TFile file2((folder + "file2.root").data(), "RECREATE");
  TFile file3((folder + "file3.root").data(), "RECREATE");

  TF1 func1("func1", "0.7 + sin(x)/x",0,60);

  double x[] = {10, 15, 20, 25, 30};
  double y[] = {0.65, 0.6, 0.65, 0.7, 0.77};
  double ex[] = {0.1, 0.2, 0.3, 0.4, 0.5};
  double ey[] = {0.01, 0.008, 0.005, 0.005, 0.01};
  TGraphErrors graph1(5, x, y, ex, ey);
  graph1.SetName("graph1");
  file1.cd();
  graph1.Write();

  file1.mkdir("dir1");
  file1.cd("dir1");
  func1.Write();
  
  TH1F hist1("hist1","",100,0,6);
  hist1.FillRandom("gaus",10000);
  hist1.Sumw2();
  hist1.Write();

  file1.mkdir("dir2/subdir1");
  file1.cd("dir2/subdir1");

  TF1 mygaus("mygaus","TMath::Gaus(x,3,.5)", 0, 6);

  TH1F hist2("hist2","",100,0,6);
  hist2.FillRandom("mygaus", 10000);
  hist2.Sumw2();
  hist2.Write();

  TF1 mygaus2("mygaus2","TMath::Gaus(x,4,.8)", 0, 6);
  TH1F hist3("hist3","",100,0,6);
  hist3.FillRandom("mygaus2", 10000);
  hist3.Sumw2();
  hist3.Write();

  file1.mkdir("dir2/subdir2");
  file1.cd("dir2/subdir2");

  TF1 mygaus3("mygaus3","TMath::Gaus(x,2,.8)", 0, 6);
  TH1F hist3b("hist3","",100,0,6);
  hist3b.FillRandom("mygaus3", 10000);
  hist3b.Sumw2();
  hist3b.Write();

  file2.cd();
  
  TF1 mygaus4("mygaus4","TMath::Gaus(x,5,.8)", 0, 6);
  TH1F hist4("hist4","",100,0,6);
  hist4.FillRandom("mygaus4", 100000);
  hist4.Sumw2();
  hist4.Write();
  
  TList list1;
  list1.SetOwner(false);

  TF1 mygaus5("mygaus5","TMath::Gaus(x,3.3,.1)", 0, 6);
  TH1F hist5("hist5","",100,0,6);
  hist5.FillRandom("mygaus5", 100000);
  hist5.Sumw2();
  list1.Add(&hist5);
  
  TF1 mygaus6("mygaus6","TMath::Gaus(x,0.3,.2)", 0, 6);
  TH1F hist6("hist6","",100,0,6);
  hist6.FillRandom("mygaus6", 100000);
  hist6.Sumw2();
  list1.Add(&hist6);

  list1.Write("list1",  TObject::kSingleKey);

  list1.Clear();
  
  file3.cd();
  
  TF1 mysin("mysin", "x + x*sin(x)", 0, 6);
  TH1F hist7("hist7","",100,0,6);
  hist7.FillRandom("mysin", 100000);
  hist7.Sumw2();
  hist7.Write();


  TF1 mysin2("mysin2", "x + x*sin(2*x)", 0, 6);
  TH1F hist8("hist8","",100,0,6);
  hist8.FillRandom("mysin2", 100000);
  hist8.Sumw2();
  hist8.Write();
  
}
