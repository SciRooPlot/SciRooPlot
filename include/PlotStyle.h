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

#ifndef PlotStyle_h
#define PlotStyle_h

#include "PlottingFramework.h"

using namespace PlottingFramework;
namespace PlottingFramework {

//****************************************************************************************
/**
 * Class defining the layout of a plot.
 */
//****************************************************************************************
class PlotStyle
{
  // todo add default intialization values!!
  // todo a plot is a pad containing other pads, member pads by default use all pad propertys of plot unless overridden
public:
  // for specific pad as well would be nice SetPadProperties(int iPad, string properties)
  // options that can be added to plotStyle: grids?, X+ (draw x axis on top) and Y+
  class PadStyle;
  PlotStyle(string name) {mName = name;}
  string& GetName(){return mName;}
  double& GetWidth(){return mWidth;}
  double& GetHeight(){return mHeight;}
  int GetNPads() {return mPadStyles.size();};
  vector<PadStyle>& GetPadStyles() {return mPadStyles;}
  void AddPadStyles(vector<PadStyle> padStyles) {mPadStyles = padStyles;}

  int GetPallette() {return mPalette;}
  int GetDefaultColor(int colorIndex) {return mDefaultColors[colorIndex % mDefaultColors.size()];}
  int GetDefaultMarker(int markerIndex) {return mDefaultMarkers[markerIndex % mDefaultMarkers.size()];}
  int GetDefaultMarkerFull(int markerIndex) {return mDefaultMarkersFull[markerIndex % mDefaultMarkersFull.size()];}
  int GetDefaultMarkerOpen(int markerIndex) {return mDefaultMarkersOpen[markerIndex % mDefaultMarkersOpen.size()];}
  int GetDefaultLineStyle(int lineIndex) {return mDefaultLineStyles[lineIndex % mDefaultLineStyles.size()];}
  string GetDefault2DStyle(){return m2dStyle;}

  void SetPalette(int palette){mPalette = palette;} // TODO: per pad, accessor through plot gStyle->SetPalette(palette);
  void SetTextSize(double size){mTextSize = size;}
  void SetLableSizeScaling(double scaleFactor){mLableSizeScaling = scaleFactor;}
  void SetTitleSizeScaling(double scaleFactor){mTitleSizeScaling = scaleFactor;}
  void SetTextFont(int font){(font > 0 && font < 16) ? mTextFont = font * 10 + 3 : mTextFont = 43;}
  void SetLableFont(int font){(font > 0 && font < 16) ? mLableFont = font * 10 + 3 : mLableFont = 43;}
  void SetTitleFont(int font){(font > 0 && font < 16) ? mTitleFont = font * 10 + 3 : mTitleFont = 43;}
  void SetBackgroundColor(int color) {SetTransparent(false); mFillColor = color;}

  void SetMarkerSize(double size){mMarkerSize = size;}
  void SetLineWidth(double width){mLineWidth = width;}
  void SetMarkerSizeThick(double sizeThick){mMarkerSizeThick = sizeThick;}
  void SetLineWidthThick(double widthThick){mLineWidthThick = widthThick;}

  void SetDefaultColors(vector<int> colors){mDefaultColors = colors;}
  void SetDefaultMarkers(vector<int> markers){mDefaultMarkers = markers;}
  void SetDefaultMarkersFull(vector<int> markers){mDefaultMarkersFull = markers;}
  void SetDefaultMarkersOpen(vector<int> markers){mDefaultMarkersOpen = markers;}
  void SetDefaultLineStyles(vector<int> lineStyles){mDefaultLineStyles = lineStyles;}
  void SetDefault2DStyle(string style = "COLZ"){m2dStyle = style;}

  
  void SetWidth(int width = 710){mWidth = width;}
  void SetHeight(int height = 710){mHeight = height;}
  void SetFromat(int height, double aspectRatio){mHeight = height; mWidth = height * aspectRatio;}
  
  void SetTransparent(bool transparent = true)
  {
    if(transparent) mFillStyle = 4000;
    else mFillStyle = 4100;
  }
  
  int GetFillStyle(){return mFillStyle;}

  void SetDrawTimestamps(bool drawTimestamps = true) {mDrawTimestamps = drawTimestamps;}
  void SetTimestampPosition(double x, double y) {mTimestampPosition = {x, y};}
  bool IsDrawTimestamps(){return mDrawTimestamps;}
  pair<double, double> GetTimestampPositon(){return mTimestampPosition;}

  void LinkAxes(string axis, vector<int> padIDs)
  {
    mLinkedAxes[axis].push_back(padIDs);
  }
  
