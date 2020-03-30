// Plotting Framework
//
// Copyright (C) 2019-2020  Mario Krüger
// Contact: mario.kruger@cern.ch
// For a full list of contributors please see docs/Credits
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

#ifndef Plot_h
#define Plot_h

#include "PlottingFramework.h"

using namespace PlottingFramework;
namespace PlottingFramework {

//****************************************************************************************
/**
 * Class containing internal representation of a plot.
 * It stores:
 * - user defined plot properties
 * - data location and all corresponding user defined drawing properties
 * - position and content of legend and text boxes
 * - axis ranges
 */
//****************************************************************************************
class Plot
{
public:
  class Data; // move to private
  class Histogram;
  class Ratio;
  class Graph;
  class Function;
  class Axis;
  class Box;
  class TextBox;
  class LegendBox;
  
  
  Plot();
  Plot(ptree &plotTree);
  Plot(string name, string figureGroup, string plotStyle = "default");
  Plot(const Plot& otherPlot) = default;
  Plot(Plot& otherPlot) = default;
  
  // accessors for internal use by manager
  string& GetName(){return mName;}
  string& GetControlString(int padID){return mControlString[padID];}
  string& GetFigureGroup(){return mFigureGroup;}
  string& GetFigureCategory(){return mFigureCategory;}
  map<unsigned int, vector<shared_ptr<Data>>>& GetData(){return mData;}
  vector<shared_ptr<Data>>& GetData(int padID){return mData[padID];}
  vector<shared_ptr<Box>>& GetBoxes(int padID){return mBoxes[padID];}
  map<string, set<string>> GetRequiredInputData();
  
  void SetFigureCategory(string figureCategory){mFigureCategory = figureCategory;}
  
  void Print();
  
  // User accessors:
  void AddFrame(string histName, string inputIdentifier);
  void AddHisto(string histName, string inputIdentifier = "", string lable = "", int marker = 0, int color = 0, string drawingOptions = "", double cutoff = -999, double cutoffLow = -999);
  void AddRatio(string numerHist, string numerHistIdentifier, string denomHist, string denomHistIdentifier = "", string lable = "", int marker = 0, int color = 0, string errorStyle = "", double cutoff = -999, double cutoffLow = -999);
  void AddGraph(string graphName, string inputIdentifier = "", string lable = "", int marker = 0, int color = 0, string drawingOptions = "", double cutoff = -999, double cutoffLow = -999);
  void AddTextBox(double xPos, double yPos, string text = "", bool userCoord = false, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);
  void AddLegendBox(double xPos, double yPos, string title = "", bool userCoordinates = false, int nColumns = 1, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);
  void AddLegendBox(string title = "", int nColumns = 1, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);
  void SetAxisTitle(string axis, string axisTitle);
  void SetAxisRange(string axis, double low, double high);
  inline void SetDrawingProperties(string controlString){mControlString[mCurrPad] = controlString;}
  inline void SetPlotStyle(string plotStyle){mPlotStyle = plotStyle;}
  inline void ChangePad(int padID) {mCurrPad = padID;}
  
  // functions to modify
  const string& GetPlotStyle(){return mPlotStyle;}
  // maybe also getlegends gettexts
  string GetUniqueName(){return mName + gNameGroupSeparator + mFigureGroup + ((mFigureCategory != "") ? ":" + mFigureCategory : "");}
  
  int GetNumRequiredPads(){return mData.size();}
  ptree GetPropetyTree();
  
  //    pair<double, double> GetAxisRange(int padID, string axis) {return mAxes[padID][axis].GetAxisRange()}
  bool IsAxisDefined(unsigned int padID, string axis) {return (mAxes[padID][axis] != nullptr);}
  shared_ptr<Axis> GetAxis(unsigned int padID, string axis) {return mAxes[padID][axis];}
  
protected:
  // put some functions here that only manager should access. maybe the getters...
private:
  friend class PlotManager; // allow PlotManager access to private and protected members
  string mName;
  string mFigureGroup;
  string mFigureCategory;
  string mPlotStyle;
  
  unsigned int mCurrPad; // current pad to assign histograms to
  map<unsigned int, vector<shared_ptr<Data>>> mData; // mData[padID][dataID]
  map<unsigned int, vector<shared_ptr<Box>>> mBoxes; // mBoxes[padID][boxID]
  map<unsigned int, map<string, shared_ptr<Axis>>> mAxes; // padID, "x", axis properties
  
