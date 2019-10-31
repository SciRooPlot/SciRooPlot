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

  Plot::Plot(ptree &plotTree)
  {
    try{
      mName = plotTree.get<string>("name");
      mFigureGroup = plotTree.get<string>("figureGroup");
      mPlotStyle = plotTree.get<string>("plotStyle");
      mOutputFileName = plotTree.get<string>("outputFileName");
      
      // loop over pads defined in property tree
      int padID = 1;
      for(auto& pad : plotTree)
      {
        if(pad.first.find("PAD") != string::npos)
        {
          mControlString[padID] = pad.second.get<string>("controlString");
          for(auto& content : pad.second)
          {
            // add data
            if(content.first.find("DATA") != string::npos)
            {
              string type = content.second.get<string>("type");
              if(type == "graph")
              {
                mData[padID].push_back(std::make_shared<Graph>(content.second));
              }
              if(type == "hist")
              {
                mData[padID].push_back(std::make_shared<Histogram>(content.second));
              }
              if(type == "ratio")
              {
                mData[padID].push_back(std::make_shared<Ratio>(content.second));
              }
            }
            
            // add boxes
            if(content.first.find("BOX") != string::npos)
            {
              string type = content.second.get<string>("type");
              if(type == "legend")
              {
                mBoxes[padID].push_back(std::make_shared<LegendBox>(content.second));
              }
              if(type == "text")
              {
                mBoxes[padID].push_back(std::make_shared<TextBox>(content.second));
              }
            }
            
            // add axes
            if(content.first.find("AXIS") != string::npos)
            {
              string axis = content.second.get<string>("name");
              mAxes[padID][axis] = std::make_shared<Axis>(content.second);
            }
          }
          padID++;
        }
      }
      
      
    }catch(...){
      cout << "ERROR: could not construct data from ptree." << endl;
    }
    mClearCutoffBin = false;
    mCurrPad = 1;
  }

  void Plot::AddGraph(string graphName, string inputIdentifier, string lable, int marker, int color, string drawingOptions, double cutoff, double cutoffLow)
  {
    if(inputIdentifier == "") inputIdentifier = mFigureGroup; // default identifier to figuregroup id
    mData[mCurrPad].push_back(std::make_shared<Graph>(graphName, inputIdentifier, lable, color, marker, 0, drawingOptions, std::make_pair(cutoffLow, cutoff)));
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
    int padID = (mData[mCurrPad].empty()) ? mCurrPad : 2; //mCurrPad TODO:: remove this stupid hack
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
  }
    
  
  ptree Plot::GetPropetyTree()
  {
    // convert properties of plot to ptree
    ptree plotTree;
    plotTree.put("name", mName);
    plotTree.put("figureGroup", mFigureGroup);
    plotTree.put("plotStyle", mPlotStyle);
    plotTree.put("outputFileName", mOutputFileName);

    
    for(auto& padData : mData)
    {
      int padID = padData.first;
      ptree padTree;
      padTree.put("controlString", mControlString[padID]);

      // add data of pad
      int dataID = 1;
      for(auto& data : padData.second){
        padTree.put_child("DATA_" + std::to_string(dataID), data->GetPropertyTree());
        dataID++;
      }

      // add boxes of pad
      int boxID = 1;
      for(auto& box : mBoxes[padID])
      {
        padTree.put_child("BOX_" + std::to_string(boxID), box->GetPropertyTree());
        boxID++;
      }
      
      // add axes of pad
      vector<string> axisTitles = {"X", "Y", "Z"};
      for(auto& axisTitle : axisTitles)
      {
        if(IsAxisDefined(padID, axisTitle))
        {
          padTree.put_child("AXIS_" + axisTitle, GetAxis(padID, axisTitle)->GetPropertyTree());
        }
      }
      plotTree.put_child("PAD_" + std::to_string(padID), padTree);
    }
    return plotTree;
  }
}
