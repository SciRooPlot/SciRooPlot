#ifndef Plot_h
#define Plot_h


#include "Plot.h"
#include "TObjArray.h"
#include "TCanvas.h"


#include <iostream>
using std::cout;
using std::endl;
using std::flush;
using std::string;
using std::vector;

// TODO: extend so multipad plots are possible

class Plot
{
public:

  typedef struct TextBox {
    double fX;
    double fY;
    string fText;
    int fBorderStyle;
    int fBorderSize;
    int fBorderColor;
    int fUserCoordinates;
    int fNColumns;
  } TextBox;

  typedef struct Axis {
    vector<double> fRange;
    string fTitle;
  } Axis;

  struct Histogram {
    string fName;
    string fLable;
    int fMarker;
    int fColor;
    string fErrorStyle; // normal, boxes, hist
    double fCutoff; // todo multi-dimensional
    double fCutoffLow; // todo multi-dimensional
    int fLableBoxID;
  };

  struct Function {
    string fName;
    string fLable;
    string fFunction;
    string fFitHist;
    int fColor;
    int fMarker;
  };

  Plot();
  Plot(string name) : Plot() {fName = name;}

  void Print();

  void AddGraph(string histName, string identifier = "", string lable = "", int marker = 0, int color = 0, string errorStyle = "", double cutoff = -999, double cutoffLow = -999);
  void AddHisto(string histName, string identifier = "", string lable = "", int marker = 0, int color = 0, string errorStyle = "", double cutoff = -999, double cutoffLow = -999);
  void AddRatio(string numerHist, string numerHistIdentifier, string denomHist, string denomHistIdentifier = "", string lable = "", int marker = 0, int color = 0, string errorStyle = "", double cutoff = -999, double cutoffLow = -999);

  void AddTextBox(double xPos, double yPos, string text = "", bool userCoord = kFALSE, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);
  void AddLegendBox(double xPos, double yPos, string title = "", int nColums = 1, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);

  void SetAxisTitle(string axis, string axisTitle);

  void SetAxisRange(string axis, double low, double high);
  void SetRatioRange(double yLow, double yHigh){}

  vector<double>& GetAxisRange(string axis);

  bool IsRatioPlot() const {return !fRatios.empty();}

  void ClearCutoffBin(){}
  string GetName(){return fName;}
  string GetControlString(){return fControlString;}
  string GetFigureGroup(){return fFigureGroup;}

  void SetDrawingProperties(string controlString){fControlString = controlString;}
  void SetFigureGroup(string figureGroupName){fFigureGroup = figureGroupName;}
  void SetPlotStyle(string plotStyle){fPlotStyle = plotStyle;}
  string GetPlotStyle(){return fPlotStyle;}

  vector<Histogram>& GetHistoTemplates() {return fHistos;}
  vector<Histogram>& GetRatioTemplates() {return fRatios;}
  string& GetAxisTitle(string axis);
  int GetNumSubPlots() {if(IsRatioPlot()) return 2; else return 1;}

  vector<TextBox>& GetLegends() {return fLegends;}
  vector<TextBox>& GetTextBoxes() {return fTexts;}

  vector<Histogram> fGraphs; // dirty hack for legend errorstyles....
  vector<Histogram> fHistos; // dirty hack for legend errorstyles....
  vector<Histogram> fRatios;

private:
  Axis& GetAxis(string axis);

  string fName;
  string fPlotStyle;
  string fFigureGroup;

  Axis fXaxis, fYaxis, fZaxis, fYaxisRatio;

  string fControlString;
  bool fClearCutoffBin;

  vector<TextBox> fTexts;
  vector<TextBox> fLegends;
};

#endif