  map<unsigned int, string> mControlString; // mControlString[padID]
  
  bool mClearCutoffBin;
};
//========================================================================================

//****************************************************************************************
/**
 * Base class for representation of drawable data.
 */
//****************************************************************************************
class Plot::Data
{
public:
  Data(const Data& otherPlot) = default;
  Data(Data& otherPlot) = default;
  
  const string& GetType(){return mType;}
  const string& GetName() {return mName;}
  const string& GetInputIdentifier(){return mInputIdentifier;}
  const string& GetLable(){return mLable;}
  const int& GetColor(){return mColor;}
  const int& GetStyle(){return mStyle;}
  const int& GetSize(){return mSize;}
  const string& GetDrawingOptions(){return mDrawingOptions;}
  
  //virtual bool isValidDrawingOption() = 0;
  string GetUniqueName(){return mName + gNameGroupSeparator + mInputIdentifier;}
  virtual ptree GetPropertyTree(){
    ptree dataTree;
    dataTree.put("type", mType);
    dataTree.put("name", mName);
    dataTree.put("inputIdentifier", mInputIdentifier);
    dataTree.put("lable", mLable);
    dataTree.put("color", mColor);
    dataTree.put("style", mStyle);
    dataTree.put("size", mSize);
    dataTree.put("drawingOptions", mDrawingOptions);
    return dataTree;
  }
  
  // constructor to define entry from file
  Data(ptree &dataTree){
    try{
      mType = dataTree.get<string>("type");
      mName = dataTree.get<string>("name");
      mInputIdentifier = dataTree.get<string>("inputIdentifier");
      mLable = dataTree.get<string>("lable");
      mColor = dataTree.get<int>("color");
      mStyle = dataTree.get<int>("style");
      mSize = dataTree.get<int>("size");
      mDrawingOptions = dataTree.get<string>("drawingOptions");
    }catch(...){
      ERROR("Could not construct data from ptree.");
    }
  }
  
  
protected:
  Data(string name, string inputIdentifier, string lable, int color, int style, int size, string drawingOptions)
  : mType("none"), mName(name), mInputIdentifier(inputIdentifier), mLable(lable), mColor(color), mStyle(style), mSize(size), mDrawingOptions(drawingOptions)
  {
    // if input was specified further via inputIdentifier:some/path/in/file
    auto subPathPos = inputIdentifier.find(":");
    if(subPathPos != string::npos)
    {
      // prepend path to plot name
      mName = inputIdentifier.substr(subPathPos+1) + "/" + name;
      mInputIdentifier = inputIdentifier.substr(0, subPathPos);
    }
  }
  void SetType(string type){mType = type;}
  
private:
  string mType; // for introspection: "hist", "ratio", "graph", ...
  string mName;
  string mInputIdentifier;
  string mLable;
  
  int mColor;
  int mStyle; // markerStyle or lineStyle
  int mSize;  // markerSize or lineWidth
  string mDrawingOptions; // "normal", "boxes", "hist", "band" root: "text", "func" ...
  // can be different for graphs and histos and functions
  // put useful options in documentation!!!
  // for functions "LSAME"!! smooth: "C" fill area: "FC"
  // TODO: use root version of options + map more readable ones to corresponding root ones
  // static const vector<string> allowedDrawingOptions = {"L", "C", "FC"};
};

//****************************************************************************************
/**
 * Representation of a histogram.
 */
//****************************************************************************************
class Plot::Histogram : public Plot::Data
{
public:
  Histogram(const Histogram& otherHistogram) = default;
  Histogram(Histogram& otherHistogram) = default;
  virtual ~Histogram() = default;
  
  Histogram(string name, string inputIdentifier, string lable, int color, int style, int size, string drawingOptions, int scale, pair<double, double> histoRangeX, pair<double, double> histoRangeY)
  : Data(name, inputIdentifier, lable, color, style, size, drawingOptions), mScale(scale), mHistoRangeX(histoRangeX), mHistoRangeY(histoRangeY)
  {
    SetType("hist");
  }
  
  const double& GetScaleFactor(){return mScale;}
  // TODO better names for this + add Yaxis getters
  const double& GetHistCutHigh(){return mHistoRangeX.second;}
  const double& GetHistCutLow(){return mHistoRangeX.first;}
  
