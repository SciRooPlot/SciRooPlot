#include "Plot.h"

using namespace PlottingFramework;
namespace PlottingFramework {

  Plot::Plot()
  {
    mName = "dummyName";
    mPlotStyle = "default";
    mControlString.clear();
    mClearCutoffBin = false;
    mFigureGroup = "";
    mCurrPad = 1;
  }

  void Plot::AddGraph(string graphName, string inputIdentifier, string lable, int marker, int color, string drawingOptions)
  {
    if(inputIdentifier == "") inputIdentifier = mFigureGroup; // default identifier to figuregroup id
    mData[mCurrPad].push_back(std::make_shared<Graph>(graphName, inputIdentifier, lable, color, marker, 0, drawingOptions));
  }
  
  void Plot::AddHisto(string histName, string inputIdentifier, string lable, int marker, int color, string drawingOptions, double cutoff, double cutoffLow)
  {
    if(inputIdentifier == "") inputIdentifier = mFigureGroup; // default identifier to figuregroup id
    mData[mCurrPad].push_back(std::make_shared<Histogram>(histName, inputIdentifier, lable, color, marker, 0, drawingOptions, 1, std::make_pair(cutoffLow, cutoff), std::make_pair(0,0)));
    
  }
  
  void Plot::AddRatio(string numerHist, string numerHistIdentifier, string denomHist, string denomHistIdentifier, string lable, int marker, int color, string drawingOptions, double cutoff, double cutoffLow)
  {
    if(numerHistIdentifier == "") numerHistIdentifier = mFigureGroup;
    if(denomHistIdentifier == "") denomHistIdentifier = mFigureGroup;
    int padID = 2; //mCurrPad TODO:: remove this stupid hack
    mData[padID].push_back(std::make_shared<Ratio>(numerHist, numerHistIdentifier, denomHist, denomHistIdentifier, lable, color, marker, 0, drawingOptions, "", 1, std::make_pair(cutoffLow, cutoff), std::make_pair(0,0)));
  }
  
  
  void Plot::AddTextBox(double xPos, double yPos, string text, bool userCoordinates, int borderStyle, int borderSize, int borderColor)
  {
    mBoxes[mCurrPad].push_back(std::make_shared<TextBox>(userCoordinates, false, xPos, yPos, borderStyle, borderSize, borderColor, text));
  }
  // todo user coord should be possible in both cases
  void Plot::AddLegendBox(double xPos, double yPos, string title, bool userCoordinates, int nColumns, int borderStyle, int borderSize, int borderColor)
  {
    // TODO: add userCoordinates to arguments
    mBoxes[mCurrPad].push_back(std::make_shared<LegendBox>(userCoordinates, false, xPos, yPos, borderStyle, borderSize, borderColor, title, nColumns));
  }
  void Plot::AddLegendBox(string title, int nColumns, int borderStyle, int borderSize, int borderColor)
  {
    // TODO: add userCoordinates to arguments
    mBoxes[mCurrPad].push_back(std::make_shared<LegendBox>(false, true, 0, 0, borderStyle, borderSize, borderColor, title, nColumns));
  }

  void Plot::SetAxisRange(string axis, double low, double high)
  {
    int tempPadID = mCurrPad;
    if(axis == "ratio") {axis = "Y"; mCurrPad = 2;} // todo:: fix this stupid hack
    if(mAxes[mCurrPad].find(axis) != mAxes[mCurrPad].end())
    {
      mAxes[mCurrPad][axis]->SetAxisRange(low, high);
    }
    else
    {
      mAxes[mCurrPad][axis] = std::make_shared<Axis>(axis, std::make_pair(low, high));
    }
    mCurrPad = tempPadID;
  }
  
  
  void Plot::SetAxisTitle(string axis, string axisTitle)
  {
    int tempPadID = mCurrPad;
    if(axis == "ratio") {axis = "Y"; mCurrPad = 2;} // todo:: fix this stupid hack
    if(mAxes[mCurrPad].find(axis) != mAxes[mCurrPad].end())
    {
      mAxes[mCurrPad][axis]->SetAxisTitle(axisTitle);
    }
    else
    {
      mAxes[mCurrPad][axis] = std::make_shared<Axis>(axis, axisTitle);
    }
    mCurrPad = tempPadID;
  }
  
  
  void Plot::Print()
  {
    cout << " ----------" << string(mName.length(), '-')  << "----------" << endl;
    cout << " <-------- " << mName << " -------->" << endl;
    cout << " ----------" << string(mName.length(), '-')  << "----------" << endl;
    cout << endl;
  }
  
  
  
  ptree Plot::GetPropetyTree()
  {
    // FigureGroup.PlotName.PadID.DATA.HistID
    // convert properties of plot to ptree
    ptree plotTree;
    GetFigureGroup();

//    int padID = 0;
  //  for(auto& data : mData)
    //{
      //std::to_string(padID) + ".";
      //padID++;
    //}




/*
    string plotName = this->GetName();
    int iHisto = 1;
    for(auto& histo : mHistos){
      string histName = "HIST." + std::to_string(iHisto) + "." + histo.name;
      plotTree.put(histName + ".inputID", histo.inputIdentifier);
      plotTree.put(histName + ".type", "hist");
      plotTree.put(histName + ".lable", histo.lable);
      plotTree.put(histName + ".marker", histo.marker);
      iHisto++;
    }
    for(auto& histo : mRatios){
      string histName = "RATIO." + histo.inputIdentifier + "." + histo.name;
      plotTree.put(histName + ".lable", histo.lable);
      plotTree.put(histName + ".marker", histo.marker);
    }
    vector<Axis> axes = {mXaxis, mYaxis, mZaxis, mYaxisRatio};
    int iAxis = 1;
    for(auto& axis : axes){
      string axisName = "AXES." + std::to_string(iAxis);
      if(axis.title != "") plotTree.put(axisName + ".title", axis.title);
      int i = 0; // todo axis name
      for(auto& limit : axis.range){
        plotTree.put(axisName + ".range[" + std::to_string(i) + "]", limit);
      }
      iAxis++;
    }

    int iLegends = 1;
    for(auto& legend : mLegends){
      string legendName = "LEGENDS." + std::to_string(iLegends);
      plotTree.put(legendName + ".x", legend.x);
      plotTree.put(legendName + ".y", legend.y);
    }
*/
    return plotTree;
  }

}
