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
    Plot(string name, string figureGroup, string plotStyle = "default")
    : Plot()
    {
      mName = name; mFigureGroup = figureGroup; mPlotStyle = plotStyle;
    }
    Plot(const Plot& otherPlot){
      cout << "FATAL ERROR: copy constructor of Plot class not yet implemented!!" << endl;
      // be careful to deep copy heap objects (data)
    }
    Plot(Plot&& otherPlot) = default; // moving pointers to heap objects is ok
    
    // accessors for internal use by manager
    string& GetName(){return mName;}
    string& GetControlString(int padID){return mControlString[padID];}
    string& GetFigureGroup(){return mFigureGroup;}
    map<unsigned int, vector<shared_ptr<Data>>>& GetData(){return mData;}
    vector<shared_ptr<Data>>& GetData(int padID){return mData[padID];}
    vector<shared_ptr<Box>>& GetBoxes(int padID){return mBoxes[padID];}

    void Print();
    
    // User accessors:
    void AddHisto(string histName, string inputIdentifier = "", string lable = "", int marker = 0, int color = 0, string drawingOptions = "", double cutoff = -999, double cutoffLow = -999);
    void AddRatio(string numerHist, string numerHistIdentifier, string denomHist, string denomHistIdentifier = "", string lable = "", int marker = 0, int color = 0, string errorStyle = "", double cutoff = -999, double cutoffLow = -999);
    void AddGraph(string graphName, string inputIdentifier = "", string lable = "", int marker = 0, int color = 0, string drawingOptions = "");
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
    const string& GetOutputFileName(){return mOutputFileName;}
    // maybe also getlegends gettexts
    string GetUniqueName(){return mName + "_@_" + mFigureGroup;}

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
    string mPlotStyle;
    string mOutputFileName;

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
    const string& GetType(){return mType;}
    const string& GetName() {return mName;}
    const string& GetInputIdentifier(){return mInputIdentifier;}
    const string& GetLable(){return mLable;}
    const int& GetColor(){return mColor;}
    const int& GetStyle(){return mStyle;}
    const int& GetSize(){return mSize;}
    const string& GetDrawingOptions(){return mDrawingOptions;}

    //virtual bool isValidDrawingOption() = 0;
    string GetUniqueName(){return mName + "_@_" + mInputIdentifier;}
    virtual ptree GetPropertyTree(){} // returns property tree with base prperties, move return value
    
  protected:
    Data(string name, string inputIdentifier, string lable, int color, int style, int size, string drawingOptions)
    : mType("none"), mName(name), mInputIdentifier(inputIdentifier), mLable(lable), mColor(color), mStyle(style), mSize(size), mDrawingOptions(drawingOptions)
    {
      // constructor code
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
    Histogram(string name, string inputIdentifier, string lable, int color, int style, int size, string drawingOptions, int scale, pair<double, double> histoRangeX, pair<double, double> histoRangeY)
    : Data(name, inputIdentifier, lable, color, style, size, drawingOptions), mScale(scale), mHistoRangeX(histoRangeX), mHistoRangeY(histoRangeY)
    {
      SetType("hist");
    }
    
    const double& GetScaleFactor(){return mScale;}
    // TODO better names for this + add Yaxis getters
    const double& GetHistCutHigh(){return mHistoRangeX.second;}
    const double& GetHistCutLow(){return mHistoRangeX.first;}

    virtual ptree GetPropertyTree(){} //{Data::GetPropertyTree();} // call  move return value
    
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
    Ratio(string name, string inputIdentifier, string denomName, string denomInputIdentifier, string lable, int color, int style, int size, string drawingOptions, string divideMethod, int scale, pair<double, double> histoRangeX, pair<double, double> histoRangeY)
    : Histogram(name, inputIdentifier, lable, color, style, size, drawingOptions, scale, histoRangeX, histoRangeY), mDenomName(denomName), mDenomInputIdentifier(denomInputIdentifier), mDivideMethod(divideMethod)
    {
      SetType("ratio");
    }
    string GetUniqueNameDenom(){return mDenomName + "_@_" + mDenomInputIdentifier;}
    virtual ptree GetPropertyTree(){} // {Histogram::GetPropertyTree();}
    
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
    Graph(string name, string inputIdentifier, string lable, int color, int style, int size, string drawingOptions)
    : Data(name, inputIdentifier, lable, color, style, size, drawingOptions)
    {
      SetType("graph");
    }
    
    virtual ptree GetPropertyTree(){} //{Data::GetPropertyTree();} // call  move return value
    
  private:
    // specifics for graph...
  };
  

  //****************************************************************************************
  /**
   * Representation of a function.
   */
  //****************************************************************************************
  class Plot::Function : public Plot::Data
  {
  public:
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
    virtual ptree GetPropertyTree();
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
    Axis(): mName("dummy"), mTitle(""), mRange(std::make_pair(0,0)){}
    Axis(string axisName, string title) : mName(axisName), mTitle(title), mRange(std::make_pair(0,0))
    {
      
    }
    Axis(string axisName, pair<double, double> range) : mName(axisName), mTitle(""), mRange(range)
    {
      
    }
    const string& GetTitle() {return mTitle;}
    void SetAxisTitle(string title) {mTitle = title;}
    void SetAxisRange(double low, double high) {mRange = std::make_pair(low, high);}
    pair<double, double>& GetAxisRange() {return mRange;}
    
    bool IsRangeSet(){return !((mRange.first == 0) && (mRange.second == 0));}
    bool IsTitleSet(){return (!mTitle.empty());}

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
    TextBox(bool userCoordinates, bool autoPlacement, double x, double y, int borderStyle, int borderSize, int borderColor, string text)
    : Box(userCoordinates, autoPlacement, x, y, borderStyle, borderSize, borderColor), mText(text)
    {
      SetType("text");
      // constructor code
    }
    const string& GetText();
    void SetText(string text);
    //void SetDelimiter(string delimiter){mDelimiter = delimiter;}
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
    LegendBox(bool userCoordinates, bool autoPlacement, double x, double y, int borderStyle, int borderSize, int borderColor, string title, int nColumns)
    : Box(userCoordinates, autoPlacement, x, y, borderStyle, borderSize, borderColor), mTitle(title), mNumColumns(nColumns)
    {
      SetType("legend");
      // constructor code
    }
    string& GetTitle(){return mTitle;}
    int GetNumColumns(){return mNumColumns;}
  private:
    int GetNumEntries();
    string mTitle;
    int mNumColumns;
  };
  
} // end of namespace



#endif