  ptree GetPropertyTree(){
    ptree dataTree = Data::GetPropertyTree();
    dataTree.put("scale", mScale);
    dataTree.put("rangeX_low", mHistoRangeX.first);
    dataTree.put("rangeX_high", mHistoRangeX.second);
    dataTree.put("rangeY_low", mHistoRangeY.first);
    dataTree.put("rangeY_high", mHistoRangeY.second);
    return dataTree;
  }
  
  // constructor to define entry from file
  Histogram(ptree &dataTree) : Data(dataTree){
    try{
      mHistoRangeX.first = dataTree.get<double>("rangeX_low");
      mHistoRangeX.second = dataTree.get<double>("rangeX_high");
      mHistoRangeY.first = dataTree.get<double>("rangeX_low");
      mHistoRangeY.second = dataTree.get<double>("rangeX_high");
    }catch(...){
      ERROR("Could not construct histogram from ptree.");
    }
  }
  
private:
  double mScale;
  pair<double, double> mHistoRangeX;
  pair<double, double> mHistoRangeY;
};

//****************************************************************************************
/**
 * Representation of a ratio.
 */
//****************************************************************************************
class Plot::Ratio : public Plot::Histogram
{
public:
  Ratio(const Ratio& otherRatio) = default;
  Ratio(Ratio& otherRatio) = default;
  virtual ~Ratio() = default;
  
  Ratio(string name, string inputIdentifier, string denomName, string denomInputIdentifier, string lable, int color, int style, int size, string drawingOptions, string divideMethod, int scale, pair<double, double> histoRangeX, pair<double, double> histoRangeY)
  : Histogram(name, inputIdentifier, lable, color, style, size, drawingOptions, scale, histoRangeX, histoRangeY), mDenomName(denomName), mDenomInputIdentifier(denomInputIdentifier), mDivideMethod(divideMethod)
  {
    SetType("ratio");
  }
  string GetDenomIdentifier(){return mDenomInputIdentifier;}
  string GetDenomName(){return mDenomName;}
  string GetUniqueNameDenom(){return mDenomName + gNameGroupSeparator + mDenomInputIdentifier;}
  ptree GetPropertyTree(){
    ptree dataTree = Histogram::GetPropertyTree();
    dataTree.put("denomName", mDenomName);
    dataTree.put("denomInputID", mDenomInputIdentifier);
    dataTree.put("divideMethod", mDivideMethod);
    return dataTree;
  }
  
  // constructor to define entry from file
  Ratio(ptree &dataTree) : Histogram(dataTree){
    try{
      mDenomName = dataTree.get<string>("denomName");
      mDenomInputIdentifier = dataTree.get<string>("denomInputID");
      mDivideMethod = "";
      //mDivideMethod = dataTree.get<double>("divideMethod"); // TODO:: it should not crash if xml entry is empty!!
    }catch(...){
      ERROR("Could not construct ratio from ptree.");
    }
  }
  
private:
  string mDenomName;
  string mDenomInputIdentifier;
  string mDivideMethod;  // "tspline3", "binomial", etc.
};


//****************************************************************************************
/**
 * Representation of a graph.
 */
//****************************************************************************************
class Plot::Graph : public Plot::Data
{
public:
  Graph(const Graph& otherGraph) = default;
  Graph(Graph& otherGraph) = default;
  virtual ~Graph() = default;
  
  Graph(string name, string inputIdentifier, string lable, int color, int style, int size, string drawingOptions, pair<double, double> graphRangeX)
  : Data(name, inputIdentifier, lable, color, style, size, drawingOptions), mGraphRangeX(graphRangeX)
  {
    SetType("graph");
  }
  const double& GetGraphCutLow(){return mGraphRangeX.first;}
  const double& GetGraphCutHigh(){return mGraphRangeX.second;}
  
  ptree GetPropertyTree(){
    ptree dataTree = Data::GetPropertyTree();
    dataTree.put("rangeX_low", mGraphRangeX.first);
    dataTree.put("rangeX_high", mGraphRangeX.second);
    return dataTree;
  }
  
