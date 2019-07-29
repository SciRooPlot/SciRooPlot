#include "Plot.h"

namespace PlottingProject {
  
  using std::cout;
  using std::endl;
  using std::flush;
  using std::string;
  using std::vector;
  
  Plot::Plot()
  {
    fName = "dummyName";
    fPlotStyle = "default";
    fControlString = "";
    fClearCutoffBin = false;
    fFigureGroup = "";
    
    fXaxis.fTitle = "";
    fYaxis.fTitle = "";
    fZaxis.fTitle = "";
    fYaxisRatio.fTitle = "";
  }
  
  void Plot::AddGraph(string histName, string identifier, string lable, int marker, int color, string errorStyle, double cutoff, double cutoffLow)
  {
    if(identifier == "") identifier = fFigureGroup; // default identifier to figuregroup id
    string internalName = histName + "_@_" + identifier;
    fGraphs.push_back({internalName, lable, marker, color, errorStyle, cutoff, cutoffLow});
  }
  
  void Plot::AddHisto(string histName, string identifier, string lable, int marker, int color, string errorStyle, double cutoff, double cutoffLow)
  {
    if(identifier == "") identifier = fFigureGroup; // default identifier to figuregroup id
    string internalName = histName + "_@_" + identifier;
    fHistos.push_back({internalName, lable, marker, color, errorStyle, cutoff, cutoffLow});
  }
  
  void Plot::AddRatio(string numerHist, string numerHistIdentifier, string denomHist, string denomHistIdentifier, string lable, int marker, int color, string errorStyle, double cutoff, double cutoffLow)
  {
    if(numerHistIdentifier == "") numerHistIdentifier = fFigureGroup;
    if(denomHistIdentifier == "") denomHistIdentifier = fFigureGroup;
    string ratioName = numerHist  + "_@_" + numerHistIdentifier + " / " + denomHist + "_@_" + denomHistIdentifier;
    fRatios.push_back({ratioName, lable,marker, color, errorStyle, cutoff, cutoffLow});
  }
  
  
  void Plot::AddTextBox(double xPos, double yPos, string text, bool userCoord, int borderStyle, int borderSize, int borderColor)
  {
    fTexts.push_back({xPos, yPos, text, borderStyle, borderSize, borderColor, userCoord});
  }
  
  void Plot::AddLegendBox(double xPos, double yPos, string title, int nColumns, int borderStyle, int borderSize, int borderColor)
  {
    fLegends.push_back({xPos, yPos, title, borderStyle, borderSize, borderColor, false, nColumns});
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
  
  vector<double>& Plot::GetAxisRange(string axis)
  {
    return GetAxis(axis).fRange;
  }
  
  string& Plot::GetAxisTitle(string axis)
  {
    return GetAxis(axis).fTitle;
  }
  
  void Plot::SetAxisRange(string axis, double low, double high)
  {
    vector<double>& range = GetAxisRange(axis);
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
  
}
