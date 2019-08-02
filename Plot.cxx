#include "Plot.h"

namespace PlottingFramework {

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
  
  
  
  ptree Plot::GetProperties()
  {
    // convert properties of plot to ptree
    ptree plotTree;
    string plotName = this->GetName();
    for(auto& histo : mHistos){
      string histName = "HIST." + histo.name;
      plotTree.put(histName + ".lable", histo.lable);
      plotTree.put(histName + ".marker", histo.marker);
    }
    for(auto& histo : mRatios){
      string histName = "RATIO." + histo.name;
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

    
    /*
    // traverse plot tree and set properties
    for(auto& plot : plotTree)
    {
      ptree& myPlot = plot.second;
      cout << plot.first << " named " << myPlot.get<string>("name") << endl;

      for(auto& plotProperties : myPlot)
      {
        if(plotProperties.first.find("histo") != string::npos)
        {
          ptree myHist = myPlot.get_child(plotProperties.first);
          try{
            cout << "name: " << myHist.get<string>("name") << endl;
            cout << "lable: " << myHist.get<string>("lable") << endl;
            cout << "marker: " << myHist.get<int>("marker") << endl;
          }
          catch(...){
            cout << "ERROR: could not extract all plot properties from ptree." << endl;
            cout << typeid(std::current_exception()).name() << endl;
          }
        }
      }
    }
    */
    return plotTree;
  }

//  void Plot::SetProperties(ptree& plotBlueprint)
  //{
    // use ptree to fill plot
    //resultTree.add_child("plot1", plotTree);
    //write_json(configFileName + ".JSON", resultTree);
    //write_json(cout, resultTree.get_child("plot2"));
    
    /* read in file
     
     ptree configIn;
     read_json(configFileName + ".JSON", configIn);
     float v = configIn.get<float>("irgendwas.ptee");
     cout << v << endl;
     */
  //}

}
