#include "Plot.h"

Plot::Plot()
{
  fName = "dummyName";
  fPlotStyle = "default";
  fControlString = "";
  fClearCutoffBin = kFALSE;
  fFigureGroup = "";

  fXaxis.fTitle = "";
  fYaxis.fTitle = "";
  fZaxis.fTitle = "";
  fYaxisRatio.fTitle = "";
}

void Plot::AddHisto(string histName, string identifier, string lable, Int_t marker, Int_t color, string errorStyle)
{
  if(identifier == "") identifier = fFigureGroup; // default identifier to figuregroup id
  string internalName = histName + "_@_" + identifier;
  fHistos.push_back({internalName, lable, marker, color, errorStyle});
}

void Plot::AddRatio(string numerHist, string numerHistIdentifier, string denomHist, string denomHistIdentifier, string lable)
{
  if(numerHistIdentifier == "") numerHistIdentifier = fFigureGroup;
  if(denomHistIdentifier == "") denomHistIdentifier = fFigureGroup;
  string ratioName = numerHist  + "_@_" + numerHistIdentifier + " / " + denomHist + "_@_" + denomHistIdentifier;
  fRatios.push_back({ratioName, lable, 0, 0});
}


void Plot::AddTextBox(Double_t xPos, Double_t yPos, string text, Int_t borderStyle, Int_t borderSize, Int_t borderColor)
{
  fTexts.push_back({xPos, yPos, text, borderStyle, borderSize, borderColor});
}

void Plot::AddLegendBox(Double_t xPos, Double_t yPos, string title, Int_t borderStyle, Int_t borderSize, Int_t borderColor)
{
  fLegends.push_back({xPos, yPos, title, borderStyle, borderSize, borderColor});
}

Plot::Axis& Plot::GetAxis(string axis)
{
  if(axis == "X") return fXaxis;
  else if(axis == "Y") return fYaxis;
  else if(axis == "Z") return fZaxis;
  else if(axis == "ratio") return fYaxisRatio;
  else {
    cout << "ERROR: " << axis << "-Axis does not exist!" << endl;
    return fXaxis; // default return xaxis
  }
}

vector<Double_t>& Plot::GetAxisRange(string axis)
{
  return GetAxis(axis).fRange;
}

string& Plot::GetAxisTitle(string axis)
{
  return GetAxis(axis).fTitle;
}

void Plot::SetAxisRange(string axis, Double_t low, Double_t high)
{
  vector<Double_t>& range = GetAxisRange(axis);
  range.clear();
  range.push_back(low);
  range.push_back(high);
}


void Plot::SetAxisTitle(string axis, string axisTitle)
{
  GetAxis(axis).fTitle = axisTitle;
}


void Plot::Print()
{
  cout << " ----------" << string(fName.length(), '-')  << "----------" << endl;
  cout << " <-------- " << fName << " -------->" << endl;
  cout << " ----------" << string(fName.length(), '-')  << "----------" << endl;
  if(!fControlString.empty())cout << "  Control string: '" << fControlString << "'" << endl;
  cout << "  Histograms:" << endl;
  for(auto &histo : fHistos) cout << "   - " << histo.fName << endl;

  if(!fRatios.empty()){
    cout << "  Ratios:" << endl;
    for(auto &ratio : fRatios) cout << "   - " << ratio.fName << endl;
  }
  cout << endl;

}