  // constructor to define entry from file
  Graph(ptree &dataTree) : Data(dataTree){
    try{
      mGraphRangeX.first = dataTree.get<double>("rangeX_low");
      mGraphRangeX.second = dataTree.get<double>("rangeX_high");
    }catch(...){
      ERROR("Could not construct graph from ptree.");
    }
  }
private:
  // specifics for graph...
  pair<double, double> mGraphRangeX;
  
};


//****************************************************************************************
/**
 * Representation of a function.
 */
//****************************************************************************************
class Plot::Function : public Plot::Data
{
public:
  Function(const Function& otherFunction) = default;
  Function(Function& otherFunction) = default;
  
  Function(string name, string inputIdentifier, string lable, int color, int style, int size, string drawingOptions)
  : Data(name, inputIdentifier, lable, color, style, size, drawingOptions), mFormula("")
  {
    SetType("func");
  }
  Function(string functionName, string formula, int xMin, int xMax, string lable, int color, int style, int size, string drawingOptions)  : Data(functionName, "", lable, color, style, size, drawingOptions), mFormula(formula)
  
  {
    SetType("userFunc");
    // CAREFUL this has to be handled different by manager!!
  }
  
  ptree GetPropertyTree(){
    ptree dataTree = Data::GetPropertyTree();
    dataTree.put("fromula", mFormula);
    return dataTree;
  }
  
  // constructor to define entry from file
  Function(ptree &dataTree) : Data(dataTree){
    try{
      mFormula = dataTree.get<string>("formula");
    }catch(...){
      ERROR("Could not construct function from ptree.");
    }
  }
  
private:
  string mFormula;
  // TODO: option to fit to loaded data? what about ratios, that exist only in manager?
};


//****************************************************************************************
/**
 * Representation of an axis.
 */
//****************************************************************************************
class Plot::Axis {
public:
  Axis(const Axis& otherAxis) = default;
  Axis(Axis& otherAxis) = default;
  
  Axis(): mName("dummy"), mTitle(""), mRange(std::make_pair(0,0)){}
  Axis(string axisName, string title) : mName(axisName), mTitle(title), mRange(std::make_pair(0,0)), mNumTicks(0)
  {
    
  }
  Axis(string axisName, pair<double, double> range) : mName(axisName), mTitle(""), mRange(range), mNumTicks(0)
  {
    
  }
  const string& GetTitle() {return mTitle;}
  void SetAxisTitle(string title) {mTitle = title;}
  void SetAxisRange(double low, double high) {mRange = std::make_pair(low, high);}
  pair<double, double>& GetAxisRange() {return mRange;}
  
  bool IsRangeSet(){return !((mRange.first == 0) && (mRange.second == 0));}
  bool IsTitleSet(){return (!mTitle.empty());}
  
  ptree GetPropertyTree(){
    ptree axisTree;
    axisTree.put("name", mName);
    axisTree.put("title", mTitle);
    axisTree.put("range_low", mRange.first);
    axisTree.put("range_high", mRange.second);
    axisTree.put("numTicks", mNumTicks);
    return axisTree;
  }
  
  // constructor to define entry from file
  Axis(ptree &axisTree){
    try{
      mName = axisTree.get<string>("name");
      mTitle = axisTree.get<string>("title");
      mRange.first = axisTree.get<double>("range_low");
      mRange.second = axisTree.get<double>("range_high");
      mNumTicks = axisTree.get<int>("numTicks");
    }catch(...){
      ERROR("Could not construct axis from ptree.");
    }
  }
  
private:
  string mName;
  string mTitle;
  pair<double, double> mRange;
  int mNumTicks;
};


//****************************************************************************************
/**
 * Representation of a box.
 */
//****************************************************************************************
class Plot::Box {
public:
  Box(const Box& otherBox) = default;
  Box(Box& otherBox) = default;
  
  Box(bool userCoordinates, bool autoPlacement, double x, double y, int borderStyle, int borderSize, int borderColor)
  : mType("none"), mUserCoordinates(userCoordinates), mAutoPlacement(autoPlacement), mX(x), mY(y), mBorderStyle(borderStyle), mBorderSize(borderSize), mBorderColor(borderColor)
  {
    // constructor code
  }
  double GetXPosition(){return mX;}
  double GetYPosition(){return mY;}
  int GetBorderStyle(){return mBorderStyle;}
  int GetBorderSize(){return mBorderSize;}
  int GetBorderColor(){return mBorderColor;}
  bool IsUserCoordinates(){return mUserCoordinates;}
  bool IsAutoPlacement(){return mAutoPlacement;}
  
