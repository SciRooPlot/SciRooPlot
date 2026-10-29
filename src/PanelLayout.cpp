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

#include "SciRooPlot/Logger.h"
#include "SciRooPlot/Plot.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
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

Panel& Panel::Span(uint8_t columns, uint8_t rows)
{
  if (columns == 0 || rows == 0) {
    logger::throw_invalid_argument("A panel must span at least one column and one row (got {} x {}).", columns, rows);
  }
  mColumns = columns;
  mRows = rows;
  return *this;
}

Panel Gap()
{
  Panel gap;
  gap.mGap = true;
  gap.mSized = false;
  return gap;
}

Panel Empty()
{
  Panel empty;
  empty.mEmpty = true;
  empty.mSized = false;
  return empty;
}

Panel Empty(double_t width, double_t height)
{
  Panel empty(width, height);
  empty.mEmpty = true;
  return empty;
}

Panel ZAxis()
{
  Panel cell;
  cell.mScaleCell = true;
  cell.mSized = false;
  return cell;
}

Panel ZAxis(uint8_t padID)
{
  if (padID == 0) logger::throw_invalid_argument("ZAxis(padID): the pads are counted from 1.");
  Panel cell = ZAxis();
  cell.mScaleOf = padID;
  return cell;
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

PanelLayout& PanelLayout::SetZAxisOffset(double_t pixel)
{
  mZAxisOffset = pixel;
  return *this;
}

PanelLayout& PanelLayout::SetZAxisWidth(double_t pixel)
{
  mZAxisWidth = pixel;
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

PanelLayout& PanelLayout::SetCommonTitles(bool common)
{
  mCommonTitles = common;
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
 * size in pixels, such that they have the same length in every panel (also the distance and the width of the colour
 * scale, fractions of the pad width in ROOT). Line widths and marker sizes are
 * pixels in ROOT already and taken over as they are. All edges are rounded to whole pixels before any
 * fraction is computed, so that neighbouring panels line up exactly.
 */
//**************************************************************************************************
void PanelLayout::ApplyTo(Plot& plot) const
{
  // read the arrangement into a grid: every listed cell (one pad) covers a block of grid positions, starting at the first
  // position of its row that no cell spanning down from a row above covers; rows containing only gaps separate grid rows
  struct Cell {
    const Panel* panel;
    size_t row;  // first row and column of its block
    size_t col;
  };
  vector<Cell> cells;             // in reading order: cell i is pad i + 1
  vector<vector<int32_t>> owner;  // [row][column]: the cell covering the position (-1: free)
  vector<int8_t> gapAfterColumn;  // gap between column c and c + 1: 1 or 0, -1 while only spanned over
  vector<bool> gapAfterRow;       // gap between row r and r + 1
  size_t nRows = 0;
  size_t nCols = 0;
  bool pendingRowGap = false;
  for (size_t r = 0; r < mRows.size(); ++r) {
    const auto& row = mRows[r];
    if (row.empty()) logger::throw_invalid_argument("Layout: row {} is empty.", r + 1);
    const bool gapRow = std::all_of(row.begin(), row.end(), [](const Panel& cell) { return cell.mGap; });
    if (gapRow) {
      if (nRows == 0) logger::throw_invalid_argument("Layout: the first row must contain panels.");
      if (pendingRowGap) logger::throw_invalid_argument("Layout: two gap rows after row {}.", nRows);
      pendingRowGap = true;
      continue;
    }
    const size_t gridRow = nRows++;
    if (gridRow > 0) gapAfterRow.push_back(pendingRowGap);
    pendingRowGap = false;
    if (owner.size() <= gridRow) owner.emplace_back();
    auto isFree = [&](size_t col) { return col >= owner[gridRow].size() || owner[gridRow][col] < 0; };
    vector<size_t> gaps;  // columns after which this row lists a gap
    size_t col = 0;
    bool lastWasGap = true;
    for (const auto& cell : row) {
      if (cell.mGap) {
        if (lastWasGap) logger::throw_invalid_argument("Layout: row {} has a gap at its beginning or two gaps in a row.", r + 1);
        gaps.push_back(col - 1);
        lastWasGap = true;
        continue;
      }
      lastWasGap = false;
      while (!isFree(col))
        ++col;
      const auto index = static_cast<int32_t>(cells.size());
      cells.push_back({&cell, gridRow, col});
      for (size_t rr = gridRow; rr < gridRow + cell.mRows; ++rr) {
        if (owner.size() <= rr) owner.emplace_back();
        if (owner[rr].size() < col + cell.mColumns) owner[rr].resize(col + cell.mColumns, -1);
        for (size_t cc = col; cc < col + cell.mColumns; ++cc) {
          if (owner[rr][cc] >= 0) logger::throw_invalid_argument("Layout: a cell of row {} overlaps a cell spanning down from a row above.", r + 1);
          owner[rr][cc] = index;
        }
      }
      col += cell.mColumns;
    }
    if (gridRow == 0) {
      nCols = owner[0].size();
      gapAfterColumn.assign(nCols - 1, -1);
    }
    auto& positions = owner[gridRow];
    const auto covered = std::count_if(positions.begin(), positions.end(), [](int32_t index) { return index >= 0; });
    if (positions.size() > nCols || static_cast<size_t>(covered) != nCols) {
      logger::throw_invalid_argument("Layout: row {} covers {} columns, the first row {}.", r + 1, std::max(positions.size(), static_cast<size_t>(covered)), nCols);
    }
    if (!gaps.empty() && gaps.back() + 1 >= nCols) logger::throw_invalid_argument("Layout: row {} ends with a gap.", r + 1);
    // the gaps must be at the same places in all rows; a row tells for the boundaries right of the cells it lists
    for (size_t c = 0; c + 1 < nCols; ++c) {
      if (positions[c] == positions[c + 1] || cells[positions[c]].row != gridRow) continue;
      const int8_t gap = std::find(gaps.begin(), gaps.end(), c) != gaps.end() ? 1 : 0;
      if (gapAfterColumn[c] >= 0 && gapAfterColumn[c] != gap) {
        logger::throw_invalid_argument("Layout: the gaps of row {} are not at the same places as in the rows above.", r + 1);
      }
      gapAfterColumn[c] = gap;
    }
  }
  if (nRows == 0) logger::throw_invalid_argument("Layout: no panels.");
  if (pendingRowGap) logger::throw_invalid_argument("Layout: the last row must contain panels.");
  if (owner.size() > nRows) logger::throw_invalid_argument("Layout: a cell spans below the last row.");
  if (cells.size() > std::numeric_limits<uint8_t>::max()) {
    logger::throw_invalid_argument("Layout: {} cells would exceed the maximum of {} pads.", cells.size(), std::numeric_limits<uint8_t>::max());
  }

  // sizes of the columns and rows in pixels, from the cells that cover a single one (and whether a column has a colour scale,
  // of its panels or as ZAxis() cells: these have the width of the room of a colour scale)
  vector<double_t> colWidth(nCols, 0.);
  vector<double_t> rowHeight(nRows, 0.);
  vector<bool> colZAxis(nCols, false);
  vector<bool> colScale(nCols, false);
  for (const auto& [cell, r, c] : cells) {
    if (cell->mScaleCell) {
      if (cell->mColumns > 1) logger::throw_invalid_argument("Layout: a ZAxis() cell spans one column (row {}, column {}).", r + 1, c + 1);
      colScale[c] = true;
      colWidth[c] = mZAxisMargin;
    }
  }
  for (const auto& [cell, r, c] : cells) {
    if (cell->mZAxis) colZAxis[c + cell->mColumns - 1] = true;
    if (!cell->mEmpty && !cell->mScaleCell) {
      for (size_t cc = c; cc < c + cell->mColumns; ++cc) {
        if (colScale[cc]) logger::throw_invalid_argument("Layout: column {} holds a ZAxis() cell and a panel.", cc + 1);
      }
    }
    if (!cell->mSized) continue;
    if (cell->mColumns == 1) {
      const double_t width = cell->mWidth * mPanelSize;
      if (colWidth[c] == 0.) {
        colWidth[c] = width;
      } else if (std::abs(colWidth[c] - width) > 1e-6) {
        logger::throw_invalid_argument("Layout: the panels of column {} have different widths.", c + 1);
      }
    }
    if (cell->mRows == 1) {
      const double_t height = cell->mHeight * mPanelSize;
      if (rowHeight[r] == 0.) {
        rowHeight[r] = height;
      } else if (std::abs(rowHeight[r] - height) > 1e-6) {
        logger::throw_invalid_argument("Layout: the panels of row {} have different heights.", r + 1);
      }
    }
  }
  for (size_t c = 0; c < nCols; ++c) {
    if (colWidth[c] == 0.) logger::throw_invalid_argument("Layout: column {} has no width: it holds only empty cells without size or cells spanning further columns.", c + 1);
  }
  for (size_t r = 0; r < nRows; ++r) {
    if (rowHeight[r] == 0.) logger::throw_invalid_argument("Layout: row {} has no height: it holds only empty cells without size or cells spanning further rows.", r + 1);
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
    if (gapAfterColumn[c] == 1) {
      colSep[c] = Separation{mMarginRight, mMarginLeft, false};
    } else if (colScale[c + 1]) {
      colSep[c] = Separation{0., 0., false};  // the cell of a colour scale holds its room: it starts at the frames on its left
    } else if (colScale[c]) {
      colSep[c] = Separation{0., mMarginLeft, false};
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
  const double_t canvasWidth = edges(mMarginLeft, colWidth, colSep, colScale.back() ? 0. : (colZAxis.back() ? mZAxisMargin : mMarginRight), frameLeft, frameRight, boundaryX);
  const double_t canvasHeight = edges(mMarginTop, rowHeight, rowSep, mMarginBottom, frameTop, frameBottom, boundaryY);
  plot.SetDimensions(static_cast<int32_t>(canvasWidth), static_cast<int32_t>(canvasHeight), true);

  // one pad per cell, numbered in reading order
  struct Neighbours {
    bool panel{};  // a panel touches this side
    bool empty{};  // an empty cell touches this side
    bool scale{};  // a ZAxis() cell touches this side
  };
  auto neighbours = [&](size_t r0, size_t r1, size_t c0, size_t c1) {  // the cells covering the given block of positions
    Neighbours side;
    for (size_t r = r0; r <= r1; ++r) {
      for (size_t c = c0; c <= c1; ++c) {
        const Panel* other = cells[owner[r][c]].panel;
        (other->mScaleCell ? side.scale : (other->mEmpty ? side.empty : side.panel)) = true;
      }
    }
    return side;
  };
  auto rightMargin = [&](size_t col) { return colZAxis[col] ? mZAxisMargin : mMarginRight; };  // room right of a frame for labels or the colour scale
  const bool singlePanel = (nRows == 1 && nCols == 1);
  for (size_t i = 0; i < cells.size(); ++i) {
    const auto& [cell, r0, c0] = cells[i];
    const auto padID = static_cast<uint8_t>(i + 1);
    const size_t r1 = r0 + cell->mRows - 1;
    const size_t c1 = c0 + cell->mColumns - 1;
    const Neighbours left = (c0 > 0) ? neighbours(r0, r1, c0 - 1, c0 - 1) : Neighbours{};
    const Neighbours right = (c1 + 1 < nCols) ? neighbours(r0, r1, c1 + 1, c1 + 1) : Neighbours{};
    const Neighbours above = (r0 > 0) ? neighbours(r0 - 1, r0 - 1, c0, c1) : Neighbours{};
    const Neighbours below = (r1 + 1 < nRows) ? neighbours(r1 + 1, r1 + 1, c0, c1) : Neighbours{};
    // a pad reaches to the canvas edge on the outside and to the boundary towards its neighbours; next to empty cells only
    // a panel keeps the margin for its labels, and an empty cell gets what the labels of the panels around it do not need
    double_t x0{}, x1{}, y0{}, y1{};
    if (cell->mScaleCell) {
      x0 = frameLeft[c0];
      x1 = frameRight[c1];
      y0 = (r0 == 0) ? 0. : (!above.panel ? frameTop[r0] - mMarginTop : boundaryY[r0 - 1]);
      y1 = (r1 + 1 == nRows) ? canvasHeight : (!below.panel ? frameBottom[r1] + mMarginBottom : boundaryY[r1]);
    } else if (cell->mEmpty) {
      x0 = (c0 == 0) ? 0. : (left.panel ? frameRight[c0 - 1] + rightMargin(c0 - 1) : boundaryX[c0 - 1]);
      x1 = (c1 + 1 == nCols) ? canvasWidth : (right.panel ? frameLeft[c1 + 1] - mMarginLeft : boundaryX[c1]);
      y0 = (r0 == 0) ? 0. : (above.panel ? frameBottom[r0 - 1] + mMarginBottom : boundaryY[r0 - 1]);
      y1 = (r1 + 1 == nRows) ? canvasHeight : (below.panel ? frameTop[r1 + 1] - mMarginTop : boundaryY[r1]);
    } else {
      x0 = (c0 == 0) ? 0. : (!left.panel && !left.scale ? frameLeft[c0] - mMarginLeft : boundaryX[c0 - 1]);
      x1 = (c1 + 1 == nCols) ? canvasWidth : (!right.panel && !right.scale ? frameRight[c1] + rightMargin(c1) : boundaryX[c1]);
      y0 = (r0 == 0) ? 0. : (!above.panel ? frameTop[r0] - mMarginTop : boundaryY[r0 - 1]);
      y1 = (r1 + 1 == nRows) ? canvasHeight : (!below.panel ? frameBottom[r1] + mMarginBottom : boundaryY[r1]);
    }
    x0 = std::round(x0);
    x1 = std::round(x1);
    y0 = std::round(y0);
    y1 = std::round(y1);
    if (cell->mEmpty && (x1 <= x0 || y1 <= y0)) {
      // no pad is created, otherwise it would be drawn over the whole plot for lack of a position
      logger::warning("Layout: the empty cell in row {}, column {} leaves no space for pad {}.", r0 + 1, c0 + 1, padID);
      continue;
    }
    Plot::Pad& pad = plot[padID];
    pad.SetPosition(x0 / canvasWidth, 1. - y1 / canvasHeight, x1 / canvasWidth, 1. - y0 / canvasHeight);
    if (cell->mScaleCell) {
      // the scale is drawn along the frames it belongs to (the vertical margins) at the distance of the scale of a panel from its frame
      const double_t width = x1 - x0;
      const double_t height = y1 - y0;
      const double_t minSide = std::min(width, height);
      uint8_t scaleOf = cell->mScaleOf;
      if (scaleOf == 0 && c0 > 0) {
        const auto leftCell = static_cast<size_t>(owner[r0][c0 - 1]);
        if (!cells[leftCell].panel->mEmpty && !cells[leftCell].panel->mScaleCell) scaleOf = static_cast<uint8_t>(leftCell + 1);
      }
      pad.mColorScaleOf = scaleOf;  // 0: no panel to show (warned when drawn)
      pad.SetMargins((frameTop[r0] - y0) / height, (y1 - frameBottom[r1]) / height, 0.f, 0.f);
      pad.SetDefaultTextSize(mTextSize / minSide);
      pad['Z']
        .SetTitleSize(mTitleSize / minSide)
        .SetTitleOffset(mZTitleOffset * minSide / (1.6 * mTitleSize * width))
        .SetLabelOffset(0.005 * (mMarginLeft + mPanelSize + mZAxisMargin) / width)  // ROOT's 0.005 of the standard pad with a colour scale
        .SetColorScaleOffset(static_cast<float_t>(mZAxisOffset / width))
        .SetColorScaleWidth(static_cast<float_t>(mZAxisWidth / width));
      continue;
    }
    if (cell->mEmpty) {
      pad.mIsEmptyCell = true;
      pad.SetDefaultTextSize(mTextSize / std::min(x1 - x0, y1 - y0));
      continue;
    }

    // the sizes of a single panel become the defaults of the plot (pad 0), such that they also apply to pads added by the user
    Plot::Pad& target = singlePanel ? plot[0] : pad;
    const double_t width = x1 - x0;
    const double_t height = y1 - y0;
    const double_t minSide = std::min(width, height);
    const double_t frameWidth = frameRight[c1] - frameLeft[c0];
    const double_t frameHeight = frameBottom[r1] - frameTop[r0];
    target.SetMargins((frameTop[r0] - y0) / height, (y1 - frameBottom[r1]) / height, (frameLeft[c0] - x0) / width, (x1 - frameRight[c1]) / width);
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
    if (colZAxis[c1]) {
      target['Z']
        .SetTitleSize(mTitleSize / minSide)
        .SetTitleOffset(mZTitleOffset * minSide / (1.6 * mTitleSize * width))
        .SetColorScaleOffset(static_cast<float_t>(mZAxisOffset / width))
        .SetColorScaleWidth(static_cast<float_t>(mZAxisWidth / width));
    }
    // axes shared with a touching panel: labels and title are only shown once (a label next to a panel would run into its frame)
    if (left.panel && colSep[c0 - 1].shared) pad['Y'].SetTitleSize(0.).SetLabelSize(0.);
    if (below.panel && rowSep[r1].shared) pad['X'].SetTitleSize(0.).SetLabelSize(0.);
    // touching panels of another length share no axis (see below), but the labels between them are hidden all the same
    auto otherLength = [&](size_t r, size_t c, bool sameRows) {
      const auto& [other, rr, cc] = cells[owner[r][c]];
      return !other->mEmpty && !other->mScaleCell && (sameRows ? (rr != r0 || other->mRows != cell->mRows) : (cc != c0 || other->mColumns != cell->mColumns));
    };
    bool unsharedLeft{}, unsharedBelow{};
    for (size_t r = r0; left.panel && colSep[c0 - 1].shared && r <= r1; ++r)
      unsharedLeft = unsharedLeft || otherLength(r, c0 - 1, true);
    for (size_t c = c0; below.panel && rowSep[r1].shared && c <= c1; ++c)
      unsharedBelow = unsharedBelow || otherLength(r1 + 1, c, false);
    if (unsharedLeft || unsharedBelow) {
      logger::warning("Layout: pad {} touches a panel of another {} {}: their {} axes cannot be shared, yet the labels between them are hidden. Separate them with a Gap().",
                      padID, unsharedBelow ? "width" : "height", unsharedBelow ? "below it" : "left of it", unsharedBelow ? "x" : "y");
    }
  }
  plot[0].SetDefaultLineWidth(mLineWidth);
  plot[0].SetDefaultMarkerSize(mMarkerSize);

  // touching panels share their axes: x within a column (panels touching above or below), y within a row (touching left or right);
  // and the outer title of a row (x) or column (y) of touching panels is drawn once: groups of the panels showing that title
  // (the x title of a panel without a touching panel below, the y title of one without a touching panel on its left)
  auto isPanel = [&](int32_t index) { return !cells[index].panel->mEmpty && !cells[index].panel->mScaleCell; };
  auto groupsOf = [&](auto&& joined) {  // the connected panels, joined(a, b) for neighbouring cells a and b
    vector<size_t> root(cells.size());
    for (size_t i = 0; i < root.size(); ++i)
      root[i] = i;
    auto find = [&](size_t i) {
      while (root[i] != i)
        i = root[i] = root[root[i]];
      return i;
    };
    for (size_t r = 0; r < nRows; ++r) {
      for (size_t c = 0; c < nCols; ++c) {
        const int32_t a = owner[r][c];
        if (r + 1 < nRows && owner[r + 1][c] != a && isPanel(a) && isPanel(owner[r + 1][c]) && joined(a, owner[r + 1][c], false)) root[find(a)] = find(owner[r + 1][c]);
        if (c + 1 < nCols && owner[r][c + 1] != a && isPanel(a) && isPanel(owner[r][c + 1]) && joined(a, owner[r][c + 1], true)) root[find(a)] = find(owner[r][c + 1]);
      }
    }
    std::map<size_t, vector<uint8_t>> groups;
    for (size_t i = 0; i < cells.size(); ++i) {
      if (isPanel(static_cast<int32_t>(i))) groups[find(i)].push_back(static_cast<uint8_t>(i + 1));
    }
    vector<vector<uint8_t>> result;
    for (auto& [first, pads] : groups) {
      if (pads.size() > 1) result.push_back(std::move(pads));
    }
    return result;
  };
  auto bottomRow = [&](int32_t index) { return cells[index].row + cells[index].panel->mRows - 1; };
  auto touchesBelow = [&](int32_t index) {
    const size_t r1 = bottomRow(index);
    if (r1 + 1 >= nRows || !rowSep[r1].shared) return false;
    for (size_t c = cells[index].col; c < cells[index].col + cells[index].panel->mColumns; ++c) {
      if (isPanel(owner[r1 + 1][c])) return true;
    }
    return false;
  };
  auto touchesLeft = [&](int32_t index) {
    const size_t c0 = cells[index].col;
    if (c0 == 0 || !colSep[c0 - 1].shared) return false;
    for (size_t r = cells[index].row; r <= bottomRow(index); ++r) {
      if (isPanel(owner[r][c0 - 1])) return true;
    }
    return false;
  };
  auto shared = [&](int32_t b, bool horizontal) {  // the separation in front of cell b (right of or below its neighbour)
    return horizontal ? colSep[cells[b].col - 1].shared : rowSep[cells[b].row - 1].shared;
  };
  // only panels of the same width share x (the same height y): a panel spanning two columns has another scale than the narrow ones
  auto sameColumns = [&](int32_t a, int32_t b) { return cells[a].col == cells[b].col && cells[a].panel->mColumns == cells[b].panel->mColumns; };
  auto sameRows = [&](int32_t a, int32_t b) { return cells[a].row == cells[b].row && cells[a].panel->mRows == cells[b].panel->mRows; };
  auto columns = groupsOf([&](int32_t a, int32_t b, bool horizontal) { return !horizontal && shared(b, horizontal) && sameColumns(a, b); });
  auto rows = groupsOf([&](int32_t a, int32_t b, bool horizontal) { return horizontal && shared(b, horizontal) && sameRows(a, b); });
  for (auto& pads : columns) {
    plot.mSharedAxes.push_back({'X', std::move(pads), true});
  }
  for (auto& pads : rows) {
    plot.mSharedAxes.push_back({'Y', std::move(pads), true});
  }
  if (!mCommonTitles) return;
  auto titleRows = groupsOf([&](int32_t a, int32_t b, bool horizontal) {
    return horizontal && shared(b, horizontal) && !touchesBelow(a) && !touchesBelow(b) && bottomRow(a) == bottomRow(b);
  });
  auto titleColumns = groupsOf([&](int32_t a, int32_t b, bool horizontal) {
    return !horizontal && shared(b, horizontal) && !touchesLeft(a) && !touchesLeft(b) && cells[a].col == cells[b].col;
  });
  for (auto& pads : titleRows) {
    plot.mCommonTitles.emplace_back('X', std::move(pads));
  }
  for (auto& pads : titleColumns) {
    plot.mCommonTitles.emplace_back('Y', std::move(pads));
  }
}

}  // namespace SciRooPlot
