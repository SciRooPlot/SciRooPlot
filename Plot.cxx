#include "Plot.h"

namespace PlottingProject {

  Plot::Plot()
  {
    mName = "dummyName";
    mPlotStyle = "default";
    mControlString = "";
    mClearCutoffBin = false;
    mFigureGroup = "";
    
    mXaxis.title = "";
    mYaxis.title = "";
    mZaxis.title = "";
    mYaxisRatio.title = "";
  }
  
  void Plot::AddGraph(string histName, string identifier, string lable, int marker, int color, string errorStyle, double cutoff, double cutoffLow)
  {
    if(identifier == "") identifier = mFigureGroup; // default identifier to figuregroup id
    string internalName = histName + "_@_" + identifier;
    mGraphs.push_back({internalName, lable, marker, color, errorStyle, cutoff, cutoffLow});
  }
  
  void Plot::AddHisto(string histName, string identifier, string lable, int marker, int color, string errorStyle, double cutoff, double cutoffLow)
  {
    if(identifier == "") identifier = mFigureGroup; // default identifier to figuregroup id
    string internalName = histName + "_@_" + identifier;
    mHistos.push_back({internalName, lable, marker, color, errorStyle, cutoff, cutoffLow});
  }
  
  void Plot::AddRatio(string numerHist, string numerHistIdentifier, string denomHist, string denomHistIdentifier, string lable, int marker, int color, string errorStyle, double cutoff, double cutoffLow)
  {
    if(numerHistIdentifier == "") numerHistIdentifier = mFigureGroup;
    if(denomHistIdentifier == "") denomHistIdentifier = mFigureGroup;
    string ratioName = numerHist  + "_@_" + numerHistIdentifier + " / " + denomHist + "_@_" + denomHistIdentifier;
    mRatios.push_back({ratioName, lable,marker, color, errorStyle, cutoff, cutoffLow});
  }
  
  
  void Plot::AddTextBox(double xPos, double yPos, string text, bool userCoord, int borderStyle, int borderSize, int borderColor)
  {
    mTexts.push_back({xPos, yPos, text, borderStyle, borderSize, borderColor, userCoord});
  }
  
  void Plot::AddLegendBox(double xPos, double yPos, string title, int nColumns, int borderStyle, int borderSize, int borderColor)
  {
    mLegends.push_back({xPos, yPos, title, borderStyle, borderSize, borderColor, false, nColumns});
  }
  
  Plot::Axis& Plot::GetAxis(string axis)
  {
    if(axis == "X") return mXaxis;
    else if(axis == "Y") return mYaxis;
    else if(axis == "Z") return mZaxis;
    else if(axis == "ratio") return mYaxisRatio;
    else {
      cout << "ERROR: " << axis << "-Axis does not exist!" << endl;
      return mXaxis; // default return xaxis
    }
  }
  
  vector<double>& Plot::GetAxisRange(string axis)
  {
    return GetAxis(axis).range;
  }
  
  string& Plot::GetAxisTitle(string axis)
  {
    return GetAxis(axis).title;
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
    GetAxis(axis).title = axisTitle;
  }
  
  
  void Plot::Print()
  {
    cout << " ----------" << string(mName.length(), '-')  << "----------" << endl;
    cout << " <-------- " << mName << " -------->" << endl;
    cout << " ----------" << string(mName.length(), '-')  << "----------" << endl;
    if(!mControlString.empty())cout << "  Control string: '" << mControlString << "'" << endl;
    cout << "  Histograms:" << endl;
    for(auto &histo : mHistos) cout << "   - " << histo.name << endl;
    
    if(!mRatios.empty()){
      cout << "  Ratios:" << endl;
      for(auto &ratio : mRatios) cout << "   - " << ratio.name << endl;
    }
    cout << endl;
    
  }
  
}
