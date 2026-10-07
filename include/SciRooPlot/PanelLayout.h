/*
 ******************************************************************************************
 * --------------------------------------- SciRooPlot -------------------------------------
 * Copyright (c) 2019-2026 Mario Krüger
 * Contact: mario.kruger@cern.ch
 * For a full list of contributors please see doc/CONTRIBUTORS.md.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation in version 3 (or later) of the License.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * The GNU General Public License can be found here: <https://www.gnu.org/licenses/>.
 ******************************************************************************************
 */

#ifndef INCLUDE_SCIROOPLOT_PANELLAYOUT_H_
#define INCLUDE_SCIROOPLOT_PANELLAYOUT_H_

#include <cmath>
#include <vector>

namespace SciRooPlot
{
class Plot;

//**************************************************************************************************
/**
 * One axis frame of a layout. Sizes are given in units of the standard panel (see Layout::SetPanelSize).
 */
//**************************************************************************************************
class Panel
{
 public:
  explicit Panel(double_t width = 1., double_t height = 1.);
  Panel& ZAxis();  // room for the colour scale to the right of this panel

 private:
  friend class PanelLayout;
  friend Panel Gap();
  friend Panel Empty();
  double_t mWidth{1.};
  double_t mHeight{1.};
  bool mZAxis{false};
  bool mGap{false};
  bool mEmpty{false};
};

// Separates independent panels: both sides keep their axis labels and titles.
// Between the panels of a row it separates columns, a row containing only a Gap() separates two rows.
Panel Gap();

// A cell of the grid without axis frame: it keeps the size of its row and column, the neighbouring panels
// show their axes towards it and the remaining space becomes a pad (numbered like the panels) for texts.
Panel Empty();

//**************************************************************************************************
/**
 * Arrangement of panels (rows from top to bottom, panels from left to right) and the lengths of the
 * standard plot. All lengths are pixels of the plot at scale 1 (see srp screenscale and srp bitmapscale);
 * a plot made from a layout has them in every panel, so that plots of different layouts look the same.
 * Panels that touch share the axis between them and their inner labels and titles are hidden.
 * The pads of the resulting plot are numbered in reading order, gaps have no pad.
 * Lengths are floating-point pixels of the canvas (the unit of ROOT's line widths and pixel fonts), the defaults are
 * the proportions of the standard 788 x 788 pixel plot: text 0.04 x 788 = 31.52 pixel, axis titles 0.05 x 788 = 39.4, ...
 * Note that ROOT renders text with whole-pixel font sizes (0.934 x size, rounded), so text sizes that differ by less
 * than a pixel can render identically or jump by one pixel.
 */
//**************************************************************************************************
class PanelLayout
{
 public:
  explicit PanelLayout(std::vector<std::vector<Panel>> rows = {{Panel()}});
  PanelLayout(const PanelLayout& sizes, std::vector<std::vector<Panel>> rows);  // same lengths, other panels

  PanelLayout& SetPanelSize(double_t pixel);                                              // 638: axis frame of Panel(1, 1)
  PanelLayout& SetMargins(double_t top, double_t bottom, double_t left, double_t right);  // 32, 118, 118, 32: space around the panels (bottom and left hold the labels and titles)
  PanelLayout& SetZAxisMargin(double_t pixel);                                            // 268: right margin of a panel with colour scale
  PanelLayout& SetPanelSpacing(double_t pixel);                                           // 0: space between touching panels (their axes stay shared)
  PanelLayout& SetTextSize(double_t pixel);                                               // 31.52: axis labels, legends and texts
  PanelLayout& SetTitleSize(double_t pixel);                                              // 39.4: axis titles
  PanelLayout& SetTickLength(double_t pixel);                                             // 19.15
  PanelLayout& SetLabelOffset(double_t pixel);                                            // 3.94: distance of the labels from the axis
  PanelLayout& SetXTitleOffset(double_t pixel);                                           // 63.04: distance of the x title from the axis
  PanelLayout& SetYTitleOffset(double_t pixel);                                           // 0: automatic, next to the widest label (otherwise the distance from the axis)
  PanelLayout& SetZTitleOffset(double_t pixel);                                           // 122.9: distance of the z title from the axis
  PanelLayout& SetLineWidth(float_t width);                                               // 5 (ROOT line width in pixel)
  PanelLayout& SetMarkerSize(float_t size);                                               // 1.4 (ROOT marker size, 1 = 8 pixel)

 private:
  friend class Plot;
  void ApplyTo(Plot& plot) const;

  std::vector<std::vector<Panel>> mRows;
  double_t mPanelSize{638.};
  double_t mMarginTop{32.};
  double_t mMarginBottom{118.};
  double_t mMarginLeft{118.};
  double_t mMarginRight{32.};
  double_t mZAxisMargin{268.};
  double_t mPanelSpacing{0.};
  double_t mTextSize{31.52};
  double_t mTitleSize{39.4};
  double_t mTickLength{19.15};
  double_t mLabelOffset{3.94};
  double_t mXTitleOffset{63.04};
  double_t mYTitleOffset{0.};
  double_t mZTitleOffset{122.9};
  float_t mLineWidth{5.};
  float_t mMarkerSize{1.4};
};

}  // namespace SciRooPlot
#endif  // INCLUDE_SCIROOPLOT_PANELLAYOUT_H_
