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
    Double_t fX;
    Double_t fY;
    string fText;
    Int_t fBorderStyle;
    Int_t fBorderSize;
    Int_t fBorderColor;
  } TextBox;

  typedef struct Axis {
    vector<Double_t> fRange;
    string fTitle;
  } Axis;

  struct Histogram {
    string fName;
    string fLable;
    Int_t fMarker;
    Int_t fColor;
    string fErrorStyle; // normal, boxes, noerr
    Int_t fLableBoxID;
  };

  struct Function {
    string fName;
    string fLable;
    string fFunction;
    string fFitHist;
    Int_t fColor;
    Int_t fMarker;
  };

  Plot();
  Plot(string name) : Plot() {fName = name;}

  void Print();

  void AddHisto(string histName, string identifier = "", string lable = "", Int_t marker = 0, Int_t color = 0, string errorStyle = "");
  void AddRatio(string numerHist, string numerHistIdentifier, string denomHist, string denomHistIdentifier = "", string lable = "");

  void AddTextBox(Double_t xPos, Double_t yPos, string text = "", Int_t borderStyle = kSolid, Int_t borderSize = 0, Int_t borderColor = kBlack);
  void AddLegendBox(Double_t xPos, Double_t yPos, string title = "", Int_t borderStyle = kSolid, Int_t borderSize = 0, Int_t borderColor = kBlack);

  void SetAxisTitle(string axis, string axisTitle);

  void SetAxisRange(string axis, Double_t low, Double_t high);
  void SetRatioRange(Double_t yLow, Double_t yHigh){}

  vector<Double_t>& GetAxisRange(string axis);

  Bool_t IsRatioPlot() const {return !fRatios.empty();}

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
  Int_t GetNumSubPlots() {if(IsRatioPlot()) return 2; else return 1;}

  vector<TextBox>& GetLegends() {return fLegends;}
  vector<TextBox>& GetTextBoxes() {return fTexts;}

private:
  Axis& GetAxis(string axis);

  string fName;
  string fPlotStyle;
  string fFigureGroup;

  Axis fXaxis, fYaxis, fZaxis, fYaxisRatio;

  string fControlString;
  Bool_t fClearCutoffBin;

  vector<TextBox> fTexts;
  vector<TextBox> fLegends;
  vector<Histogram> fHistos;
  vector<Histogram> fRatios;
};

#endif
