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

#include "SciRooPlot/PanelLayout.h"

#include "SciRooPlot/Logging.h"
#include "SciRooPlot/Plot.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

using std::vector;

namespace SciRooPlot
{

Panel::Panel(double_t width, double_t height) : mWidth(width), mHeight(height)
{
  if (!(width > 0.) || !(height > 0.)) {
    logger::throw_invalid_argument("Panel sizes must be positive (got {} x {}).", width, height);
  }
}

Panel& Panel::ZAxis()
{
  mZAxis = true;
  return *this;
}

Panel Gap()
{
  Panel gap;
  gap.mGap = true;
  return gap;
}

Panel Empty()
{
  Panel empty;
  empty.mEmpty = true;
  return empty;
}

PanelLayout::PanelLayout(vector<vector<Panel>> rows) : mRows(std::move(rows)) {}

PanelLayout::PanelLayout(const PanelLayout& sizes, vector<vector<Panel>> rows) : PanelLayout(sizes)
{
  mRows = std::move(rows);
}

PanelLayout& PanelLayout::SetPanelSize(double_t pixel)
{
  mPanelSize = pixel;
  return *this;
}

PanelLayout& PanelLayout::SetMargins(double_t top, double_t bottom, double_t left, double_t right)
{
  mMarginTop = top;
  mMarginBottom = bottom;
  mMarginLeft = left;
  mMarginRight = right;
  return *this;
}

PanelLayout& PanelLayout::SetZAxisMargin(double_t pixel)
{
  mZAxisMargin = pixel;
  return *this;
}

PanelLayout& PanelLayout::SetPanelSpacing(double_t pixel)
{
  mPanelSpacing = pixel;
  return *this;
}

PanelLayout& PanelLayout::SetTextSize(double_t pixel)
{
  mTextSize = pixel;
  return *this;
}

PanelLayout& PanelLayout::SetTitleSize(double_t pixel)
{
  mTitleSize = pixel;
  return *this;
}

PanelLayout& PanelLayout::SetTickLength(double_t pixel)
{
  mTickLength = pixel;
  return *this;
}

PanelLayout& PanelLayout::SetLabelOffset(double_t pixel)
{
  mLabelOffset = pixel;
  return *this;
}

PanelLayout& PanelLayout::SetXTitleOffset(double_t pixel)
{
  mXTitleOffset = pixel;
  return *this;
}

PanelLayout& PanelLayout::SetYTitleOffset(double_t pixel)
{
  mYTitleOffset = pixel;
  return *this;
}

PanelLayout& PanelLayout::SetZTitleOffset(double_t pixel)
{
  mZTitleOffset = pixel;
  return *this;
}

PanelLayout& PanelLayout::SetLineWidth(float_t width)
{
  mLineWidth = width;
  return *this;
}

PanelLayout& PanelLayout::SetMarkerSize(float_t size)
{
  mMarkerSize = size;
  return *this;
}

//**************************************************************************************************
/**
 * Computes the canvas size and the pads of the plot from the arrangement and the lengths of the layout.
 *
 * The lengths of the layout are pixels of the canvas, while ROOT defines most of them relative to the
 * pad (see TAttText::Modify and TGaxis::PaintAxis): text sizes as fractions of the smaller pad side,
 * tick lengths as fractions of the axis length drawn perpendicular to it (i.e. in pixels scaled by the
 * pad aspect ratio), label and title offsets as fractions of the pad size perpendicular to the axis
 * (title offsets additionally in units of 1.6 x title size). All of these are converted per pad from its
 * size in pixels, such that they have the same length in every panel. Line widths and marker sizes are
 * pixels in ROOT already and taken over as they are. All edges are rounded to whole pixels before any
 * fraction is computed, so that neighbouring panels line up exactly.
 */
//**************************************************************************************************
void PanelLayout::ApplyTo(Plot& plot) const
{
  // read the arrangement: panel rows (with the gaps between their panels) and gap rows between them
  vector<vector<const Panel*>> grid;  // [row][column]: panels and empty cells
  vector<bool> gapAfterColumn;        // gap between column c and c + 1 (identical for all rows)
  vector<bool> gapAfterRow;           // gap between row r and r + 1
  bool pendingRowGap = false;
  for (size_t r = 0; r < mRows.size(); ++r) {
    const auto& row = mRows[r];
    if (row.empty()) logger::throw_invalid_argument("Layout: row {} is empty.", r + 1);
    const bool gapRow = std::all_of(row.begin(), row.end(), [](const Panel& cell) { return cell.mGap; });
    if (gapRow) {
      if (grid.empty()) logger::throw_invalid_argument("Layout: the first row must contain panels.");
      if (pendingRowGap) logger::throw_invalid_argument("Layout: two gap rows after row {}.", grid.size());
      pendingRowGap = true;
      continue;
    }
    vector<const Panel*> cells;
    vector<bool> gaps;
    bool lastWasGap = true;
    for (const auto& cell : row) {
      if (cell.mGap) {
        if (lastWasGap) logger::throw_invalid_argument("Layout: row {} has a gap at its beginning or two gaps in a row.", r + 1);
        gaps.back() = true;
      } else {
        cells.push_back(&cell);
        gaps.push_back(false);
      }
      lastWasGap = cell.mGap;
    }
    if (lastWasGap) logger::throw_invalid_argument("Layout: row {} ends with a gap.", r + 1);
    if (grid.empty()) {
      gapAfterColumn = gaps;
    } else {
      if (cells.size() != grid.front().size()) {
        logger::throw_invalid_argument("Layout: row {} has {} cells, the first row has {}.", r + 1, cells.size(), grid.front().size());
      }
      if (gaps != gapAfterColumn) logger::throw_invalid_argument("Layout: the gaps of row {} are not at the same places as in the first row.", r + 1);
      gapAfterRow.push_back(pendingRowGap);
    }
    pendingRowGap = false;
    grid.push_back(cells);
  }
  if (grid.empty()) logger::throw_invalid_argument("Layout: no panels.");
  if (pendingRowGap) logger::throw_invalid_argument("Layout: the last row must contain panels.");
  const size_t nRows = grid.size();
  const size_t nCols = grid.front().size();

  // sizes of the columns and rows in pixels (and whether a column has a colour scale)
  vector<double_t> colWidth(nCols, 0.);
  vector<double_t> rowHeight(nRows, 0.);
  vector<bool> colZAxis(nCols, false);
  for (size_t r = 0; r < nRows; ++r) {
    for (size_t c = 0; c < nCols; ++c) {
      const Panel* cell = grid[r][c];
      if (cell->mEmpty) continue;
      const double_t width = cell->mWidth * mPanelSize;
      const double_t height = cell->mHeight * mPanelSize;
      if (colWidth[c] == 0.) {
        colWidth[c] = width;
      } else if (std::abs(colWidth[c] - width) > 1e-6) {
        logger::throw_invalid_argument("Layout: the panels of column {} have different widths.", c + 1);
      }
      if (rowHeight[r] == 0.) {
        rowHeight[r] = height;
      } else if (std::abs(rowHeight[r] - height) > 1e-6) {
        logger::throw_invalid_argument("Layout: the panels of row {} have different heights.", r + 1);
      }
      if (cell->mZAxis) colZAxis[c] = true;
    }
  }
  for (size_t c = 0; c < nCols; ++c) {
    if (colWidth[c] == 0.) logger::throw_invalid_argument("Layout: column {} contains only empty cells.", c + 1);
  }
  for (size_t r = 0; r < nRows; ++r) {
    if (rowHeight[r] == 0.) logger::throw_invalid_argument("Layout: row {} contains only empty cells.", r + 1);
  }

  // space between neighbouring columns and rows: the part belonging to the first pad, the part belonging to the second one,
  // and whether the two panels share the axis between them
  struct Separation {
    double_t first{};
    double_t second{};
    bool shared{};
  };
  vector<Separation> colSep(nCols > 1 ? nCols - 1 : 0);
  vector<Separation> rowSep(nRows > 1 ? nRows - 1 : 0);
  for (size_t c = 0; c + 1 < nCols; ++c) {
    if (gapAfterColumn[c]) {
      colSep[c] = Separation{mMarginRight, mMarginLeft, false};
    } else if (colZAxis[c]) {
      colSep[c] = Separation{mZAxisMargin, mMarginLeft, false};
    } else {
      colSep[c] = Separation{mPanelSpacing / 2., mPanelSpacing / 2., true};
    }
  }
  for (size_t r = 0; r + 1 < nRows; ++r) {
    rowSep[r] = gapAfterRow[r] ? Separation{mMarginBottom, mMarginTop, false} : Separation{mPanelSpacing / 2., mPanelSpacing / 2., true};
  }

  // edges of the axis frames and boundaries between the pads, rounded to pixels (x from the left, y from the top)
  auto edges = [](double_t lowMargin, const vector<double_t>& sizes, const vector<Separation>& separations, double_t highMargin,
                  vector<double_t>& low, vector<double_t>& high, vector<double_t>& boundary) {
    double_t pos = lowMargin;
    for (size_t i = 0; i < sizes.size(); ++i) {
      low.push_back(std::round(pos));
      pos += sizes[i];
      high.push_back(std::round(pos));
      if (i + 1 < sizes.size()) {
        boundary.push_back(std::round(pos + separations[i].first));
        pos += separations[i].first + separations[i].second;
      }
    }
    return std::round(pos + highMargin);
  };
  vector<double_t> frameLeft, frameRight, boundaryX;
  vector<double_t> frameTop, frameBottom, boundaryY;
  const double_t canvasWidth = edges(mMarginLeft, colWidth, colSep, colZAxis.back() ? mZAxisMargin : mMarginRight, frameLeft, frameRight, boundaryX);
  const double_t canvasHeight = edges(mMarginTop, rowHeight, rowSep, mMarginBottom, frameTop, frameBottom, boundaryY);
  plot.SetDimensions(static_cast<int32_t>(canvasWidth), static_cast<int32_t>(canvasHeight), true);

  // one pad per cell, numbered in reading order
  auto cellAt = [&](size_t r, size_t c) -> const Panel* { return (r < nRows && c < nCols) ? grid[r][c] : nullptr; };
  auto isPanel = [](const Panel* cell) { return cell && !cell->mEmpty; };
  auto isEmpty = [](const Panel* cell) { return cell && cell->mEmpty; };
  const bool singlePanel = (nRows == 1 && nCols == 1);
  uint8_t padID = 1;
  for (size_t r = 0; r < nRows; ++r) {
    for (size_t c = 0; c < nCols; ++c, ++padID) {
      const Panel* cell = grid[r][c];
      const Panel* left = (c > 0) ? cellAt(r, c - 1) : nullptr;
      const Panel* right = cellAt(r, c + 1);
      const Panel* above = (r > 0) ? cellAt(r - 1, c) : nullptr;
      const Panel* below = cellAt(r + 1, c);
      // a pad reaches to the canvas edge on the outside and to the boundary towards its neighbours; next to an empty cell
      // a panel keeps the margin for its labels, and the empty cell gets what the labels of the panels around it do not need
      double_t x0{}, x1{}, y0{}, y1{};
      if (cell->mEmpty) {
        x0 = (c == 0) ? 0. : (isPanel(left) ? frameRight[c - 1] + mMarginRight : boundaryX[c - 1]);
        x1 = (c + 1 == nCols) ? canvasWidth : (isPanel(right) ? frameLeft[c + 1] - mMarginLeft : boundaryX[c]);
        y0 = (r == 0) ? 0. : (isPanel(above) ? frameBottom[r - 1] + mMarginBottom : boundaryY[r - 1]);
        y1 = (r + 1 == nRows) ? canvasHeight : (isPanel(below) ? frameTop[r + 1] - mMarginTop : boundaryY[r]);
      } else {
        x0 = (c == 0) ? 0. : (isEmpty(left) ? frameLeft[c] - mMarginLeft : boundaryX[c - 1]);
        x1 = (c + 1 == nCols) ? canvasWidth : (isEmpty(right) ? frameRight[c] + mMarginRight : boundaryX[c]);
        y0 = (r == 0) ? 0. : (isEmpty(above) ? frameTop[r] - mMarginTop : boundaryY[r - 1]);
        y1 = (r + 1 == nRows) ? canvasHeight : (isEmpty(below) ? frameBottom[r] + mMarginBottom : boundaryY[r]);
      }
      x0 = std::round(x0);
      x1 = std::round(x1);
      y0 = std::round(y0);
      y1 = std::round(y1);
      Plot::Pad& pad = plot[padID];
      if (cell->mEmpty) {
        if (x1 <= x0 || y1 <= y0) {
          WARNING("Layout: the empty cell in row {}, column {} leaves no space for pad {}.", r + 1, c + 1, padID);
          continue;
        }
        pad.SetPosition(x0 / canvasWidth, 1. - y1 / canvasHeight, x1 / canvasWidth, 1. - y0 / canvasHeight);
        pad.SetDefaultTextSize(mTextSize / std::min(x1 - x0, y1 - y0));
        continue;
      }
      pad.SetPosition(x0 / canvasWidth, 1. - y1 / canvasHeight, x1 / canvasWidth, 1. - y0 / canvasHeight);

      // the sizes of a single panel become the defaults of the plot (pad 0), such that they also apply to pads added by the user
      Plot::Pad& target = singlePanel ? plot[0] : pad;
      const double_t width = x1 - x0;
      const double_t height = y1 - y0;
      const double_t minSide = std::min(width, height);
      const double_t frameWidth = frameRight[c] - frameLeft[c];
      const double_t frameHeight = frameBottom[r] - frameTop[r];
      target.SetMargins((frameTop[r] - y0) / height, (y1 - frameBottom[r]) / height, (frameLeft[c] - x0) / width, (x1 - frameRight[c]) / width);
      target.SetDefaultTextSize(mTextSize / minSide);
      target['X']
        .SetTitleSize(mTitleSize / minSide)
        .SetTickLength(mTickLength * width / (frameWidth * height))
        .SetLabelOffset(mLabelOffset / height)
        .SetTitleOffset(mXTitleOffset * minSide / (1.6 * mTitleSize * height));
      target['Y']
        .SetTitleSize(mTitleSize / minSide)
        .SetTickLength(mTickLength * height / (frameHeight * width))
        .SetLabelOffset(mLabelOffset / width)
        .SetTitleOffset((mYTitleOffset > 0.) ? mYTitleOffset * minSide / (1.6 * mTitleSize * width) : 0.);  // 0: automatic placement (see PlotPainter)
      if (colZAxis[c]) {
        target['Z'].SetTitleSize(mTitleSize / minSide).SetTitleOffset(mZTitleOffset * minSide / (1.6 * mTitleSize * width));
      }
      // axes shared with a touching panel: labels and title are only shown once
      if (isPanel(left) && colSep[c - 1].shared) pad['Y'].SetTitleSize(0.).SetLabelSize(0.);
      if (isPanel(below) && rowSep[r].shared) pad['X'].SetTitleSize(0.).SetLabelSize(0.);
    }
  }
  plot[0].SetDefaultLineWidth(mLineWidth);
  plot[0].SetDefaultMarkerSize(mMarkerSize);
}

}  // namespace SciRooPlot
