#ifndef Plot_h
#define Plot_h


#include "Plot.h"
#include "TObjArray.h"
#include "TCanvas.h"
#include <iostream>

namespace PlottingProject {
  
  class Plot
  {
  public:
    
    typedef struct TextBox {
      double fX;
      double fY;
      std::string fText;
      int fBorderStyle;
      int fBorderSize;
      int fBorderColor;
      int fUserCoordinates;
      int fNColumns;
    } TextBox;
    
    typedef struct Axis {
      std::vector<double> fRange;
      std::string fTitle;
    } Axis;
    
    struct Histogram {
      std::string fName;
      std::string fLable;
      int fMarker;
      int fColor;
      std::string fErrorStyle; // normal, boxes, hist
      double fCutoff; // todo multi-dimensional
      double fCutoffLow; // todo multi-dimensional
      int fLableBoxID;
    };
    
    struct Function {
      std::string fName;
      std::string fLable;
      std::string fFunction;
      std::string fFitHist;
      int fColor;
      int fMarker;
    };
    
    Plot();
    Plot(std::string name) : Plot() {fName = name;}
    
    void Print();
    
    void AddGraph(std::string histName, std::string identifier = "", std::string lable = "", int marker = 0, int color = 0, std::string errorStyle = "", double cutoff = -999, double cutoffLow = -999);
    void AddHisto(std::string histName, std::string identifier = "", std::string lable = "", int marker = 0, int color = 0, std::string errorStyle = "", double cutoff = -999, double cutoffLow = -999);
    void AddRatio(std::string numerHist, std::string numerHistIdentifier, std::string denomHist, std::string denomHistIdentifier = "", std::string lable = "", int marker = 0, int color = 0, std::string errorStyle = "", double cutoff = -999, double cutoffLow = -999);
    
    void AddTextBox(double xPos, double yPos, std::string text = "", bool userCoord = false, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);
    void AddLegendBox(double xPos, double yPos, std::string title = "", int nColums = 1, int borderStyle = kSolid, int borderSize = 0, int borderColor = kBlack);
    
    void SetAxisTitle(std::string axis, std::string axisTitle);
    
    void SetAxisRange(std::string axis, double low, double high);
    void SetRatioRange(double yLow, double yHigh){}
    
    std::vector<double>& GetAxisRange(std::string axis);
    
    bool IsRatioPlot() const {return !fRatios.empty();}
    
    void ClearCutoffBin(){}
    std::string GetName(){return fName;}
    std::string GetControlString(){return fControlString;}
    std::string GetFigureGroup(){return fFigureGroup;}
    
    void SetDrawingProperties(std::string controlString){fControlString = controlString;}
    void SetFigureGroup(std::string figureGroupName){fFigureGroup = figureGroupName;}
    void SetPlotStyle(std::string plotStyle){fPlotStyle = plotStyle;}
    std::string GetPlotStyle(){return fPlotStyle;}
    
    std::vector<Histogram>& GetHistoTemplates() {return fHistos;}
    std::vector<Histogram>& GetRatioTemplates() {return fRatios;}
    std::string& GetAxisTitle(std::string axis);
    int GetNumSubPlots() {if(IsRatioPlot()) return 2; else return 1;}
    
    std::vector<TextBox>& GetLegends() {return fLegends;}
    std::vector<TextBox>& GetTextBoxes() {return fTexts;}
    
    std::vector<Histogram> fGraphs; // dirty hack for legend errorstyles....
    std::vector<Histogram> fHistos; // dirty hack for legend errorstyles....
    std::vector<Histogram> fRatios;
    
  private:
    Axis& GetAxis(std::string axis);
    
    std::string fName;
    std::string fPlotStyle;
    std::string fFigureGroup;
    
    Axis fXaxis, fYaxis, fZaxis, fYaxisRatio;
    
    std::string fControlString;
    bool fClearCutoffBin;
    
    std::vector<TextBox> fTexts;
    std::vector<TextBox> fLegends;
  };
}
#endif