  const string& GetType(){return mType;}
  
  virtual ptree GetPropertyTree(){
    ptree boxTree;
    boxTree.put("type", mType);
    boxTree.put("userCoordinates", mUserCoordinates);
    boxTree.put("autoPlacement", mAutoPlacement);
    boxTree.put("x", mX);
    boxTree.put("y", mY);
    boxTree.put("borderStyle", mBorderStyle);
    boxTree.put("borderSize", mBorderSize);
    boxTree.put("borderColor", mBorderColor);
    return boxTree;
  };
  
  // constructor to define entry from file
  Box(ptree &boxTree){
    try{
      mType = boxTree.get<string>("type");
      mUserCoordinates = boxTree.get<bool>("userCoordinates");
      mAutoPlacement = boxTree.get<bool>("autoPlacement");
      mX = boxTree.get<double>("x");
      mY = boxTree.get<double>("y");
      mBorderStyle = boxTree.get<int>("borderStyle");
      mBorderSize = boxTree.get<int>("borderSize");
      mBorderColor = boxTree.get<int>("borderColor");
    }catch(...){
      ERROR("Could not construct box from ptree.");
    }
    
  }
  
  
protected:
  void SetType(string type){mType = type;}
private:
  string mType; // for introspection
  bool mUserCoordinates;
  bool mAutoPlacement;
  double mX;
  double mY;
  int mBorderStyle;
  int mBorderSize;
  int mBorderColor;
};


//****************************************************************************************
/**
 * Representation of a text box.
 */
//****************************************************************************************
class Plot::TextBox : public Plot::Box {
public:
  TextBox(const TextBox& otherTextBox) = default;
  TextBox(TextBox& otherTextBox) = default;
  virtual ~TextBox() = default;
  
  TextBox(bool userCoordinates, bool autoPlacement, double x, double y, int borderStyle, int borderSize, int borderColor, string text)
  : Box(userCoordinates, autoPlacement, x, y, borderStyle, borderSize, borderColor), mText(text)
  {
    SetType("text");
    // constructor code
  }
  string GetText(){return mText;}
  void SetText(string text);
  //void SetDelimiter(string delimiter){mDelimiter = delimiter;}
  
  ptree GetPropertyTree(){
    ptree boxTree = Box::GetPropertyTree();
    boxTree.put("text", mText);
    return boxTree;
  };
  
  // constructor to define entry from file
  TextBox(ptree &boxTree) : Box(boxTree)
  {
    try{
      mText = boxTree.get<string>("text");
    }catch(...){
      ERROR("Could not construct textbox from ptree.");
    }
  }
  
private:
  int GetNumLines();
  //string mDelimiter;
  string mText;
};


//****************************************************************************************
/**
 * Representation of a legend box.
 */
//****************************************************************************************
class Plot::LegendBox : public Plot::Box {
public:
  LegendBox(const LegendBox& otherLegendBox) = default;
  LegendBox(LegendBox& otherLegendBox) = default;
  virtual ~LegendBox() = default;
  
  LegendBox(bool userCoordinates, bool autoPlacement, double x, double y, int borderStyle, int borderSize, int borderColor, string title, int nColumns)
  : Box(userCoordinates, autoPlacement, x, y, borderStyle, borderSize, borderColor), mTitle(title), mNumColumns(nColumns)
  {
    SetType("legend");
    // constructor code
  }
  string& GetTitle(){return mTitle;}
  int GetNumColumns(){return mNumColumns;}
  
  ptree GetPropertyTree(){
    ptree boxTree = Box::GetPropertyTree();
    boxTree.put("title", mTitle);
    boxTree.put("numColumns", mNumColumns);
    return boxTree;
  };
  
  // constructor to define entry from file
  LegendBox(ptree &boxTree) : Box(boxTree)
  {
    try{
      mTitle = boxTree.get<string>("title");
      mNumColumns = boxTree.get<int>("numColumns");
    }catch(...){
      ERROR("Could not construct legendbox from ptree.");
    }
  }
  
private:
  int GetNumEntries();
  string mTitle;
  int mNumColumns;
};

} // end namespace PlottingFramework
#endif /* Plot_h */