  vector<int> GetLinkedPads(string axis, int padID){
    set<int> linkedPads = {padID};
    for(auto& pads : mLinkedAxes[axis]){
      if(std::find(pads.begin(), pads.end(), padID) != pads.end()) std::copy(pads.begin(), pads.end(), std::inserter(linkedPads, linkedPads.end()));
    }
    vector<int> target(linkedPads.begin(), linkedPads.end());
    return target;
  }

  int GetTextSize(){return mTextSize;}
  int GetLableSize(){return mTextSize*mLableSizeScaling;}
  int GetTitleSize(){return mTextSize*mTitleSizeScaling;}

  int GetTextFont(){return mTextFont;}
  int GetLableFont(){return mLableFont;}
  int GetTitleFont(){return mTitleFont;}
  int GetMarkerSize(){return mMarkerSize;}
  int GetMarkerSizeThick(){return mMarkerSizeThick;}
  int GetLineWidthThick(){return mLineWidthThick;}
  void SetFixedAspectRatio(bool isFixedAspectRatio = true){mIsFixedAspectRatio = isFixedAspectRatio;}
  bool IsFixedAspectRatio(){return mIsFixedAspectRatio;}

private:
  string mName;
  double mWidth;
  double mHeight;
  vector<PadStyle> mPadStyles;
  vector<int> mRatioPads;
  map<string, vector<vector<int>>> mLinkedAxes;
  // ratio corresponding to padID plots
  
  bool mDrawTimestamps;
  pair<double, double> mTimestampPosition; // x, y

  
  // maybe move this to a base class
  int mFillStyle;
  int mFillColor;
  
  int mTextFont;
  int mLableFont;
  int mTitleFont;

  double mTextSize;
  double mLableSizeScaling;
  double mTitleSizeScaling;

  double mMarkerSize;
  double mLineWidth;

  double mMarkerSizeThick;
  double mLineWidthThick;
  
  int mPalette;
  vector<int> mDefaultColors;
  vector<int> mDefaultMarkers;
  vector<int> mDefaultMarkersFull;
  vector<int> mDefaultMarkersOpen;
  vector<int> mDefaultLineStyles;
  string m2dStyle;
  bool mIsFixedAspectRatio;
};


//****************************************************************************************
/**
 * Class defining the layout of a pad.
 */
//****************************************************************************************
class PlotStyle::PadStyle{
public:
  PadStyle(){}
  PadStyle(string title) : mTitle(title) {}
  // user accessors
  void SetCorners(pair<double, double> lowerLeftCorner, pair<double, double> upperRightCorner){mCorners = {lowerLeftCorner, upperRightCorner};}
  
  void SetMargins(array<double, 4> margins) {mMargins = margins;}
  void SetTopMargin(double margin) {mMargins[0] = margin;}
  void SetBottomMargin(double margin) {mMargins[1] = margin;}
  void SetLeftMargin(double margin) {mMargins[2] = margin;}
  void SetRightMargin(double margin) {mMargins[3] = margin;}

  void SetDrawingOptions(string drawingOptions) {mDrawingOptions = drawingOptions;}

  void SetTitleOffsets(array<double, 3> offsets) {mTitleOffsets = offsets;}
  void SetTitleOffsetX(double offset) {mTitleOffsets[0] = offset;}
  void SetTitleOffsetY(double offset) {mTitleOffsets[1] = offset;}
  void SetTitleOffsetZ(double offset) {mTitleOffsets[2] = offset;}
  
  // convert to root info
  double& GetXLow(){return mCorners.first.first;}
  double& GetYLow(){return mCorners.first.second;}
  double& GetXUp(){return mCorners.second.first;}
  double& GetYUp(){return mCorners.second.second;}
  
  double& GetTopMargin(){return mMargins[0];}
  double& GetBottomMargin(){return mMargins[1];}
  double& GetLeftMargin(){return mMargins[2];}
  double& GetRightMargin(){return mMargins[3];}
  
  double& GetTitleOffsetX(){return mTitleOffsets[0];}
  double& GetTitleOffsetY(){return mTitleOffsets[1];}
  double& GetTitleOffsetZ(){return mTitleOffsets[2];}
  
  string GetTitle() {return mTitle;}
  
private:
  string mTitle;
  pair<pair<double, double>, pair<double, double>> mCorners;  // lowerLeftCorner{xlow, ylow}, upperRightCorner{xup, yup}
  array<double, 4> mMargins; // top, bottom, left, right
  array<double, 3> mTitleOffsets; // x, y, z
  string mDrawingOptions; // eg. X+, Y+ for axis on top or right, gridX, gridY, logX, logY
};

} // end namespace PlottingFramework
#endif /* PlotStyle_h */
