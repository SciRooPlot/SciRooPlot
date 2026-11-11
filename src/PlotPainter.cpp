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

#include "PlotPainter.h"

#include "SciRooPlot/Logger.h"

#include <TApplication.h>
#include <TArrow.h>
#include <TAttMarker.h>
#include <TAxisModLab.h>
#include <TBox.h>
#include <TCanvas.h>
#include <TEfficiency.h>
#include <TEllipse.h>
#include <TExec.h>
#include <TF1.h>
#include <TF2.h>
#include <TF3.h>
#include <TFrame.h>
#include <TGWindow.h>
#include <TGaxis.h>
#include <TGraph.h>
#include <TGraph2D.h>
#include <TGraph2DAsymmErrors.h>
#include <TGraph2DErrors.h>
#include <TGraphAsymmErrors.h>
#include <TGraphBentErrors.h>
#include <TGraphErrors.h>
#include <TGraphMultiErrors.h>
#include <TGraphPolar.h>
#include <TGraphPolargram.h>
#include <TGraphSmooth.h>
#include <TH1.h>
#include <TH1D.h>
#include <TH2.h>
#include <TH2D.h>
#include <TH2Poly.h>
#include <TH3.h>
#include <TH3D.h>
#include <THLimitsFinder.h>
#include <THStack.h>
#include <THashList.h>
#include <THn.h>
#include <THnSparse.h>
#include <TImage.h>
#include <TIterator.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TLegendEntry.h>
#include <TLine.h>
#include <TList.h>
#include <TMultiGraph.h>
#include <TObjArray.h>
#include <TObject.h>
#include <TObjectTable.h>
#include <TPad.h>
#include <TPave.h>
#include <TPaveStats.h>
#include <TPaveText.h>
#include <TPolyLine.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TProfile3D.h>
#include <TROOT.h>
#include <TRootCanvas.h>
#include <TSpline.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TText.h>
#include <TTimeStamp.h>
#include <TView.h>
#include <TVirtualPad.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Validation.h"
#include "util/Optional.h"
#include "util/Paths.h"
#include "util/ScopeGuard.h"
#include "util/Strings.h"
#include "util/TypeTraits.h"

using std::array;
using std::nullopt;
using std::optional;
using std::shared_ptr;
using std::string;
using std::tuple;
using std::unique_ptr;
using std::unordered_map;
using std::variant;
using std::vector;

namespace SciRooPlot
{
using util::expand_path;
using util::get_first;
using util::get_first_or;
using util::ignore_root_messages_below;
using util::is_2d;
using util::is_3d;
using util::is_func;
using util::is_func_1d;
using util::is_func_2d;
using util::is_func_3d;
using util::is_graph_1d;
using util::is_graph_2d;
using util::is_hist;
using util::is_hist_1d;
using util::is_hist_2d;
using util::is_hist_3d;
using util::is_one_of_v;
using util::make_scope_guard;
using util::pick;
using util::plural;
using util::str_contains;
using util::str_starts_with;
using util::str_upper;

//**************************************************************************************************
/**
 * Copy the line, fill and marker attributes of an object; for functions also title and number of points (CopyFuncAttributes).
 */
//**************************************************************************************************
template <typename From, typename To>
void PlotPainter::CopyStyle(const From& from, To& to)
{
  from.TAttLine::Copy(to);
  from.TAttFill::Copy(to);
  from.TAttMarker::Copy(to);
}
template <typename FuncT>
void PlotPainter::CopyFuncAttributes(FuncT* from, FuncT* to)
{
  CopyStyle(*from, *to);
  to->SetTitle(from->GetTitle());
  to->SetNpx(from->GetNpx());
  to->SetBit(kCanDelete);
}

//**************************************************************************************************
/**
 * Bin range of a histogram axis for a user range: the bins reaching into it, as TAxis::SetRangeUser selects them.
 */
//**************************************************************************************************
bool PlotPainter::SetBinRange(TAxis* axis, const optional<double_t>& min, const optional<double_t>& max)
{
  const auto [first, last] = BinRange(axis, min, max);
  axis->SetRange(first, last);
  return std::max(first, 1) <= std::min(last, axis->GetNbins());  // false: no bin in the range, ROOT then selects all bins
}
std::pair<int32_t, int32_t> PlotPainter::BinRange(const TAxis* axis, const optional<double_t>& min, const optional<double_t>& max)
{
  int32_t first = axis->GetFirst();
  int32_t last = axis->GetLast();
  if (min) {
    first = axis->FindFixBin(*min);
    if (axis->GetBinUpEdge(first) <= *min) ++first;
  }
  if (max) {
    last = axis->FindFixBin(*max);
    if (axis->GetBinLowEdge(last) >= *max) --last;
  }
  return {first, last};
}

//**************************************************************************************************
/**
 * Restricts the bin range of a histogram axis to the bins within the frame (a bin reaching into the frame stays, as with TAxis::SetRangeUser):
 * ROOT clips only bars and fill areas of a histogram to the frame, not its markers, error bars and curves.
 */
//**************************************************************************************************
void PlotPainter::LimitToFrame(TAxis* axis, const TAxis* frameAxis)
{
  const double_t frameMin = frameAxis->GetBinLowEdge(frameAxis->GetFirst());
  const double_t frameMax = frameAxis->GetBinUpEdge(frameAxis->GetLast());
  constexpr double_t relTol = 1e-9;
  int32_t first = axis->FindFixBin(frameMin);
  if (axis->GetBinUpEdge(first) <= frameMin || TMath::AreEqualRel(axis->GetBinUpEdge(first), frameMin, relTol)) ++first;
  int32_t last = axis->FindFixBin(frameMax);
  if (axis->GetBinLowEdge(last) >= frameMax || TMath::AreEqualRel(axis->GetBinLowEdge(last), frameMax, relTol)) --last;
  first = std::max(first, axis->GetFirst());
  last = std::min(last, axis->GetLast());
  if (first != axis->GetFirst() || last != axis->GetLast()) axis->SetRange(first, last);
}

//**************************************************************************************************
/**
 * Conversions between 1d histograms and graphs, so both can be drawn with the same drawing option aliases.
 * The converted object replaces the original and keeps its name, title, axis titles and appearance.
 * The histogram is kept by the caller (its statistics box and placeholders), the graph is deleted.
 */
//**************************************************************************************************
TGraph* PlotPainter::ToGraph(const TH1* hist)
{
  auto* graph = new TGraphErrors();
  for (int32_t bin = 1; bin <= hist->GetNbinsX(); ++bin) {
    if (hist->GetBinContent(bin) == 0. && hist->GetBinError(bin) == 0.) continue;  // empty bins are not drawn for histograms either
    const int32_t point = graph->GetN();
    graph->SetPoint(point, hist->GetBinCenter(bin), hist->GetBinContent(bin));
    graph->SetPointError(point, 0.5 * hist->GetBinWidth(bin), hist->GetBinError(bin));
  }
  graph->SetName(hist->GetName());
  graph->SetTitle(hist->GetTitle());
  CopyStyle(*hist, *graph);
  // the axis frame of the graph is the one the histogram would have had (same x range and bin labels),
  // with the y range ROOT determined for the points
  const TAxis* xAxis = hist->GetXaxis();
  TH1F* frame = (xAxis->IsVariableBinSize()) ? new TH1F("", "", xAxis->GetNbins(), xAxis->GetXbins()->GetArray())
                                             : new TH1F("", "", xAxis->GetNbins(), xAxis->GetXmin(), xAxis->GetXmax());
  frame->SetDirectory(nullptr);
  xAxis->Copy(*frame->GetXaxis());
  hist->GetYaxis()->Copy(*frame->GetYaxis());
  frame->SetMinimum(graph->GetHistogram()->GetMinimum());
  frame->SetMaximum(graph->GetHistogram()->GetMaximum());
  graph->SetHistogram(frame);  // deletes the automatic one
  graph->SetBit(kCanDelete);
  return graph;
}
//**************************************************************************************************
/**
 * Graph of a 1d histogram for the clipped aliases (only the bins in the range of its x axis):
 * the steps (two points per bin, at its edges) for hist_clipped and hist_open_clipped, for hist_clipped also the vertical lines at the outer edges,
 * which go down out of the frame (see ClippedBase) until the frame is final (then to ROOT's base line, see GeneratePlot); the bin centres for
 * curve_clipped. ROOT cuts the lines of the graph at the frame.
 */
//**************************************************************************************************
TGraph* PlotPainter::ToClippedGraph(const TH1* hist, draw_option_t alias, bool logY)
{
  const TAxis* axis = hist->GetXaxis();
  vector<double_t> x;
  vector<double_t> y;
  for (int32_t bin = axis->GetFirst(); bin <= axis->GetLast(); ++bin) {
    const double_t content = hist->GetBinContent(bin);
    if (alias == curve_clipped) {
      x.push_back(axis->GetBinCenter(bin));
      y.push_back(content);
    } else {
      x.insert(x.end(), {axis->GetBinLowEdge(bin), axis->GetBinUpEdge(bin)});
      y.insert(y.end(), {content, content});
    }
  }
  auto* graph = new TGraph(static_cast<int32_t>(x.size()), x.data(), y.data());
  if (alias == hist_clipped && graph->GetN() > 0) {
    const double_t base = ClippedBase(graph, logY);
    graph->InsertPointBefore(0, x.front(), base);
    graph->SetPoint(graph->GetN(), x.back(), base);
  }
  graph->SetName(hist->GetName());
  graph->SetTitle(hist->GetTitle());
  CopyStyle(*hist, *graph);
  graph->SetBit(kCanDelete);
  return graph;
}

//**************************************************************************************************
/**
 * A y value below any frame that shows the graph: zero on a log axis (ROOT draws a line to a non-positive value down to the edge of the pad),
 * otherwise far below the smallest point.
 */
//**************************************************************************************************
double_t PlotPainter::ClippedBase(const TGraph* graph, bool logY)
{
  if (logY) return 0.;
  double_t minY = 0.;
  double_t maxAbsY = 0.;
  for (int32_t i = 0; i < graph->GetN(); ++i) {
    minY = std::min(minY, graph->GetPointY(i));
    maxAbsY = std::max(maxAbsY, std::abs(graph->GetPointY(i)));
  }
  return minY - 1e6 * (maxAbsY + 1.);
}

TH1* PlotPainter::ToHist(TGraph* graph, bool warn, bool warnAsymmetric)
{
  SortGraph(graph);
  const int32_t n = graph->GetN();
  if (n == 0) {
    if (warn) logger::warning("Graph {} cannot be drawn as a histogram (it has no points).", graph->GetName());
    return nullptr;
  }
  // bin edges from the x errors (if the graph has them): the bins of neighbouring points must touch (within precision),
  // gaps between them become empty bins and overlaps are split in the middle; without x errors the edges are half way between the points
  vector<double_t> edges;
  vector<int32_t> binOfPoint(n);
  bool hasErrorsX = false;
  bool overlaps = false;
  for (int32_t i = 0; i < n; ++i) {
    hasErrorsX |= (graph->GetErrorXlow(i) > 0. || graph->GetErrorXhigh(i) > 0.);
  }
  if (hasErrorsX) {
    for (int32_t i = 0; i < n && hasErrorsX; ++i) {
      const double_t low = graph->GetPointX(i) - graph->GetErrorXlow(i);
      const double_t high = graph->GetPointX(i) + graph->GetErrorXhigh(i);
      if (!(high > low)) {
        hasErrorsX = false;
      } else if (i == 0) {
        edges.push_back(low);
      } else if (const double_t previousHigh = edges.back(); std::abs(low - previousHigh) <= 1e-6 * (high - low)) {
        // touching the previous bin
      } else if (low > previousHigh) {
        edges.push_back(low);  // gap: an empty bin in between
      } else if (const double_t middle = 0.5 * (low + previousHigh);
                 middle - edges[edges.size() - 2] > 1e-6 * (previousHigh - edges[edges.size() - 2]) && high - middle > 1e-6 * (high - low)) {
        edges.back() = middle;  // overlapping the previous bin
        overlaps = true;
      } else {
        hasErrorsX = false;  // the point lies within the previous bin
      }
      edges.push_back(high);
      binOfPoint[i] = static_cast<int32_t>(edges.size()) - 1;
    }
    if (!hasErrorsX && warn) logger::warning("The x errors of graph {} do not define bin edges: it is drawn with bins half way between the points.", graph->GetName());
    if (hasErrorsX && overlaps && warn) logger::warning("The x errors of graph {} overlap: the bin edges are placed in the middle of the overlaps.", graph->GetName());
  }
  if (!hasErrorsX) {
    edges.resize(n + 1);
    for (int32_t i = 0; i <= n; ++i) {
      if (n == 1) {
        edges[i] = graph->GetPointX(0) + (i ? 0.5 : -0.5);
      } else if (i == 0) {
        edges[i] = graph->GetPointX(0) - 0.5 * (graph->GetPointX(1) - graph->GetPointX(0));
      } else if (i == n) {
        edges[i] = graph->GetPointX(n - 1) + 0.5 * (graph->GetPointX(n - 1) - graph->GetPointX(n - 2));
      } else {
        edges[i] = 0.5 * (graph->GetPointX(i - 1) + graph->GetPointX(i));
      }
      if (i > 0 && !(edges[i] > edges[i - 1])) {
        if (warn) logger::warning("Graph {} cannot be drawn as a histogram (it needs points with distinct x values).", graph->GetName());
        return nullptr;
      }
    }
    std::iota(binOfPoint.begin(), binOfPoint.end(), 1);
  }
  auto* hist = new TH1D(graph->GetName(), graph->GetTitle(), static_cast<int32_t>(edges.size()) - 1, edges.data());
  hist->SetDirectory(nullptr);
  bool asymmetricErrors = false;
  for (int32_t i = 0; i < n; ++i) {
    hist->SetBinContent(binOfPoint[i], graph->GetPointY(i));
    // a plain TGraph has no errors: GetErrorY* then return -1, which would become an error of 1
    const double errorLow = std::max(0.0, graph->GetErrorYlow(i)), errorHigh = std::max(0.0, graph->GetErrorYhigh(i));
    hist->SetBinError(binOfPoint[i], 0.5 * (errorLow + errorHigh));
    asymmetricErrors |= (errorLow != errorHigh);
  }
  if (asymmetricErrors && warn && warnAsymmetric) logger::warning("Graph {} has asymmetric y errors: it is drawn with their mean, since histograms have symmetric errors.", graph->GetName());
  CopyStyle(*graph, *hist);
  hist->SetBit(kCanDelete);
  delete graph;
  return hist;
}

//**************************************************************************************************
/**
 * The colours the n-th data item of a pad takes when neither the data nor its pad nor pad 0 sets one; after the last they start over.
 * ROOT colours that stay readable on white paper and projectors (contrast against white at least 3:1) and stay apart from each other,
 * the first ones most, also for colour-blind viewers (mostly from M. Petroff's accessible colour sets).
 */
//**************************************************************************************************
const vector<color_t>& PlotPainter::DefaultColors()
{
  static const vector<color_t> colors = {kBlack, kP10Blue, kP10Red, kP10Violet, kP8Orange, kP10Ash, kP10Brown, kTeal + 3};
  return colors;
}

//**************************************************************************************************
/**
 * The locals of GeneratePlot for one pad that the steps drawing its data and finishing it share: references to them (they live in GeneratePlot),
 * bound to their names at the start of each step.
 */
//**************************************************************************************************
struct PlotPainter::pad_state_t {
  using fill_defaults_t = std::function<const optional<vector<color_t>>&(const Plot::Pad& pad, const Plot::Pad::Data& data)>;
  struct used_defaults_t {
    uint16_t color, markerStyle, lineStyle, fillStyle;  // one colour entry per data item for marker, line and fill alike
  };
  Plot::Pad& padDefaults;
  const fill_defaults_t& fillDefaults;
  const data_buffer_t& dataBuffer;
  bool& fail;
  bool& anyView;
  Plot::Pad& pad;
  const uint8_t padID;
  TPad*& pad_ptr;
  vector<shared_ptr<Plot::Pad::Data>>& drawData;
  string& padSetup;
  optional<int16_t>& textFont;
  optional<float_t>& textSize;
  optional<color_t>& textColor;
  optional<float_t>& textAlpha;
  string& activeTextFormat;
  bool& hasRefFunc;
  const Plot::Pad::Data*& frameSource;
  TH1*& axisHist_ptr;
  bool& polarFrame;
  TGraphPolargram*& polargram;
  unique_ptr<TH1>& polarAxisHist;
  string& padProjection;
  bool& contentFrame;
  vector<std::pair<unique_ptr<TPaveStats>, std::shared_ptr<const Plot::Pad::StatsBox>>>& statsBoxes;
  vector<std::pair<TGraph*, bool>>& clippedSteps;
  string& drawOption;
  uint16_t& dataIndex;
  bool& mapDrawn;
  std::pair<bool, bool>& storedRangeZ;
  optional<array<double_t, 4>>& frameFuncRange;
  used_defaults_t& usedDefaults;
};

//**************************************************************************************************
/**
 * The locals of GeneratePlot for the data item drawn, with those of its pad.
 */
//**************************************************************************************************
struct PlotPainter::item_state_t {
  pad_state_t& padState;
  const shared_ptr<Plot::Pad::Data>& data;
  bool& isSampledFunc;
  bool& repeated;
  bool& numeratorMeans;
  bool& denominatorMeans;
};

//**************************************************************************************************
/**
 * Function to generate the plot.
 */
//**************************************************************************************************
unique_ptr<TCanvas> PlotPainter::GeneratePlot(Plot& plot, const data_buffer_t& dataBuffer)
{
  bool fail = false;
  auto sourceHistsGuard = make_scope_guard([this]() {  // needed until the boxes of the plot are made
    mSourceHists.clear();
    mDataDefinitions.clear();
    mProjectionAxisTitles.clear();
    mGraphFrameRangeX.clear();
    mGraph2DFrameRange.clear();
    mUndrawn.clear();
    mNotPainted.clear();
  });

  // ROOT reads some absolute sizes from gStyle only at paint time (see end of this function): undo the scale of a previous plot before drawing
  if (sUnscaledStyle) {
    gStyle->SetLineWidth(sUnscaledStyle->lineWidth);
    gStyle->SetHatchesLineWidth(sUnscaledStyle->hatchesLineWidth);
    gStyle->SetEndErrorSize(sUnscaledStyle->endErrorSize);
    gStyle->SetGridStyle(sUnscaledStyle->gridStyle);
    for (int32_t style = 1; style <= static_cast<int32_t>(sUnscaledStyle->lineStyles.size()); ++style) {
      gStyle->SetLineStyleString(style, sUnscaledStyle->lineStyles[style - 1].data());
    }
    sUnscaledStyle.reset();
  }

  double_t canvasWidth = std::round(mScale * plot.GetWidth().value_or(gStyle->GetCanvasDefW()));
  double_t canvasHeight = std::round(mScale * plot.GetHeight().value_or(gStyle->GetCanvasDefH()));
  if (canvasWidth <= 0 || canvasHeight <= 0) {
    logger::error("Plot {} has invalid dimensions {}x{}.", plot.GetName(), canvasWidth, canvasHeight);
    return nullptr;
  }
  // generate canvas with 'invisible' dummy size to avoid annoying popup window
  unique_ptr<TCanvas> canvas_ptr{new TCanvas("SRP_empty_scratch_canvas", plot.GetUniqueName().data(), 1., 1.)};
  // set actual name after ctor to avoid potential deletion of canvas with same name in root session
  canvas_ptr->SetName(plot.GetUniqueName().data());
  if (gROOT->IsBatch()) {
    canvas_ptr->SetCanvasSize(canvasWidth, canvasHeight);
  } else {
    auto canvasImp = static_cast<TRootCanvas*>(canvas_ptr->GetCanvasImp());
    canvasImp->UnmapWindow();
    canvas_ptr->SetCanvasSize(canvasWidth, canvasHeight);
    canvas_ptr->SetWindowPosition(gStyle->GetCanvasDefX(), gStyle->GetCanvasDefY());
    // define window size such that canvas size is correct
    canvasImp->Resize(canvasWidth, canvasHeight);
    canvasImp->FitCanvas();
    canvasImp->Resize(canvasWidth + (canvasWidth - canvas_ptr->GetWw()), canvasHeight + (canvasHeight - canvas_ptr->GetWh()));
    canvasImp->FitCanvas();
  }

  canvas_ptr->SetMargin(0., 0., 0., 0.);

  // apply user settings for plot
  if (plot.GetFillColor()) canvas_ptr->SetFillColor(plot.GetFillColor()->ToIndex());
  if (plot.GetFillStyle()) canvas_ptr->SetFillStyle(*plot.GetFillStyle());
  if (plot.GetFillAlpha()) canvas_ptr->SetFillColor(TColor::GetColorTransparent(canvas_ptr->GetFillColor(), *plot.GetFillAlpha()));

  if (plot.IsFixAspectRatio()) canvas_ptr->SetFixedAspectRatio(*plot.IsFixAspectRatio());

  auto& padDefaults = plot[0];
  // data without a colour of its own takes the next of SciRooPlot's default colours, unless its pad or pad 0 sets a list or a gradient;
  // fills only where SciRooPlot sets the fill style (see fillDefaults): the style stored with a ROOT histogram (1001) would otherwise fill it
  if (!padDefaults.GetDefaultMarkerColors() && !padDefaults.GetDefaultMarkerColorsGradient().rgbEndpoints) padDefaults.SetDefaultMarkerColors(DefaultColors());
  if (!padDefaults.GetDefaultLineColors() && !padDefaults.GetDefaultLineColorsGradient().rgbEndpoints) padDefaults.SetDefaultLineColors(DefaultColors());
  static const optional<vector<color_t>> defaultColors = DefaultColors();
  const pad_state_t::fill_defaults_t fillDefaults = [&padDefaults](const Plot::Pad& pad, const Plot::Pad::Data& data) -> const optional<vector<color_t>>& {
    const bool styleSet = data.GetFillStyle() || pad.GetDefaultFillStyles() || padDefaults.GetDefaultFillStyles();
    return (padDefaults.GetDefaultFillColors() || !styleSet) ? padDefaults.GetDefaultFillColors() : defaultColors;
  };
  if (!padDefaults.GetShapes().empty()) logger::warning("The shapes of pad 0 are not drawn: pad 0 only holds the defaults of the pads.");
  mFrameRanges.clear();
  mFrameHists.clear();
  mScaleHists.clear();
  mContentScales.clear();
  // the panels of a z group whose colour scale a ColorScale() cell shows draw none of their own (a panel in no z group: its own scale is shown)
  mScalesInCells.clear();
  for (const auto& [padID, pad] : plot.GetPads()) {
    if (padID == 0 || !pad.GetColorScaleOf() || *pad.GetColorScaleOf() == 0) continue;
    const uint8_t shown = *pad.GetColorScaleOf();
    bool inGroup = false;
    for (const auto& group : mSharedAxes['Z']) {
      if (std::find(group.begin(), group.end(), shown) == group.end()) continue;
      mScalesInCells.insert(group.begin(), group.end());
      inGroup = true;
    }
    if (!inGroup) mScalesInCells.insert(shown);
  }
  vector<std::pair<TPad*, string>> padSetups;  // the setup of each pad painted (see below)
  bool anyView{};                              // a pad draws in 3d: the viewing angles of pad 0 have a meaning
  if (padDefaults.GetXLow()) ReportIgnored("The position of pad 0", "pad 0 holds the defaults of all pads and is not drawn");
  for (const auto& [padID, dummy] : plot.GetPads()) {
    if (padID == 0) continue;  // pad 0 is used only to define the defaults
    auto& pad = plot[padID];   // needed because processData lambda cannot capture variable from structured binding ('dummy')
    if (pad.GetColorScaleOf()) {
      // a ColorScale() cell is drawn once all panels are drawn (see DrawColorScaleCell)
      if (!pad.GetData().empty() || pad.GetFrame()) logger::warning("Pad {} is the cell of a colour scale: its data is not drawn.", padID);
      continue;
    }

    // Pad placing
    array<double_t, 4> padPos = {0., 0., 1., 1.};

    auto xLow = pad.GetXLow();
    auto yLow = pad.GetYLow();
    auto xUp = pad.GetXUp();
    auto yUp = pad.GetYUp();

    const bool valid =
      xLow && yLow && xUp && yUp &&
      *xLow >= 0 && *xLow <= 1 &&
      *xUp >= 0 && *xUp <= 1 &&
      *yLow >= 0 && *yLow <= 1 &&
      *yUp >= 0 && *yUp <= 1 &&
      *xLow < *xUp &&
      *yLow < *yUp;

    if (!valid) {
      // a plot with only one pad and no position for it is drawn over the whole plot by design
      const bool isSinglePad = std::count_if(plot.GetPads().begin(), plot.GetPads().end(), [](const auto& p) { return p.first != 0; }) == 1;
      if (!isSinglePad || xLow || yLow || xUp || yUp) {
        logger::warning("Position of pad {} is not defined properly, drawing it over the whole plot.", padID);
      }
    } else {
      padPos = {*xLow, *yLow, *xUp, *yUp};
    }

    // get the settings for this pad
    auto textFont = get_first(pad.GetDefaultTextFont(), padDefaults.GetDefaultTextFont());
    auto textSize = get_first(pad.GetDefaultTextSize(), padDefaults.GetDefaultTextSize());
    auto textColor = get_first(pad.GetDefaultTextColor(), padDefaults.GetDefaultTextColor());
    auto textAlpha = get_first(pad.GetDefaultTextAlpha(), padDefaults.GetDefaultTextAlpha());

    canvas_ptr->cd();
    string padName = "Pad_" + std::to_string(padID);

    TPad* pad_ptr = new TPad(padName.data(), "", padPos[0], padPos[1], padPos[2], padPos[3]);

    // TAttPad drops a margin that leaves no room for the frame with the opposite margin as it is at that moment, without a word: each pair
    // is checked as a whole and set starting from zero, so that a valid pair does not depend on the order of setting (top 0.9, bottom 0.05)
    const float_t marginTop = get_first_or(pad_ptr->GetTopMargin(), pad.GetMarginTop(), padDefaults.GetMarginTop());
    const float_t marginBottom = get_first_or(pad_ptr->GetBottomMargin(), pad.GetMarginBottom(), padDefaults.GetMarginBottom());
    const float_t marginLeft = get_first_or(pad_ptr->GetLeftMargin(), pad.GetMarginLeft(), padDefaults.GetMarginLeft());
    const float_t marginRight = get_first_or(pad_ptr->GetRightMargin(), pad.GetMarginRight(), padDefaults.GetMarginRight());
    if (marginTop + marginBottom < 1.f) {
      pad_ptr->SetBottomMargin(0.);
      pad_ptr->SetTopMargin(marginTop);
      pad_ptr->SetBottomMargin(marginBottom);
    } else {
      logger::warning("The top and bottom margins of pad {} ({:g} and {:g}) leave no room for the frame: ignored.", padID, marginTop, marginBottom);
    }
    if (marginLeft + marginRight < 1.f) {
      pad_ptr->SetLeftMargin(0.);
      pad_ptr->SetRightMargin(marginRight);
      pad_ptr->SetLeftMargin(marginLeft);
    } else {
      logger::warning("The left and right margins of pad {} ({:g} and {:g}) leave no room for the frame: ignored.", padID, marginLeft, marginRight);
    }
    if (auto padFillColor = get_first(pad.GetFillColor(), padDefaults.GetFillColor())) pad_ptr->SetFillColor(padFillColor->ToIndex());
    if (auto padFillStyle = get_first(pad.GetFillStyle(), padDefaults.GetFillStyle())) pad_ptr->SetFillStyle(*padFillStyle);
    if (auto padFillAlpha = get_first(pad.GetFillAlpha(), padDefaults.GetFillAlpha())) pad_ptr->SetFillColor(TColor::GetColorTransparent(pad_ptr->GetFillColor(), *padFillAlpha));
    if (auto frameFillColor = get_first(pad.GetFrameFillColor(), padDefaults.GetFrameFillColor())) pad_ptr->SetFrameFillColor(frameFillColor->ToIndex());
    if (auto frameFillStyle = get_first(pad.GetFrameFillStyle(), padDefaults.GetFrameFillStyle())) pad_ptr->SetFrameFillStyle(*frameFillStyle);
    if (auto frameFillAlpha = get_first(pad.GetFrameFillAlpha(), padDefaults.GetFrameFillAlpha())) pad_ptr->SetFrameFillColor(TColor::GetColorTransparent(pad_ptr->GetFrameFillColor(), *frameFillAlpha));
    if (auto frameBorderColor = get_first(pad.GetFrameBorderColor(), padDefaults.GetFrameBorderColor())) pad_ptr->SetFrameLineColor(frameBorderColor->ToIndex());
    if (auto frameBorderAlpha = get_first(pad.GetFrameBorderAlpha(), padDefaults.GetFrameBorderAlpha())) pad_ptr->SetFrameLineColor(TColor::GetColorTransparent(pad_ptr->GetFrameLineColor(), *frameBorderAlpha));
    if (auto frameBorderStyle = get_first(pad.GetFrameBorderStyle(), padDefaults.GetFrameBorderStyle())) pad_ptr->SetFrameLineStyle(*frameBorderStyle);
    if (auto frameBorderWidth = get_first(pad.GetFrameBorderWidth(), padDefaults.GetFrameBorderWidth())) pad_ptr->SetFrameLineWidth(*frameBorderWidth);

    // palette, candle ranges and text format are global state that ROOT reads while painting: they are set by TExec primitives,
    // which run whenever the pad is painted (also when the canvas is read back from a file) and leave the defaults behind afterwards
    string padSetup = fmt::format("gStyle->SetPaintTextFormat(\"g\"); TCandle::SetBoxRange({}); TCandle::SetWhiskerRange({});",
                                  get_first_or(0.5, pad.GetDefaultCandleBoxRange(), padDefaults.GetDefaultCandleBoxRange()),
                                  get_first_or(1., pad.GetDefaultCandleWhiskerRange(), padDefaults.GetDefaultCandleWhiskerRange()));

    // a colour gradient gets one colour per data item that will actually take a colour from the defaults: the marker, line and fill
    // lists share one index (a data item taking any of the three moves it on), so all three are as long as the number of such items
    // (sized each by its own count, a shorter list would wrap around and give the marker and line of one item different shades);
    // the rule is that of ApplyDataStyle: marker and line colours always have a default list, fill colours only a list or gradient of
    // the pad or pad 0, or the default colours where SciRooPlot sets the fill style (fillDefaults)
    const bool hasFillList = pad.GetDefaultFillColors() || pad.GetDefaultFillColorsGradient().rgbEndpoints || padDefaults.GetDefaultFillColorsGradient().rgbEndpoints;
    const int32_t nTakingColors = static_cast<int32_t>(std::count_if(pad.GetData().begin(), pad.GetData().end(), [&](const auto& data) {
      const bool takesFill = !data->GetFillColor() && (hasFillList || fillDefaults(pad, *data));
      return !data->GetKeepInputStyle() && (!data->GetMarkerColor() || !data->GetLineColor() || takesFill);
    }));
    // a gradient is generated only if a data item takes colours from it, or if its number of colours is given explicitly
    // a gradient of pad 0 is turned into colours of this pad: written into pad 0, it would be replaced by the colours sized for the first pad
    auto applyGradient = [](const auto& gradient, int32_t nTaking, auto&& setColors) {
      if (nTaking == 0 && !gradient.nColors) return;
      const vector<int16_t> indices = GenerateGradientColors(get_first_or(nTaking, gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha));
      setColors(vector<color_t>(indices.begin(), indices.end()));
    };
    if (pad.GetDefaultMarkerColorsGradient().rgbEndpoints) {
      applyGradient(pad.GetDefaultMarkerColorsGradient(), nTakingColors, [&](const auto& colors) { pad.SetDefaultMarkerColors(colors); });
    } else if (padDefaults.GetDefaultMarkerColorsGradient().rgbEndpoints && !pad.GetDefaultMarkerColors()) {
      applyGradient(padDefaults.GetDefaultMarkerColorsGradient(), nTakingColors, [&](const auto& colors) { pad.SetDefaultMarkerColors(colors); });
    }
    if (pad.GetDefaultLineColorsGradient().rgbEndpoints) {
      applyGradient(pad.GetDefaultLineColorsGradient(), nTakingColors, [&](const auto& colors) { pad.SetDefaultLineColors(colors); });
    } else if (padDefaults.GetDefaultLineColorsGradient().rgbEndpoints && !pad.GetDefaultLineColors()) {
      applyGradient(padDefaults.GetDefaultLineColorsGradient(), nTakingColors, [&](const auto& colors) { pad.SetDefaultLineColors(colors); });
    }
    if (pad.GetDefaultFillColorsGradient().rgbEndpoints) {
      applyGradient(pad.GetDefaultFillColorsGradient(), nTakingColors, [&](const auto& colors) { pad.SetDefaultFillColors(colors); });
    } else if (padDefaults.GetDefaultFillColorsGradient().rgbEndpoints && !pad.GetDefaultFillColors()) {
      applyGradient(padDefaults.GetDefaultFillColorsGradient(), nTakingColors, [&](const auto& colors) { pad.SetDefaultFillColors(colors); });
    }
    if (pad.GetPaletteGradient().rgbEndpoints) {
      const auto& gradient = pad.GetPaletteGradient();
      padSetup += GradientPaletteCommand(get_first_or(255, gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha));
    } else if (padDefaults.GetPaletteGradient().rgbEndpoints && !pad.GetPalette()) {
      const auto& gradient = padDefaults.GetPaletteGradient();
      padSetup += GradientPaletteCommand(get_first_or(255, gradient.nColors), *gradient.rgbEndpoints, get_first_or(1.f, gradient.alpha));
    } else {
      // a ROOT palette with the alpha set together with it (the pad's own palette or else that of pad 0)
      const auto& alpha = (pad.GetPalette()) ? pad.GetPaletteGradient().alpha : padDefaults.GetPaletteGradient().alpha;
      padSetup += fmt::format(" gStyle->SetPalette({}, nullptr, {});", get_first_or(static_cast<int32_t>(kBird), pad.GetPalette(), padDefaults.GetPalette()), get_first_or(1.f, alpha));
    }
    pad_ptr->SetNumber(padID);
    pad_ptr->Draw();
    pad_ptr->cd();
    bool hasRefFunc = false;
    string activeTextFormat = "g";  // set by the pad's setup; changed by TExec primitives in front of items with another format
    auto drawData = pad.GetData();
    const bool hasData = !drawData.empty();
    unique_ptr<TH1> emptyFrame;  // frame made from the axis ranges of a pad without data and frame input
    // the data whose copy gives the frame
    const Plot::Pad::Data* frameSource{nullptr};

    if (pad.GetFrame()) {
      // the frame input of the pad is drawn only as frame (in front of the data, with the ranges stored with it, like data that defines the frame)
      const bool dataDefinesFrame = std::any_of(drawData.begin(), drawData.end(), [](const auto& curData) { return curData->GetDefinesFrame(); });
      if (dataDefinesFrame) {
        logger::warning("Pad {} has a frame ({}), data marked SetDefinesFrame() does not define it.", padID, pad.GetFrame()->GetName());
      }
      drawData.insert(drawData.begin(), std::make_shared<Plot::Pad::Data>(*pad.GetFrame()));
      drawData[0]->SetDefinesFrame();
    } else if (hasData) {
      const size_t frameDataID = FrameDataIndex(drawData, dataBuffer);
      // make a copy of data that will serve as axis frame and put it in front of data vector
      if (drawData[frameDataID]->GetType() == "ratio") {
        drawData.insert(drawData.begin(), std::make_shared<Plot::Pad::Ratio>(*std::dynamic_pointer_cast<Plot::Pad::Ratio>(drawData[frameDataID])));
      } else {
        drawData.insert(drawData.begin(), std::make_shared<Plot::Pad::Data>(*drawData[frameDataID]));
      }
      frameSource = drawData[frameDataID + 1].get();
    } else {
      // without data and frame input the frame is an empty histogram with one bin spanning the x range (and y range for 2d, if z has a range);
      // the ranges of the pad are applied to it as to any frame (an Empty() cell of a layout, room for a legend or a text, takes no ranges from pad 0)
      auto range = [&](char axisLabel) {
        optional<double_t> min;
        optional<double_t> max;
        for (Plot::Pad& curPad : {std::ref(padDefaults), std::ref(pad)}) {
          if (&curPad == &padDefaults && pad.GetIsEmptyCell()) continue;
          if (curPad.GetAxes().find(axisLabel) == curPad.GetAxes().end()) continue;
          if (curPad[axisLabel].GetMinRange()) min = curPad[axisLabel].GetMinRange();
          if (curPad[axisLabel].GetMaxRange()) max = curPad[axisLabel].GetMaxRange();
        }
        return (min && max && *min < *max) ? optional<std::pair<double_t, double_t>>{{*min, *max}} : nullopt;
      };
      auto rangeX = range('X');
      auto rangeY = range('Y');
      if (rangeX && rangeY) {
        if (range('Z')) {
          emptyFrame = std::make_unique<TH2D>("frame", "", 1, rangeX->first, rangeX->second, 1, rangeY->first, rangeY->second);
        } else {
          emptyFrame = std::make_unique<TH1D>("frame", "", 1, rangeX->first, rangeX->second);
        }
        emptyFrame->SetDirectory(nullptr);
        drawData.push_back(std::make_shared<Plot::Pad::Data>("frame", "EMPTY_FRAME", nullopt));
      } else if (pad.GetLegendBoxes().empty() && pad.GetTextBoxes().empty() && !pad.GetIsEmptyCell()) {
        logger::warning("Nothing to be drawn in pad {}: it has no data, no frame and no{}{} range for an empty frame.", padID, (rangeX) ? "" : " X", (rangeY) ? "" : " Y");
        continue;
      }
    }
    if (!drawData.empty()) drawData[0]->SetLegendLabel("");  // axis frame should not appear in legend

    if (hasData) {
      // put reference function in data vector right after the axis histogram
      auto refFunc = (pad.GetRefFunc()) ? pad.GetRefFunc() : padDefaults.GetRefFunc();
      if (refFunc && !refFunc->GetName().empty()) {  // an empty one: no reference function
        drawData.insert(drawData.begin() + 1, refFunc);
        hasRefFunc = true;
      }
    }

    TH1* axisHist_ptr{nullptr};
    // the frame of a flat polar drawing: a disc in a square frame without cartesian axes, measured by a radial axis
    bool polarFrame{};
    // the polar grid of a pad with polar graphs, drawn instead of a cartesian frame, and the axis settings of such a pad (not drawn)
    TGraphPolargram* polargram{nullptr};
    unique_ptr<TH1> polarAxisHist;
    // the map projection the frame is drawn with (AITOFF, MERCATOR, SINUSOIDAL, PARABOLIC, MOLLWEIDE), empty if none
    string padProjection;
    // the frame is a 3d histogram: its colour scale shows the contents (the C axis), not the binned z axis
    bool contentFrame{};
    vector<std::pair<unique_ptr<TPaveStats>, std::shared_ptr<const Plot::Pad::StatsBox>>> statsBoxes;  // filled by ROOT while drawing the data, placed after all data are drawn
    string drawOption;
    uint16_t dataIndex{};
    // steps drawn with hist_clipped (false) or hist_open_clipped (true): ROOT's steps cut at the frame once it is final
    vector<std::pair<TGraph*, bool>> clippedSteps;
    bool mapDrawn{};  // a 2d or 3d histogram of the pad is drawn (the first one gives the frame its colour levels)
    // minimum, maximum stored with the 2d data defining the frame: kept like a range set by the user
    std::pair<bool, bool> storedRangeZ{};
    // the range a function giving the frame is sampled over for it (x and y): it is drawn with the same samples
    optional<array<double_t, 4>> frameFuncRange;
    pad_state_t::used_defaults_t usedDefaults{};  // entries of the pad's default lists taken so far
    pad_state_t padState{padDefaults, fillDefaults, dataBuffer, fail, anyView, pad, padID, pad_ptr, drawData, padSetup, textFont, textSize, textColor, textAlpha, activeTextFormat, hasRefFunc, frameSource, axisHist_ptr, polarFrame, polargram, polarAxisHist, padProjection, contentFrame, statsBoxes, clippedSteps, drawOption, dataIndex, mapDrawn, storedRangeZ, frameFuncRange, usedDefaults};
    for (const auto& data : drawData) {
      if (fail) break;
      // data 0 is the copy of the first data that gives the frame: ROOT's messages about it come again with the data itself
      auto quiet = ignore_root_messages_below(dataIndex ? kUnset : kFatal);
      if (data->GetDrawOption()) {
        // SAME (also SAMES, SAME0, SAMES0) is SciRooPlot's: the first data of a pad is also drawn as its frame, into the empty pad, where
        // SAME made ROOT paint a title box (histograms), a garbled picture (SAME0) or an extra copy (graphs); the stats boxes get SAMES anyway.
        // Removed silently, since all data of a pad is drawn into the same frame as asked; only the own colour range of SAME0 is lost
        string option = *data->GetDrawOption();
        string upper = str_upper(option);
        bool ownRange{};
        for (size_t pos = upper.find("SAME"); pos != string::npos; pos = upper.find("SAME", pos)) {
          size_t end = pos + 4;
          if (end < upper.size() && upper[end] == 'S') ++end;
          if (end < upper.size() && upper[end] == '0') {
            ++end;
            ownRange = true;
          }
          upper.erase(pos, end - pos);
          option.erase(pos, end - pos);
        }
        if (ownRange && dataIndex != 0) logger::warning("Drawing option '{}' of {}: SAME0 is removed, all data of a pad share the colour range of its frame.", *data->GetDrawOption(), data->GetName());
        // AXIS and AXIG (axes or grid only) would draw no data and leave the frame drawn from it without labels: the axes are SciRooPlot's
        // (warned once per data: the frame copy of the first data, index 0, is drawn from the same item again)
        bool axesOnly{};
        for (const char* token : {"AXIS", "AXIG"}) {
          for (size_t pos = upper.find(token); pos != string::npos; pos = upper.find(token, pos)) {
            upper.erase(pos, 4);
            option.erase(pos, 4);
            axesOnly = true;
          }
        }
        if (axesOnly && dataIndex != 0) logger::warning("Drawing option '{}' of {}: AXIS and AXIG are removed, the axes are drawn by SciRooPlot (a frame without data: Pad::SetFrame).", *data->GetDrawOption(), data->GetName());
        // PLC, PMC, PFC: ROOT counts every object drawn with them (TPad::IncrementPaletteColor) and gives each a colour spread over the palette
        // when it is painted; the frame, a copy of the first data drawn twice, would take colours of its own and shift those of the data
        if (dataIndex == 0) {
          for (const char* palette : {"PLC", "PMC", "PFC"}) {
            for (size_t pos = upper.find(palette); pos != string::npos; pos = upper.find(palette, pos)) {
              upper.erase(pos, 3);
              option.erase(pos, 3);
            }
          }
        }
        drawOption += option;
      }
      bool isSampledFunc{};  // a function given by C++ code is written as values sampled over its range: it has no values outside of it
      bool repeated{};       // an input prepared a second time (the denominator of a function numerator): reported only once
      // the inputs of the ratio were profiles, divided as their means (see PrepareOperand)
      bool numeratorMeans{}, denominatorMeans{};
      item_state_t state{padState, data, isSampledFunc, repeated, numeratorMeans, denominatorMeans};
      auto dropStoredFunctions = [&](auto&&... args) { return DropStoredFunctions(state, std::forward<decltype(args)>(args)...); };
      auto prepareOperand = [&](auto&&... args) { return PrepareOperand(state, std::forward<decltype(args)>(args)...); };
      auto processData = [&](auto&&... args) { return ProcessData(state, std::forward<decltype(args)>(args)...); };

      optional<data_ptr_t> rawData;
      try {
        // ranges stored with an input are what data marked SetDefinesFrame is there for: they are kept for its copy that draws the frame
        const bool keepStoredRanges = (dataIndex == 0) && data->GetDefinesFrame() && data->GetType() != "ratio";
        TObject* input = (dataIndex == 0 && emptyFrame) ? emptyFrame.get() : dataBuffer.at(data->GetDataSource()).at(data->GetName() + data->GetDataInfo().GetNameSuffix()).get();
        rawData = GetDataClone(input, data->GetProjInfo(), keepStoredRanges);
      } catch (std::out_of_range&) {
        rawData = nullopt;
      }

      if (rawData) {
        bool numeratorFailed{};  // a function numerator that could not be sampled (reported)
        if (data->GetType() == "ratio") {
          std::visit([&](auto* ptr) { dropStoredFunctions(ptr, "division"); }, *rawData);
          auto ratio = std::dynamic_pointer_cast<Plot::Pad::Ratio>(data);
          numeratorMeans = prepareOperand(*rawData, ratio->GetNumModify(), data->GetName());
          // a function as numerator (e.g. model / data) is sampled where the denominator has its values (the points of a graph, the bin
          // centres of a histogram, after the modifiers of the denominator): the division is then that of a graph without errors, which
          // takes the errors of the denominator; a function of the inputs only within its range, a formula of the plot definition everywhere
          if (auto* func = std::get_if<TF1*>(&*rawData)) {
            optional<data_ptr_t> denom;
            try {
              denom = GetDataClone(dataBuffer.at(ratio->GetDenomDataSource()).at(ratio->GetDenomName() + ratio->GetDenomDataInfo().GetNameSuffix()).get(), ratio->GetDenomProjInfo());
            } catch (std::out_of_range&) {
              denom = nullopt;
            }
            if (denom) {
              repeated = true;  // reported when it is divided by
              prepareOperand(*denom, ratio->GetDenomModify(), ratio->GetDenomName());
              repeated = false;
              const bool fullRange = (data->GetDataSource() == kUserFunctions);
              auto* sampled = new TGraph();
              sampled->SetBit(kCanDelete);  // owned by the pad, as the clone it replaces
              auto sample = [&](double_t x) {
                if (fullRange || ((*func)->GetXmin() <= x && x <= (*func)->GetXmax())) sampled->AddPoint(x, (*func)->Eval(x));
              };
              std::visit(
                [&](auto* ptr) {
                  using denom_type = std::decay_t<decltype(ptr)>;
                  if constexpr (is_graph_1d<denom_type>()) {
                    for (int32_t i = 0; i < ptr->GetN(); ++i)
                      sample(ptr->GetX()[i]);
                  } else if constexpr (is_hist_1d<denom_type>()) {
                    for (int32_t bin = 1; bin <= ptr->GetNbinsX(); ++bin)
                      sample(ptr->GetXaxis()->GetBinCenter(bin));
                  } else if constexpr (is_func_1d<denom_type>()) {
                    // function / function: sampled at the numerator's SetNpx points where both are defined (a formula of the plot
                    // definition everywhere: then the range of the other function, of the ratio data or of the frame)
                    const bool denomFullRange = (ratio->GetDenomDataSource() == kUserFunctions);
                    double_t lo = -std::numeric_limits<double_t>::infinity(), hi = std::numeric_limits<double_t>::infinity();
                    if (!fullRange) {
                      lo = (*func)->GetXmin();
                      hi = (*func)->GetXmax();
                    }
                    if (!denomFullRange) {
                      lo = std::max(lo, ptr->GetXmin());
                      hi = std::min(hi, ptr->GetXmax());
                    }
                    if (data->GetMinRangeX()) lo = std::max(lo, *data->GetMinRangeX());
                    if (data->GetMaxRangeX()) hi = std::min(hi, *data->GetMaxRangeX());
                    if (axisHist_ptr && (!std::isfinite(lo) || !std::isfinite(hi))) {
                      if (!std::isfinite(lo)) lo = axisHist_ptr->GetXaxis()->GetXmin();
                      if (!std::isfinite(hi)) hi = axisHist_ptr->GetXaxis()->GetXmax();
                    }
                    if (!std::isfinite(lo) || !std::isfinite(hi)) {
                      logger::error("Ratio {}: two formulas of the plot definition have no range to be divided in: set one with SetRangeX.", data->GetName());
                      numeratorFailed = true;
                    } else if (lo >= hi) {
                      logger::error("Ratio {}: the ranges of the functions do not overlap.", data->GetName());
                      numeratorFailed = true;
                    } else {
                      const int32_t npx = std::max((*func)->GetNpx(), 2);
                      for (int32_t i = 0; i < npx; ++i) {
                        const double_t x = lo + (hi - lo) * i / (npx - 1);
                        sampled->AddPoint(x, (*func)->Eval(x));
                      }
                    }
                  }
                  delete ptr;
                },
                *denom);
              if (sampled->GetN() > 0) {
                sampled->SetName((*func)->GetName());
                sampled->SetTitle((*func)->GetTitle());
                delete *func;
                rawData = sampled;
              } else {
                delete sampled;  // a denominator of another kind: reported as unsupported division below
              }
            }
          }
        }
        if (numeratorFailed) {
          fail = true;  // reported above: not again as an unsupported division
        } else {
          std::visit(processData, *rawData);
        }
      } else {
        fail = true;
      }
    }  // end data code

    if (fail) {
      return nullptr;
    }
    FinishPad(padState);

    padSetups.emplace_back(pad_ptr, std::move(padSetup));
  }
  if (!anyView && (padDefaults.GetViewTheta() || padDefaults.GetViewPhi())) ReportIgnored("The view of pad 0", "no pad of the plot draws in 3d");

  // the setup has to be the first primitive of each pad, so that it runs before anything of the pad is painted: added once all pads are
  // painted, since every update while the later pads are made repaints the earlier ones, and each run of a setup goes through the
  // interpreter, which keeps about 30 kB of memory per run (ROOT bug 15); the pads paint with their final setups at the last update
  for (auto& [setupPad, setup] : padSetups) {
    TExec* padSetupExec = new TExec(fmt::format("setup_pad_{}", setupPad->GetNumber()).data(), setup.data());
    padSetupExec->SetBit(kCanDelete);
    setupPad->GetListOfPrimitives()->AddFirst(padSetupExec);
    setupPad->Modified();
  }

  // what needs all pads drawn: the cells of colour scales, legends with rows for data of other pads, and the titles drawn once for
  // a row or column of touching panels
  for (const auto& [padID, pad] : plot.GetPads()) {
    if (padID > 0 && pad.GetColorScaleOf()) DrawColorScaleCell(canvas_ptr.get(), plot, padID);
  }
  for (auto& [padID, pad] : plot.GetPads()) {
    if (padID == 0) continue;
    auto* pad_ptr = dynamic_cast<TPad*>(canvas_ptr->GetPrimitive(("Pad_" + std::to_string(padID)).data()));
    uint8_t legendIndex{};
    for (const auto& box : pad.GetLegendBoxes()) {
      ++legendIndex;
      if (!box->HasForeignRows() || !pad_ptr) continue;
      box->MergeLegendEntries(static_cast<uint16_t>(pad.GetData().size()));
      // a row for the data of another pad takes the legend label and the row style of that data, unless the row sets its own
      auto& rows = box->mLegendEntries;
      for (auto row = rows.begin(); row != rows.end();) {
        if (!row->GetRefPadID()) {
          ++row;
          continue;
        }
        const uint8_t refPad = *row->GetRefPadID();
        const uint16_t refData = row->GetRefDataID().value_or(0);
        auto source = plot.GetPads().find(refPad);
        if (refPad == 0 || source == plot.GetPads().end() || refData < 1 || refData > source->second.GetData().size()) {
          logger::warning("Legend {} of pad {}: the row for data {} of pad {} is left out, the plot has no such data.", legendIndex, padID, refData, refPad);
          row = rows.erase(row);
          continue;
        }
        const auto& data = *source->second.GetData()[refData - 1];
        Plot::Pad::LegendEntry merged = data.GetLegendEntryIfSet().value_or(Plot::Pad::LegendEntry{});
        merged += *row;
        if (!merged.mLabel) merged.mLabel = data.GetLegendLabel().value_or("");
        *row = merged;
        ++row;
      }
      pad_ptr->cd();
      if (!DrawBox(box, pad, padDefaults, pad_ptr, "LegendBox_" + std::to_string(legendIndex))) {
        logger::warning("Legend {} was not added since it is empty.", legendIndex);
      }
    }
    uint8_t textIndex{};
    for (const auto& box : pad.GetTextBoxes()) {
      ++textIndex;
      if (!box->RefersToOtherPads() || !pad_ptr) continue;
      pad_ptr->cd();
      if (!DrawBox(box, pad, padDefaults, pad_ptr, "TextBox_" + std::to_string(textIndex))) {
        logger::warning("Text {} was not added since it is empty.", textIndex);
      }
    }
  }
  for (size_t i = 0; i < plot.GetCommonTitles().size(); ++i) {
    DrawCommonTitle(canvas_ptr.get(), plot.GetCommonTitles()[i].first, plot.GetCommonTitles()[i].second, i + 1);
  }

  CheckFontSizes(canvas_ptr->GetListOfPrimitives());
  if (mScale != 1. || mOnScreen) {
    // axes, hatched fill areas, the ends of error bars and the dashes of line styles are painted with the absolute sizes defined in gStyle
    sUnscaledStyle = StyleSizes{gStyle->GetLineWidth(), gStyle->GetHatchesLineWidth(), gStyle->GetEndErrorSize(), gStyle->GetGridStyle(), {}};
    gStyle->SetLineWidth(ScaleLineWidth(gStyle->GetLineWidth()));
    gStyle->SetHatchesLineWidth(ScaleLineWidth(static_cast<Width_t>(gStyle->GetHatchesLineWidth())));
    gStyle->SetEndErrorSize(gStyle->GetEndErrorSize() * mScale);
    // line styles are lists of dash and gap lengths (see TStyle::SetLineStyleString)
    for (int32_t style = 1; style < 30; ++style) {
      const string dashes = gStyle->GetLineStyleString(style);
      sUnscaledStyle->lineStyles.push_back(dashes);
      string scaledDashes;
      std::istringstream stream(dashes);
      for (int32_t length{}; stream >> length;) {
        scaledDashes += std::to_string(std::lround(length * mScale)) + " ";
      }
      if (!scaledDashes.empty()) gStyle->SetLineStyleString(style, scaledDashes.data());
    }
    // the X11 backend draws styles 2-4 with fixed dash patterns instead of these strings (the strings at scale 1, but never scaled):
    // on screen they are drawn with free user-defined styles (the highest ones without a string) that carry the strings of 2-4
    mScreenDashStyles = {};
    if (mOnScreen) {
      size_t next{};
      for (int32_t style = 29; style > 4 && next < mScreenDashStyles.size(); --style) {
        if (sUnscaledStyle->lineStyles[style - 1].find_first_not_of(' ') != string::npos) continue;  // unset styles hold blanks
        mScreenDashStyles[next] = static_cast<Style_t>(style);
        gStyle->SetLineStyleString(style, gStyle->GetLineStyleString(static_cast<int32_t>(next) + 2));
        ++next;
      }
      gStyle->SetGridStyle(ScreenLineStyle(gStyle->GetGridStyle()));
    }
    std::set<TObject*> done;
    ApplyScale(canvas_ptr.get(), done);
  }
  canvas_ptr->cd();
  // the colours of the canvas that are not predefined by ROOT are defined by its first primitive, so that a canvas read from a file
  // in another session paints with them (ROOT itself stores the complete colour list, and only with the first canvas written);
  // it runs before the setup of any pad, so that the palettes these set take colour indices above them instead of being overwritten
  std::set<int32_t> customColors;
  CollectCustomColors(canvas_ptr.get(), customColors);
  if (!customColors.empty()) {
    TExec* colorExec = new TExec("define_colors", ColorDefinitionCommand(customColors).data());
    colorExec->SetBit(kCanDelete);
    canvas_ptr->GetListOfPrimitives()->AddFirst(colorExec);
  }
  // last primitive of the canvas: restores the global settings after the pads were painted
  TExec* resetExec = new TExec("reset_globals", fmt::format("gStyle->SetPalette({}); gStyle->SetPaintTextFormat(\"g\"); gStyle->SetOptTitle(1); TCandle::SetBoxRange(0.5); TCandle::SetWhiskerRange(1.);", static_cast<int32_t>(kBird)).data());
  resetExec->SetBit(kCanDelete);
  resetExec->Draw();
  canvas_ptr->Modified();
  canvas_ptr->Update();
  return canvas_ptr;
}

//**************************************************************************************************
/**
 * Finishes the pad once its data are drawn: room beside the axes, colour scales and z titles, shapes, boxes and statistics boxes, view angles, labels clipped at the edge.
 */
//**************************************************************************************************
void PlotPainter::FinishPad(pad_state_t& padState)
{
  auto& padDefaults = padState.padDefaults;
  auto& anyView = padState.anyView;
  auto& pad = padState.pad;
  auto& padID = padState.padID;
  auto& pad_ptr = padState.pad_ptr;
  auto& drawData = padState.drawData;
  auto& padSetup = padState.padSetup;
  auto& textFont = padState.textFont;
  auto& textSize = padState.textSize;
  auto& textColor = padState.textColor;
  auto& hasRefFunc = padState.hasRefFunc;
  auto& axisHist_ptr = padState.axisHist_ptr;
  auto& polarFrame = padState.polarFrame;
  auto& polargram = padState.polargram;
  auto& padProjection = padState.padProjection;
  auto& contentFrame = padState.contentFrame;
  auto& statsBoxes = padState.statsBoxes;
  auto& clippedSteps = padState.clippedSteps;

  const bool flatFrame = axisHist_ptr && !pad_ptr->GetView() && !polarFrame && !polargram;
  if (flatFrame) AddAxisRoom(pad_ptr, axisHist_ptr, padDefaults, pad, padID, (hasRefFunc) ? drawData[1].get() : nullptr, padSetup);
  if (mScalesInCells.count(padID)) {
    // the colour scale of the pad is shown by a ColorScale() cell: the drawing options lose their Z (ROOT then removes the scale it made)
    static const std::regex withScale("(COL|BOX[0-9]?|CONT[0-9]?|LEGO[0-9]?|SURF[0-9]?|TRI[0-9]?)Z", std::regex::icase);
    for (TObjLink* link = pad_ptr->GetListOfPrimitives()->FirstLink(); link; link = link->Next()) {
      link->SetOption(std::regex_replace(string(link->GetOption()), withScale, "$1").data());
    }
  }
  // the placement of the boxes below relies on the painted data (painting creates e.g. the palette of a colour plot):
  // the pad is updated once for all data, since every update repaints everything drawn so far
  pad_ptr->Update();
  // the clipped steps are ROOT's steps cut at the frame, which is known now: HIST goes down to the base line of the steps at the outer edges
  // and fills down to it (0, or the lower frame edge if that is above 0 or on a log axis); HIST ][ (hist_open) also leaves out the bins at
  // the ends that lie on the base line, with the step down to them: bins with the content of the base and, if the base is the lower frame
  // edge, all bins below it (TGraphPainter::PaintGrapHist). The steps (two points per bin, for hist_clipped between the two points of the
  // outer edges) and the outline of their fill follow
  if (!clippedSteps.empty()) {
    const bool logX = pad_ptr->GetLogx();
    const bool logY = pad_ptr->GetLogy();
    const double_t bottom = (logY) ? std::pow(10., pad_ptr->GetUymin()) : pad_ptr->GetUymin();
    const double_t top = (logY) ? std::pow(10., pad_ptr->GetUymax()) : pad_ptr->GetUymax();
    const double_t base = (logY) ? bottom : std::clamp(0., bottom, top);
    // left of the frame: steps without any bin in the range of the frame are drawn there (ROOT refuses a graph without points)
    const double_t outside = (logX) ? std::pow(10., 2. * pad_ptr->GetUxmin() - pad_ptr->GetUxmax()) : 2. * pad_ptr->GetUxmin() - pad_ptr->GetUxmax();
    auto onBase = [&](double_t y) { return y == base || (y < base && base == bottom); };
    for (auto [steps, open] : clippedSteps) {
      vector<double_t> x(steps->GetX(), steps->GetX() + steps->GetN());
      vector<double_t> y(steps->GetY(), steps->GetY() + steps->GetN());
      if (open) {
        size_t first = 0;
        size_t last = x.size();  // points [first, last) are kept, whole bins
        while (first < last && onBase(y[first]))
          first += 2;
        while (last > first && onBase(y[last - 1]))
          last -= 2;
        if (first < last) {  // as ROOT: steps all on the base line are drawn whole, along it
          x = vector<double_t>(x.begin() + first, x.begin() + last);
          y = vector<double_t>(y.begin() + first, y.begin() + last);
        }
      } else if (!y.empty()) {
        y.front() = y.back() = base;
      }
      auto setPoints = [&](TGraph* graph, bool closed) {
        graph->Set(0);
        if (x.empty()) {
          for (int32_t i = 0; i < 3; ++i)
            graph->SetPoint(i, outside, base);
          return;
        }
        if (closed) graph->SetPoint(0, x.front(), base);
        for (size_t i = 0; i < x.size(); ++i)
          graph->SetPoint(graph->GetN(), x[i], y[i]);
        if (closed) graph->SetPoint(graph->GetN(), x.back(), base);
      };
      setPoints(steps, false);
      // the fill: the outline of the steps closed at the base line (those of hist_clipped are already)
      if (auto* fill = dynamic_cast<TGraph*>(pad_ptr->GetListOfPrimitives()->FindObject((string("fill_") + steps->GetName()).data()))) {
        setPoints(fill, open);
      }
    }
  }
  // z titles with offset 0 next to their labels: the z axis of a 3D view and the colour scales (the range of the frame is final now)
  if (axisHist_ptr && axisHist_ptr->GetDimension() == 3) {
    // the binned z axis has a title of its own; the colour scale shows the contents (the C axis). ROOT makes the scale of a 3d histogram
    // once, with the look of the z axis at that time: it is made again with the look of the scale (the texts of the pad and the defaults
    // of a layout, then the settings of C; nothing of Z) and its title placed by the content labels, then the z axis gets its own look back
    // (the scale keeps its copy)
    TAxis* zAxis = axisHist_ptr->GetZaxis();
    PlaceZTitle(pad_ptr, zAxis, {{zAxis->GetBinLowEdge(zAxis->GetFirst()), zAxis->GetBinUpEdge(zAxis->GetLast()), false}});
    TList* functions = axisHist_ptr->GetListOfFunctions();
    if (TObject* palette = functions->FindObject("palette")) {
      TAttAxis zLook;
      zAxis->TAttAxis::Copy(zLook);
      // the bits of the look are kept apart from TAttAxis: they are given back with it
      constexpr std::array lookBits{TAxis::kCenterTitle, TAxis::kCenterLabels, TAxis::kNoExponent, TAxis::kTickPlus, TAxis::kTickMinus};
      std::array<bool, lookBits.size()> zBits{};
      for (size_t i = 0; i < lookBits.size(); ++i)
        zBits[i] = zAxis->TestBit(lookBits[i]);
      zAxis->ResetAttAxis("Z");  // ROOT's look of a z axis (gStyle), the title next to the labels
      zAxis->SetTitleOffset(0.f);
      if (textFont) {
        zAxis->SetTitleFont(*textFont);
        zAxis->SetLabelFont(*textFont);
      }
      if (textColor) {
        zAxis->SetTitleColor(textColor->ToIndex());
        zAxis->SetLabelColor(textColor->ToIndex());
      }
      if (textSize) zAxis->SetLabelSize(*textSize);
      if (auto titleSize = LayoutValue(pad, padDefaults, "title_size")) zAxis->SetTitleSize(*titleSize);
      if (auto titleOffset = LayoutValue(pad, padDefaults, "title_offset_z")) zAxis->SetTitleOffset(*titleOffset);
      bool ownTitleOffset = zAxis->GetTitleOffset() != 0.f;  // a fixed distance from the layout
      optional<float_t> titleAlpha, labelAlpha, axisAlpha;
      for (const auto* contentSettings : AxisSettings(pad, padDefaults, 'C')) {
        const auto& content = *contentSettings;
        if (content.GetTitleFont()) zAxis->SetTitleFont(*content.GetTitleFont());
        if (content.GetTitleSize()) zAxis->SetTitleSize(*content.GetTitleSize());
        if (content.GetTitleColor()) zAxis->SetTitleColor(content.GetTitleColor()->ToIndex());
        if (content.GetTitleOffset()) zAxis->SetTitleOffset(*content.GetTitleOffset());
        ownTitleOffset = ownTitleOffset || content.GetTitleOffset();
        if (content.GetTitleCenter()) zAxis->CenterTitle(*content.GetTitleCenter());
        if (content.GetLabelFont()) zAxis->SetLabelFont(*content.GetLabelFont());
        if (content.GetLabelSize()) zAxis->SetLabelSize(*content.GetLabelSize());
        if (content.GetLabelColor()) zAxis->SetLabelColor(content.GetLabelColor()->ToIndex());
        if (content.GetLabelOffset()) zAxis->SetLabelOffset(*content.GetLabelOffset());
        if (content.GetLabelCenter()) zAxis->CenterLabels(*content.GetLabelCenter());
        if (content.GetAxisColor()) zAxis->SetAxisColor(content.GetAxisColor()->ToIndex());
        if (content.GetTickLength()) zAxis->SetTickLength(*content.GetTickLength());
        if (content.GetTickOrientation()) zAxis->SetTicks((*content.GetTickOrientation()).data());
        if (content.GetNumDivisions()) zAxis->SetNdivisions(*content.GetNumDivisions());
        if (content.GetMaxDigits()) zAxis->SetMaxDigits(*content.GetMaxDigits());
        if (content.GetNoExponent()) zAxis->SetNoExponent(*content.GetNoExponent());
        if (content.GetTitleAlpha()) titleAlpha = content.GetTitleAlpha();
        if (content.GetLabelAlpha()) labelAlpha = content.GetLabelAlpha();
        if (content.GetAxisAlpha()) axisAlpha = content.GetAxisAlpha();
      }
      if (titleAlpha) zAxis->SetTitleColor(TColor::GetColorTransparent(zAxis->GetTitleColor(), *titleAlpha));
      if (labelAlpha) zAxis->SetLabelColor(TColor::GetColorTransparent(zAxis->GetLabelColor(), *labelAlpha));
      if (axisAlpha) zAxis->SetAxisColor(TColor::GetColorTransparent(zAxis->GetAxisColor(), *axisAlpha));
      if (!ownTitleOffset) {
        // TPaletteAxis reads the divisions (with the max digits, which decide the power of ten taken out of the labels) from the z axis at
        // every paint, i.e. those given back below: the labels are measured with them
        TAttAxis made;
        zAxis->TAttAxis::Copy(made);
        const Int_t painted = zLook.GetNdivisions();
        zAxis->SetNdivisions(std::abs(painted) % 1000000, painted > 0);
        if (std::abs(painted) / 1000000 > 0) zAxis->SetMaxDigits(std::abs(painted) / 1000000);
        PlaceZTitle(pad_ptr, zAxis, {{axisHist_ptr->GetMinimum(), axisHist_ptr->GetMaximum(), false}});
        const float_t offset = zAxis->GetTitleOffset();
        made.Copy(*zAxis);
        zAxis->SetTitleOffset(offset);
      }
      functions->Remove(palette);
      delete palette;
      pad_ptr->Modified();
      pad_ptr->Update();
      zLook.Copy(*zAxis);
      for (size_t i = 0; i < lookBits.size(); ++i)
        zAxis->SetBit(lookBits[i], zBits[i]);
      pad_ptr->Modified();
    }
  } else if (axisHist_ptr && axisHist_ptr->GetDimension() == 2) {
    const vector<label_range_t> ranges{{axisHist_ptr->GetMinimum(), axisHist_ptr->GetMaximum(), static_cast<bool>(pad_ptr->GetLogz())}};
    if (IsViewFrame(pad_ptr, axisHist_ptr)) PlaceZTitle(pad_ptr, axisHist_ptr->GetZaxis(), ranges);
    for (TObject* primitive : *pad_ptr->GetListOfPrimitives()) {
      auto* hist = dynamic_cast<TH1*>(primitive);
      if (hist && hist->GetDimension() == 2 && hist->GetListOfFunctions()->FindObject("palette")) PlaceZTitle(pad_ptr, hist->GetZaxis(), ranges);
    }
    pad_ptr->Modified();
  }
  if (flatFrame) {
    // the ranges as drawn, for the axes shared with other pads (pad coordinates of a log axis are decades)
    mFrameRanges[{padID, 'X'}] = {FromPad(pad_ptr->GetUxmin(), pad_ptr->GetLogx()), FromPad(pad_ptr->GetUxmax(), pad_ptr->GetLogx())};
    mFrameRanges[{padID, 'Y'}] = {FromPad(pad_ptr->GetUymin(), pad_ptr->GetLogy()), FromPad(pad_ptr->GetUymax(), pad_ptr->GetLogy())};
    mFrameHists[padID] = axisHist_ptr;
  }
  // the colour axis of a 2d frame is z, its range the minimum and maximum, also in a 3D view (x and y of a view are projected, not shared);
  // the colour axis of a 3d histogram is its contents (C), kept under Z for the z groups (a 3d function has no colour scale)
  if (axisHist_ptr && (axisHist_ptr->GetDimension() == 2 || contentFrame) && !polarFrame && !polargram) {
    mFrameRanges[{padID, 'Z'}] = {axisHist_ptr->GetMinimum(), axisHist_ptr->GetMaximum()};
    mScaleHists[padID] = axisHist_ptr;
    if (contentFrame) mContentScales.insert(padID);
  }

  // the colour scale ROOT made while painting starts 0.005 of the pad width right of the frame and is 0.045 wide: other values
  // (Axis::SetColorScaleOffset/SetColorScaleWidth, which the base plots set from pixels) move it before the boxes are placed
  optional<float_t> scaleOffset;
  optional<float_t> scaleWidth;
  // (the scale of a 3d histogram shows its contents: it is set on the C axis; the defaults of a layout are kept apart from the axes)
  const bool th3Frame = axisHist_ptr && axisHist_ptr->GetDimension() == 3;
  const char scaleAxis = th3Frame ? 'C' : 'Z';
  for (const auto* axis : AxisSettings(pad, padDefaults, scaleAxis)) {
    if (axis->GetColorScaleOffset()) scaleOffset = axis->GetColorScaleOffset();
    if (axis->GetColorScaleWidth()) scaleWidth = axis->GetColorScaleWidth();
  }
  if (!scaleOffset) scaleOffset = LayoutValue(pad, padDefaults, "color_scale_offset");
  if (!scaleWidth) scaleWidth = LayoutValue(pad, padDefaults, "color_scale_width");
  if (scaleOffset || scaleWidth) {
    for (TObject* primitive : *pad_ptr->GetListOfPrimitives()) {
      auto* hist = dynamic_cast<TH1*>(primitive);
      TObject* found = (hist) ? hist->GetListOfFunctions()->FindObject("palette") : nullptr;
      // the colour scale of a 3D view of 2d data stays where ROOT put it; a map projection paints in a view too, but its scale sits beside a
      // flat frame, and so does the scale of a 3d histogram (its contents)
      if (!found || !found->InheritsFrom("TPaletteAxis") || (pad_ptr->GetView() && padProjection.empty() && !th3Frame)) continue;
      auto* palette = static_cast<TPave*>(found);  // TPaletteAxis lives in libHistPainter, which is not linked: its TPave part is enough
      const double_t x1 = 1. - pad_ptr->GetRightMargin() + scaleOffset.value_or(0.005f);
      palette->SetX1NDC(x1);
      palette->SetX2NDC(x1 + scaleWidth.value_or(0.045f));
      palette->ConvertNDCtoPad();
    }
  }

  const bool redrawAxes = get_first_or(false, pad.GetRedrawAxes(), padDefaults.GetRedrawAxes());
  if (polarFrame) {
    // the radial axis of a flat polar drawing: below the disc, from the centre to the right edge (the radius is measured from the centre),
    // with the attributes and the title of the y axis
    TAxis* yAxis = axisHist_ptr->GetYaxis();
    const bool logRadius = pad_ptr->GetLogy();
    // half of ROOT's divisions of an axis spanning the frame, unless the user set them: the radius runs over half of it
    optional<int16_t> divisions;
    for (const auto* axis : AxisSettings(pad, padDefaults, 'Y')) {
      if (axis->GetNumDivisions()) divisions = axis->GetNumDivisions();
    }
    auto* radial = new TGaxis(0.5 * (pad_ptr->GetUxmin() + pad_ptr->GetUxmax()), pad_ptr->GetUymin(), pad_ptr->GetUxmax(), pad_ptr->GetUymin(),
                              FromPad(pad_ptr->GetUymin(), logRadius), FromPad(pad_ptr->GetUymax(), logRadius), divisions.value_or(505), (logRadius) ? "G" : "");
    radial->ImportAxisAttributes(yAxis);
    // the title font is the text font of the TGaxis, which ImportAxisAttributes leaves at its default (its text size is unused: set
    // to the title size, so that font and size agree)
    radial->SetTitleFont(yAxis->GetTitleFont());
    radial->SetTitleSize(yAxis->GetTitleSize());
    radial->SetTextSize(yAxis->GetTitleSize());
    radial->SetName("radial_axis");
    radial->SetBit(kCanDelete);
    radial->Draw();
  } else if (redrawAxes && axisHist_ptr && !pad_ptr->GetView() && !polargram && padProjection.empty()) {  // a map projection has no frame box (ROOT removes it)
    // re-draw frame
    TLine line;
    pad_ptr->GetFrame()->TAttLine::Copy(line);  // only the line attributes: TFrame::Copy would write a whole TWbox into the TLine
    double_t lm = pad_ptr->GetLeftMargin();
    double_t rm = 1. - pad_ptr->GetRightMargin();
    double_t tm = 1. - pad_ptr->GetTopMargin();
    double_t bm = pad_ptr->GetBottomMargin();
    line.DrawLineNDC(rm, bm, rm, tm);
    line.DrawLineNDC(lm, bm, lm, tm);
    line.DrawLineNDC(lm, tm, rm, tm);
    line.DrawLineNDC(lm, bm, rm, bm);
    // re-draw axes; for a frame with colour scale with a copy without it, since ROOT removes the colour scale of a histogram painted
    // without Z (THistPainter::PaintTable) and makes a new one at the default position with the next paint
    TObject* palette = axisHist_ptr->GetListOfFunctions()->FindObject("palette");
    // a frame drawn with HBAR shows its x axis vertically, but ROOT repaints it horizontally (THistPainter::PaintAxis swaps the axes only for
    // alphanumeric labels): the copy gets the look of the axis drawn at each place
    const bool swapAxes = IsHBarFrame(pad_ptr, axisHist_ptr) && !axisHist_ptr->GetXaxis()->GetLabels() && !axisHist_ptr->GetYaxis()->GetLabels();
    TH1* axes = axisHist_ptr;
    if (palette || swapAxes) {
      axes = static_cast<TH1*>(axisHist_ptr->Clone());
      axes->SetDirectory(nullptr);
      axes->SetBit(kCanDelete);
    }
    if (palette) delete axes->GetListOfFunctions()->Remove(axes->GetListOfFunctions()->FindObject("palette"));
    if (swapAxes) {
      for (auto [from, to] : {std::pair{axisHist_ptr->GetYaxis(), axes->GetXaxis()}, std::pair{axisHist_ptr->GetXaxis(), axes->GetYaxis()}}) {
        from->TAttAxis::Copy(*to);
        for (const auto bit : {TAxis::kCenterTitle, TAxis::kRotateTitle, TAxis::kCenterLabels, TAxis::kNoExponent, TAxis::kTickPlus, TAxis::kTickMinus, TAxis::kMoreLogLabels}) {
          to->SetBit(bit, from->TestBit(bit));
        }
        to->SetDecimals(from->GetDecimals());
        to->SetTimeDisplay(from->GetTimeDisplay());
        to->SetTimeFormat(from->GetTimeFormat());
        to->SetTitle(from->GetTitle());
      }
    }
    axes->Draw("SAME AXIS");
  }

  // shapes need the final frame: behind the data right after the frame histogram, the others after the data (and before the boxes, which avoid them)
  const bool canDrawShapes = axisHist_ptr && !pad_ptr->GetView() && !polargram;
  if (!pad.GetShapes().empty() && !canDrawShapes) {
    logger::warning("The shapes of pad {} are not drawn: they need a cartesian axis frame (no 3d view, no polar graphs).", padID);
  } else if (!pad.GetShapes().empty()) {
    DrawShapes(pad_ptr, pad, padDefaults, Plot::Pad::Shape::layer_t::behind_data, axisHist_ptr, padID);
    DrawShapes(pad_ptr, pad, padDefaults, Plot::Pad::Shape::layer_t::after_data, nullptr, padID);
  }

  // legends, texts and images: the ones with a given position before the statistics boxes, the automatically placed ones after them,
  // so that every automatic placement knows the boxes already there
  auto placeBoxes = [&](bool fixedPositions) {
    uint8_t legendIndex{};
    for (const auto& box : pad.GetLegendBoxes()) {
      ++legendIndex;
      if (box->IsAutoPlacement() == fixedPositions || box->HasForeignRows()) continue;  // a legend with rows of other pads comes once all are drawn
      box->MergeLegendEntries(static_cast<uint16_t>(pad.GetData().size()));             // put the rows added by the user between those of the data
      if (!DrawBox(box, pad, padDefaults, pad_ptr, "LegendBox_" + std::to_string(legendIndex))) {
        logger::warning("Legend {} was not added since it is empty.", legendIndex);
      }
    }
    uint8_t textIndex{};
    for (const auto& box : pad.GetTextBoxes()) {
      ++textIndex;
      if (box->IsAutoPlacement() == fixedPositions || box->RefersToOtherPads()) continue;  // a text naming data of other pads comes once all are drawn
      if (!DrawBox(box, pad, padDefaults, pad_ptr, "TextBox_" + std::to_string(textIndex))) {
        logger::warning("Text {} was not added since it is empty.", textIndex);
      }
    }
    uint8_t imageIndex{};
    for (const auto& box : pad.GetImageBoxes()) {
      ++imageIndex;
      if (box->IsAutoPlacement() == fixedPositions) continue;
      DrawImage(*box, get_first_or(frame_coordinates, box->GetCoordinateSystem(), pad.GetDefaultBoxCoordinates(), padDefaults.GetDefaultBoxCoordinates()), pad_ptr, "ImageBox_" + std::to_string(imageIndex));
    }
  };
  placeBoxes(true);

  // statistics boxes, placed after the boxes with a given position and before the automatically placed ones
  for (auto& [stats, statsBox] : statsBoxes) {
    const auto [padWidthPixel, padHeightPixel] = PadSizePixel(pad_ptr);
    auto textWidth = [&, textFont = stats->GetTextFont(), textSize = stats->GetTextSize()](const TString& text) {
      TLatex latex(0, 0, text.Data());
      latex.SetTextFont(textFont);
      latex.SetTextSize(textSize);
      return static_cast<double_t>(std::get<0>(GetTextDimensions(latex, pad_ptr)));
    };
    // TPaveStats::Paint writes the title centered, "label = value" in two columns (the value right-aligned, '-' as #minus)
    // and the under- and overflows of 2D histograms ("a | b | c") in three
    double_t titleWidth{};
    double_t labelWidth{};
    double_t valueWidth{};
    double_t cellWidth{};
    bool isTitle = (stats->GetOptStat() % 10 != 0);
    for (auto* line : *stats->GetListOfLines()) {
      const TString text = line->GetTitle();
      if (isTitle) {
        titleWidth = textWidth(text);
        isTitle = false;
      } else if (const Ssiz_t pos = text.First('='); pos != kNPOS) {
        TString value = text(pos + 1, text.Length());
        value = value.Strip(TString::kBoth);
        value.ReplaceAll("-", "#minus");
        labelWidth = std::max(labelWidth, textWidth(text(0, pos)));
        valueWidth = std::max(valueWidth, textWidth(value));
      } else {
        unique_ptr<TObjArray> cells(text.Tokenize("|"));
        for (auto* cell : *cells) {
          cellWidth = std::max(cellWidth, textWidth(cell->GetName()));
        }
      }
    }
    const double_t lineHeightPixel = TextHeightPixel(stats->GetTextFont(), stats->GetTextSize(), pad_ptr);
    const double_t lineSpacing = statsBox->GetLineSpacing().value_or(0.3);  // as for legends and texts
    const double_t contentWidthPixel = std::max({labelWidth + lineHeightPixel + valueWidth, titleWidth, 3. * cellWidth});
    // the margin of TPaveStats is a fraction of the box width, the one set for a box is in NDC (as for legends and texts)
    double_t widthNDC = (statsBox->GetMargin()) ? contentWidthPixel / padWidthPixel + 2. * *statsBox->GetMargin() : contentWidthPixel / (1. - 2. * stats->GetMargin()) / padWidthPixel;
    double_t heightNDC = (1. + lineSpacing) * lineHeightPixel * stats->GetListOfLines()->GetSize() / padHeightPixel;
    const coordinates_t coordinates = get_first_or(frame_coordinates, statsBox->GetCoordinateSystem(), pad.GetDefaultBoxCoordinates(), padDefaults.GetDefaultBoxCoordinates());
    const auto [unitX, unitY] = SizeUnit(coordinates, pad_ptr);
    if (statsBox->GetWidth()) widthNDC = *statsBox->GetWidth() * unitX;
    if (statsBox->GetHeight()) heightNDC = *statsBox->GetHeight() * unitY;
    if (statsBox->GetMargin()) stats->SetMargin(*statsBox->GetMargin() / widthNDC);

    // the border is drawn centered on the edges of the box, so half of it sticks out on each side
    const double_t borderX = mScale * stats->GetLineWidth() / padWidthPixel;
    const double_t borderY = mScale * stats->GetLineWidth() / padHeightPixel;
    const auto [upperLeftX, upperLeftY] = PlaceBox(*statsBox, coordinates, pad_ptr, widthNDC, heightNDC, borderX, borderY, "statistics box");
    if (!statsBox->IsAutoPlacement()) WarnIfBeyondPad("statistics box", upperLeftX, upperLeftY, widthNDC, heightNDC);
    stats->SetX1NDC(upperLeftX);
    stats->SetY1NDC(upperLeftY - heightNDC);
    stats->SetX2NDC(upperLeftX + widthNDC);
    stats->SetY2NDC(upperLeftY);
    pad_ptr->cd();
    stats->SetUniqueID(coordinates);  // a double-click in show mode reports the position as given
    stats.release()->Draw();
  }

  placeBoxes(false);
  if (canDrawShapes) DrawShapes(pad_ptr, pad, padDefaults, Plot::Pad::Shape::layer_t::on_top, nullptr, padID);

  // viewing angles of 3D drawings: the pad's own, else those of pad 0
  if (auto theta = get_first(pad.GetViewTheta(), padDefaults.GetViewTheta())) {
    pad_ptr->SetTheta(*theta);
  }
  if (auto phi = get_first(pad.GetViewPhi(), padDefaults.GetViewPhi())) {
    pad_ptr->SetPhi(*phi);
  }
  if (pad_ptr->GetView()) {
    anyView = true;
  } else if (pad.GetViewTheta() || pad.GetViewPhi()) {
    ReportIgnored(fmt::format("The view of pad {}", padID), "it draws nothing in 3d");
  }

  const bool hbarFrame = axisHist_ptr && IsHBarFrame(pad_ptr, axisHist_ptr);
  for (auto axisLabel : {'X', 'Y'}) {
    bool hide{};
    for (const auto* axis : AxisSettings(pad, padDefaults, axisLabel)) {
      if (axis->GetHideClippedLabels()) hide = *axis->GetHideClippedLabels();
    }
    if (hide && axisHist_ptr && !polarFrame && !polargram) HideClippedLabels(pad_ptr, ((axisLabel == 'X') != hbarFrame) ? axisHist_ptr->GetXaxis() : axisHist_ptr->GetYaxis(), axisLabel);
  }
  // the numbers of a linear x axis without exponent (the shipped look: no factor at the axis end, where a touching panel or a colour
  // scale would be) run into each other for wide or offset ranges: said, with the remedy
  if (axisHist_ptr && !polarFrame && !polargram && !pad_ptr->GetView() && !pad_ptr->GetLogx()) {
    TAxis* xAxis = (hbarFrame) ? axisHist_ptr->GetYaxis() : axisHist_ptr->GetXaxis();
    if (xAxis->GetNoExponent() && !xAxis->GetLabels() && xAxis->GetLabelSize() > 0.f) {
      const vector<double_t> values = LabelValues(pad_ptr, xAxis, 'X');
      const double_t pixelsPerUnit = PadSizePixel(pad_ptr).first * (1. - pad_ptr->GetLeftMargin() - pad_ptr->GetRightMargin()) / (pad_ptr->GetUxmax() - pad_ptr->GetUxmin());
      // the labels drawn (HideClippedLabels hides some with size 0, by value or by number), each pair of neighbours by their own widths
      const auto isHidden = [&](size_t index) {
        if (!xAxis->GetModifiedLabels()) return false;
        for (auto* change : TRangeDynCast<TAxisModLab>(xAxis->GetModifiedLabels())) {
          if (!change || change->GetSize() != 0.) continue;
          const int32_t number = change->GetLabNum();
          if (number != 0 && static_cast<int64_t>(index) == ((number > 0) ? number - 1 : static_cast<int64_t>(values.size()) + number)) return true;
          if (number == 0 && std::abs(change->GetLabValue() - values[index]) < 1e-9 * (std::abs(values[index]) + 1.)) return true;
        }
        return false;
      };
      bool overlap{};
      double_t previousValue{}, previousWidth{-1.};
      for (size_t index = 0; index < values.size() && !overlap; ++index) {
        if (isHidden(index)) continue;
        const double_t width = MaxLabelWidth(pad_ptr, xAxis, {values[index]}, false);
        if (previousWidth >= 0. && 0.5 * (width + previousWidth) > (values[index] - previousValue) * pixelsPerUnit) overlap = true;
        previousValue = values[index];
        previousWidth = width;
      }
      if (overlap) {
        if (mReportedIgnored.insert("overlapping x labels").second) {
          logger::warning("The labels of the X axis in pad {} run into each other: fewer divisions (SetNumDivisions), or SetNoExponent(false) for ROOT's power of ten (with room for it at the end of the axis, e.g. a right margin).", padID);
        }
      }
    }
  }
  if (axisHist_ptr && !polarFrame && !polargram && !hbarFrame && !pad_ptr->GetView()) WarnIfOutsideFrame(pad_ptr);
}

//**************************************************************************************************
/**
 * Draws the data (its appearance is already final) with the given drawing option alias.
 */
//**************************************************************************************************
template <typename T>
void PlotPainter::DrawItem(item_state_t& state, T data_ptr, optional<draw_option_t> optionAlias)
{
  pad_state_t& padState = state.padState;
  auto& data = state.data;
  auto& padDefaults = padState.padDefaults;
  auto& pad = padState.pad;
  auto& pad_ptr = padState.pad_ptr;
  auto& padSetup = padState.padSetup;
  auto& textFont = padState.textFont;
  auto& textSize = padState.textSize;
  auto& activeTextFormat = padState.activeTextFormat;
  auto& hasRefFunc = padState.hasRefFunc;
  auto& axisHist_ptr = padState.axisHist_ptr;
  auto& polargram = padState.polargram;
  auto& padProjection = padState.padProjection;
  auto& statsBoxes = padState.statsBoxes;
  auto& drawOption = padState.drawOption;
  auto& dataIndex = padState.dataIndex;
  auto& mapDrawn = padState.mapDrawn;
  auto& storedRangeZ = padState.storedRangeZ;

  using data_type = std::decay_t<decltype(data_ptr)>;
  draw_option_t alias = optionAlias.value_or(static_cast<draw_option_t>(255));  // 255: no alias
  if constexpr (is_graph_2d<data_type>() || is_func_2d<data_type>()) {
    if (!data->GetDrawOption() && !optionAlias) drawOption += "COLZ";  // without an option coloured like 2d histograms
  }
  if (optionAlias) {
    const std::map<draw_option_t, string>* options{};
    const char* typeName{};
    if constexpr (is_hist_2d<data_type>()) {
      options = &defaultDrawOptions_Hist2d;
      typeName = "2d histogram";
    } else if constexpr (is_hist_3d<data_type>()) {
      options = &defaultDrawOptions_Hist3d;
      typeName = "3d histogram";
    } else if constexpr (is_hist_1d<data_type>()) {
      options = &defaultDrawOptions_Hist;
      typeName = "1d histogram";
    } else if constexpr (is_graph_1d<data_type>()) {
      options = &defaultDrawOptions_Graph;
      typeName = "graph";
    } else if constexpr (is_graph_2d<data_type>()) {
      options = &defaultDrawOptions_Graph2d;
      typeName = "2d graph";
    } else if constexpr (is_func_1d<data_type>()) {
      options = &defaultDrawOptions_Func;
      typeName = "function";
    } else if constexpr (is_func_2d<data_type>()) {
      options = &defaultDrawOptions_Func2d;
      typeName = "2d function";
    } else if constexpr (is_func_3d<data_type>()) {
      options = &defaultDrawOptions_Func3d;
      typeName = "3d function";
    }
    if (auto it = (options) ? options->find(*optionAlias) : decltype(options->end()){}; options && it != options->end()) {
      drawOption += it->second;
    } else {
      if (dataIndex != 0) logger::warning("Drawing option {} is not available for {} {}, using the default of ROOT.", draw_option_name(*optionAlias), (options) ? typeName : data_ptr->ClassName(), data->GetName());
      alias = static_cast<draw_option_t>(255);
    }
  }

  // TGraph::Draw clears the pad when the option contains an 'A' (new axes) outside of SAME, which would delete the frame
  // (and the frame drawn with it has no axes): the axes are SciRooPlot's; RX and RY mirror the points and let TGraphPainter draw
  // reversed axes in place of the frame's, so in SciRooPlot's frame the points would sit at wrong axis values
  // (the polar grid takes N and O of a polar graph, see below)
  string polarGridOption;
  if constexpr (is_graph_1d<data_type>()) {
    TString option(drawOption);
    option.ToLower();  // as TGraph::Draw reads it
    option.ReplaceAll("same", "");
    const bool isPolar = data_ptr->InheritsFrom(TGraphPolar::Class());  // its own letters (R, D, G: unit of the angle)
    const bool hasAxes = option.Contains("a");
    const bool reversed = !isPolar && (option.Contains("rx") || option.Contains("ry"));
    if (isPolar) {
      // N (no angle labels) and O (angle labels along the radius) are options of the polar grid, which the first polar graph of
      // the pad makes (TGraphPainter::PaintGraphPolar); without other letters ROOT draws the graph as line, as for an empty option
      // (the SAME of SciRooPlot would leave a blank option, which draws nothing)
      for (const char* letter : {"n", "o"}) {
        if (option.Contains(letter)) polarGridOption += letter;
      }
      if (!polarGridOption.empty()) {
        option.ReplaceAll("n", "").ReplaceAll("o", "");
        if (option.Strip(TString::kBoth).IsNull()) option += "l";
        if (!hasAxes) drawOption = ((dataIndex != 0) ? "SAME " : "") + string(option.Data());
      }
    }
    if (hasAxes || reversed) {
      if (dataIndex != 0 && hasAxes) logger::warning("Drawing option '{}' of {}: A (new axes) is removed, the axes of the pad are drawn by SciRooPlot.", *data->GetDrawOption(), data->GetName());
      if (dataIndex != 0 && reversed) logger::warning("Drawing option '{}' of {}: RX and RY (reversed axes) are removed, SciRooPlot cannot reverse the axes of the frame.", *data->GetDrawOption(), data->GetName());
      if (reversed) option.ReplaceAll("rx", "").ReplaceAll("ry", "");
      option.ReplaceAll("a", "");
      drawOption = ((dataIndex != 0) ? "SAME " : "") + string(option.Data());
    }
  }

  // the angle of the values written by TEXT (Data::SetTextAngle) is part of ROOT's option: TEXTnn, whole degrees from 0 to 90
  // (an angle given in the option string itself is kept); points_text writes its values itself, in the same range
  if (const auto& textAngle = data->GetTextAngle()) {
    if (dataIndex != 0 && (*textAngle < 0.f || *textAngle > 90.f)) logger::warning("The text angle {} of {} is outside of the 0 to 90 degrees ROOT writes values at: it is drawn at the nearest end.", *textAngle, data->GetName());
    if (alias != points_text) {
      TString upper(drawOption);
      upper.ToUpper();
      const Ssiz_t pos = upper.Index("TEXT");
      if (pos == kNPOS) {
        if (dataIndex != 0) logger::warning("The text angle of {} has no effect: it is drawn without values (drawing option text or points_text).", data->GetName());
      } else if (pos + 4 == upper.Length() || !std::isdigit(static_cast<unsigned char>(upper[pos + 4]))) {
        drawOption.insert(static_cast<size_t>(pos) + 4, std::to_string(std::lround(std::clamp(*textAngle, 0.f, 90.f))));
      }
    }
  }

  // a 2d histogram drawn as a map: ROOT's projection options select CONT4, which paints in projected coordinates, so data and frame
  // fit together only when they cover the same range (data drawn SAME is projected over its own range into the frame of the first)
  bool projected{};
  string projection;  // the projection keyword
  if constexpr (is_hist_2d<data_type>()) {
    TString upper(drawOption);
    upper.ToUpper();
    // ROOT paints a TH2Poly in 3d only with OpenGL (GLLEGO): LEGO and SURF paint the empty cell array of TH2 instead of the bins
    // (a flat colour map in a 3d frame lands at the wrong place, so there it is not drawn at all)
    if (data_ptr->InheritsFrom(TH2Poly::Class()) && !upper.Contains("GL")) {
      if (dataIndex != 0 && pad_ptr->GetView()) {
        logger::warning("TH2Poly {} cannot be drawn in the 3d frame of its pad (ROOT draws polygon bins in 3d only with OpenGL). It is not drawn.", data->GetName());
        delete data_ptr;
        ++dataIndex;  // the next data keeps its number
        drawOption = "SAME ";
        return;
      }
      if (upper.Contains("LEGO") || upper.Contains("SURF")) {
        const char* scale = (upper.Contains("Z")) ? "Z" : "";
        if (dataIndex != 0) logger::warning("Drawing option {} is not available for TH2Poly {} (ROOT draws polygon bins in 3d only with OpenGL), drawing it as col{}.", (optionAlias) ? draw_option_name(*optionAlias) : data->GetDrawOption().value_or(""), data->GetName(), (*scale) ? "z" : "");
        drawOption = ((dataIndex != 0) ? "SAME COL" : "COL") + string(scale);
        alias = (*scale) ? colz : col;
        upper = drawOption;
      }
    }
    for (const char* keyword : {"AITOFF", "MERCATOR", "SINUSOIDAL", "PARABOLIC", "MOLLWEIDE"}) {
      if (upper.Contains(keyword)) projection = keyword;
    }
    projected = !projection.empty();
  }
  if (dataIndex == 0) padProjection = projection;

  // appearances that ROOT has no drawing option for are prepared on the copy that is drawn
  if constexpr (is_graph_2d<data_type>()) {
    // the points of a 2d graph with errors are drawn with their error bars (as the points of 1d data)
    if ((alias == points || alias == points_col || alias == points_colz) && (data_ptr->InheritsFrom(TGraph2DErrors::Class()) || data_ptr->InheritsFrom(TGraph2DAsymmErrors::Class()))) drawOption += " ERR";
  }
  if constexpr (is_graph_1d<data_type>()) {
    // a polar graph draws its error bars only with E (TGraphPainter::PaintGraphPolar), the other graphs with every points alias
    const bool pointsAlias = alias == points || alias == points_xerr || alias == points_endcaps || alias == points_xerr_endcaps || alias == points_line || alias == points_text || alias == points_arrows;
    if (pointsAlias && data_ptr->InheritsFrom(TGraphPolar::Class())) drawOption += " E";
    if (alias == points || alias == points_endcaps || alias == points_line || alias == points_text || alias == points_arrows) {
      // x errors are only drawn with the _xerr aliases (as for histograms with X0)
      for (double_t* ex : {data_ptr->GetEX(), data_ptr->GetEXlow(), data_ptr->GetEXhigh()}) {  // symmetric or asymmetric errors
        if (ex) std::fill(ex, ex + data_ptr->GetN(), 0.);
      }
    } else if (alias == area && data_ptr->GetN() > 0 && dataIndex != 0) {
      // the data range applies to the points, not to the polygon closed at zero below (its corners would be removed with them);
      // not to the copy that draws the frame, as for all other aliases
      if (!pad_ptr->GetView()) RemoveGraphPointsOutside(static_cast<TGraph*>(data_ptr), data->GetMinRangeX(), data->GetMaxRangeX(), data->GetMinRangeY(), data->GetMaxRangeY());
    }
    if (alias == area && data_ptr->GetN() > 0) {
      // the area between the points and zero (F would close the polygon from the last point to the first)
      auto* polygon = new TGraph(data_ptr->GetN() + 2);
      polygon->SetName(data_ptr->GetName());
      polygon->SetTitle(data_ptr->GetTitle());
      polygon->SetBit(kCanDelete);  // owned by the pad, as the copy it replaces
      CopyStyle(*data_ptr, *polygon);
      // the area below the points needs them in the order of x: points in another order (a closed contour, a trajectory) would change their shape
      const double_t* x = data_ptr->GetX();
      if (dataIndex != 0 && !std::is_sorted(x, x + data_ptr->GetN())) logger::warning("The points of graph {} are not in the order of x: they are sorted to draw the area below them.", data->GetName());
      SortGraph(data_ptr);
      polygon->SetPoint(0, data_ptr->GetPointX(0), 0.);
      for (int32_t i = 0; i < data_ptr->GetN(); ++i) {
        polygon->SetPoint(i + 1, data_ptr->GetPointX(i), data_ptr->GetPointY(i));
      }
      polygon->SetPoint(data_ptr->GetN() + 1, data_ptr->GetPointX(data_ptr->GetN() - 1), 0.);
      // the histogram it was made from (statistics box, placeholders) belongs to the polygon now: a later graph at the freed address is another one
      if (auto source = mSourceHists.extract(data_ptr)) {
        source.key() = polygon;
        mSourceHists.insert(std::move(source));
      }
      delete data_ptr;
      data_ptr = polygon;
    }
  }

  // first data is only used to define the axes
  if (dataIndex == 0) {
    SetUpFrame(state, data_ptr, polarGridOption);
  } else {
    // data drawn in 3d needs the 3d frame of a first data drawn in 3d, flat data a flat frame: ROOT paints a 3d view over a flat map
    // (its box over the frame) and a flat drawing in a 3d frame not at all or at the wrong place; likewise a map projection needs a
    // frame of the same projection (flat data in a projected frame, or a map in an unprojected one, is painted in other coordinates)
    if constexpr (is_2d<data_type>() || is_3d<data_type>()) {
      bool drawn3d = is_3d<data_type>() || IsView3d(drawOption);
      if constexpr (is_graph_2d<data_type>()) {
        bool triangles{}, points{};
        Paints3dGraph(drawOption, triangles, points);
        drawn3d |= triangles || points;
      }
      if (drawn3d != (pad_ptr->GetView() != nullptr)) {
        TString option = (optionAlias) ? TString(draw_option_name(*optionAlias).data()) : TString(drawOption).ReplaceAll("SAME", "");
        option = option.Strip(TString::kBoth);
        if (drawn3d) {
          logger::warning("Data {} is drawn in 3d ({}) and needs a 3d frame: the first data of the pad drawn in 3d (e.g. lego, surf, triangles or points). It is not drawn.", data->GetName(), option.Data());
        } else {
          logger::warning("Data {} is drawn flat ({}) and cannot be drawn in the 3d frame of its pad (the first data of the pad is drawn in 3d). It is not drawn.", data->GetName(), option.Data());
        }
        delete data_ptr;
        ++dataIndex;  // the next data keeps its number
        drawOption = "SAME ";
        return;
      }
      if (projection != padProjection) {
        auto drawn = [](const string& keyword) { return (keyword.empty()) ? string("without a map projection") : "as a map (" + keyword + ")"; };
        logger::warning("Data {} is drawn {} and cannot be drawn in its pad, whose first data is drawn {}. It is not drawn.", data->GetName(), drawn(projection), drawn(padProjection));
        delete data_ptr;
        ++dataIndex;  // the next data keeps its number
        drawOption = "SAME ";
        return;
      }
    }
    // do not draw the colour scale a second time (only types that have one: for graphs 'Z' means no end caps on the error bars)
    // the 'Z' is removed, not blanked, so that suffixes stay attached to their option (COLZ2 -> COL2)
    if constexpr (is_2d<data_type>() || is_3d<data_type>()) {
      drawOption.erase(std::remove_if(drawOption.begin(), drawOption.end(), [](char c) { return c == 'Z' || c == 'z'; }), drawOption.end());
    }

    ApplyDataStyle(state, data_ptr, alias);

    // data ranges
    const bool hasRangeX = data->GetMinRangeX() || data->GetMaxRangeX();
    const bool hasRangeY = data->GetMinRangeY() || data->GetMaxRangeY();
    DataRanges(state, data_ptr, alias, projected, hasRangeX, hasRangeY);
    // colour levels: the frame, whose colour scale is drawn (and from which COL drawn SAME takes its colours), takes those of the first
    // map of the pad (a 2d or 3d histogram, a 2d graph or a 2d function), the one it is made from, also fewer than ROOT's 20, so that the
    // scale shows the levels of the map; levels of later data (e.g. contour lines over a map) are their own, the frame takes them only if they are more
    if constexpr (!is_2d<data_type>() && !is_3d<data_type>()) {
      if (data->GetContours() || data->GetNContours()) ReportIgnored(fmt::format("The contours of {}", data->GetName()), "only 2d and 3d data has colour levels");
    }
    if constexpr (is_2d<data_type>() || is_hist_3d<data_type>()) {
      const bool firstMap = !mapDrawn;
      mapDrawn = true;
      const auto& contours = data->GetContours();
      const int32_t nLevels = contours ? static_cast<int32_t>(contours->size()) : data->GetNContours().value_or(0);
      if (nLevels > 0) {
        if constexpr (!is_graph_2d<data_type>()) {  // a 2d graph gets them when painted (below)
          if (contours) {
            data_ptr->SetContour(nLevels, contours->data());
          } else {
            data_ptr->SetContour(nLevels);
          }
        }
        if (firstMap || axisHist_ptr->GetContour() < nLevels) {
          if (contours) {
            axisHist_ptr->SetContour(nLevels, contours->data());
          } else {
            axisHist_ptr->SetContour(nLevels);
          }
        }
        if constexpr (is_hist_3d<data_type>()) {
          if (firstMap) {
            // ROOT colours the boxes of a 3d histogram from the whole palette, linearly in the content (THistPainter::PaintH3Box), and only
            // the colour scale from the levels: the pad gets a palette with one colour per level, the one the scale shows for it (TPaletteAxis:
            // level j has colour (j + 0.99) * nColors / nLevels), each repeated 200 times, since a box takes colour nColors * fraction - 1
            // (with fewer copies the boxes would be one level off)
            if (contours) logger::warning("Colour levels of {}: the boxes of a 3d histogram are coloured in {} equal steps, not at the levels given.", data->GetName(), nLevels);
            padSetup += fmt::format(
              " {{ const Int_t n0 = TColor::GetNumberOfColors(), nb = {}, k = 200; std::vector<Int_t> c(nb * k);"
              " for (Int_t i = 0; i < nb * k; ++i) c[i] = TColor::GetColorPalette(Int_t((std::min(nb - 1, (i + 1) / k) + 0.99) * n0 / nb));"
              " gStyle->SetPalette(nb * k, c.data()); }}",
              nLevels);
          }
        }
      }
    }
    auto drawExec = [&](const string& name, const string& command) {
      TExec* exec = new TExec(name.data(), command.data());
      exec->SetBit(kCanDelete);
      exec->Draw();
    };
    if (const string textFormat = data->GetTextFormat().value_or("g"); textFormat != activeTextFormat) {
      string quoted;  // the format goes into interpreted code
      for (const char c : textFormat) {
        if (c == '"' || c == '\\') quoted += '\\';
        quoted += c;
      }
      drawExec("text_format_" + std::to_string(dataIndex), fmt::format("gStyle->SetPaintTextFormat(\"{}\");", quoted));
      activeTextFormat = textFormat;
    }

    // disallow moving around the points of a graph in interactive mode
    if constexpr (is_graph_1d<data_type>()) {
      data_ptr->SetEditable(false);
    }
    bool showStats{false};
    TH1* statsHist{nullptr};  // the drawn histogram, or the one a graph was converted from
    if constexpr (is_hist<data_type>()) {
      statsHist = data_ptr;
    } else if constexpr (is_graph_1d<data_type>()) {
      if (auto source = mSourceHists.find(data_ptr); source != mSourceHists.end()) {
        statsHist = source->second.get();
      }
    }
    if (statsHist) {
      statsHist->SetStats(false);  // also removes a stats box stored with the input, which shows the numbers of the input instead of the drawn histogram
      showStats = data->GetStatsFormat() && (!data->GetStatsFormat()->empty() || data->GetStatsTitle());
      if constexpr (is_hist<data_type>()) {
        if (showStats && str_starts_with(drawOption, "SAME")) drawOption.insert(4, "S");  // ROOT fills a stats box only with SAMES, not with SAME
      }
      if (!(data->GetShowOverflowBins() && *data->GetShowOverflowBins())) {
        // TH2Poly keeps its bins outside the cell array of TH1, whose loop over the global cells would zero the real bins;
        // its overflow regions are never drawn anyway
        bool isPoly{false};
        if constexpr (is_hist_2d<data_type>()) isPoly = data_ptr->InheritsFrom(TH2Poly::Class());
        if (!isPoly) statsHist->ClearUnderflowAndOverflow();
      }
    } else if (data->GetStatsFormat()) {
      logger::warning("Statistics boxes are available only for histograms, not for {}.", data->GetName());
    }
    // named as in the plot definition (<name>), without data source, request and projection
    data_ptr->SetName((std::to_string(dataIndex - hasRefFunc) + ":" + data_ptr->GetName()).data());
    mDataDefinitions[data_ptr] = data;
    if constexpr (is_graph_2d<data_type>()) {
      // ROOT paints a 2d graph through a histogram it makes when painting and does not store with the graph: it gets the colour levels at every paint
      const auto& contours = data->GetContours();
      if (contours ? !contours->empty() : data->GetNContours().value_or(0) > 0) {
        string levels = (contours) ? fmt::format("{}, std::vector<Double_t>{{", contours->size()) : std::to_string(*data->GetNContours());
        if (contours) {
          for (const double_t level : *contours)
            levels += fmt::format("{}, ", level);
          levels += "}.data()";
        }
        string quoted;  // the name goes into interpreted code
        for (const char c : string(data_ptr->GetName())) {
          if (c == '"' || c == '\\') quoted += '\\';
          quoted += c;
        }
        drawExec("levels_" + std::to_string(dataIndex - hasRefFunc),
                 fmt::format("if (auto* graph = dynamic_cast<TGraph2D*>(gPad->GetPrimitive(\"{}\"))) graph->GetHistogram()->SetContour({});", quoted, levels));
      }
    }
    if constexpr (!is_hist<data_type>()) {
      if (statsHist) {
        statsHist->SetName(data_ptr->GetName());  // for <name> and the title line of the statistics box
        mDataDefinitions[statsHist] = data;
      }
    }
    if constexpr (is_hist<data_type>()) {
      // an interactive zoom sets its range on all histograms of the pad (and unzooming shows all their bins):
      // the setup of the pad, which runs at every paint, restores the range of the data
      string keepRange;
      if (hasRangeX && !pad_ptr->GetView()) {
        keepRange += fmt::format(" h->GetXaxis()->SetRange({}, {});", data_ptr->GetXaxis()->GetFirst(), data_ptr->GetXaxis()->GetLast());
      }
      if constexpr (is_hist_2d<data_type>() || is_hist_3d<data_type>()) {
        if (hasRangeY && !pad_ptr->GetView()) {
          keepRange += fmt::format(" h->GetYaxis()->SetRange({}, {});", data_ptr->GetYaxis()->GetFirst(), data_ptr->GetYaxis()->GetLast());
        }
      } else {
        if (data->GetMinRangeY()) keepRange += fmt::format(" h->SetMinimum({});", *data->GetMinRangeY());
        if (data->GetMaxRangeY()) keepRange += fmt::format(" h->SetMaximum({});", *data->GetMaxRangeY());
      }
      if (!keepRange.empty()) padSetup += HistCommand(data_ptr->GetName(), keepRange);
    }
    bool hasFilledSteps{false};  // a histogram drawn with hist_clipped or hist_open_clipped and a fill: the steps are the outline of the filled areas
    if constexpr (is_graph_1d<data_type>()) {
      if ((alias == hist_clipped || alias == hist_open_clipped) && data_ptr->GetFillStyle() != 0 && data_ptr->GetFillColor() != 0 && data_ptr->GetN() > 0) {
        // the polygon of the steps closed far below the frame (at ROOT's base line once the frame is final): ROOT cuts it at the frame
        vector<double_t> x(data_ptr->GetX(), data_ptr->GetX() + data_ptr->GetN());
        vector<double_t> y(data_ptr->GetY(), data_ptr->GetY() + data_ptr->GetN());
        if (alias == hist_open_clipped) {  // the steps of hist_clipped already go down there at the outer edges
          const double_t base = ClippedBase(data_ptr, pad_ptr->GetLogy());
          x.insert(x.begin(), x.front());
          y.insert(y.begin(), base);
          x.push_back(x.back());
          y.push_back(base);
        }
        auto* polygon = new TGraph(static_cast<int32_t>(x.size()), x.data(), y.data());
        polygon->SetName((string("fill_") + data_ptr->GetName()).data());  // not "<number>:...", so legend rows refer to the steps
        data_ptr->TAttLine::Copy(*polygon);
        data_ptr->TAttFill::Copy(*polygon);
        polygon->SetEditable(false);
        polygon->SetBit(kCanDelete);
        polygon->Draw("SAME F");
        hasFilledSteps = true;
      }
    }
    // ROOT maps the colours of 2d data drawn on top with the range of the frame, which also draws the colour axis:
    // at each end without a z range set by the user, that range covers all 2d data of the pad (not only the data the frame is made of)
    if constexpr (is_hist_2d<data_type>() || is_graph_2d<data_type>() || is_func_2d<data_type>()) {
      const optional<double_t> userMinZ = MinRange(pad, padDefaults, 'Z');
      optional<double_t> userMaxZ;
      for (const auto* axis : AxisSettings(pad, padDefaults, 'Z')) {
        if (axis->GetMaxRange()) userMaxZ = axis->GetMaxRange();
      }
      bool isEmpty{};
      if constexpr (is_graph_2d<data_type>()) isEmpty = data_ptr->GetN() == 0;  // not drawn (and ROOT reads its first point)
      if (!(userMinZ && userMaxZ) && !isEmpty && axisHist_ptr->GetDimension() == 2 && !pad_ptr->GetView()) {
        double_t minimum{}, maximum{};
        if constexpr (is_hist_2d<data_type>()) {
          minimum = data_ptr->GetMinimum();
          maximum = data_ptr->GetMaximum();
        } else if constexpr (is_func_2d<data_type>()) {
          const TH1* samples = data_ptr->GetHistogram();  // the values ROOT paints
          minimum = samples->GetMinimum();
          maximum = samples->GetMaximum();
        } else {
          minimum = data_ptr->GetZmin();
          maximum = data_ptr->GetZmax();
        }
        if (!userMaxZ && !storedRangeZ.second && maximum > axisHist_ptr->GetMaximum()) axisHist_ptr->SetMaximum(maximum);
        if (!userMinZ && !storedRangeZ.first && !pad_ptr->GetLogz() && minimum < axisHist_ptr->GetMinimum()) axisHist_ptr->SetMinimum(minimum);
      }
    }
    if constexpr (is_graph_2d<data_type>()) {
      // the points are painted from an empty histogram of the graph, unless it already has one: then its contents (interpolated e.g.
      // by SetMinimum above) are painted as well, as colours behind PCOL: the contents are cleared
      bool triangles{}, points{};
      Paints3dGraph(drawOption, triangles, points);
      if (points && !triangles) data_ptr->GetHistogram()->Reset("ICE");  // not its functions: they hold the triangulation
      // painted as triangles or points, a 2d graph fills the pad behind the 3d box with the frame fill colour (TView3D::PadRange), also
      // drawn after other data: everything painted before it would be covered
      if (triangles || points) pad_ptr->SetFrameFillColor(0);
    }
    if (projected) {
      // ROOT projects the range of a map only when it is painted without SAME (THistPainter::RecalculateRange): drawn SAME, a map
      // that does not cover the whole sphere is painted over its unprojected range into the projected frame (too small, the frame
      // showing around it). Added to the pad without SAME (Draw would clear the pad), it takes the projected range of the frame.
      TString option(drawOption);
      option.ReplaceAll("SAMES", "").ReplaceAll("SAME", "");
      data_ptr->SetBit(TH1::kNoTitle);
      data_ptr->AppendPad(option.Data());
    } else if constexpr (is_graph_1d<data_type>() || is_graph_2d<data_type>()) {
      // ROOT reports an empty graph as an error at every paint
      if (data_ptr->GetN() == 0 && mSourceHists.count(data_ptr)) {
        // a histogram without filled bins drawn as a graph: nothing to paint, but its legend row and statistics box, as drawn as histogram
        mNotPainted[{pad_ptr, static_cast<uint16_t>(dataIndex - hasRefFunc)}] = {data_ptr, drawOption};
        mUndrawn.emplace_back(data_ptr);
      } else if (data_ptr->GetN() == 0) {
        logger::warning("Graph {} has no points: it is not drawn.", data->GetName());
        mUndrawn.emplace_back(data_ptr);
        // no legend row and no statistics box either, as for other data that is not drawn (TH2Poly in 3d)
        ++dataIndex;  // the next data keeps its number
        drawOption = "SAME ";
        return;
      } else {
        if constexpr (is_graph_1d<data_type>()) {
          // a graph with several sets of y errors draws them as the parts of the option after ';' say, one per set, the first part being
          // the graph itself (TGraphPainter::PaintGraphMultiErrors): an alias gives each set the error style it gives other graphs
          auto* multi = dynamic_cast<TGraphMultiErrors*>(data_ptr);
          if (multi && multi->GetNYErrors() > 0 && alias != static_cast<draw_option_t>(255) && drawOption.find(';') == string::npos) {
            string global, errors;
            for (char c : drawOption) {
              const bool errorStyle = (c >= '2' && c <= '5');                                     // boxes and bands
              const bool both = std::string_view("XxZz[]|> ").find(c) != std::string_view::npos;  // no errors, no end lines, brackets, arrows
              if (!errorStyle) global += c;
              if (errorStyle || both) errors += c;
            }
            drawOption = global;
            for (int32_t set = 0; set < multi->GetNYErrors(); ++set)
              drawOption += ";" + errors;  // one part per set (with fewer, ROOT takes the first part also for the first set)
          }
        }
        data_ptr->Draw(drawOption.data());
      }
    } else {
      data_ptr->Draw(drawOption.data());
    }
    if (showStats) {
      // ROOT makes the box when painting a histogram: one drawn as graph is drawn for this paint only
      if constexpr (!is_hist<data_type>()) statsHist->Draw("HIST SAMES");
      // ROOT fills the box with the global setting when painting the histogram: the requested one is set for this paint only
      // (with gPad hidden, since TStyle::SetOptStat would otherwise make all stats boxes of the pad follow the global setting and repaint the pad)
      auto setOptStat = [](auto mode) {
        TVirtualPad::TContext noPad(nullptr);
        gStyle->SetOptStat(mode);
      };
      const Int_t userOptStat = gStyle->GetOptStat();
      setOptStat((*data->GetStatsFormat() + ((data->GetStatsTitle()) ? "n" : "")).data());
      statsHist->SetStats(true);
      pad_ptr->Modified();
      pad_ptr->Update();
      setOptStat(userOptStat);
      if constexpr (!is_hist<data_type>()) pad_ptr->GetListOfPrimitives()->Remove(statsHist);
      // the box becomes a plain object of the pad with fixed content: so it does not depend on the global setting when painted again
      // and is not refilled from another histogram (the web canvas does so for boxes called "stats")
      if (auto* stats = dynamic_cast<TPaveStats*>(statsHist->GetListOfFunctions()->FindObject("stats"))) {
        statsHist->GetListOfFunctions()->Remove(stats);
        stats->SetParent(nullptr);
        statsHist->SetStats(false);
        stats->SetName(("stats_" + std::to_string(dataIndex - hasRefFunc)).data());
        if (const auto& title = data->GetStatsTitle()) {
          string titleText = *title;
          ReplacePlaceholders(titleText, statsHist);
          DropEmptyGroups(titleText);
          static_cast<TText*>(stats->GetListOfLines()->First())->SetTitle(titleText.data());  // instead of the name of the histogram
        }
        // without settings for the box: placed automatically, framed in the line colour of the data
        auto statsBox = (data->GetStatsBox()) ? data->GetStatsBox() : std::make_shared<const Plot::Pad::StatsBox>();
        stats->SetTextFont(statsBox->GetTextFont().value_or(textFont.value_or(43)));
        stats->SetTextSize(statsBox->GetTextSize().value_or(textSize.value_or(24.f)));
        if (statsBox->GetTextColor()) stats->SetTextColor(statsBox->GetTextColor()->ToIndex());
        if (statsBox->GetTextAlpha()) stats->SetTextColor(TColor::GetColorTransparent(stats->GetTextColor(), *statsBox->GetTextAlpha()));
        stats->SetLineColor(data_ptr->GetLineColor());
        stats->SetLineWidth(1);
        stats->SetBorderSize(1);
        stats->SetFillStyle(0);
        ApplyBoxStyle(*statsBox, stats);
        statsBoxes.emplace_back(stats, statsBox);
      } else {
        logger::warning("ROOT did not create a statistics box for {}.", data->GetName());
      }
    }
    if (alias == points_text) {
      // the values are written above the error bars (ROOT draws the bin contents of a histogram only when its errors are not drawn)
      TLatex label;
      // tilted as the values ROOT writes with TEXTnn (THistPainter::PaintText): from the point upwards, centred above it when upright
      const float_t textAngle = std::clamp(data->GetTextAngle().value_or(0.f), 0.f, 90.f);
      label.SetTextAngle(textAngle);
      label.SetTextAlign((textAngle == 0.f) ? 21 : ((textAngle == 90.f) ? 12 : 11));
      label.SetTextColor(data_ptr->GetMarkerColor());
      if (textFont) label.SetTextFont(*textFont);
      if (textSize) label.SetTextSize(*textSize);
      const string format = "%" + data->GetTextFormat().value_or("g");
      // a small gap between the end of the error bar and the value, so the text does not sit on the bar
      const double_t gapY = 0.015 * (pad_ptr->GetUymax() - pad_ptr->GetUymin());
      auto drawLabel = [&](double_t x, double_t y, double_t value) {
        const bool logY = pad_ptr->GetLogy();
        if (!logY || y > 0.) y = FromPad(((logY) ? std::log10(y) : y) + gapY, logY);
        label.DrawLatex(x, y, TString::Format(format.data(), value).Data());
      };
      // only drawn points get a value: the labels, unlike the markers, are not clipped at the frame
      const double_t frameMinX = FromPad(pad_ptr->GetUxmin(), pad_ptr->GetLogx());
      const double_t frameMaxX = FromPad(pad_ptr->GetUxmax(), pad_ptr->GetLogx());
      const double_t frameMinY = FromPad(pad_ptr->GetUymin(), pad_ptr->GetLogy());
      const double_t frameMaxY = FromPad(pad_ptr->GetUymax(), pad_ptr->GetLogy());
      auto inFrame = [&](double_t x, double_t y) { return x >= frameMinX && x <= frameMaxX && y >= frameMinY && y <= frameMaxY; };
      if constexpr (is_hist_1d<data_type>()) {
        // the drawn bins (data range, cut at the frame) and the y range of the data (its minimum and maximum)
        const double_t minY = (data_ptr->GetMinimumStored() != -1111) ? data_ptr->GetMinimumStored() : -std::numeric_limits<double_t>::infinity();
        const double_t maxY = (data_ptr->GetMaximumStored() != -1111) ? data_ptr->GetMaximumStored() : std::numeric_limits<double_t>::infinity();
        for (int32_t bin = data_ptr->GetXaxis()->GetFirst(); bin <= data_ptr->GetXaxis()->GetLast(); ++bin) {
          const double_t content = data_ptr->GetBinContent(bin);
          if ((content == 0. && data_ptr->GetBinError(bin) == 0.) || content < minY || content > maxY || !inFrame(data_ptr->GetBinCenter(bin), content)) continue;
          drawLabel(data_ptr->GetBinCenter(bin), content + data_ptr->GetBinError(bin), content);
        }
      } else if constexpr (is_graph_1d<data_type>()) {
        if (polargram && data_ptr->InheritsFrom(TGraphPolar::Class())) {
          // the points where TGraphPainter::PaintGraphPolar puts them: the radial range from the centre to the edge of the grid, the
          // range of angles over a full turn, the points beyond the edge not drawn
          const double_t unit = (polargram->IsDegree()) ? 180. / TMath::Pi() : ((polargram->IsGrad()) ? 100. / TMath::Pi() : 1.);
          const double_t radialRange = polargram->GetRMax() - polargram->GetRMin();
          const double_t turn = (polargram->GetTMax() - polargram->GetTMin()) / TMath::TwoPi();
          for (int32_t i = 0; i < data_ptr->GetN(); ++i) {
            const double_t radius = (data_ptr->GetPointY(i) - polargram->GetRMin()) / radialRange;
            if (!(radius >= 0. && radius <= 1.)) continue;
            const double_t angle = unit * (data_ptr->GetPointX(i) - polargram->GetTMin()) / turn;
            drawLabel(radius * std::cos(angle), radius * std::sin(angle), data_ptr->GetPointY(i));
          }
        } else {
          // points outside the data range are removed already
          for (int32_t i = 0; i < data_ptr->GetN(); ++i) {
            if (!inFrame(data_ptr->GetPointX(i), data_ptr->GetPointY(i))) continue;
            drawLabel(data_ptr->GetPointX(i), data_ptr->GetPointY(i) + std::max(data_ptr->GetErrorYhigh(i), 0.), data_ptr->GetPointY(i));  // -1 for graphs without errors
          }
        }
      }
    }

    // in case a label was specified for the data, add it to corresponding legend
    const auto& legendBoxVector = pad.GetLegendBoxes();
    if (legendBoxVector.size() && data->GetLegendLabel() && !data->GetLegendLabel()->empty()) {
      // by default place legend entries in first legend
      uint8_t legendID{1u};
      // explicit user choice overrides this
      if (data->GetLegendID()) legendID = *data->GetLegendID();

      if (legendID > 0u && legendID <= legendBoxVector.size()) {
        optional<Plot::Pad::LegendEntry> legendEntry = data->GetLegendEntryIfSet();
        if (hasFilledSteps && !(legendEntry && legendEntry->GetSymbolElements())) {  // the row of filled steps shows the fill, as for hist
          if (!legendEntry) legendEntry.emplace();
          legendEntry->SetSymbolElements("F");
        }
        legendBoxVector[legendID - 1]->AddDataRow(*data->GetLegendLabel(), (dataIndex - hasRefFunc), legendEntry);
      } else {
        logger::warning("The legend set for data {} in {} does not exist in its pad (SetLegend counts the legends added by the plot itself, not those of its base plot).", data->GetName(), data->GetDataSource());
      }
    }
  }
  ++dataIndex;
  drawOption = "SAME ";  // next data should be drawn to same pad
}

//**************************************************************************************************
/**
 * Applies the ranges of the data (in a 3d view those of the axes of the frame); a TH2Poly is replaced by the copy of its bins in range.
 */
//**************************************************************************************************
template <typename T>
void PlotPainter::DataRanges(item_state_t& state, T& data_ptr, draw_option_t alias, bool projected, bool hasRangeX, bool hasRangeY)
{
  pad_state_t& padState = state.padState;
  auto& data = state.data;
  auto& pad_ptr = padState.pad_ptr;
  auto& frameSource = padState.frameSource;
  auto& axisHist_ptr = padState.axisHist_ptr;
  auto& drawOption = padState.drawOption;
  auto& dataIndex = padState.dataIndex;
  auto& frameFuncRange = padState.frameFuncRange;

  using data_type = std::decay_t<decltype(data_ptr)>;

  double_t xmin = 0, xmax = 0, ymin = 0, ymax = 0;
  pad_ptr->GetRangeAxis(xmin, ymin, xmax, ymax);
  xmin = FromPad(xmin, pad_ptr->GetLogx());
  xmax = FromPad(xmax, pad_ptr->GetLogx());
  ymin = FromPad(ymin, pad_ptr->GetLogy());
  ymax = FromPad(ymax, pad_ptr->GetLogy());
  if (auto view = pad_ptr->GetView()) {
    // for 3d view ignore individual data ranges and always let it coincide with the axes: the ranges of the frame's axes, since the
    // view holds the coordinates it paints in (-1 to 1 in polar, cylindrical and spherical systems, logarithms on a log scale)
    double_t minArr[3];
    double_t maxArr[3];
    view->GetRange(minArr, maxArr);
    const TAxis* frameX = axisHist_ptr->GetXaxis();
    const TAxis* frameY = axisHist_ptr->GetYaxis();
    xmin = frameX->GetBinLowEdge(frameX->GetFirst());
    xmax = frameX->GetBinUpEdge(frameX->GetLast());
    ymin = frameY->GetBinLowEdge(frameY->GetFirst());
    ymax = frameY->GetBinUpEdge(frameY->GetLast());
    data_ptr->SetMinimum(axisHist_ptr->GetMinimum());
    data_ptr->SetMaximum(axisHist_ptr->GetMaximum());
    if constexpr (is_hist_3d<data_type>()) {
      data_ptr->GetZaxis()->SetRangeUser(minArr[2], maxArr[2]);
    }
    if constexpr (is_func_3d<data_type>()) {
      // a 3d function is painted over its own range: that of the frame, which on a log axis starts at its first positive bin edge
      const TAxis* frameZ = axisHist_ptr->GetZaxis();
      data_ptr->SetRange(xmin, ymin, frameZ->GetBinLowEdge(frameZ->GetFirst()), xmax, ymax, frameZ->GetBinUpEdge(frameZ->GetLast()));
    }
  }
  if constexpr (is_hist_1d<data_type>()) {
    // ROOT draws the bars and fill areas of a histogram from its own minimum, also in an existing frame: start them at the lower edge of the frame
    if (!pad_ptr->GetView()) data_ptr->SetMinimum((TString(drawOption).Contains("HBAR", TString::kIgnoreCase)) ? xmin : ymin);
  }
  bool cutXAtFrame = true;
  bool cutYAtFrame = (axisHist_ptr->GetDimension() > 1);  // the y axis of a 1d frame has no bins
  if constexpr (is_hist_2d<data_type>()) {
    // candles and violins show the distribution of the contents along one axis: cutting it would change their statistics
    TString options(drawOption);
    options.ToUpper();
    if (options.Contains("CANDLE") || options.Contains("VIOLIN")) {
      ((options.Contains("CANDLEY") || options.Contains("VIOLINY")) ? cutXAtFrame : cutYAtFrame) = false;
    }
  }
  if constexpr (is_func_2d<data_type>()) {
    auto [rangeMinX, rangeMaxX] = FuncRange(state, 'x', xmin, xmax, data_ptr->GetXmin(), data_ptr->GetXmax(), data->GetMinRangeX(), data->GetMaxRangeX());
    auto [rangeMinY, rangeMaxY] = FuncRange(state, 'y', ymin, ymax, data_ptr->GetYmin(), data_ptr->GetYmax(), data->GetMinRangeY(), data->GetMaxRangeY());
    if (frameFuncRange && data.get() == frameSource) std::tie(rangeMinX, rangeMaxX, rangeMinY, rangeMaxY) = std::tuple_cat(*frameFuncRange);
    data_ptr->SetRange(rangeMinX, rangeMinY, rangeMaxX, rangeMaxY);
  } else if constexpr (is_func_1d<data_type>()) {
    auto [rangeMin, rangeMax] = FuncRange(state, 'x', xmin, xmax, data_ptr->GetXmin(), data_ptr->GetXmax(), data->GetMinRangeX(), data->GetMaxRangeX());
    if (frameFuncRange && data.get() == frameSource) std::tie(rangeMin, rangeMax) = std::make_pair((*frameFuncRange)[0], (*frameFuncRange)[1]);
    data_ptr->SetRange(rangeMin, rangeMax);
  } else if constexpr (is_graph_1d<data_type>()) {
    // (histograms converted for a clipped alias got the x range of the data as bins before, the y range does not apply to them)
    const bool isClippedHist = (alias == hist_clipped || alias == hist_open_clipped || alias == curve_clipped) && mSourceHists.count(data_ptr);
    if (!pad_ptr->GetView() && !isClippedHist && alias != area) RemoveGraphPointsOutside(static_cast<TGraph*>(data_ptr), data->GetMinRangeX(), data->GetMaxRangeX(), data->GetMinRangeY(), data->GetMaxRangeY());
    if (axisHist_ptr && dataIndex != 0 && !pad_ptr->GetView()) {
      const TAxis* frameX = axisHist_ptr->GetXaxis();
      mGraphFrameRangeX[data_ptr] = {frameX->GetBinLowEdge(frameX->GetFirst()), frameX->GetBinUpEdge(frameX->GetLast())};
    }
  } else {
    if (projected) {
      TString upper(drawOption);
      upper.ToUpper();
      if (hasRangeX || hasRangeY) logger::warning("The range of {} does not apply to a map projection, which must cover the range of the frame: set the range on the pad instead.", data->GetName());
      if (upper.Contains("COL")) logger::warning("{}: ROOT paints COL with a map projection twice, unprojected below the projected map. For a colour scale add Z to the projection alone.", data->GetName());
    }
    if constexpr (is_graph_2d<data_type>()) {
      if (!pad_ptr->GetView()) {
        const TAxis* frameX = axisHist_ptr->GetXaxis();
        const TAxis* frameY = axisHist_ptr->GetYaxis();
        mGraph2DFrameRange[data_ptr] = {frameX->GetBinLowEdge(frameX->GetFirst()), frameX->GetBinUpEdge(frameX->GetLast()), frameY->GetBinLowEdge(frameY->GetFirst()), frameY->GetBinUpEdge(frameY->GetLast())};
        // the points inside the data range: ROOT paints a 2d graph through a histogram it makes when painting and does not store, so a
        // range on that one would be lost in the saved canvas
        if (hasRangeX || hasRangeY) {
          auto outside = [&](int32_t i) {
            const double_t x = data_ptr->GetX()[i];
            const double_t y = data_ptr->GetY()[i];
            return (data->GetMinRangeX() && x < *data->GetMinRangeX()) || (data->GetMaxRangeX() && x > *data->GetMaxRangeX()) || (data->GetMinRangeY() && y < *data->GetMinRangeY()) ||
                   (data->GetMaxRangeY() && y > *data->GetMaxRangeY());
          };
          int32_t nInside{};
          for (int32_t i = 0; i < data_ptr->GetN(); ++i) {
            if (!outside(i)) ++nInside;
          }
          if (nInside == 0) {
            logger::warning("The range of {} contains none of its points: all points are drawn.", data->GetName());
          } else {
            for (int32_t i = data_ptr->GetN() - 1; i >= 0; --i) {  // from the back, so that the indices of the remaining points stay valid
              if (outside(i)) RemoveGraphPoint(data_ptr, i);
            }
          }
        }
      }
    }
    if (!pad_ptr->GetView() || projected) {
      if (hasRangeX && !projected && !is_graph_2d<data_type>() && !SetBinRange(data_ptr->GetXaxis(), data->GetMinRangeX(), data->GetMaxRangeX())) {
        logger::warning("The x range of {} contains none of its bins: all bins are drawn.", data->GetName());
      }
      if (cutXAtFrame) LimitToFrame(data_ptr->GetXaxis(), axisHist_ptr->GetXaxis());
    }
    if constexpr (is_hist_1d<data_type>()) {
      // the y axis of a 1d histogram has no bins: as for the frame, its range is the minimum and maximum (ROOT uses them e.g. as baseline of bars)
      if (data->GetMinRangeY()) data_ptr->SetMinimum(*data->GetMinRangeY());
      if (data->GetMaxRangeY()) data_ptr->SetMaximum(*data->GetMaxRangeY());
    }
  }
  if constexpr (is_func<data_type>()) {
    if (data->GetNpx()) data_ptr->SetNpx(*data->GetNpx());
    if constexpr (is_func_2d<data_type>() || is_func_3d<data_type>()) {
      if (data->GetNpy()) data_ptr->SetNpy(*data->GetNpy());
    }
  } else if (data->GetNpx()) {
    logger::warning("SetNpx applies only to functions, not to {}.", data->GetName());
  }
  if constexpr (is_hist_2d<data_type>() || is_hist_3d<data_type>()) {
    if (!pad_ptr->GetView() || projected) {
      if (hasRangeY && !projected && !SetBinRange(data_ptr->GetYaxis(), data->GetMinRangeY(), data->GetMaxRangeY())) {
        logger::warning("The y range of {} contains none of its bins: all bins are drawn.", data->GetName());
      }
      if (cutYAtFrame) LimitToFrame(data_ptr->GetYaxis(), axisHist_ptr->GetYaxis());
    }
    if constexpr (std::is_same_v<data_type, TH2*>) {
      // a TH2Poly draws and sums all its polygons whatever the axis ranges: a copy keeps those with their centre inside the ranges
      auto* poly = dynamic_cast<TH2Poly*>(data_ptr);
      if (poly && !projected && (hasRangeX || hasRangeY)) {
        const TAxis* x = poly->GetXaxis();
        const TAxis* y = poly->GetYaxis();
        const double_t xMin = x->GetBinLowEdge(x->GetFirst()), xMax = x->GetBinUpEdge(x->GetLast());
        const double_t yMin = y->GetBinLowEdge(y->GetFirst()), yMax = y->GetBinUpEdge(y->GetLast());
        auto* kept = new TH2Poly(poly->GetName(), poly->GetTitle(), x->GetXmin(), x->GetXmax(), y->GetXmin(), y->GetXmax());
        kept->SetDirectory(nullptr);
        kept->Sumw2();
        for (TObject* obj : *poly->GetBins()) {
          auto* polyBin = static_cast<TH2PolyBin*>(obj);
          const double_t cx = 0.5 * (polyBin->GetXMin() + polyBin->GetXMax());
          const double_t cy = 0.5 * (polyBin->GetYMin() + polyBin->GetYMax());
          if (cx < xMin || cx > xMax || cy < yMin || cy > yMax) continue;
          const int32_t bin = kept->AddBin(polyBin->GetPolygon()->Clone());
          kept->SetBinContent(bin, polyBin->GetContent());
          kept->SetBinError(bin, poly->GetBinError(polyBin->GetBinNumber()));
        }
        kept->SetEntries(poly->GetEntries());
        CopyStyle(*poly, *kept);
        for (auto [from, to] : {std::pair{poly->GetXaxis(), kept->GetXaxis()}, std::pair{poly->GetYaxis(), kept->GetYaxis()}, std::pair{poly->GetZaxis(), kept->GetZaxis()}}) {
          from->TAttAxis::Copy(*to);
          to->SetTitle(from->GetTitle());
        }
        kept->GetXaxis()->SetRange(x->GetFirst(), x->GetLast());
        kept->GetYaxis()->SetRange(y->GetFirst(), y->GetLast());
        kept->SetBit(kCanDelete, poly->TestBit(kCanDelete));
        mUndrawn.emplace_back(poly);
        data_ptr = kept;
      }
    }
    if (projected) {
      // ROOT takes the colours of COL drawn SAME from the frame, those of CONT4 from the histogram itself, and CONT4 also paints
      // the x and y axes of the histogram itself when drawn SAME: they get the look and titles of the frame's axes
      data_ptr->SetMinimum(axisHist_ptr->GetMinimum());
      data_ptr->SetMaximum(axisHist_ptr->GetMaximum());
      for (auto [frameAxis, dataAxis] : {std::pair{axisHist_ptr->GetXaxis(), data_ptr->GetXaxis()}, std::pair{axisHist_ptr->GetYaxis(), data_ptr->GetYaxis()}}) {
        frameAxis->TAttAxis::Copy(*dataAxis);
        dataAxis->SetTitle(frameAxis->GetTitle());
      }
    }
  }
}

//**************************************************************************************************
/**
 * Defines the appearance of the data: its own settings, else the defaults of its pad and of pad 0 (the drawing option alias decides which apply).
 */
//**************************************************************************************************
template <typename T>
void PlotPainter::ApplyDataStyle(item_state_t& state, T data_ptr, draw_option_t alias)
{
  pad_state_t& padState = state.padState;
  auto& data = state.data;
  auto& padDefaults = padState.padDefaults;
  auto& fillDefaults = padState.fillDefaults;
  auto& pad = padState.pad;
  auto& hasRefFunc = padState.hasRefFunc;
  auto& drawOption = padState.drawOption;
  auto& dataIndex = padState.dataIndex;
  auto& usedDefaults = padState.usedDefaults;

  using data_type = std::decay_t<decltype(data_ptr)>;

  if (hasRefFunc && dataIndex == 1) {
    // the reference function is not part of the data: it is a black solid line (unless set) and does not use up entries of the default lists
    data_ptr->SetLineColor(data->GetLineColor().value_or(kBlack).ToIndex());
    if (auto lineAlpha = get_first(data->GetLineAlpha(), pad.GetDefaultLineAlpha(), padDefaults.GetDefaultLineAlpha())) {
      data_ptr->SetLineColor(TColor::GetColorTransparent(data_ptr->GetLineColor(), *lineAlpha));
    }
    data_ptr->SetLineStyle(data->GetLineStyle().value_or(kSolid));
    if (auto lineWidth = get_first(data->GetLineWidth(), pad.GetDefaultLineWidth(), padDefaults.GetDefaultLineWidth())) {
      data_ptr->SetLineWidth(*lineWidth);
    }
  } else {
    // the defaults of the pad apply to what the data does not set, unless the style stored with the input is kept
    const bool keep = data->GetKeepInputStyle();
    auto fromPad = [keep](auto value) { return (keep) ? decltype(value){} : value; };
    // marker, line and fill colours not set for the data take the same entry of the default lists (SetColor sets all three);
    // data that takes any of them moves the next data on to the next entry
    bool tookColor{};
    if (auto markerColor = get_first(data->GetMarkerColor(),
                                     fromPad(pick(usedDefaults.color, pad.GetDefaultMarkerColors())),
                                     fromPad(pick(usedDefaults.color, padDefaults.GetDefaultMarkerColors())))) {
      if (!data->GetMarkerColor()) tookColor = true;
      data_ptr->SetMarkerColor(markerColor->ToIndex());
    }
    if (auto markerAlpha = get_first(data->GetMarkerAlpha(),
                                     fromPad(pad.GetDefaultMarkerAlpha()),
                                     fromPad(padDefaults.GetDefaultMarkerAlpha()))) {
      data_ptr->SetMarkerColor(TColor::GetColorTransparent(data_ptr->GetMarkerColor(), *markerAlpha));
    }
    if (auto markerStyle = get_first(data->GetMarkerStyle(),
                                     fromPad(pick(usedDefaults.markerStyle, pad.GetDefaultMarkerStyles())),
                                     fromPad(pick(usedDefaults.markerStyle, padDefaults.GetDefaultMarkerStyles())))) {
      if (!data->GetMarkerStyle()) usedDefaults.markerStyle++;
      data_ptr->SetMarkerStyle(*markerStyle);
    }
    if (auto markerSize = get_first(data->GetMarkerSize(),
                                    fromPad(pad.GetDefaultMarkerSize()),
                                    fromPad(padDefaults.GetDefaultMarkerSize()))) {
      data_ptr->SetMarkerSize(*markerSize);
    }
    if constexpr (is_hist_1d<data_type>()) {
      if (alias == boxes_nomarkers || alias == band || alias == band_smooth) data_ptr->SetMarkerSize(0.);  // E2, E5 and E6 always draw the markers
    }
    if (auto lineColor = get_first(data->GetLineColor(),
                                   fromPad(pick(usedDefaults.color, pad.GetDefaultLineColors())),
                                   fromPad(pick(usedDefaults.color, padDefaults.GetDefaultLineColors())))) {
      if (!data->GetLineColor()) tookColor = true;
      data_ptr->SetLineColor(lineColor->ToIndex());
    }
    if (auto lineAlpha = get_first(data->GetLineAlpha(),
                                   fromPad(pad.GetDefaultLineAlpha()),
                                   fromPad(padDefaults.GetDefaultLineAlpha()))) {
      data_ptr->SetLineColor(TColor::GetColorTransparent(data_ptr->GetLineColor(), *lineAlpha));
    }
    if (auto lineStyle = get_first(data->GetLineStyle(),
                                   fromPad(pick(usedDefaults.lineStyle, pad.GetDefaultLineStyles())),
                                   fromPad(pick(usedDefaults.lineStyle, padDefaults.GetDefaultLineStyles())))) {
      if (!data->GetLineStyle()) usedDefaults.lineStyle++;
      data_ptr->SetLineStyle(*lineStyle);
    }
    // the cells of a flat polar drawing are outlined with the line width: only a width the data sets itself outlines them (colz has no outlines)
    if (!data->GetLineWidth() && IsFlatPolar(drawOption)) {
      data_ptr->SetLineWidth(0);
      data_ptr->SetLineColor(TColor::GetColorTransparent(data_ptr->GetLineColor(), 0.f));  // pdf draws width 0 as a hairline
    } else if (is_2d<data_type>() && !data->GetLineWidth() && !keep && TString(drawOption).Contains("ARR", TString::kIgnoreCase)) {
      // ROOT outlines every arrow of ARR (fixed head size) with the line width: the wide default lines of the pad make blobs that hide a map below
      data_ptr->SetLineWidth(1);
    } else if (!data->GetLineWidth() && !keep && (is_3d<data_type>() || (is_2d<data_type>() && IsView3d(drawOption)))) {
      // ROOT outlines every box, bar, surface cell and triangle of a 3d view with the line width: the wide default lines of the pad
      // darken the view and hide the colours of small boxes, so the edges are thin unless the data sets a width (iso surfaces have no edges)
      data_ptr->SetLineWidth(1);
    } else if (auto lineWidth = get_first(data->GetLineWidth(),
                                          fromPad(pad.GetDefaultLineWidth()),
                                          fromPad(padDefaults.GetDefaultLineWidth()))) {
      data_ptr->SetLineWidth(*lineWidth);
    }
    // an input without a visible fill (ROOT's default fill colour 0 with a solid style) keeps none when the style is kept and the data
    // sets a fill colour (SetColor sets all three) but no fill style, for drawing options that draw no area (points, lines): their
    // fill would otherwise show (as the area of a graph drawn with a line); options that fill (band, area, hist, ...) keep it
    static const std::set<draw_option_t> fillingAliases{hist, hist_clipped, bars, hbars, area, area_smooth, band, band_smooth, boxes, boxes_nomarkers};
    const bool keptWithoutFill = keep && !data->GetFillStyle() && data_ptr->GetFillColor() == 0 && alias != static_cast<draw_option_t>(255) && !fillingAliases.count(alias);
    if (auto fillColor = get_first(data->GetFillColor(),
                                   fromPad(pick(usedDefaults.color, pad.GetDefaultFillColors())),
                                   fromPad(pick(usedDefaults.color, fillDefaults(pad, *data))))) {
      if (!data->GetFillColor()) tookColor = true;
      data_ptr->SetFillColor(fillColor->ToIndex());
    }
    if (tookColor) usedDefaults.color++;
    if constexpr (is_func_3d<data_type>()) {
      // ROOT paints the surface f = 0 of a 3d function with flat faces in the fill colour (whatever the fill style) and its mesh in the
      // line colour: without a fill colour of its own, white faces with the mesh in the data's colour (as ROOT's default), not a silhouette
      // (before the alpha, which applies to them too)
      if (!data->GetFillColor() && !keep) data_ptr->SetFillColor(kWhite);
    }
    if (auto fillAlpha = get_first(data->GetFillAlpha(),
                                   fromPad(pad.GetDefaultFillAlpha()),
                                   fromPad(padDefaults.GetDefaultFillAlpha()))) {
      data_ptr->SetFillColor(TColor::GetColorTransparent(data_ptr->GetFillColor(), *fillAlpha));
    }
    if (auto fillStyle = get_first(data->GetFillStyle(),
                                   fromPad(pick(usedDefaults.fillStyle, pad.GetDefaultFillStyles())),
                                   fromPad(pick(usedDefaults.fillStyle, padDefaults.GetDefaultFillStyles())))) {
      if (!data->GetFillStyle()) usedDefaults.fillStyle++;
      data_ptr->SetFillStyle(*fillStyle);
    }
    if (keptWithoutFill) data_ptr->SetFillStyle(0);
    if constexpr (is_hist_1d<data_type>() || is_func_1d<data_type>()) {
      if (alias == line || alias == curve) data_ptr->SetFillStyle(0);  // HIST L and HIST C would also fill the histogram, L and C the area below a function
    }
  }
}

//**************************************************************************************************
/**
 * Draws the frame of the pad (data 0: a copy of the data that gives it, drawn as its first item) and applies the axis settings to it.
 */
//**************************************************************************************************
template <typename T>
void PlotPainter::SetUpFrame(item_state_t& state, T data_ptr, const string& polarGridOption)
{
  pad_state_t& padState = state.padState;
  auto& data = state.data;
  auto& padDefaults = padState.padDefaults;
  auto& pad = padState.pad;
  auto& padID = padState.padID;
  auto& pad_ptr = padState.pad_ptr;
  auto& padSetup = padState.padSetup;
  auto& textFont = padState.textFont;
  auto& textSize = padState.textSize;
  auto& textColor = padState.textColor;
  auto& textAlpha = padState.textAlpha;
  auto& axisHist_ptr = padState.axisHist_ptr;
  auto& polarFrame = padState.polarFrame;
  auto& polargram = padState.polargram;
  auto& polarAxisHist = padState.polarAxisHist;
  auto& padProjection = padState.padProjection;
  auto& contentFrame = padState.contentFrame;
  auto& drawOption = padState.drawOption;
  auto& storedRangeZ = padState.storedRangeZ;
  auto& frameFuncRange = padState.frameFuncRange;

  using data_type = std::decay_t<decltype(data_ptr)>;

  // a function that defines the frame has no frame to span: it is drawn over its own range, narrowed or widened by the range the
  // user gave with SetRangeX / SetRangeY; a formula from the plot definition has no range of its own (only ROOT's default 0 to 1):
  // it spans the range of the pad axis where it has none of its own
  if constexpr (is_func_1d<data_type>() || is_func_2d<data_type>()) {
    auto [rangeMinX, rangeMaxX] = FuncRange(state, 'x', data_ptr->GetXmin(), data_ptr->GetXmax(), data_ptr->GetXmin(), data_ptr->GetXmax(), data->GetMinRangeX(), data->GetMaxRangeX());
    // the frame is the function sampled over its range: a pad range inside it is sampled alone, so the frame ends exactly there
    // (not at the edges of the sampled bins) and the automatic y range (1d) or colour scale (2d) is that of the visible part;
    // a pad range beyond the function widens the frame later, as for any data (a formula without a range of its own spans it).
    // On a log axis a function crossing zero is sampled from the first of its sample edges above zero, where the frame starts
    // (see below): the drawn function is sampled there as well
    const bool isFormula = (data->GetDataSource() == kUserFunctions);
    auto padRange = [&](char padLabel, double_t& min, double_t& max, int32_t nSamples, const optional<double_t>& dataMin, const optional<double_t>& dataMax) {
      const optional<double_t> userMin = MinRange(pad, padDefaults, padLabel);
      optional<double_t> userMax;
      bool log{};
      for (const auto* axis : AxisSettings(pad, padDefaults, padLabel)) {
        if (axis->GetMaxRange()) userMax = axis->GetMaxRange();
        if (axis->GetLog()) log = *axis->GetLog();
      }
      const double_t newMin = (userMin && ((isFormula && !dataMin) || (*userMin > min && *userMin < max))) ? *userMin : min;
      const double_t newMax = (userMax && ((isFormula && !dataMax) || (*userMax < max && *userMax > min))) ? *userMax : max;
      if (newMin < newMax) std::tie(min, max) = std::make_pair(newMin, newMax);
      if (log && min <= 0. && max > 0. && nSamples > 0) {
        const double_t width = (max - min) / nSamples;  // the bin edges of TAxis
        int32_t edge = 1;
        while (min + edge * width <= 0.)
          ++edge;
        min += edge * width;
      }
    };
    padRange('X', rangeMinX, rangeMaxX, data_ptr->GetNpx(), data->GetMinRangeX(), data->GetMaxRangeX());
    if constexpr (is_func_2d<data_type>()) {
      auto [rangeMinY, rangeMaxY] = FuncRange(state, 'y', data_ptr->GetYmin(), data_ptr->GetYmax(), data_ptr->GetYmin(), data_ptr->GetYmax(), data->GetMinRangeY(), data->GetMaxRangeY());
      padRange('Y', rangeMinY, rangeMaxY, data_ptr->GetNpy(), data->GetMinRangeY(), data->GetMaxRangeY());
      data_ptr->SetRange(rangeMinX, rangeMinY, rangeMaxX, rangeMaxY);
      frameFuncRange = {rangeMinX, rangeMaxX, rangeMinY, rangeMaxY};
    } else {
      data_ptr->SetRange(rangeMinX, rangeMaxX);
      frameFuncRange = {rangeMinX, rangeMaxX, 0., 0.};
    }
  }
  // points with a positive value (x, y, value; for 1d graphs the ends of the x error bars and the value), needed for a log scale:
  // graphs keep them, since their axis frame is an empty histogram (and drawing it below deletes the graph); the smallest value
  // shown in the frame's ranges is taken once those are set
  optional<vector<array<double_t, 3>>> graphPositivePoints;
  // the same for the x axis of 1d graphs, from the lower ends of the error bars (from the points where those are not positive)
  optional<double_t> graphPositiveMinimumX;
  // the range of the points of a 2d graph in z, including their errors, and the minimum and maximum stored with it (read here: drawing
  // the frame below deletes the graph)
  std::pair<double_t, double_t> graphRangeZ{};
  std::pair<double_t, double_t> graphStoredZ{-1111., -1111.};
  if constexpr (is_graph_2d<data_type>()) {
    if (data_ptr->GetN() > 0) graphRangeZ = {data_ptr->GetZminE(), data_ptr->GetZmaxE()};  // ROOT reads the first point of an empty graph
    graphStoredZ = {data_ptr->GetMinimum(), data_ptr->GetMaximum()};
  }
  // the bins of a TH2Poly with a positive content (x and y ranges, content), needed for a log scale as well (read here: a pad range
  // widening its frame makes that a histogram of its search grid)
  optional<vector<array<double_t, 5>>> polyPositiveBins;
  if constexpr (is_hist_2d<data_type>()) {
    if (auto* poly = dynamic_cast<TH2Poly*>(data_ptr)) {
      polyPositiveBins.emplace();
      for (TObject* obj : *poly->GetBins()) {
        auto* bin = static_cast<TH2PolyBin*>(obj);  // (its getters are not const)
        if (bin->GetContent() > 0.) polyPositiveBins->push_back({bin->GetXMin(), bin->GetXMax(), bin->GetYMin(), bin->GetYMax(), bin->GetContent()});
      }
    }
  }
  if constexpr (is_graph_1d<data_type>() || is_graph_2d<data_type>()) {
    graphPositivePoints.emplace();
    if constexpr (is_graph_1d<data_type>()) graphPositiveMinimumX = std::numeric_limits<double_t>::infinity();
    for (int32_t i = 0; i < data_ptr->GetN(); ++i) {
      if constexpr (is_graph_1d<data_type>()) {
        const double_t value = data_ptr->GetPointY(i);
        const double_t x = data_ptr->GetPointX(i);
        const double_t xLow = x - std::max(data_ptr->GetErrorXlow(i), 0.);  // -1 for graphs without errors
        if (xLow > 0.) {
          graphPositiveMinimumX = std::min(*graphPositiveMinimumX, xLow);
        } else if (x > 0.) {
          graphPositiveMinimumX = std::min(*graphPositiveMinimumX, x);
        }
        if (value > 0.) graphPositivePoints->push_back({xLow, x + std::max(data_ptr->GetErrorXhigh(i), 0.), value});
      } else {
        if (const double_t value = data_ptr->GetZ()[i]; value > 0.) graphPositivePoints->push_back({data_ptr->GetX()[i], data_ptr->GetY()[i], value});
      }
    }
  }
  if constexpr (is_2d<data_type>()) polarFrame = IsFlatPolar(drawOption);
  if (polarFrame) {
    // the disc is round only in a square frame: the margins of the longer side grow, both by the same amount
    const auto [padWidth, padHeight] = PadSizePixel(pad_ptr);
    const double_t frameWidth = padWidth * (1. - pad_ptr->GetLeftMargin() - pad_ptr->GetRightMargin());
    const double_t frameHeight = padHeight * (1. - pad_ptr->GetTopMargin() - pad_ptr->GetBottomMargin());
    if (frameWidth > frameHeight) {
      const double_t extra = 0.5 * (frameWidth - frameHeight) / padWidth;
      pad_ptr->SetLeftMargin(pad_ptr->GetLeftMargin() + extra);
      pad_ptr->SetRightMargin(pad_ptr->GetRightMargin() + extra);
    } else {
      const double_t extra = 0.5 * (frameHeight - frameWidth) / padHeight;
      pad_ptr->SetTopMargin(pad_ptr->GetTopMargin() + extra);
      pad_ptr->SetBottomMargin(pad_ptr->GetBottomMargin() + extra);
    }
    // the cartesian axes and the frame box measure nothing on the disc: option A leaves out the axes (the radial axis follows the data),
    // the box ROOT paints in any case (THistPainter::PaintFrame) has no line
    drawOption += " A";
    pad_ptr->SetFrameLineWidth(0);
  }
  // a polar graph is drawn into a polar grid (TGraphPolargram) that ROOT makes from the points of the first polar graph painted
  // in a pad and that the later ones use (TGraphPainter::PaintGraphPolar): here it is made from the frame data instead of a
  // cartesian frame, and takes the ranges and labels of the axes below; the histogram of the graph only holds those axis settings.
  // As in ROOT, the unit of the angle (R, D, G) and the labels (N, O) come from the option of that graph
  if constexpr (is_graph_1d<data_type>()) {
    if (data_ptr->InheritsFrom(TGraphPolar::Class())) {
      polargram = static_cast<TGraphPolar*>(data_ptr)->CreatePolargram(drawOption.data());
      axisHist_ptr = static_cast<TH1*>(data_ptr->GetHistogram()->Clone());
      axisHist_ptr->SetDirectory(nullptr);
      polarAxisHist.reset(axisHist_ptr);
      mSourceHists.erase(data_ptr);
      delete data_ptr;
      if (!polargram) {  // no points: a grid of radius 1 over the full turn
        polargram = new TGraphPolargram("Polargram", 0., 1., 0., TMath::TwoPi(), drawOption.data());
      }
      polargram->SetBit(kCanDelete);
      polargram->Draw(polarGridOption.data());
      padSetup += " gStyle->SetOptTitle(0);";  // otherwise each polar graph paints its title as a box at the top
    }
  }
  if constexpr (is_hist_3d<data_type>()) {
    // the colour boxes of a TH3 fill the pad behind the 3d box with the frame fill colour (TView3D::PadRange), but in the colour
    // last used for the colour scale when that colour is not 0 (walls in the colour of the palette maximum)
    if (IsColorBoxes3d(drawOption)) pad_ptr->SetFrameFillColor(0);
  }
  if (!polargram) {
    data_ptr->Draw(drawOption.data());
    if constexpr (is_hist<data_type>()) {
      axisHist_ptr = data_ptr;
    } else {
      axisHist_ptr = static_cast<TH1*>(data_ptr->GetHistogram()->Clone());
      axisHist_ptr->SetDirectory(nullptr);
      axisHist_ptr->SetBit(kCanDelete);
      if constexpr (is_graph_1d<data_type>()) {
        // ROOT's y range of a graph (TGraph::GetHistogram) comes from all its points: with an x range of the pad, or on log x, it is
        // taken from the points drawn (their x errors reaching into the range, on log x above zero), as for histograms and functions,
        // by ROOT for those points alone
        static thread_local std::pair<double_t, double_t> shownX;
        const optional<double_t> userMin = MinRange(pad, padDefaults, 'X');
        optional<double_t> userMax;
        bool logX{};
        for (const auto* axis : AxisSettings(pad, padDefaults, 'X')) {
          if (axis->GetMaxRange()) userMax = axis->GetMaxRange();
          if (axis->GetLog()) logX = *axis->GetLog();
        }
        shownX = {userMin.value_or(-std::numeric_limits<double_t>::infinity()), userMax.value_or(std::numeric_limits<double_t>::infinity())};
        if (logX) shownX.first = std::max(shownX.first, std::numeric_limits<double_t>::min());
        static const auto isShown = [](const TGraph* graph, int32_t i) {
          const double_t x = graph->GetPointX(i);
          return x + std::max(graph->GetErrorXhigh(i), 0.) >= shownX.first && x - std::max(graph->GetErrorXlow(i), 0.) <= shownX.second;
        };
        int32_t nShown = 0;
        for (int32_t i = 0; i < data_ptr->GetN() && (userMin || userMax || logX); ++i) {
          if (isShown(data_ptr, i)) ++nShown;
        }
        if (nShown > 0 && nShown < data_ptr->GetN()) {
          std::unique_ptr<TGraph> shown;
          if (auto* multi = dynamic_cast<TGraphMultiErrors*>(data_ptr)) {
            // a TGraphMultiErrors cannot be cut that way (ROOT's Sort leaves its y errors in place, Set crashes on a clone): made anew
            auto* subset = new TGraphMultiErrors(nShown, multi->GetNYErrors());
            shown.reset(subset);
            for (int32_t i = 0, n = 0; i < multi->GetN(); ++i) {
              if (!isShown(multi, i)) continue;
              subset->SetPoint(n, multi->GetPointX(i), multi->GetPointY(i));
              subset->SetPointEX(n, multi->GetErrorXlow(i), multi->GetErrorXhigh(i));
              for (int32_t e = 0; e < multi->GetNYErrors(); ++e)
                subset->SetPointEY(n, e, multi->GetEYlow(e)[i], multi->GetEYhigh(e)[i]);
              ++n;
            }
            subset->SetSumErrorsMode(multi->GetSumErrorsMode());
            if (multi->GetMinimum() != -1111) subset->SetMinimum(multi->GetMinimum());  // a range stored with the graph
            if (multi->GetMaximum() != -1111) subset->SetMaximum(multi->GetMaximum());
          } else {
            // the points shown first (ROOT sorts with SwapPoints of each graph class, which keeps their errors), the others cut off
            shown.reset(static_cast<TGraph*>(data_ptr->Clone()));
            shown->Sort([](const TGraph* graph, int32_t i, int32_t j) -> Bool_t { return isShown(graph, i) < isShown(graph, j); });
            shown->Set(nShown);
            shown->SetBit(TGraph::kResetHisto);  // the histogram cloned with the points would be kept
          }
          axisHist_ptr->SetMinimum(shown->GetHistogram()->GetMinimum());
          axisHist_ptr->SetMaximum(shown->GetHistogram()->GetMaximum());
        }
      }
      if constexpr (is_graph_2d<data_type>()) {
        // the histogram of a 2d graph holds its Delaunay triangulation, which the frame does not use (see below) and which a
        // macro cannot save (the line ROOT writes for it does not compile)
        TList* functions = axisHist_ptr->GetListOfFunctions();
        vector<TObject*> triangulations;
        for (TObject* function : *functions) {
          if (function->InheritsFrom("TGraphDelaunay2D") || function->InheritsFrom("TGraphDelaunay")) triangulations.push_back(function);
        }
        for (TObject* triangulation : triangulations) {
          functions->Remove(triangulation);
          delete triangulation;
        }
      }
    }
  }
  string drawOptAxis = "AXIS";
  pad_ptr->Update();
  if constexpr (is_graph_2d<data_type>()) {
    // the options that paint the triangles or the points of the graph in 3d (the rule of TGraph2D::Paint: TRI..., P except POL, SPH
    // and PSR, LINE, ERR) need the Delaunay triangulation, which the copy of its histogram that draws the frame does not have
    // (ROOT crashes on TRI and draws P as a flat map): the frame is drawn as an empty lego plot (the same box and axes),
    // with the colour axis if the option asks for one
    bool triangles{}, points{};
    Paints3dGraph(drawOption, triangles, points);
    if (triangles || points) drawOption = (TString(drawOption).Contains("z", TString::kIgnoreCase)) ? "LEGOZ" : "LEGO";
  }
  if constexpr (is_func_3d<data_type>()) {
    // TF3 (the option of the surface of a 3d function) would make the histogram that draws the frame paint a surface without function
    TString option(drawOption);
    option.ReplaceAll("TF3", "").ReplaceAll("tf3", "");
    drawOption = option.Data();
  }
  if constexpr (is_2d<data_type>()) {
    // the frame is emptied below (contents 0) and still painted with its option: a surface would put a plane at 0, or at the floor
    // of the box if the minimum is above 0; as a lego plot without its empty bins (0) it gives the same box and axes and paints nothing
    if (pad_ptr->GetView()) {
      static const std::regex surface("SURF[0-9]?", std::regex::icase);
      drawOption = std::regex_replace(drawOption, surface, "LEGO");
      if (TString(drawOption).Contains("LEGO", TString::kIgnoreCase) && drawOption.find('0') == string::npos) drawOption += " 0";
    }
  }
  // the frame of a 2d histogram, and of a coloured 2d graph or function (drawn from its histogram), also draws the colour axis:
  // also with options that ask for the colour scale without COL (CONT1Z, CONTZ), which with AXIS would paint none
  const TString option2d(drawOption);  // raw user options may be lower case
  const bool isColored2d = (is_graph_2d<data_type>() || is_func_2d<data_type>()) &&
                           (option2d.Contains("COL", TString::kIgnoreCase) || option2d.Contains("Z", TString::kIgnoreCase));
  if (pad_ptr->GetView() || is_hist_2d<data_type>() || isColored2d) {
    drawOptAxis = "";
  }
  if (!polargram) {
    // drawn without SAME: the pad is cleared, which deletes a graph drawn above (a later graph at its address is another one)
    if constexpr (is_graph_1d<data_type>() || is_graph_2d<data_type>()) {
      if (data_ptr->TestBit(kCanDelete)) mSourceHists.erase(data_ptr);
    }
    axisHist_ptr->Draw((drawOption + drawOptAxis).data());
    axisHist_ptr->Draw((drawOption + "SAME AXIG").data());
  }
  const int32_t frameDim = axisHist_ptr->GetDimension();
  if constexpr (is_hist_3d<data_type>()) contentFrame = true;
  const bool hbarFrame = IsHBarFrame(pad_ptr, axisHist_ptr);  // with HBAR the X and Y of the pad are the y and x axis of the histogram

  // a user range beyond the axes of the frame histogram needs a wider frame, since ROOT shows at most the axis of a histogram:
  // the frame is replaced by a histogram with extended axes (changing the axis alone would break the bin storage of the histogram)
  if (!pad_ptr->GetView() && !polargram) {
    for (auto axisLabel : {'X', 'Y'}) {  // the axis of the frame histogram
      if (axisLabel == 'Y' && frameDim == 1) continue;
      const char padLabel = (hbarFrame) ? ((axisLabel == 'X') ? 'Y' : 'X') : axisLabel;  // where it is drawn
      const char valueLabel = (hbarFrame) ? 'X' : 'Y';                                   // the pad axis of the contents of a 1d frame
      optional<double_t> userMin = MinRange(pad, padDefaults, padLabel);
      optional<double_t> userMax;
      optional<bool> logY;
      for (const auto* axis : AxisSettings(pad, padDefaults, valueLabel)) {
        if (axis->GetLog()) logY = axis->GetLog();
      }
      for (const auto* axis : AxisSettings(pad, padDefaults, padLabel)) {
        if (axis->GetMaxRange()) userMax = axis->GetMaxRange();
      }
      const TAxis* axis = (axisLabel == 'X') ? axisHist_ptr->GetXaxis() : axisHist_ptr->GetYaxis();
      const optional<double_t> rangeMin = userMin, rangeMax = userMax;  // the user's range, before the reset below
      constexpr double_t relTol = 1e-12;
      // a log axis shows the bins from the first positive edge on: a positive minimum below it is beyond the shown axis as well
      // (ExtendFrameAxis drops the bins up to it), whether the bins start at 0.5, 0 or -0.5
      optional<bool> logAxis;
      const char logLabel = (IsHBarFrame(pad_ptr, axisHist_ptr)) ? ((axisLabel == 'X') ? 'Y' : 'X') : axisLabel;  // the pad axis showing it
      for (const auto* settings : AxisSettings(pad, padDefaults, logLabel)) {
        if (settings->GetLog()) logAxis = settings->GetLog();
      }
      double_t shownMin = axis->GetXmin();
      if (logAxis.value_or(false) && shownMin <= 0. && userMin && *userMin > 0. && !axis->GetLabels()) {
        int32_t bin = 1;
        while (bin <= axis->GetNbins() && axis->GetBinLowEdge(bin) <= 0.)
          ++bin;
        shownMin = axis->GetBinLowEdge(bin);
      }
      if (userMin && !(*userMin < shownMin && !TMath::AreEqualRel(*userMin, shownMin, relTol))) userMin.reset();
      if (userMax && !(*userMax > axis->GetXmax() && !TMath::AreEqualRel(*userMax, axis->GetXmax(), relTol))) userMax.reset();
      if (!userMin && !userMax) continue;
      // the y range of a 1d frame is taken from the pad below, and ROOT computes it differently for a log scale (margins in decades):
      // the scale is applied first, so the extended frame keeps the log range (one extra paint, only for extended frames with log y)
      if (frameDim == 1 && logY.value_or(false) && !((hbarFrame) ? pad_ptr->GetLogx() : pad_ptr->GetLogy()) && axisHist_ptr->GetMaximum() > 0.) {
        (hbarFrame) ? pad_ptr->SetLogx(true) : pad_ptr->SetLogy(true);
        pad_ptr->Update();
      }
      TH1* extended = ExtendFrameAxis(axisHist_ptr, axisLabel, userMin, userMax, rangeMin, rangeMax);
      // the added empty bins must not change the automatic range of the dependent axis: keep the one of the original frame
      // (for 1d the range ROOT determined when drawing it, for 2d the extremes of the contents, which define the colour scale below)
      if (axisHist_ptr->GetMinimumStored() == -1111 && axisHist_ptr->GetMaximumStored() == -1111) {
        if (frameDim > 1) {
          extended->SetMinimum(axisHist_ptr->GetMinimum());
          extended->SetMaximum(axisHist_ptr->GetMaximum());
        } else if (hbarFrame) {
          extended->SetMinimum(FromPad(pad_ptr->GetUxmin(), pad_ptr->GetLogx()));
          extended->SetMaximum(FromPad(pad_ptr->GetUxmax(), pad_ptr->GetLogx()));
        } else {
          extended->SetMinimum(FromPad(pad_ptr->GetUymin(), pad_ptr->GetLogy()));
          extended->SetMaximum(FromPad(pad_ptr->GetUymax(), pad_ptr->GetLogy()));
        }
      }
      TList* primitives = pad_ptr->GetListOfPrimitives();
      while (primitives->Remove(axisHist_ptr)) {
      }
      delete axisHist_ptr;
      axisHist_ptr = extended;
      axisHist_ptr->Draw(drawOption.data());
      axisHist_ptr->Draw((drawOption + drawOptAxis).data());
      axisHist_ptr->Draw((drawOption + "SAME AXIG").data());
    }
  }
  axisHist_ptr->SetName(string("axis_hist_pad_" + std::to_string(padID)).data());
  axisHist_ptr->SetStats(false);
  axisHist_ptr->SetTitle("");
  axisHist_ptr->SetBit(TH1::kNoTitle);

  // apply axis settings
  if (frameDim != 3 && pad.GetAxes().count('C')) {
    logger::warning("The C axis of pad {} is ignored: it is the content of a 3d histogram (for other data the Y or Z axis is the value axis).", padID);
  }
  // the binned axes first: the positive minimum for a log value axis is taken from the bins in their range (with HBAR the value axis is the pad's X)
  for (auto axisLabel : (hbarFrame) ? array<char, 3>{'Y', 'X', 'Z'} : array<char, 3>{'X', 'Y', 'Z'}) {
    // axisLabel: the axis of the pad (X horizontal, Y vertical); histLabel: the axis of the frame histogram drawn there
    const char histLabel = (hbarFrame && axisLabel != 'Z') ? ((axisLabel == 'X') ? 'Y' : 'X') : axisLabel;
    TAxis* axis_ptr = nullptr;
    if (histLabel == 'X')
      axis_ptr = axisHist_ptr->GetXaxis();
    else if (histLabel == 'Y')
      axis_ptr = axisHist_ptr->GetYaxis();
    else if (axisLabel == 'Z')
      axis_ptr = axisHist_ptr->GetZaxis();
    if (!axis_ptr) continue;

    auto textFontTitle = textFont;
    auto textSizeTitle = textSize;
    auto textColorTitle = textColor;
    auto textAlphaTitle = textAlpha;
    auto textFontLabel = textFont;
    auto textSizeLabel = textSize;
    auto textColorLabel = textColor;
    auto textAlphaLabel = textAlpha;
    optional<float_t> labelAngle;

    optional<double_t> userRangeMin;
    optional<double_t> userRangeMax;
    bool userTitleOffset{};
    bool userTitleCenter{};

    // first apply default pad values and then settings for this specific pad
    for (const auto* axisSettings : AxisSettings(pad, padDefaults, axisLabel)) {
      const auto& axisLayout = *axisSettings;
      if (axisLayout.GetTitle()) axis_ptr->SetTitle((*axisLayout.GetTitle()).data());

      if (axisLayout.GetTitleFont()) textFontTitle = axisLayout.GetTitleFont();
      if (axisLayout.GetLabelFont()) textFontLabel = axisLayout.GetLabelFont();

      if (axisLayout.GetTitleColor()) textColorTitle = axisLayout.GetTitleColor();
      if (axisLayout.GetLabelColor()) textColorLabel = axisLayout.GetLabelColor();

      if (axisLayout.GetTitleAlpha()) textAlphaTitle = axisLayout.GetTitleAlpha();
      if (axisLayout.GetLabelAlpha()) textAlphaLabel = axisLayout.GetLabelAlpha();

      if (axisLayout.GetTitleSize()) textSizeTitle = axisLayout.GetTitleSize();
      if (axisLayout.GetLabelSize()) textSizeLabel = axisLayout.GetLabelSize();
      if (axisLayout.GetLabelAngle()) labelAngle = axisLayout.GetLabelAngle();

      if (axisLayout.GetTitleCenter()) {
        axis_ptr->CenterTitle(*axisLayout.GetTitleCenter());
        userTitleCenter = true;
      }
      if (axisLayout.GetLabelCenter()) axis_ptr->CenterLabels(*axisLayout.GetLabelCenter());

      if (axisLayout.GetAxisColor()) axis_ptr->SetAxisColor(axisLayout.GetAxisColor()->ToIndex());
      if (axisLayout.GetAxisAlpha()) axis_ptr->SetAxisColor(TColor::GetColorTransparent(axis_ptr->GetAxisColor(), *axisLayout.GetAxisAlpha()));

      if (axisLayout.GetTitleOffset()) {
        axis_ptr->SetTitleOffset(*axisLayout.GetTitleOffset());
        userTitleOffset = true;
      }
      if (axisLayout.GetLabelOffset()) axis_ptr->SetLabelOffset(*axisLayout.GetLabelOffset());

      if (axisLayout.GetTickLength()) axis_ptr->SetTickLength(*axisLayout.GetTickLength());
      if (axisLayout.GetMaxDigits()) axis_ptr->SetMaxDigits(*axisLayout.GetMaxDigits());

      if (axisLayout.GetNumDivisions()) axis_ptr->SetNdivisions(*axisLayout.GetNumDivisions());

      if (axisLayout.GetOppositeTicks()) {
        if (axisLabel == 'X') {
          pad_ptr->SetTickx(*axisLayout.GetOppositeTicks());
        } else if (axisLabel == 'Y') {
          pad_ptr->SetTicky(*axisLayout.GetOppositeTicks());
        }
      }
      if (axisLayout.GetNoExponent()) {
        axis_ptr->SetNoExponent(*axisLayout.GetNoExponent());
      }
      if (axisLayout.GetTimeFormat()) {
        axis_ptr->SetTimeDisplay(1);
        axis_ptr->SetTimeFormat((*axisLayout.GetTimeFormat()).data());
      }
      if (axisLayout.GetTickOrientation()) {
        axis_ptr->SetTicks((*axisLayout.GetTickOrientation()).data());
      }

      // the ranges are applied below, once the default and pad-specific settings are merged
      if (axisLayout.GetMinRange()) userRangeMin = axisLayout.GetMinRange();
      if (axisLayout.GetMaxRange()) userRangeMax = axisLayout.GetMaxRange();

      if (axisLayout.GetLog()) {
        if (axisLabel == 'X') {
          pad_ptr->SetLogx(*axisLayout.GetLog());
        } else if (axisLabel == 'Y') {
          pad_ptr->SetLogy(*axisLayout.GetLog());
        } else if (axisLabel == 'Z') {
          pad_ptr->SetLogz(*axisLayout.GetLog());
        }
      }
      if (axisLabel != 'Z' && (axisLayout.GetColorScaleOffset() || axisLayout.GetColorScaleWidth())) {
        ReportIgnored(fmt::format("The colour scale offset or width of the {} axis", axisLabel), "only the Z axis (C of a 3d histogram) has a colour scale");
      }
      if (axisLabel == 'Z' && (axisLayout.GetGrid() || axisLayout.GetOppositeTicks())) {
        ReportIgnored("A grid or opposite ticks of the Z axis", "ROOT draws them on X and Y only");
      }
      if (axisLayout.GetGrid()) {
        if (axisLabel == 'X') {
          pad_ptr->SetGridx(*axisLayout.GetGrid());
        } else if (axisLabel == 'Y') {
          pad_ptr->SetGridy(*axisLayout.GetGrid());
        }
      }
    }
    // the title set above or stored with the input
    if (string title = axis_ptr->GetTitle(); title.find('\\') != string::npos) {
      DropEmptyGroups(title);
      axis_ptr->SetTitle(title.data());
    }

    // labels and title a layout hides towards touching panels (see PanelLayout::ApplyTo): in a flat frame (known from an earlier paint, see
    // SetFlatPads; a 3D view or a polar drawing keeps its own, it shares no axis), unless the pad sets their size itself
    if (pad.GetHiddenNextTo().count(axisLabel)) {
      const bool hide = !mFlatPads || mFlatPads->count(padID);
      const auto own = pad.GetAxes().find(axisLabel);
      const bool ownTitleSize = own != pad.GetAxes().end() && own->second.GetTitleSize();
      const bool ownLabelSize = own != pad.GetAxes().end() && own->second.GetLabelSize();
      if (hide) {
        if (!ownTitleSize) textSizeTitle = 0.f;
        if (!ownLabelSize) textSizeLabel = 0.f;
      } else if (auto titleSize = LayoutValue(pad, padDefaults, "title_size"); titleSize && !ownTitleSize) {
        textSizeTitle = titleSize;
      }
    }

    // a lower end at or below zero cannot be shown on a log scale: the axis is drawn as without it (as in ROOT), with a warning below
    optional<double_t> ignoredRangeMin;
    if (userRangeMin && !MinRange(pad, padDefaults, axisLabel)) ignoredRangeMin = std::exchange(userRangeMin, nullopt);

    // the title offsets of a layout panel are those of a flat frame; a 3D view draws its axes in perspective (THistPainter::PaintLegoAxis):
    // x and y titles 1.5 title heights from the axis (ROOT's own value for y) and centred (at the end of the axis they run into the next
    // pad or the colour scale), the z title next to its labels (offset 0, see PlaceZTitle)
    const bool viewFrame = IsViewFrame(pad_ptr, axisHist_ptr);
    if (viewFrame && axisLabel != 'Z' && !userTitleCenter) axis_ptr->CenterTitle();
    if (!userTitleOffset) {
      if (viewFrame) {
        axis_ptr->SetTitleOffset((axisLabel == 'Z') ? 0.f : 1.5f);
      } else if (auto layoutOffset = LayoutValue(pad, padDefaults, string("title_offset_") + static_cast<char>(std::tolower(axisLabel)))) {
        axis_ptr->SetTitleOffset(*layoutOffset);
      }
    }

    // THistPainter::PaintAxis scales the primary divisions of the horizontal axis with the width of the pad relative to the canvas
    // when the setting carries a max digits part (ROOT bug: the same axis without max digits is not scaled), so a narrow pad
    // would get fewer labels: the primary divisions are raised so that ROOT's scaling gives back the ones set
    // (3D views paint their axes without that scaling)
    if (const int32_t nDiv = axis_ptr->GetNdivisions(); axisLabel == 'X' && nDiv > 1000 && pad_ptr->GetAbsWNDC() > 0. && !IsViewFrame(pad_ptr, axisHist_ptr)) {
      const double_t padWidth = pad_ptr->GetAbsWNDC();
      const int32_t primary = std::max(1, nDiv % 100);
      // ROOT: Int_t(Float_t(primary) * pad->GetAbsWNDC())
      const auto scaled = [padWidth](int32_t divisions) { return static_cast<int32_t>(static_cast<float>(divisions) * padWidth); };
      int32_t compensated = std::min(99, static_cast<int32_t>(std::ceil(primary / padWidth)));
      while (compensated < 99 && scaled(compensated) < primary)
        ++compensated;
      if (compensated > primary) axis_ptr->SetNdivisions(nDiv - nDiv % 100 + compensated);
    }

    // ranges are only touched if the user set one: ROOT's bin edges are not exactly reproducible (e.g. 4.000000000000001 instead of 4),
    // so feeding the drawn range back in through SetRangeUser would add the overflow bin
    if (userRangeMin || userRangeMax) {
      const bool isBinnedAxis = (histLabel == 'X') || (frameDim > 1 && histLabel == 'Y') || (frameDim == 3 && histLabel == 'Z');
      const bool isPolygonAxis = histLabel != 'Z' && axisHist_ptr->InheritsFrom(TH2Poly::Class());
      if ((is_graph_1d<data_type>() && histLabel == 'X') || (is_graph_2d<data_type>() && histLabel != 'Z') || isPolygonAxis) {
        // the frame of a graph is an empty histogram whose bins mean nothing (for a 2d graph the grid it is interpolated on, for a
        // TH2Poly the grid it finds its polygons with): its x (and y) axis is set to the user values instead of the edges of the bins
        // containing them (the dependent axis has no bins)
        const double_t min = userRangeMin.value_or(axis_ptr->GetBinLowEdge(std::max(axis_ptr->GetFirst(), 1)));
        const double_t max = userRangeMax.value_or(axis_ptr->GetBinUpEdge(std::min(axis_ptr->GetLast(), axis_ptr->GetNbins())));
        if (min < max) {
          axis_ptr->Set(axis_ptr->GetNbins(), min, max);
          axis_ptr->SetRange();
        } else {
          // one side set beyond the other end of the data (SetMinRange(20) for points up to 10)
          logger::warning("The range {} to {} of the {} axis in pad {} is empty: the whole axis is shown.", min, max, axisLabel, padID);
        }
      } else if (isBinnedAxis) {
        // bin selection as in TAxis::SetRangeUser, but a side the user did not set keeps its current bin
        const auto [first, last] = BinRange(axis_ptr, userRangeMin, userRangeMax);
        if (std::max(first, 1) > std::min(last, axis_ptr->GetNbins())) {
          // ROOT shows the whole axis for an empty bin range (one side set beyond the other end of the data, SetMinRange(20) for bins up to 10)
          logger::warning("The range {} to {} of the {} axis in pad {} contains no bins of the data ({} to {}): the whole axis is shown.",
                          userRangeMin ? fmt::format("{}", *userRangeMin) : "its start", userRangeMax ? fmt::format("{}", *userRangeMax) : "its end", axisLabel, padID, axis_ptr->GetXmin(), axis_ptr->GetXmax());
        }
        // never the underflow or overflow bin: a range ending at the edge of the axis (e.g. one extended above) gives the overflow bin,
        // whose low edge ROOT computes as for equal bins, which for variable bins can lie just below the edge (14.999999999999998 for 15)
        axis_ptr->SetRange(std::max(first, 1), std::min(last, axis_ptr->GetNbins()));
      } else {
        // the dependent axis (y of 1d, z of 2d) has no bins: its range is the minimum and maximum of the histogram
        if (userRangeMin) axisHist_ptr->SetMinimum(*userRangeMin);
        if (userRangeMax) axisHist_ptr->SetMaximum(*userRangeMax);
        // one side set beyond the other end of the data (SetMinRange(50) for contents up to 10): ROOT would draw an inverted range
        if (userRangeMin.has_value() != userRangeMax.has_value()) {
          const double_t min = userRangeMin.value_or(axisHist_ptr->GetMinimum());
          const double_t max = userRangeMax.value_or(axisHist_ptr->GetMaximum());
          if (min >= max) {
            logger::warning("The range {} to {} of the {} axis in pad {} is empty: the automatic range is used.", min, max, axisLabel, padID);
            (userRangeMin) ? axisHist_ptr->SetMinimum() : axisHist_ptr->SetMaximum();
          }
        }
      }
    }

    // a log scale needs a positive range: ROOT otherwise complains and draws nothing (or the data outside of the frame);
    // in a 3D view only the binned axes (the height of a lego or surface is ROOT's)
    const bool isDependentAxis = (frameDim < 3) && (histLabel == ((frameDim == 2) ? 'Z' : 'Y'));  // the z axis of 3d data is binned
    if (!pad_ptr->GetView() || !isDependentAxis) {
      const bool isLog = (axisLabel == 'X') ? pad_ptr->GetLogx() : ((axisLabel == 'Y') ? pad_ptr->GetLogy() : pad_ptr->GetLogz());
      auto disableLog = [&]() {
        if (axisLabel == 'X')
          pad_ptr->SetLogx(false);
        else if (axisLabel == 'Y')
          pad_ptr->SetLogy(false);
        else
          pad_ptr->SetLogz(false);
      };
      if (isLog && isDependentAxis) {
        // the smallest positive value is taken from the data drawn inside the frame's ranges (the binned axes are set before):
        // GetMinimum(0.) would return the stored minimum (which every graph frame has)
        double_t positiveMinimum = std::numeric_limits<double_t>::infinity();
        const TAxis* xAxis = axisHist_ptr->GetXaxis();
        const TAxis* yAxis = axisHist_ptr->GetYaxis();
        const auto binRange = [](const TAxis* axis) { return std::make_pair(std::max(axis->GetFirst(), 1), std::min(axis->GetLast(), axis->GetNbins())); };
        if (graphPositivePoints) {
          // the frame of a 1d graph has its x axis set to the range (a point counts where its x error bar reaches into it), that of a
          // 2d graph its bins selected
          const auto inRange = [&](const TAxis* axis, double_t low, double_t high) {
            const auto [first, last] = binRange(axis);
            return high >= axis->GetBinLowEdge(first) && low <= axis->GetBinUpEdge(last);
          };
          for (const auto& [a, b, value] : *graphPositivePoints) {
            const bool shown = (frameDim == 1) ? inRange(xAxis, a, b) : inRange(xAxis, a, a) && inRange(yAxis, b, b);
            if (shown) positiveMinimum = std::min(positiveMinimum, value);
          }
        } else if (polyPositiveBins) {
          // the polygons reaching into the x and y ranges of the frame
          const auto [firstX, lastX] = binRange(xAxis);
          const auto [firstY, lastY] = binRange(yAxis);
          for (const auto& [x0, x1, y0, y1, content] : *polyPositiveBins) {
            const bool shownX = x1 >= xAxis->GetBinLowEdge(firstX) && x0 <= xAxis->GetBinUpEdge(lastX);
            if (shownX && y1 >= yAxis->GetBinLowEdge(firstY) && y0 <= yAxis->GetBinUpEdge(lastY)) positiveMinimum = std::min(positiveMinimum, content);
          }
        } else {
          const auto [firstX, lastX] = binRange(xAxis);
          const auto [firstY, lastY] = (frameDim > 1) ? binRange(yAxis) : std::make_pair(0, 0);
          const auto [firstZ, lastZ] = (frameDim > 2) ? binRange(axisHist_ptr->GetZaxis()) : std::make_pair(0, 0);
          for (int32_t binZ = firstZ; binZ <= lastZ; ++binZ) {
            for (int32_t binY = firstY; binY <= lastY; ++binY) {
              for (int32_t binX = firstX; binX <= lastX; ++binX) {
                const double_t content = axisHist_ptr->GetBinContent(binX, binY, binZ);
                if (content > 0.) positiveMinimum = std::min(positiveMinimum, content);
              }
            }
          }
        }
        if (std::isinf(positiveMinimum)) {
          // a positive user range is enough for ROOT (e.g. an empty histogram as frame), as is a positive minimum stored with a histogram
          // that defines the frame (graph frames always have a stored minimum, computed from the points)
          const bool hasPositiveMinimum = (userRangeMin && *userRangeMin > 0.) || (!graphPositivePoints && axisHist_ptr->GetMinimumStored() > 0.);
          if (!hasPositiveMinimum) {
            logger::warning("Log scale of {} axis in pad {} ignored: the data has no positive values.", axisLabel, padID);
            disableLog();
          }
        } else if (frameDim == 1 && ((axisHist_ptr->GetMinimumStored() != -1111) ? (axisHist_ptr->GetMinimumStored() <= 0.) : (axisHist_ptr->GetMinimum() < 0.))) {
          // ROOT cannot draw a log axis from a stored minimum at or below zero or from negative contents (zero contents it skips itself);
          // a 2d frame gets ROOT's floor for log z below, whether its minimum is stored or not
          axisHist_ptr->SetMinimum(0.5 * positiveMinimum);
        }
      } else if (isLog && !isDependentAxis) {
        const int32_t firstBin = std::max(axis_ptr->GetFirst(), 1);
        const int32_t lastBin = std::min(axis_ptr->GetLast(), axis_ptr->GetNbins());
        const bool isGraphFrame = (axisLabel == 'X') && graphPositiveMinimumX && !std::isinf(*graphPositiveMinimumX);
        if (isGraphFrame && axis_ptr->GetBinLowEdge(firstBin) <= 0. && axis_ptr->GetBinUpEdge(lastBin) > 0.) {
          // the frame of a graph is an empty histogram with linear bins, whose first positive edge can lie above the smallest points:
          // its axis is redefined to start just below them, as ROOT does for a graph drawn on a log x axis (or at a positive minimum set by the user)
          const double_t newMin = (userRangeMin && *userRangeMin > 0.) ? *userRangeMin : 0.9 * *graphPositiveMinimumX;
          axis_ptr->Set(axis_ptr->GetNbins(), newMin, axis_ptr->GetBinUpEdge(lastBin));
          axis_ptr->SetRange();
        } else if (axis_ptr->GetBinLowEdge(firstBin) < 0.) {  // ROOT itself copes with a range starting at zero
          if (axis_ptr->GetBinUpEdge(lastBin) <= 0.) {
            logger::warning("Log scale of {} axis in pad {} ignored: the axis range has no positive values.", axisLabel, padID);
            disableLog();
          } else {
            int32_t bin = firstBin;
            while (axis_ptr->GetBinLowEdge(bin) <= 0.) {
              ++bin;
            }
            // start at the first positive bin edge (ROOT would otherwise draw the bins with negative edges left of the frame); set as bins:
            // SetRangeUser up to the last upper edge would add the overflow bin. The empty frame of a 3d function (whose bins mean nothing)
            // gets an axis starting there instead: ROOT paints an empty 3d histogram on a log axis from its first bin (ROOT bug)
            if constexpr (is_func_3d<data_type>()) {
              axis_ptr->Set(axis_ptr->GetNbins(), axis_ptr->GetBinLowEdge(std::min(bin, lastBin)), axis_ptr->GetBinUpEdge(lastBin));
              axis_ptr->SetRange();
            } else {
              axis_ptr->SetRange(std::min(bin, lastBin), lastBin);
            }
          }
        }
      }
    }
    const bool stillLog = (axisLabel == 'X') ? pad_ptr->GetLogx() : ((axisLabel == 'Y') ? pad_ptr->GetLogy() : pad_ptr->GetLogz());
    if (stillLog && ignoredRangeMin && !(axisLabel == 'Z' && frameDim == 1)) {
      logger::warning("The lower range {} of the {} axis in pad {} is not positive and cannot be shown on a log scale: the axis starts at its automatic minimum.", *ignoredRangeMin, axisLabel, padID);
    }

    if (textFontTitle) axis_ptr->SetTitleFont(*textFontTitle);
    if (textSizeTitle) axis_ptr->SetTitleSize(*textSizeTitle);
    if (textColorTitle) axis_ptr->SetTitleColor(textColorTitle->ToIndex());
    if (textAlphaTitle) axis_ptr->SetTitleColor(TColor::GetColorTransparent(axis_ptr->GetTitleColor(), *textAlphaTitle));
    if (textFontLabel) axis_ptr->SetLabelFont(*textFontLabel);
    if (textSizeLabel) {
      // TGaxis::PaintAxis draws the alphanumeric bin labels of an x or y axis with a relative font at 0.66666 of the label size:
      // compensate, so the size set is the size drawn (the z labels of a palette are painted without this factor)
      const bool shrunkByRoot = axisLabel != 'Z' && axis_ptr->GetLabels() && axis_ptr->GetLabelFont() % 10 != 3;
      axis_ptr->SetLabelSize(shrunkByRoot ? *textSizeLabel / 0.66666f : *textSizeLabel);
    }
    // TGaxis::PaintAxis moves the numbers below a horizontal axis down by a multiple of the label size taken as a fraction of the pad
    // height, while ROOT sizes text by the shorter side of the pad: in a pad taller than wide they would sit too far from the axis
    if (axisLabel == 'X' && !axis_ptr->GetLabels() && axis_ptr->GetLabelFont() % 10 != 3 && !IsViewFrame(pad_ptr, axisHist_ptr)) {
      const double_t width = pad_ptr->GetWw() * pad_ptr->GetAbsWNDC();
      const double_t height = pad_ptr->GetWh() * pad_ptr->GetAbsHNDC();
      if (height > width) {
        const double_t drop = (pad_ptr->GetLogx()) ? (axis_ptr->GetNoExponent() ? 0.82 : 1.15) : 0.8;
        axis_ptr->SetLabelOffset(static_cast<float_t>(axis_ptr->GetLabelOffset() - drop * axis_ptr->GetLabelSize() * (1. - width / height)));
      }
    }
    if (textColorLabel) axis_ptr->SetLabelColor(textColorLabel->ToIndex());
    if (textAlphaLabel) axis_ptr->SetLabelColor(TColor::GetColorTransparent(axis_ptr->GetLabelColor(), *textAlphaLabel));
    if (labelAngle) {
      // with horizontal bars the bin labels are on the vertical axis, where TGaxis paints them horizontally (angle 0, as on a y axis)
      if (axisLabel == 'X' && !hbarFrame && axis_ptr->GetLabels()) {
        // TGaxis::PaintAxis knows four orientations of the bin labels (the bits LabelsOption sets), each with the alignment that fits
        // it; another angle is given to every label (ChangeLabel numbers them by bin, takes only angles >= 0) with the alignment
        // ROOT uses for its tilted ones: a label rising to the right ends at its tick, a falling one starts there
        const float_t angle = *labelAngle;
        const std::array<std::pair<float_t, TAxis::EStatusBits>, 4> orientations{
          {{0.f, TAxis::kLabelsHori}, {90.f, TAxis::kLabelsVert}, {20.f, TAxis::kLabelsUp}, {-20.f, TAxis::kLabelsDown}}};
        bool fixed{};
        for (const auto& [fixedAngle, bit] : orientations) {
          axis_ptr->SetBit(bit, angle == fixedAngle);
          fixed = fixed || angle == fixedAngle;
        }
        if (!fixed) {
          const float_t positive = std::fmod(std::fmod(angle, 360.f) + 360.f, 360.f);
          for (int32_t bin = 1; bin <= axis_ptr->GetNbins(); ++bin) {
            axis_ptr->ChangeLabel(bin, positive, -1., (positive < 180.f) ? 32 : 12);
          }
        }
      } else {
        logger::warning("The label angle of the {} axis of pad {} is not applied: ROOT tilts only the bin labels of an x axis drawn horizontally.", axisLabel, padID);
      }
    }
    // ROOT places a y title with offset 0 automatically next to the widest label, but the gap it leaves is the title size
    // read as a fraction of the pad width (see TGaxis::PaintAxis), i.e. it grows for pads that are wider than high.
    // With a pixel font the gap is the title height in every pad, so relative fonts are converted for the automatic placement.
    // Pixel sizes are stored unscaled (ApplyScale multiplies them by mScale), while the canvas already has its scaled size.
    if (axisLabel == 'Y' && axis_ptr->GetTitleOffset() == 0.f && axis_ptr->GetTitleFont() % 10 <= 2 && axis_ptr->GetTitleSize() > 0.f && axis_ptr->GetTitleSize() < 1.f) {
      const auto [padWidthPixel, padHeightPixel] = PadSizePixel(pad_ptr);
      axis_ptr->SetTitleFont(axis_ptr->GetTitleFont() / 10 * 10 + 3);
      axis_ptr->SetTitleSize(static_cast<float_t>(axis_ptr->GetTitleSize() * std::min(padWidthPixel, padHeightPixel) / mScale));
    }
  }

  if (polargram) {
    // the polar grid as the axes of the pad set it: x is the angle, y the radius; ranges and divisions only where the user set them
    // (otherwise ROOT's from the points), labels as those of the axes
    optional<double_t> minAngle, maxAngle, minRadius, maxRadius;
    optional<int16_t> divAngle, divRadius;
    for (auto [axisLabel, min, max, div] : {std::make_tuple('X', &minAngle, &maxAngle, &divAngle), std::make_tuple('Y', &minRadius, &maxRadius, &divRadius)}) {
      for (const auto* axis : AxisSettings(pad, padDefaults, axisLabel)) {
        if (axis->GetMinRange()) *min = axis->GetMinRange();
        if (axis->GetMaxRange()) *max = axis->GetMaxRange();
        if (axis->GetNumDivisions()) *div = axis->GetNumDivisions();
      }
    }
    if (minAngle || maxAngle) {
      // the labels in units of pi assume a full turn (TGraphPolargram::PaintPolarDivisions): kept for one, values otherwise
      const double_t tMin = minAngle.value_or(polargram->GetTMin());
      const double_t tMax = maxAngle.value_or(polargram->GetTMax());
      if (tMin == 0. && TMath::AreEqualRel(tMax, TMath::TwoPi(), 1e-9)) {
        polargram->ChangeRangePolar(tMin, tMax);
      } else {
        polargram->SetRangePolar(tMin, tMax);
      }
    }
    if (minRadius || maxRadius) polargram->SetRangeRadial(minRadius.value_or(polargram->GetRMin()), maxRadius.value_or(polargram->GetRMax()));
    if (divAngle) polargram->SetNdivPolar(*divAngle);
    // the radius runs over half of the pad: half of ROOT's divisions of an axis spanning it (8 are crowded with the label size of the axes)
    polargram->SetNdivRadial(divRadius.value_or(505));
    const TAxis* angle = axisHist_ptr->GetXaxis();
    const TAxis* radius = axisHist_ptr->GetYaxis();
    polargram->SetPolarLabelFont(angle->GetLabelFont());
    polargram->SetPolarLabelSize(angle->GetLabelSize());
    polargram->SetPolarLabelColor(angle->GetLabelColor());
    polargram->SetRadialLabelFont(radius->GetLabelFont());
    polargram->SetRadialLabelSize(radius->GetLabelSize());
    polargram->SetRadialLabelColor(radius->GetLabelColor());
    polargram->SetLineColor(radius->GetAxisColor());
  }

  if (frameDim > 1) {
    // reset the axis histogram which owns the z axis, while keeping default range defined by the data
    double_t min = axisHist_ptr->GetMinimum();
    double_t max = axisHist_ptr->GetMaximum();
    if constexpr (is_hist_2d<data_type>() || is_func_2d<data_type>()) storedRangeZ = {axisHist_ptr->GetMinimumStored() != -1111, axisHist_ptr->GetMaximumStored() != -1111};
    // ROOT draws a log z axis of a histogram without a positive minimum from min(1, 0.001 * maximum) (THistPainter::PaintInit),
    // but for data drawn SAME on a frame with a stored minimum of zero it leaves out the bins of the lowest contents.
    // This floor is ROOT's default on purpose, not a choice of SciRooPlot: the smallest positive content could be used instead, but it
    // can lie arbitrarily many decades below the maximum and then stretches the colour scale; a plot that needs more sets the Z range.
    if constexpr (is_graph_2d<data_type>()) {
      // the interpolated histogram of a graph can stay below its highest point (and above its lowest one): the z range includes the
      // points (with their errors), as ROOT's own range of the graph (TGraph2D::GetHistogram), unless the user set that end
      const optional<double_t> userMinZ = MinRange(pad, padDefaults, 'Z');
      optional<double_t> userMaxZ;
      for (const auto* axis : AxisSettings(pad, padDefaults, 'Z')) {
        if (axis->GetMaxRange()) userMaxZ = axis->GetMaxRange();
      }
      // a minimum or maximum stored with the graph defining the frame is kept like a range set by the user
      storedRangeZ = {graphStoredZ.first != -1111, graphStoredZ.second != -1111};
      if (!userMinZ) min = (storedRangeZ.first) ? graphStoredZ.first : std::min(min, graphRangeZ.first);
      if (!userMaxZ) max = (storedRangeZ.second) ? graphStoredZ.second : std::max(max, graphRangeZ.second);
    }
    bool contentRange{};
    if (frameDim == 3) {
      // the content of a 3d histogram is the C axis: its range is the minimum and maximum, which give the colours (and the sizes) of the
      // boxes, also of the data drawn later (it takes them from the frame); its title is the part of the z title after ';' (ROOT's place
      // for the title of the colour scale of a 3d histogram)
      optional<string> title;
      bool isLog{};
      for (const auto* contentSettings : AxisSettings(pad, padDefaults, 'C')) {
        const auto& content = *contentSettings;
        if (content.GetMinRange()) min = *content.GetMinRange();
        if (content.GetMaxRange()) max = *content.GetMaxRange();
        contentRange = contentRange || content.GetMinRange() || content.GetMaxRange();
        if (content.GetTitle()) title = content.GetTitle();
        if (content.GetLog()) isLog = *content.GetLog();
        if (content.GetGrid() || content.GetOppositeTicks() || content.GetTimeFormat() || content.GetLabelAngle()) {
          ReportIgnored("A grid, opposite ticks, time format or label angle of the C axis", "the colour scale of a 3d histogram has none of them");
        }
        if (content.GetRoomAboveMax() || content.GetRoomBelowMin()) {
          ReportIgnored("Room above or below the C axis", "the boxes of a 3d histogram are coloured over the range of the C axis, which leaves no room");
        }
        if (content.GetNumDivisions() || content.GetMaxDigits()) {
          ReportIgnored("The divisions or max digits of the C axis", "ROOT takes them for the colour scale of a 3d histogram from its Z axis");
        }
      }
      if (isLog) logger::warning("Log scale of the C axis in pad {} ignored: ROOT colours the boxes of a 3d histogram linearly.", padID);
      if (title) {
        DropEmptyGroups(*title);
        TAxis* zAxis = axisHist_ptr->GetZaxis();
        TString zTitle = zAxis->GetTitle();
        if (zTitle.Index(";") >= 0) zTitle.Remove(zTitle.Index(";"));  // a title of the scale stored with the input is replaced
        if (zTitle.IsNull()) zTitle = " ";                             // ROOT reads the part after ';' only behind a z title
        zAxis->SetTitle((zTitle + ";" + title->data()).Data());
      }
      // ROOT makes the colour scale of a 3d histogram once, with the look of the z axis at that time (the axis settings above came
      // later): it is made again at the next paint
      TList* functions = axisHist_ptr->GetListOfFunctions();
      if (TObject* palette = functions->FindObject("palette")) {
        functions->Remove(palette);
        delete palette;
      }
    }
    if (pad_ptr->GetLogz() && min <= 0. && max > 0.) min = std::min(1., 0.001 * max);
    axisHist_ptr->Reset("ICE");  // reset integral, contents and errors
    axisHist_ptr->SetMinimum(min);
    axisHist_ptr->SetMaximum(max);
    // the colour levels of the scale were computed for the range of the contents when the frame was drawn
    if (contentRange && !axisHist_ptr->TestBit(TH1::kUserContour)) axisHist_ptr->SetContour(axisHist_ptr->GetContour());
    axisHist_ptr->SetMarkerSize(0);
    axisHist_ptr->SetLineWidth(0);
    axisHist_ptr->SetFillStyle(0);
    if (!padProjection.empty() && TString(drawOption).Contains("Z", TString::kIgnoreCase) && !TString(drawOption).Contains("COL", TString::kIgnoreCase)) {  // with COL, ROOT makes one
      // ROOT reads the Z of a projection but its CONT4 painting never paints the colour scale: a copy of the frame drawn as COLZ
      // makes it, with the range and contours of the frame; its cells are moved beyond the pad, so it paints none (on a linear scale
      // ROOT paints empty cells when the minimum is negative, on a log scale cells below the minimum)
      auto* scale = static_cast<TH1*>(axisHist_ptr->Clone(fmt::format("color_scale_pad_{}", padID).data()));
      scale->SetDirectory(nullptr);
      scale->SetBit(kCanDelete);
      scale->GetXaxis()->Set(scale->GetNbinsX(), 1e300, 2e300);
      scale->Draw("COLZ SAME");
    }
  }
  pad_ptr->Update();
}

//**************************************************************************************************
/**
 * The options of 2d graphs that paint the Delaunay triangles (TRI...) or the points (P except POL, SPH and PSR, LINE, ERR) in 3d,
 * as TGraph2D::Paint decides it.
 */
//**************************************************************************************************
void PlotPainter::Paints3dGraph(const string& option, bool& triangles, bool& points)
{
  TString lower(option);
  lower.ToLower();
  triangles = lower.Contains("tri");
  points = (lower.Contains("p") && !lower.Contains("pol") && !lower.Contains("sph") && !lower.Contains("psr")) || lower.Contains("line") || lower.Contains("err");
}

//**************************************************************************************************
/**
 * A flat polar drawing (COL with POL, see THistPainter::PaintColorLevels): x is the angle, y the radius of a disc filling the frame,
 * each cell a TCrown outlined with the line attributes of the data.
 */
//**************************************************************************************************
bool PlotPainter::IsFlatPolar(const string& option)
{
  TString upper(option);
  upper.ToUpper();
  return upper.Contains("POL") && upper.Contains("COL") && !upper.Contains("LEGO") && !upper.Contains("SURF");
}

//**************************************************************************************************
/**
 * The colour boxes of a 3d histogram (BOX2, which COL stands for, see THistPainter::PaintH3): every face turned to the viewer
 * is outlined with the line attributes of the data.
 */
//**************************************************************************************************
bool PlotPainter::IsColorBoxes3d(const string& option)
{
  TString upper(option);
  upper.ToUpper();
  return upper.Contains("BOX2") || upper.Contains("COL");
}

//**************************************************************************************************
/**
 * The 3d views of 2d data (THistPainter::PaintLego, PaintSurface, TGraph2DPainter::PaintTriangles).
 */
//**************************************************************************************************
bool PlotPainter::IsView3d(const string& option)
{
  TString upper(option);
  upper.ToUpper();
  return upper.Contains("LEGO") || upper.Contains("SURF") || upper.Contains("TRI");
}

//**************************************************************************************************
/**
 * A function from an input is drawn where it is defined (its range, within the frame), one given as formula in the plot definition spans the frame;
 * a user range overrides both, except beyond the range of a sampled function.
 */
//**************************************************************************************************
std::pair<double_t, double_t> PlotPainter::FuncRange(const item_state_t& state, char axisLabel, double_t frameMin, double_t frameMax, double_t funcMin, double_t funcMax,
                                                     const optional<double_t>& userMin, const optional<double_t>& userMax)
{
  const auto& data = state.data;
  const bool isSampledFunc = state.isSampledFunc;

  double_t min = frameMin;
  double_t max = frameMax;
  if (data->GetDataSource() != kUserFunctions && funcMin < frameMax && funcMax > frameMin) {
    min = std::max(min, funcMin);
    max = std::min(max, funcMax);
  }
  if (userMin) min = *userMin;
  if (userMax) max = *userMax;
  if (isSampledFunc && (min < funcMin || max > funcMax)) {
    if (userMin || userMax) logger::warning("Function {} has no values outside of [{:g}, {:g}] in {}: it is stored as values sampled over this range.", data->GetName(), funcMin, funcMax, axisLabel);
    min = std::max(min, funcMin);
    max = std::min(max, funcMax);
  }
  return std::make_pair(min, max);
}

//**************************************************************************************************
/**
 * A warning about the data, given once: not for the frame copy of the first data (index 0), nor for an input prepared a second time.
 */
//**************************************************************************************************
template <typename... Args>
void PlotPainter::Warn(item_state_t& state, Args&&... args)
{
  pad_state_t& padState = state.padState;
  auto& repeated = state.repeated;
  auto& dataIndex = padState.dataIndex;

  if (dataIndex != 0 && !repeated) logger::warning(std::forward<decltype(args)>(args)...);
}

//**************************************************************************************************
/**
 * Functions stored with an input histogram or graph (fit results) do not follow modifiers or divisions: not drawn then.
 */
//**************************************************************************************************
template <typename ObjT>
void PlotPainter::DropStoredFunctions(item_state_t& state, ObjT* ptr, const char* reason)
{
  auto warn = [&](auto&&... args) { return Warn(state, std::forward<decltype(args)>(args)...); };

  using T = std::remove_pointer_t<decltype(ptr)>;
  if constexpr (std::is_base_of_v<TH1, T> || std::is_base_of_v<TGraph, T>) {
    TList* functions = ptr->GetListOfFunctions();
    if (!functions) return;
    for (TObjLink* link = functions->FirstLink(); link;) {
      TObject* obj = link->GetObject();
      link = link->Next();
      if (!obj->InheritsFrom(TF1::Class())) continue;
      warn("The function {} stored with {} is not drawn: it does not follow the {}.", obj->GetName(), ptr->GetName(), reason);
      delete functions->Remove(obj);
    }
  }
}

//**************************************************************************************************
/**
 * Apply the data modifiers stored in mods to ptr (plain data, ratio inputs and ratio result).
 */
//**************************************************************************************************
template <typename PtrT>
void PlotPainter::ApplyModifiers(item_state_t& state, PtrT& ptr, const Plot::Pad::Data& mods)
{
  auto warn = [&](auto&&... args) { return Warn(state, std::forward<decltype(args)>(args)...); };
  auto dropStoredFunctions = [&](auto&&... args) { return DropStoredFunctions(state, std::forward<decltype(args)>(args)...); };

  using ptr_type = std::decay_t<decltype(ptr)>;
  auto warnUnsupported = [&](bool requested, const char* what) {
    if (requested) warn("{} is not supported for {} ({}), ignoring.", what, ptr->GetName(), ptr->ClassName());
  };
  const auto& m = mods.Modify();
  // only modifiers that change something keep the stored fits (not DivideBinWidth(false), Scale(1), Rebin(1), Smooth(0), nor a
  // scale factor that is not positive, which is ignored below)
  auto changes = [](const auto& value, auto neutral) { return value && *value != neutral; };
  auto scales = [](const optional<double_t>& factor) { return factor && *factor > 0. && *factor != 1.; };
  if (m.scaleBinWidthNorm || m.normMaximum || m.cumulative || m.divideBinWidth.value_or(false) || scales(m.scaleFactor) ||
      changes(m.rebinGroupX, 1) || changes(m.rebinGroupY, 1) || changes(m.rebinGroupZ, 1) || scales(m.scaleAxisX) ||
      scales(m.scaleAxisY) || scales(m.scaleAxisZ) || changes(m.nIterSmooth, 0))
    dropStoredFunctions(ptr, "modifiers of the data");
  // multiplies a factor of Scale (axis 0) or ScaleX/Y/Z into product; a factor that is not positive is ignored
  auto multiplyFactor = [&](optional<double_t>& product, const optional<double_t>& factor, char axis = 0) {
    if (!factor) return;
    if (*factor <= 0.) {
      if (axis) {
        warn("Scale factor for {} axis of {} must be positive, ignoring.", axis, ptr->GetName());
      } else {
        warn("Scale factor for {} must be positive, ignoring.", ptr->GetName());
      }
      return;
    }
    product = product.value_or(1.) * *factor;
  };

  auto scaleAxis = [&](int16_t axisIndex, const optional<double_t>& factor) {
    if (!factor) return;
    if constexpr (is_hist_1d<ptr_type>()) {
      if (axisIndex == 1) return;  // y is bin content here - equivalent to Scale()
    } else if constexpr (is_hist_2d<ptr_type>()) {
      if (axisIndex == 2) return;  // z is bin content here - equivalent to Scale()
    } else if constexpr (is_graph_1d<ptr_type>()) {
      if (axisIndex == 1) return;  // same operation as Scale() for a graph
    } else if constexpr (is_graph_2d<ptr_type>()) {
      if (axisIndex == 2) return;  // z is the value Scale() targets for a 2d graph
    } else if constexpr (is_func<ptr_type>()) {
      return;
    }
    string axisLetter = GetAxisStr(axisIndex);
    if (*factor <= 0.) {
      warn("Scale factor for {} axis of {} must be positive, ignoring.", axisLetter, ptr->GetName());
      return;
    }
    if constexpr (is_hist_1d<ptr_type>()) {
      if (axisIndex == 2) {
        warn("Cannot scale z axis of 1d histogram {} (no such axis).", ptr->GetName());
        return;
      }
      ScaleAxis(ptr, axisIndex, *factor);
    } else if constexpr (is_hist_2d<ptr_type>()) {
      if (dynamic_cast<TH2Poly*>(ptr)) {
        warnUnsupported(true, (axisIndex == 0) ? "ScaleX" : "ScaleY");  // the polygons would stay where they are
        return;
      }
      ScaleAxis(ptr, axisIndex, *factor);
    } else if constexpr (is_hist_3d<ptr_type>()) {
      ScaleAxis(ptr, axisIndex, *factor);
    } else if constexpr (is_graph_1d<ptr_type>()) {
      if (axisIndex == 2) {
        warn("Cannot scale z axis of graph {} (graphs have no z-axis).", ptr->GetName());
        return;
      }
      ScaleGraphAxis(ptr, axisIndex, *factor);
    } else if constexpr (is_graph_2d<ptr_type>()) {
      ScaleGraphAxis(ptr, axisIndex, *factor);
    }
  };
  scaleAxis(0, mods.GetScaleAxisX());
  scaleAxis(1, mods.GetScaleAxisY());
  scaleAxis(2, mods.GetScaleAxisZ());

  if constexpr (is_hist<ptr_type>()) {
    // a TProfile3D is held as a TH3, a TH2Poly as a TH2
    [[maybe_unused]] TH2Poly* poly = (is_hist_2d<ptr_type>()) ? dynamic_cast<TH2Poly*>(ptr) : nullptr;
    constexpr bool isProfileType = is_one_of_v<ptr_type, TProfile*, TProfile2D*>();
    const bool isProfile = isProfileType || dynamic_cast<TProfile3D*>(ptr);
    // ROOT merges the bins of a labelled axis but keeps the first labels, so the drawn categories are wrong
    auto warnLabelled = [&](const TAxis* axis, const optional<uint16_t>& group, const char* what) {
      if (group && axis->GetLabels()) warn("{} on the labelled {} axis of {} mixes up the labels.", what, axis->GetName(), ptr->GetName());
    };
    warnLabelled(ptr->GetXaxis(), mods.GetRebinGroupX(), "RebinX");
    warnLabelled(ptr->GetYaxis(), mods.GetRebinGroupY(), "RebinY");
    warnLabelled(ptr->GetZaxis(), mods.GetRebinGroupZ(), "RebinZ");
    // ROOT moves the bins that make no complete group into the overflow (its warning does not say so): said here, ROOT's is silenced
    auto warnIncomplete = [&](const TAxis* axis, const optional<uint16_t>& group, const char* what) {
      // not where rebinning is refused below (TH2Poly, TProfile3D)
      if (group && *group > 0 && *group <= axis->GetNbins() && axis->GetNbins() % *group && !ptr->InheritsFrom(TH2Poly::Class()) &&
          !ptr->InheritsFrom(TProfile3D::Class())) {
        warn("{}({}): the {} bins of {} are no multiple of it, the last {} are left out (the axis ends at {:g}).", what, *group,
             axis->GetNbins(), ptr->GetName(), axis->GetNbins() % *group, axis->GetBinUpEdge(axis->GetNbins() / *group * *group));
      }
    };
    warnIncomplete(ptr->GetXaxis(), mods.GetRebinGroupX(), "RebinX");
    warnIncomplete(ptr->GetYaxis(), mods.GetRebinGroupY(), "RebinY");
    warnIncomplete(ptr->GetZaxis(), mods.GetRebinGroupZ(), "RebinZ");
    {  // ROOT's warnings silenced for the rebinning only
      auto quietRebin = ignore_root_messages_below(kError);
      if constexpr (is_hist_2d<ptr_type>()) {
        if (poly) {
          // a TH2Poly has no bins along its axes (ROOT aborts)
          warnUnsupported(mods.GetRebinGroupX().has_value(), "RebinX");
          warnUnsupported(mods.GetRebinGroupY().has_value(), "RebinY");
        } else if (mods.GetRebinGroupX() && mods.GetRebinGroupY()) {
          ptr->Rebin2D(*mods.GetRebinGroupX(), *mods.GetRebinGroupY());
        } else if (mods.GetRebinGroupX()) {
          ptr->RebinX(*mods.GetRebinGroupX());
        } else if (mods.GetRebinGroupY()) {
          ptr->RebinY(*mods.GetRebinGroupY());
        }
        warnUnsupported(mods.GetRebinGroupZ().has_value(), "RebinZ");
      } else if constexpr (is_hist_3d<ptr_type>()) {
        if (dynamic_cast<TProfile3D*>(ptr)) {
          // TProfile3D has no rebinning of its own: the one of TH3 leaves every bin empty (ROOT bug)
          warnUnsupported(mods.GetRebinGroupX() || mods.GetRebinGroupY() || mods.GetRebinGroupZ(), "Rebinning");
        } else {
          if (mods.GetRebinGroupX()) ptr->RebinX(*mods.GetRebinGroupX());
          if (mods.GetRebinGroupY()) ptr->RebinY(*mods.GetRebinGroupY());
          if (mods.GetRebinGroupZ()) ptr->RebinZ(*mods.GetRebinGroupZ());
        }
      } else {
        if (mods.GetRebinGroupX() && *mods.GetRebinGroupX() > ptr->GetNbinsX()) {
          warn("RebinX({}) needs at least as many bins, {} has {}, ignoring.", *mods.GetRebinGroupX(), ptr->GetName(), ptr->GetNbinsX());
        } else if (mods.GetRebinGroupX()) {
          ptr->RebinX(*mods.GetRebinGroupX());
        }
        warnUnsupported(mods.GetRebinGroupY().has_value(), "RebinY");
        warnUnsupported(mods.GetRebinGroupZ().has_value(), "RebinZ");
      }
    }
    bool isDensity = mods.GetScaleBinWidthNorm() && *mods.GetScaleBinWidthNorm();
    if (isProfile) {
      // ROOT does not divide the mean values of a profile by the bin widths
      warnUnsupported(mods.GetDivideBinWidth() && *mods.GetDivideBinWidth(), "DivideBinWidth");
    } else if (mods.GetDivideBinWidth() && *mods.GetDivideBinWidth()) {
      if constexpr (is_hist_2d<ptr_type>()) {
        if (poly) {
          // TH1::Scale(1, "width") leaves a TH2Poly as it is: each polygon is divided by its area
          for (TObject* obj : *poly->GetBins()) {
            auto* polyBin = static_cast<TH2PolyBin*>(obj);
            const int32_t bin = polyBin->GetBinNumber();
            const double_t area = polyBin->GetArea();
            poly->SetBinContent(bin, (area > 0.) ? poly->GetBinContent(bin) / area : 0.);
            poly->SetBinError(bin, (area > 0.) ? poly->GetBinError(bin) / area : 0.);
          }
        } else {
          ptr->Scale(1., "width");
        }
      } else {
        ptr->Scale(1., "width");
      }
      isDensity = true;
    }
    bool smoothable = true;
    if constexpr (is_hist_3d<ptr_type>()) smoothable = false;  // ROOT smooths 1d and 2d histograms only
    if constexpr (is_hist_2d<ptr_type>()) smoothable = !poly;
    if (mods.GetNiterSmooth() && !smoothable) {
      warnUnsupported(true, "Smooth");
    } else if (mods.GetNiterSmooth()) {
      if constexpr (isProfileType) {
        // ROOT's Smooth would treat the sums stored in a profile as its contents: the mean values are smoothed instead,
        // over the filled bins only (an empty bin has no mean), keeping the entries and the spread (i.e. the error) of each bin
        const int32_t nTimes = *mods.GetNiterSmooth();
        double_t* sums = ptr->GetArray();                     // per bin: sum of weight * y
        double_t* sumsSquared = ptr->GetSumw2()->GetArray();  // per bin: sum of weight * y^2
        auto setMean = [&](int32_t bin, double_t newMean) {
          const double_t entries = ptr->GetBinEntries(bin);
          const double_t mean = sums[bin] / entries;
          sums[bin] = newMean * entries;
          sumsSquared[bin] += entries * (newMean * newMean - mean * mean);
        };
        if constexpr (is_one_of_v<ptr_type, TProfile*>()) {
          // the algorithm of TH1::Smooth, applied to the sequence of filled bins
          vector<int32_t> bins;
          vector<double_t> means;
          for (int32_t bin = 1; bin <= ptr->GetNbinsX(); ++bin) {
            if (ptr->GetBinEntries(bin) == 0.) continue;
            bins.push_back(bin);
            means.push_back(ptr->GetBinContent(bin));
          }
          if (means.size() < 3) {
            warn("Smooth needs at least three filled bins in profile histogram {}, ignoring.", ptr->GetName());
          } else {
            TH1::SmoothArray(static_cast<int32_t>(means.size()), means.data(), nTimes);
            for (size_t i = 0; i < bins.size(); ++i) {
              setMean(bins[i], means[i]);
            }
          }
        } else {
          // the kernel of TH2::Smooth, normalized to the filled bins it covers (empty bins do not pull the means towards zero)
          unique_ptr<TH2D> means(ptr->ProjectionXY("smoothedMeans", "e"));
          means->SetDirectory(nullptr);
          means->Sumw2(false);
          unique_ptr<TH2D> filled(static_cast<TH2D*>(means->Clone("smoothedFilled")));
          filled->SetDirectory(nullptr);
          for (int32_t bin = 0; bin < ptr->GetNcells(); ++bin) {
            filled->SetBinContent(bin, (ptr->GetBinEntries(bin) == 0.) ? 0. : 1.);
          }
          means->Smooth(nTimes);
          filled->Smooth(nTimes);
          for (int32_t bin = 0; bin < ptr->GetNcells(); ++bin) {
            if (ptr->GetBinEntries(bin) == 0. || ptr->IsBinUnderflow(bin) || ptr->IsBinOverflow(bin)) continue;
            setMean(bin, means->GetBinContent(bin) / filled->GetBinContent(bin));
          }
        }
      } else if (ptr->GetDimension() == 1 && ptr->GetNbinsX() < 3) {
        warn("Smooth needs at least three bins in histogram {}, ignoring.", ptr->GetName());
      } else {
        ptr->Smooth(*mods.GetNiterSmooth());
      }
    }

    // Normalize and NormalizeToMaximum within a range in x: the bins reaching into it, as for the drawn range
    // (the axis range is set for the computation only; nullopt if the range contains no bins)
    auto inRangeX = [&](const optional<double_t>& min, const optional<double_t>& max, const char* modifier, auto compute) -> optional<double_t> {
      if (!min && !max) return compute();
      if constexpr (is_hist_2d<ptr_type>()) {
        if (poly) {
          // a TH2Poly ignores the ranges of its axes: the sum or largest content of the polygons reaching into the range
          const bool maximum = (std::string_view(modifier) == "NormalizeToMaximum");
          optional<double_t> result;
          for (TObject* obj : *poly->GetBins()) {
            auto* polyBin = static_cast<TH2PolyBin*>(obj);
            if ((min && polyBin->GetXMax() <= *min) || (max && polyBin->GetXMin() >= *max)) continue;
            const double_t content = poly->GetBinContent(polyBin->GetBinNumber()) * ((!maximum && isDensity) ? polyBin->GetArea() : 1.);
            result = (maximum) ? std::max(result.value_or(content), content) : result.value_or(0.) + content;
          }
          if (!result) warn("The range of {} for {} contains no bins, ignoring.", modifier, ptr->GetName());
          return result;
        }
      }
      TAxis* axis = ptr->GetXaxis();
      auto [first, last] = BinRange(axis, min, max);
      first = std::max(first, 1);
      last = std::min(last, axis->GetNbins());
      if (first > last) {
        warn("The range of {} for {} contains no bins, ignoring.", modifier, ptr->GetName());
        return nullopt;
      }
      const int32_t oldFirst = axis->GetFirst();
      const int32_t oldLast = axis->GetLast();
      const bool hadRange = axis->TestBit(TAxis::kAxisRange);
      axis->SetRange(first, last);
      const double_t result = compute();
      axis->SetRange(oldFirst, oldLast);
      axis->SetBit(TAxis::kAxisRange, hadRange);
      return result;
    };

    // normalize to the integral before Cumulative, then normalized distribution results in CDF ending at 1
    if (mods.GetScaleBinWidthNorm()) {
      string scaleMode = (isDensity) ? "width" : "";
      const optional<double_t> integral = inRangeX(mods.GetNormMin(), mods.GetNormMax(), "Normalize", [&]() { return ptr->Integral(scaleMode.data()); });
      if (integral) {
        if (*integral == 0.) {
          warn("Cannot normalize {} because its integral is zero.", ptr->GetName());
        } else {
          ptr->Scale(1. / *integral);
        }
      }
    }

    if constexpr (is_hist_1d<ptr_type>()) {
      if (mods.GetCumulative()) {
        if constexpr (is_one_of_v<ptr_type, TProfile*>()) {
          warn("Cumulative is not supported for profile histogram {} (bin content would become invalid), ignoring.", ptr->GetName());
        } else {
          if (isDensity) {
            // the running sum of a density has to be weighted with the bin widths to yield its integral
            for (int32_t i = 1; i <= ptr->GetNbinsX(); ++i) {
              double_t width = ptr->GetBinWidth(i);
              ptr->SetBinContent(i, ptr->GetBinContent(i) * width);
              ptr->SetBinError(i, ptr->GetBinError(i) * width);
            }
          }
          TH1* cumulativeHist = ptr->GetCumulative(*mods.GetCumulative(), "");  // under the name of the data (ROOT appends _cumulative)
          cumulativeHist->SetDirectory(nullptr);
          cumulativeHist->SetBit(kCanDelete);
          MoveProjectionTitles(ptr, cumulativeHist);
          delete ptr;
          ptr = cumulativeHist;
        }
      }
    } else {
      warnUnsupported(mods.GetCumulative().has_value(), "Cumulative");
    }
    // remaining factors act on the final shape (NormalizeToMaximum after Cumulative -> CDF ending at 1)
    optional<double_t> scaleFactor;
    if (mods.GetNormMaximum() && *mods.GetNormMaximum()) {
      const optional<double_t> maximum = inRangeX(mods.GetNormMaximumMin(), mods.GetNormMaximumMax(), "NormalizeToMaximum", [&]() { return ptr->GetMaximum(); });
      if (maximum) {
        if (*maximum == 0.) {
          warn("Cannot normalize {} to maximum because it is zero.", ptr->GetName());
        } else {
          scaleFactor = 1. / *maximum;
        }
      }
    }
    multiplyFactor(scaleFactor, mods.GetScaleFactor());
    if constexpr (is_hist_1d<ptr_type>()) {
      multiplyFactor(scaleFactor, mods.GetScaleAxisY(), 'y');
    } else if constexpr (is_hist_2d<ptr_type>()) {
      multiplyFactor(scaleFactor, mods.GetScaleAxisZ(), 'z');
    }
    if (scaleFactor) ptr->Scale(*scaleFactor);
  } else if constexpr (is_graph_1d<ptr_type>()) {
    warnUnsupported(mods.GetRebinGroupX().has_value(), "RebinX");
    warnUnsupported(mods.GetRebinGroupY().has_value(), "RebinY");
    warnUnsupported(mods.GetRebinGroupZ().has_value(), "RebinZ");
    warnUnsupported(mods.GetDivideBinWidth() && *mods.GetDivideBinWidth(), "DivideBinWidth");
    warnUnsupported(mods.GetShowOverflowBins().has_value(), "ShowOverflowBins");
    warnUnsupported(mods.GetCumulative().has_value(), "Cumulative");
    if (mods.GetNiterSmooth()) {
      if (ptr->GetN() < 4) {
        warn("Smooth needs at least 4 points, {} has {}; ignoring.", ptr->GetName(), ptr->GetN());
      } else {
        SortGraph(ptr);
        TGraphSmooth smoother;
        for (uint16_t iter = 0; iter < *mods.GetNiterSmooth(); ++iter) {
          TGraph* smoothGraph = smoother.SmoothSuper(ptr);
          for (int32_t i = 0; i < ptr->GetN(); ++i) {
            ptr->GetY()[i] = smoothGraph->GetY()[i];
          }
        }
      }
    }
    optional<double_t> scaleFactor;
    // the values of the points within a range in x (all points without a range)
    auto valuesInRangeX = [&](const optional<double_t>& min, const optional<double_t>& max) {
      vector<double_t> values;
      values.reserve(ptr->GetN());
      for (int32_t i = 0; i < ptr->GetN(); ++i) {
        if ((min && ptr->GetPointX(i) < *min) || (max && ptr->GetPointX(i) > *max)) continue;
        values.push_back(ptr->GetPointY(i));
      }
      return values;
    };
    if (mods.GetScaleBinWidthNorm()) {
      // like for histograms the integral is the sum of the values (TGraph::Integral is the area of the closed polygon)
      const vector<double_t> values = valuesInRangeX(mods.GetNormMin(), mods.GetNormMax());
      const double_t integral = std::accumulate(values.begin(), values.end(), 0.);
      if (values.empty() && ptr->GetN() > 0) {
        warn("The range of Normalize for {} contains no points, ignoring.", ptr->GetName());
      } else if (integral == 0.) {
        warn("Cannot normalize graph {} because the sum of its values is zero.", ptr->GetName());
      } else {
        scaleFactor = 1. / integral;
      }
      if (*mods.GetScaleBinWidthNorm()) {
        warn("Cannot normalize graph {} by width.", ptr->GetName());
      }
    }
    if (mods.GetNormMaximum() && *mods.GetNormMaximum()) {
      const vector<double_t> values = valuesInRangeX(mods.GetNormMaximumMin(), mods.GetNormMaximumMax());
      if (ptr->GetN() == 0) {
        warn("Cannot normalize graph {} to maximum because it has no points.", ptr->GetName());
      } else if (values.empty()) {
        warn("The range of NormalizeToMaximum for {} contains no points, ignoring.", ptr->GetName());
      } else if (double_t maxY = *std::max_element(values.begin(), values.end()); maxY == 0.) {
        warn("Cannot normalize graph {} to maximum because it is zero.", ptr->GetName());
      } else {
        scaleFactor = 1. / maxY;
      }
    }
    multiplyFactor(scaleFactor, mods.GetScaleFactor());
    multiplyFactor(scaleFactor, mods.GetScaleAxisY(), 'y');
    if (scaleFactor) ScaleGraphAxis(ptr, 1, *scaleFactor);
  } else if constexpr (is_graph_2d<ptr_type>()) {
    warnUnsupported(mods.GetRebinGroupX().has_value(), "RebinX");
    warnUnsupported(mods.GetRebinGroupY().has_value(), "RebinY");
    warnUnsupported(mods.GetRebinGroupZ().has_value(), "RebinZ");
    warnUnsupported(mods.GetDivideBinWidth() && *mods.GetDivideBinWidth(), "DivideBinWidth");
    warnUnsupported(mods.GetNiterSmooth().has_value(), "Smooth");
    warnUnsupported(mods.GetShowOverflowBins().has_value(), "ShowOverflowBins");
    warnUnsupported(mods.GetCumulative().has_value(), "Cumulative");
    warnUnsupported(mods.GetScaleBinWidthNorm().has_value(), "Normalize");
    optional<double_t> scaleFactor;
    if (mods.GetNormMaximum() && *mods.GetNormMaximum()) {
      // the largest value of the points within the range in x (all points without a range)
      const auto& min = mods.GetNormMaximumMin();
      const auto& max = mods.GetNormMaximumMax();
      optional<double_t> maxZ;
      for (int32_t i = 0; i < ptr->GetN(); ++i) {
        if ((min && ptr->GetX()[i] < *min) || (max && ptr->GetX()[i] > *max)) continue;
        maxZ = std::max(maxZ.value_or(ptr->GetZ()[i]), ptr->GetZ()[i]);
      }
      if (ptr->GetN() == 0) {
        warn("Cannot normalize graph {} to maximum because it has no points.", ptr->GetName());
      } else if (!maxZ) {
        warn("The range of NormalizeToMaximum for {} contains no points, ignoring.", ptr->GetName());
      } else if (*maxZ == 0.) {
        warn("Cannot normalize graph {} to maximum because it is zero.", ptr->GetName());
      } else {
        scaleFactor = 1. / *maxZ;
      }
    }
    multiplyFactor(scaleFactor, mods.GetScaleFactor());
    multiplyFactor(scaleFactor, mods.GetScaleAxisZ(), 'z');
    if (scaleFactor) ScaleGraphAxis(ptr, 2, *scaleFactor);
  } else if constexpr (is_func<ptr_type>()) {
    warnUnsupported(mods.GetRebinGroupX().has_value(), "RebinX");
    warnUnsupported(mods.GetRebinGroupY().has_value(), "RebinY");
    warnUnsupported(mods.GetRebinGroupZ().has_value(), "RebinZ");
    warnUnsupported(mods.GetDivideBinWidth() && *mods.GetDivideBinWidth(), "DivideBinWidth");
    warnUnsupported(mods.GetNiterSmooth().has_value(), "Smooth");
    warnUnsupported(mods.GetNormMaximum() && *mods.GetNormMaximum(), "NormalizeToMaximum");
    warnUnsupported(mods.GetScaleBinWidthNorm().has_value(), "Normalize");
    warnUnsupported(mods.GetShowOverflowBins().has_value(), "ShowOverflowBins");
    warnUnsupported(mods.GetCumulative().has_value(), "Cumulative");
    if constexpr (is_func_1d<ptr_type>()) {
      warnUnsupported(mods.GetScaleAxisZ().has_value(), "ScaleZ");
    }
    optional<double_t> domainX, domainY, domainZ, contentFactor;
    multiplyFactor(domainX, mods.GetScaleAxisX(), 'x');
    if constexpr (is_func_2d<ptr_type>() || is_func_3d<ptr_type>()) multiplyFactor(domainY, mods.GetScaleAxisY(), 'y');
    if constexpr (is_func_3d<ptr_type>()) multiplyFactor(domainZ, mods.GetScaleAxisZ(), 'z');
    multiplyFactor(contentFactor, mods.GetScaleFactor());
    if constexpr (is_func_1d<ptr_type>()) {
      multiplyFactor(contentFactor, mods.GetScaleAxisY(), 'y');
    } else if constexpr (is_func_2d<ptr_type>()) {
      multiplyFactor(contentFactor, mods.GetScaleAxisZ(), 'z');
    }
    const double_t domainFactorX = domainX.value_or(1.);
    const double_t domainFactorY = domainY.value_or(1.);
    const double_t domainFactorZ = domainZ.value_or(1.);
    if (domainFactorX != 1. || domainFactorY != 1. || domainFactorZ != 1. || contentFactor) {
      if constexpr (is_func_1d<ptr_type>()) {
        ptr = ScaleFunc(ptr, domainFactorX, contentFactor.value_or(1.));
      } else if constexpr (is_func_2d<ptr_type>()) {
        ptr = ScaleFunc(ptr, domainFactorX, domainFactorY, contentFactor.value_or(1.));
      } else if constexpr (is_func_3d<ptr_type>()) {
        ptr = ScaleFunc(ptr, domainFactorX, domainFactorY, domainFactorZ, contentFactor.value_or(1.));
      }
    }
  }
}

//**************************************************************************************************
/**
 * NaN or infinite values (from the input or a division by zero) spoil the axis ranges and ROOT then draws nothing; removed before
 * the modifiers, which would spread them (Normalize divides by a NaN integral, Cumulative carries one into every later bin), and
 * before a division (a spline through one gives NaN everywhere).
 */
//**************************************************************************************************
template <typename ObjT>
void PlotPainter::RemoveInvalid(item_state_t& state, ObjT* ptr, const string& name)
{
  auto warn = [&](auto&&... args) { return Warn(state, std::forward<decltype(args)>(args)...); };

  using ptr_type = decltype(ptr);
  if constexpr (is_hist<ptr_type>()) {
    // a profile keeps sums per bin: they are cleared, so the bin is empty (a TProfile3D is held as a TH3)
    auto clearProfileBin = [](auto* profile, int32_t bin) {
      profile->SetBinEntries(bin, 0.);
      profile->TArrayD::SetAt(0., bin);
      if (profile->GetSumw2N()) profile->GetSumw2()->SetAt(0., bin);
      if (profile->GetBinSumw2()->fN) profile->GetBinSumw2()->SetAt(0., bin);
    };
    TProfile3D* profile3d{};
    if constexpr (is_hist_3d<ptr_type>()) profile3d = dynamic_cast<TProfile3D*>(ptr);
    int32_t nInvalid{}, nBeyondFloat{};
    for (int32_t bin = 0; bin < ptr->GetNcells(); ++bin) {
      // ROOT finds minima and maxima (axis ranges, colour scales) only below the largest float
      if (std::abs(ptr->GetBinContent(bin)) > std::numeric_limits<float>::max() && std::isfinite(ptr->GetBinContent(bin))) ++nBeyondFloat;
      if (!std::isfinite(ptr->GetBinContent(bin)) || !std::isfinite(ptr->GetBinError(bin))) {
        if constexpr (is_one_of_v<ptr_type, TProfile*, TProfile2D*>()) {
          clearProfileBin(ptr, bin);
        } else {
          if (profile3d) {
            clearProfileBin(profile3d, bin);
          } else {
            ptr->SetBinContent(bin, 0.);
            ptr->SetBinError(bin, 0.);
          }
        }
        ++nInvalid;
      }
    }
    if (nInvalid) warn("{} has {} with NaN or infinite values, which are drawn as zero.", name, plural(nInvalid, "bin"));
    if (nBeyondFloat) {
      warn("{} has {} with contents beyond the largest float (3.4e38): ROOT finds axis ranges only below it, so they are wrong (a log scale fails).", name,
           plural(nBeyondFloat, "bin"));
    }
  } else if constexpr (is_graph_1d<ptr_type>() || is_graph_2d<ptr_type>()) {
    int32_t nInvalid{};
    for (int32_t i = ptr->GetN() - 1; i >= 0; --i) {
      bool invalid{};
      if constexpr (is_graph_2d<ptr_type>()) {
        invalid = !std::isfinite(ptr->GetX()[i]) || !std::isfinite(ptr->GetY()[i]) || !std::isfinite(ptr->GetZ()[i]) ||
                  !std::isfinite(ptr->GetErrorX(i)) || !std::isfinite(ptr->GetErrorY(i)) || !std::isfinite(ptr->GetErrorZ(i));
      } else {
        invalid = !std::isfinite(ptr->GetPointX(i)) || !std::isfinite(ptr->GetPointY(i)) || !std::isfinite(ptr->GetErrorXlow(i)) ||
                  !std::isfinite(ptr->GetErrorXhigh(i)) || !std::isfinite(ptr->GetErrorYlow(i)) || !std::isfinite(ptr->GetErrorYhigh(i));
      }
      if (invalid) {
        RemoveGraphPoint(ptr, i);
        ++nInvalid;
      }
    }
    if (nInvalid) warn("{} has {} with NaN or infinite values, which are not drawn.", name, plural(nInvalid, "point"));
  }
}

//**************************************************************************************************
/**
 * An input of a ratio gets its modifiers (Numer() / Denom()) before the division, then a profile is divided as the histogram of
 * its means (its errors as set by its error option): TProfile::Divide accepts only profiles and estimates the errors poorly;
 * returns whether it was a profile.
 */
//**************************************************************************************************
bool PlotPainter::PrepareOperand(item_state_t& state, data_ptr_t& operand, const Plot::Pad::Data::modify_t& operandModify, const string& name)
{
  auto& data = state.data;
  auto applyModifiers = [&](auto&&... args) { return ApplyModifiers(state, std::forward<decltype(args)>(args)...); };
  auto removeInvalid = [&](auto&&... args) { return RemoveInvalid(state, std::forward<decltype(args)>(args)...); };

  Plot::Pad::Data mods = *data;
  mods.Modify() = operandModify;
  std::visit(
    [&](auto& ptr) {
      removeInvalid(ptr, name);
      applyModifiers(ptr, mods);
    },
    operand);
  return ProfileMeans(operand);
}

//**************************************************************************************************
/**
 * Prepares the data (the division of a ratio, invalid values, modifiers), decides its drawing option alias and draws it.
 */
//**************************************************************************************************
template <typename T>
void PlotPainter::ProcessData(item_state_t& state, T&& data_ptr)
{
  pad_state_t& padState = state.padState;
  auto& data = state.data;
  auto& isSampledFunc = state.isSampledFunc;
  auto& numeratorMeans = state.numeratorMeans;
  auto& denominatorMeans = state.denominatorMeans;
  auto& padDefaults = padState.padDefaults;
  auto& fillDefaults = padState.fillDefaults;
  auto& dataBuffer = padState.dataBuffer;
  auto& fail = padState.fail;
  auto& pad = padState.pad;
  auto& pad_ptr = padState.pad_ptr;
  auto& axisHist_ptr = padState.axisHist_ptr;
  auto& clippedSteps = padState.clippedSteps;
  auto& dataIndex = padState.dataIndex;
  auto& usedDefaults = padState.usedDefaults;
  auto drawItem = [&](auto&&... args) { return DrawItem(state, std::forward<decltype(args)>(args)...); };
  auto warn = [&](auto&&... args) { return Warn(state, std::forward<decltype(args)>(args)...); };
  auto applyModifiers = [&](auto&&... args) { return ApplyModifiers(state, std::forward<decltype(args)>(args)...); };
  auto removeInvalid = [&](auto&&... args) { return RemoveInvalid(state, std::forward<decltype(args)>(args)...); };
  auto prepareOperand = [&](auto&&... args) { return PrepareOperand(state, std::forward<decltype(args)>(args)...); };

  using data_type = std::decay_t<decltype(data_ptr)>;
  if constexpr (is_func<data_type>()) isSampledFunc = !data_ptr->GetFormula();
  // the drawing option alias of the data (without one, the rule below decides once the data is final)
  optional<draw_option_t> optionAlias = data->GetDrawOptionAlias();

  if (data->GetType() == "ratio") {
    auto data_as_ratio = std::dynamic_pointer_cast<Plot::Pad::Ratio>(data);
    bool binomialErrors = data_as_ratio->GetCorrelated();
    auto processDenominator = [&](auto&& denom_data_ptr) {
      using denom_data_type = std::decay_t<decltype(denom_data_ptr)>;
      if constexpr (is_func<denom_data_type>()) {
        // a formula of the plot definition has no range of its own (only ROOT's default [0, 1]): it divides the whole numerator
        if (data_as_ratio->GetDenomDataSource() == kUserFunctions) {
          if constexpr (is_hist<data_type>()) {
            const TAxis* x = data_ptr->GetXaxis();
            const TAxis* y = data_ptr->GetYaxis();
            const TAxis* z = data_ptr->GetZaxis();
            denom_data_ptr->SetRange(x->GetXmin(), y->GetXmin(), z->GetXmin(), x->GetXmax(), y->GetXmax(), z->GetXmax());
          } else if constexpr (is_graph_1d<data_type>()) {
            if (data_ptr->GetN() > 0) {
              denom_data_ptr->SetRange(TMath::MinElement(data_ptr->GetN(), data_ptr->GetX()), TMath::MaxElement(data_ptr->GetN(), data_ptr->GetX()));
            }
          }
        }
      }
      auto unsupported = [&]() {
        logger::error("Cannot divide {} ({}) by {} ({}): not supported.", data_ptr->GetName(), data_ptr->ClassName(), denom_data_ptr->GetName(), denom_data_ptr->ClassName());
        fail = true;
      };
      if constexpr (is_hist<data_type>()) {
        if constexpr (is_func<denom_data_type>() || is_hist<denom_data_type>() || (is_hist_1d<data_type>() && is_graph_1d<denom_data_type>())) {
          if (!Divide(data_ptr, denom_data_ptr, binomialErrors, dataIndex != 0)) fail = true;
          if constexpr (is_hist_2d<data_type>()) {
            data_ptr->GetZaxis()->SetTitle("ratio");
          } else if constexpr (is_hist_1d<data_type>()) {
            data_ptr->GetYaxis()->SetTitle("ratio");
          }
        } else {
          unsupported();
        }
      } else if constexpr (is_graph_1d<data_type>()) {
        if constexpr (is_graph_1d<denom_data_type>() || is_hist_1d<denom_data_type>() || is_func_1d<denom_data_type>()) {
          // a numerator without errors gets the errors of the denominator (as TH1::Divide): a copy with errors of the same kind
          if constexpr (is_graph_1d<denom_data_type>() || is_hist_1d<denom_data_type>()) {
            bool asymmetric = false;
            bool denomErrors = true;  // histograms always have errors
            if constexpr (is_graph_1d<denom_data_type>()) {
              asymmetric = denom_data_ptr->GetEYlow() && denom_data_ptr->GetEYhigh();
              denomErrors = asymmetric || denom_data_ptr->GetEY();
            }
            if (denomErrors && data_ptr->IsA() == TGraph::Class()) {
              TGraph* withErrors = (asymmetric) ? static_cast<TGraph*>(new TGraphAsymmErrors(data_ptr->GetN(), data_ptr->GetX(), data_ptr->GetY()))
                                                : static_cast<TGraph*>(new TGraphErrors(data_ptr->GetN(), data_ptr->GetX(), data_ptr->GetY()));
              withErrors->SetName(data_ptr->GetName());
              withErrors->SetTitle(data_ptr->GetTitle());
              withErrors->GetXaxis()->SetTitle(data_ptr->GetXaxis()->GetTitle());
              withErrors->SetBit(kCanDelete, data_ptr->TestBit(kCanDelete));
              CopyStyle(*data_ptr, *withErrors);
              if (auto source = mSourceHists.extract(data_ptr)) {
                source.key() = withErrors;
                mSourceHists.insert(std::move(source));
              }
              delete data_ptr;
              data_ptr = withErrors;
            }
          }
          if (!Divide(data_ptr, denom_data_ptr, binomialErrors, dataIndex != 0)) fail = true;
          data_ptr->GetHistogram()->GetYaxis()->SetTitle("ratio");
        } else {
          unsupported();
        }
      } else {
        unsupported();
      }
      delete denom_data_ptr;
    };

    // retrieve the actual pointer to the denominator data
    optional<data_ptr_t> rawDenomData;
    try {
      rawDenomData = GetDataClone(dataBuffer.at(data_as_ratio->GetDenomDataSource()).at(data_as_ratio->GetDenomName() + data_as_ratio->GetDenomDataInfo().GetNameSuffix()).get(), data_as_ratio->GetDenomProjInfo());
    } catch (std::out_of_range&) {
      rawDenomData = nullopt;
    }

    if (rawDenomData) {
      denominatorMeans = prepareOperand(*rawDenomData, data_as_ratio->GetDenomModify(), data_as_ratio->GetDenomName());
      if (binomialErrors && (numeratorMeans || denominatorMeans)) {
        warn("Ratio {}: binomial errors (SetCorrelated) are not defined for the mean values of a profile, ignoring.", data->GetName());
        binomialErrors = false;
      }
      std::visit(processDenominator, *rawDenomData);
    } else {
      fail = true;
    }

    // modifiers on the ratio itself are applied as requested, but most of them are rarely meaningful there
    constexpr auto hint = "use Numer() / Denom() to apply it to the inputs before the division";
    if (data->GetRebinGroupX() || data->GetRebinGroupY() || data->GetRebinGroupZ()) {
      warn("Rebinning ratio {} sums ratio values; {}.", data_ptr->GetName(), hint);
    }
    if (data->GetCumulative()) {
      warn("Cumulative of ratio {} sums ratio values; {}.", data_ptr->GetName(), hint);
    }
    if (data->GetDivideBinWidth() && *data->GetDivideBinWidth()) {
      warn("Dividing ratio {} by bin width is rarely meaningful; {}.", data_ptr->GetName(), hint);
    }
    if (data->GetScaleBinWidthNorm()) {
      warn("Normalizing ratio {} to its integral is rarely meaningful; {}.", data_ptr->GetName(), hint);
    }
  }  // end ratio code

  removeInvalid(data_ptr, data->GetName());

  applyModifiers(data_ptr, *data);
  // the axis histogram of the graph (stored with the input or made before the modifiers changed the points) is fitted to the final points
  if constexpr (is_graph_1d<data_type>()) data_ptr->SetBit(TGraph::kResetHisto);

  // without a drawing option SciRooPlot decides, the same for histograms and graphs: a fill is drawn (hist, for a graph area),
  // so is a marker set for the data (points; the defaults of the pad always set one), data with errors is a measurement (points),
  // data without errors a shape (hist, for a graph line); 2d histograms are coloured
  if (!optionAlias && !data->GetDrawOption()) {
    if constexpr (is_hist_1d<data_type>() || is_graph_1d<data_type>()) {
      constexpr bool isHist = is_hist_1d<data_type>();
      // the fill the data will be drawn with: set for the data, else the next default of the pad (unless the style of the input is kept), else the input's
      auto drawnFill = [&](const auto& dataValue, uint16_t index, const auto& padValues, const auto& defaultValues, int16_t inputValue) -> std::decay_t<decltype(*dataValue)> {
        if (dataValue) return *dataValue;
        if (!data->GetKeepInputStyle()) {
          if (auto value = get_first(pick(index, padValues), pick(index, defaultValues))) return *value;
        }
        return inputValue;
      };
      // the input's fill is visible only with a colour (ROOT's default is colour 0 with a solid style): see keptWithoutFill
      const int16_t inputFillStyle = (data->GetKeepInputStyle() && !data->GetFillStyle() && data_ptr->GetFillColor() == 0) ? 0 : data_ptr->GetFillStyle();
      const int16_t fillStyle = drawnFill(data->GetFillStyle(), usedDefaults.fillStyle, pad.GetDefaultFillStyles(), padDefaults.GetDefaultFillStyles(), inputFillStyle);
      const color_t fillColor = drawnFill(data->GetFillColor(), usedDefaults.color, pad.GetDefaultFillColors(), fillDefaults(pad, *data), data_ptr->GetFillColor());
      bool singlePoint{};  // a line through a single point draws nothing
      if constexpr (!isHist) singlePoint = (data_ptr->GetN() == 1);
      if (fillStyle != 0 && fillColor != 0) {
        optionAlias = (isHist) ? hist : area;
      } else if (data->GetMarkerStyle() || data->GetMarkerSize() || HasErrors(data_ptr, true) || singlePoint) {
        optionAlias = points;
      } else {
        optionAlias = (isHist) ? hist : line;
      }
    } else if constexpr (is_hist_2d<data_type>() || is_hist_3d<data_type>()) {
      optionAlias = colz;
    }
  }

  // the clipped aliases draw a histogram as a graph: ROOT cuts the lines of a graph at the frame, while it moves the bins of a histogram
  // that lie outside of the frame onto its edge (HIST, and HIST C on a log axis); the axis frame (first data) is the histogram itself
  auto drawClipped = [&](TH1* histogram, draw_option_t clippedAlias) {
    if (dataIndex == 0) {
      drawItem(histogram, (clippedAlias == curve_clipped) ? curve : ((clippedAlias == hist_clipped) ? hist : hist_open));
      return;
    }
    if (data->GetMinRangeY() || data->GetMaxRangeY()) {
      logger::warning("The y range of {} is not applied with drawing option {}: it is cut at the frame.", data->GetName(), draw_option_name(clippedAlias));
    }
    if (!pad_ptr->GetView()) {
      if (data->GetMinRangeX() || data->GetMaxRangeX()) SetBinRange(histogram->GetXaxis(), data->GetMinRangeX(), data->GetMaxRangeX());
      LimitToFrame(histogram->GetXaxis(), axisHist_ptr->GetXaxis());  // the bins of the histogram that are drawn (also for its statistics and placeholders)
    }
    TGraph* graph = ToClippedGraph(histogram, clippedAlias, pad_ptr->GetLogy());
    mSourceHists[graph].reset(histogram);
    drawItem(graph, clippedAlias);
    if (clippedAlias != curve_clipped) clippedSteps.emplace_back(graph, clippedAlias == hist_open_clipped);
  };
  if (optionAlias && (*optionAlias == hist_clipped || *optionAlias == hist_open_clipped || *optionAlias == curve_clipped)) {
    if constexpr (is_hist_1d<data_type>()) {
      drawClipped(data_ptr, *optionAlias);
      return;
    } else if constexpr (is_graph_1d<data_type>()) {
      // a graph is cut at the frame anyway: only the steps need its bins
      if (*optionAlias != curve_clipped) {
        // steps show no errors: no warning about asymmetric ones
        if (TH1* histogram = ToHist(data_ptr, dataIndex != 0, false)) {
          mSourceHists.erase(data_ptr);  // deleted by ToHist: a later graph at that address is another one
          drawClipped(histogram, *optionAlias);
          return;
        }
        if (dataIndex != 0) logger::warning("Drawing option {} is not available for graph {}, using the default of ROOT.", draw_option_name(*optionAlias), data->GetName());
        optionAlias.reset();
      }
    }
  }

  // histograms and graphs can be drawn with the same aliases: where ROOT has no option for one of them, it is drawn as the other
  if constexpr (is_hist_1d<data_type>()) {
    if (optionAlias && !defaultDrawOptions_Hist.count(*optionAlias) && defaultDrawOptions_Graph.count(*optionAlias)) {
      TGraph* graph = ToGraph(data_ptr);
      mSourceHists[graph].reset(data_ptr);
      drawItem(graph, optionAlias);
      return;
    }
  } else if constexpr (is_graph_1d<data_type>()) {
    if (optionAlias && !defaultDrawOptions_Graph.count(*optionAlias) && defaultDrawOptions_Hist.count(*optionAlias)) {
      // the warning about asymmetric errors (drawn with their mean) only where the option shows errors
      static const std::set<draw_option_t> withoutErrors{hist, hist_open, hbars, area_smooth};
      if (TH1* hist = ToHist(data_ptr, dataIndex != 0, !withoutErrors.count(*optionAlias))) {
        mSourceHists.erase(data_ptr);  // deleted by ToHist: a later graph at that address is another one
        drawItem(hist, optionAlias);
        return;
      }
      if (dataIndex != 0) logger::warning("Drawing option {} is not available for graph {}, using the default of ROOT.", draw_option_name(*optionAlias), data->GetName());
      optionAlias.reset();
    }
  }
  drawItem(data_ptr, optionAlias);
}

//**************************************************************************************************
/**
 * Whether the frame histogram of a pad is drawn with HBAR: ROOT then draws its x axis vertically and its contents horizontally,
 * so the axes of the pad (X horizontal, Y vertical) are the y and x axis of the histogram.
 */
//**************************************************************************************************
bool PlotPainter::IsHBarFrame(TPad* pad, const TH1* frame)
{
  if (frame->GetDimension() != 1) return false;
  for (TObjLink* link = pad->GetListOfPrimitives()->FirstLink(); link; link = link->Next()) {
    if (link->GetObject() == frame) return TString(link->GetOption()).Contains("HBAR", TString::kIgnoreCase);
  }
  return false;
}

//**************************************************************************************************
/**
 * Whether ROOT paints the axes of a frame histogram in a 3D view (THistPainter::PaintLegoAxis: 3d histograms, LEGO, SURF, TRI).
 */
//**************************************************************************************************
bool PlotPainter::IsViewFrame(TPad* pad, const TH1* frame)
{
  if (frame->GetDimension() == 3) return true;
  for (TObjLink* link = pad->GetListOfPrimitives()->FirstLink(); link; link = link->Next()) {
    if (link->GetObject() != frame) continue;
    TString option(link->GetOption());
    option.ToUpper();
    return option.Contains("LEGO") || option.Contains("SURF") || option.Contains("TRI");
  }
  return false;
}

//**************************************************************************************************
/**
 * Room beyond the drawn data (Axis::SetRoomAboveMax/SetRoomBelowMin), applied once all data of a pad are drawn and before the pad is painted:
 * the frame gets the range in which the requested fraction of it stays free beyond the data (in decades on a log axis).
 * The room is free of all data of the pad: along a binned axis (x, y of 2d) everything the data would show without the frame (histograms
 * within their own data range, all points of graphs), along the value axis (y of 1d, z of 2d) all values within the final ranges of the
 * binned axes, which are set first; error bars count where ROOT counts them for its automatic range (histograms drawn with E, graphs).
 * Histograms, cut at the frame while they were drawn, are cut at the new one. The reference function is a guide, not data, and functions
 * given as formula span the frame: neither counts, both are stretched to a new frame. Functions from inputs and histograms drawn with a
 * clipped alias count as drawn (within the automatic frame).
 */
//**************************************************************************************************
void PlotPainter::AddAxisRoom(TPad* pad, TH1* frame, const Plot::Pad& padDefaults, const Plot::Pad& padSettings, uint8_t padID, const Plot::Pad::Data* refFunc, string& padSetup)
{
  const bool is2d = frame->GetDimension() > 1;
  const char valueAxis = (is2d) ? 'Z' : 'Y';
  // a frame drawn with HBAR shows the histogram's x axis vertically: the room set for an axis of the pad (X horizontal, Y vertical)
  // is applied to the axis of the histogram drawn there, and everything below works on the axes of the histogram
  const bool hbar = IsHBarFrame(pad, frame);
  auto histLabel = [hbar](char padLabel) { return (hbar && padLabel != 'Z') ? ((padLabel == 'X') ? 'Y' : 'X') : padLabel; };
  struct room_t {
    char axisLabel;
    optional<double_t> below;
    optional<double_t> above;
  };
  vector<room_t> rooms;  // in the order X, Y, Z: the binned axes before the value axis
  for (char axisLabel : {'X', 'Y', 'Z'}) {
    if (mCommonRanges.count({padID, axisLabel})) continue;  // the common range of a shared axis includes the room of its pads
    room_t room{axisLabel, {}, {}};
    const optional<double_t> userMin = MinRange(padSettings, padDefaults, axisLabel);
    optional<double_t> userMax;
    for (const auto* axis : AxisSettings(padSettings, padDefaults, axisLabel)) {
      if (axis->GetRoomBelowMin()) room.below = axis->GetRoomBelowMin();
      if (axis->GetRoomAboveMax()) room.above = axis->GetRoomAboveMax();
      if (axis->GetMaxRange()) userMax = axis->GetMaxRange();
    }
    if (!room.below && !room.above) continue;
    if (axisLabel == 'Z' && !is2d) {
      logger::warning("Room on the Z axis of pad {} ignored: the pad has no z axis.", padID);
      continue;
    }
    if (room.below && userMin) {
      logger::warning("Room below the minimum of the {} axis of pad {} ignored: its minimum is set explicitly.", axisLabel, padID);
      room.below.reset();
    }
    if (room.above && userMax) {
      logger::warning("Room above the maximum of the {} axis of pad {} ignored: its maximum is set explicitly.", axisLabel, padID);
      room.above.reset();
    }
    if (!room.below && !room.above) continue;
    if (room.below.value_or(0.) + room.above.value_or(0.) >= 1.) {
      logger::warning("Room on the {} axis of pad {} ignored: room below and above add up to {:g} of the frame.", axisLabel, padID, room.below.value_or(0.) + room.above.value_or(0.));
      continue;
    }
    if (histLabel(axisLabel) != valueAxis && ((histLabel(axisLabel) == 'X') ? frame->GetXaxis() : frame->GetYaxis())->GetLabels()) {
      logger::warning("Room on the {} axis of pad {} ignored: an axis with bin labels cannot be widened.", axisLabel, padID);
      continue;
    }
    room.axisLabel = histLabel(axisLabel);
    rooms.push_back(room);
  }
  if (rooms.empty()) return;

  auto isLogAxis = [pad, &histLabel](char axisLabel) {
    const char padLabel = histLabel(axisLabel);  // the swap is its own inverse
    return (padLabel == 'X') ? pad->GetLogx() : ((padLabel == 'Y') ? pad->GetLogy() : pad->GetLogz());
  };
  // the ranges drawn so far: those of the frame, painted when it was set up (data drawn on top do not change them)
  double_t xMin{}, yMin{}, xMax{}, yMax{};
  pad->GetRangeAxis(xMin, yMin, xMax, yMax);
  xMin = FromPad(xMin, pad->GetLogx());
  xMax = FromPad(xMax, pad->GetLogx());
  yMin = FromPad(yMin, pad->GetLogy());
  yMax = FromPad(yMax, pad->GetLogy());
  if (hbar) {
    std::swap(xMin, yMin);
    std::swap(xMax, yMax);
  }
  // functions given as formula in the plot definition span the frame along the axes without a range of their own
  auto spansFrame = [](const Plot::Pad::Data& data, char axisLabel) {
    if (data.GetDataSource() != kUserFunctions) return false;
    return (axisLabel == 'X') ? !data.GetMinRangeX() && !data.GetMaxRangeX() : !data.GetMinRangeY() && !data.GetMaxRangeY();
  };

  // extent of the drawn data along an axis
  auto dataExtent = [&](char axisLabel) -> optional<std::pair<double_t, double_t>> {
    const bool isLog = isLogAxis(axisLabel);
    double_t lowest = std::numeric_limits<double_t>::infinity();
    double_t highest = -std::numeric_limits<double_t>::infinity();
    auto add = [&](double_t low, double_t value, double_t high) {
      if (!std::isfinite(value)) return;
      if (isLog) {
        if (value <= 0.) return;  // not drawn on a log axis
        if (low <= 0.) low = value;
      }
      lowest = std::min(lowest, low);
      highest = std::max(highest, high);
    };
    auto inX = [&](double_t low, double_t up) { return up >= xMin && low <= xMax; };
    auto inY = [&](double_t low, double_t up) { return up >= yMin && low <= yMax; };
    auto addHist1d = [&](const TH1* hist, bool withErrors) {
      const TAxis* xAxis = hist->GetXaxis();
      for (int32_t bin = xAxis->GetFirst(); bin <= xAxis->GetLast(); ++bin) {
        const double_t low = xAxis->GetBinLowEdge(bin);
        const double_t up = xAxis->GetBinUpEdge(bin);
        if (axisLabel == 'X') {
          add(low, xAxis->GetBinCenter(bin), up);
        } else if (inX(low, up)) {
          const double_t content = hist->GetBinContent(bin);
          const double_t error = (withErrors) ? hist->GetBinError(bin) : 0.;
          add(content - error, content, content + error);
        }
      }
    };
    TIter next(pad->GetListOfPrimitives());
    while (TObject* obj = next()) {
      auto definition = mDataDefinitions.find(obj);
      if (obj == frame || definition == mDataDefinitions.end() || definition->second.get() == refFunc) continue;
      const Plot::Pad::Data& data = *definition->second;
      if (auto* hist = dynamic_cast<TH1*>(obj)) {
        if (hist->GetDimension() == 1) {
          TString option = next.GetOption();
          option.ToUpper();
          option.ReplaceAll("SAME", "");
          option.ReplaceAll("TEXT", "");
          if (axisLabel != 'Z') addHist1d(hist, option.Contains("E"));
        } else if (auto* poly = dynamic_cast<TH2Poly*>(hist)) {
          for (TObject* binObj : *poly->GetBins()) {
            auto* bin = static_cast<TH2PolyBin*>(binObj);
            if (axisLabel == 'X') {
              add(bin->GetXMin(), 0.5 * (bin->GetXMin() + bin->GetXMax()), bin->GetXMax());
            } else if (axisLabel == 'Y') {
              add(bin->GetYMin(), 0.5 * (bin->GetYMin() + bin->GetYMax()), bin->GetYMax());
            } else if (inX(bin->GetXMin(), bin->GetXMax()) && inY(bin->GetYMin(), bin->GetYMax())) {
              add(bin->GetContent(), bin->GetContent(), bin->GetContent());
            }
          }
        } else if (hist->GetDimension() == 2) {
          const TAxis* xAxis = hist->GetXaxis();
          const TAxis* yAxis = hist->GetYaxis();
          if (axisLabel == 'X') {
            add(xAxis->GetBinLowEdge(xAxis->GetFirst()), xAxis->GetBinCenter(xAxis->GetFirst()), xAxis->GetBinUpEdge(xAxis->GetLast()));
          } else if (axisLabel == 'Y') {
            add(yAxis->GetBinLowEdge(yAxis->GetFirst()), yAxis->GetBinCenter(yAxis->GetFirst()), yAxis->GetBinUpEdge(yAxis->GetLast()));
          } else {
            for (int32_t binX = xAxis->GetFirst(); binX <= xAxis->GetLast(); ++binX) {
              if (!inX(xAxis->GetBinLowEdge(binX), xAxis->GetBinUpEdge(binX))) continue;
              for (int32_t binY = yAxis->GetFirst(); binY <= yAxis->GetLast(); ++binY) {
                if (!inY(yAxis->GetBinLowEdge(binY), yAxis->GetBinUpEdge(binY))) continue;
                const double_t content = hist->GetBinContent(binX, binY);
                add(content, content, content);
              }
            }
          }
        }
      } else if (auto* graph = dynamic_cast<TGraph*>(obj)) {
        // a histogram drawn with a clipped alias is a graph of steps down to far below the frame: its bins count
        if (auto source = mSourceHists.find(obj); source != mSourceHists.end() && graph->IsA() == TGraph::Class()) {
          if (axisLabel != 'Z') addHist1d(source->second.get(), false);
          continue;
        }
        for (int32_t i = 0; i < graph->GetN(); ++i) {
          double_t x = graph->GetPointX(i);
          double_t y = graph->GetPointY(i);
          double_t xLow = std::max(graph->GetErrorXlow(i), 0.);  // -1 for graphs without errors
          double_t xHigh = std::max(graph->GetErrorXhigh(i), 0.);
          double_t yLow = std::max(graph->GetErrorYlow(i), 0.);
          double_t yHigh = std::max(graph->GetErrorYhigh(i), 0.);
          if (hbar) {
            // drawn in pad coordinates: its x runs along the value axis of the horizontal bars, its y along their binned axis
            std::swap(x, y);
            std::swap(xLow, yLow);
            std::swap(xHigh, yHigh);
          }
          if (axisLabel == 'X') {
            add(x - xLow, x, x + xHigh);
          } else if (axisLabel == 'Y' && (is2d || inX(x, x))) {
            // y of a 2d pad is binned (all points), y of a 1d pad the value axis (points within the x range)
            add(y - yLow, y, y + yHigh);
          }
        }
      } else if (auto* graph2d = dynamic_cast<TGraph2D*>(obj)) {
        for (int32_t i = 0; i < graph2d->GetN(); ++i) {
          const double_t x = graph2d->GetX()[i];
          const double_t y = graph2d->GetY()[i];
          const double_t z = graph2d->GetZ()[i];
          if (axisLabel == 'X') {
            add(x, x, x);
          } else if (axisLabel == 'Y') {
            add(y, y, y);
          } else if (axisLabel == 'Z' && inX(x, x) && inY(y, y)) {
            const double_t error = std::max(graph2d->GetErrorZ(i), 0.);  // -1 for graphs without errors
            add(z - error, z, z + error);
          }
        }
      } else if (auto* func = dynamic_cast<TF1*>(obj)) {
        if (func->GetNdim() == 1) {
          // drawn in pad coordinates: with horizontal bars its x runs along their value axis and its values along their binned axis
          const double_t from = std::max(func->GetXmin(), (hbar) ? yMin : xMin);
          const double_t to = std::min(func->GetXmax(), (hbar) ? yMax : xMax);
          if (axisLabel == histLabel('X')) {
            if (!spansFrame(data, 'X')) add(func->GetXmin(), 0.5 * (func->GetXmin() + func->GetXmax()), func->GetXmax());
          } else if (axisLabel == histLabel('Y')) {
            const int32_t nPoints = std::max(func->GetNpx(), 2);
            for (int32_t i = 0; i < nPoints && from < to; ++i) {
              const double_t value = func->Eval(from + (to - from) * i / (nPoints - 1));
              add(value, value, value);
            }
          }
        } else if (auto* func2d = dynamic_cast<TF2*>(func); func2d && func->GetNdim() == 2) {
          const double_t from = std::max(func2d->GetXmin(), xMin);
          const double_t to = std::min(func2d->GetXmax(), xMax);
          if (axisLabel == 'X') {
            if (!spansFrame(data, 'X')) add(func2d->GetXmin(), 0.5 * (func2d->GetXmin() + func2d->GetXmax()), func2d->GetXmax());
          } else if (axisLabel == 'Y') {
            if (!spansFrame(data, 'Y')) add(func2d->GetYmin(), 0.5 * (func2d->GetYmin() + func2d->GetYmax()), func2d->GetYmax());
          } else {
            const double_t fromY = std::max(func2d->GetYmin(), yMin);
            const double_t toY = std::min(func2d->GetYmax(), yMax);
            const int32_t nX = std::max(func2d->GetNpx(), 2);
            const int32_t nY = std::max(func2d->GetNpy(), 2);
            for (int32_t i = 0; i < nX && from < to; ++i) {
              for (int32_t j = 0; j < nY && fromY < toY; ++j) {
                const double_t value = func2d->Eval(from + (to - from) * i / (nX - 1), fromY + (toY - fromY) * j / (nY - 1));
                add(value, value, value);
              }
            }
          }
        }
      }
    }
    if (lowest > highest) return {};
    return std::make_pair(lowest, highest);
  };

  // the histograms of the pad were cut at the frame along its binned axes: for the extent they get back their own data range,
  // and afterwards they are cut at the frame again, whether it changed or not (candles and violins are not cut along their distribution)
  auto dataHists = [&](char axisLabel) {
    vector<std::pair<TH1*, const Plot::Pad::Data*>> hists;
    TIter next(pad->GetListOfPrimitives());
    while (TObject* obj = next()) {
      auto* hist = dynamic_cast<TH1*>(obj);
      auto definition = mDataDefinitions.find(obj);
      if (!hist || hist == frame || definition == mDataDefinitions.end() || (axisLabel == 'Y' && hist->GetDimension() < 2)) continue;
      TString option = next.GetOption();
      option.ToUpper();
      const bool isCandle = option.Contains("CANDLE") || option.Contains("VIOLIN");
      if (isCandle && ((option.Contains("CANDLEY") || option.Contains("VIOLINY")) == (axisLabel == 'X'))) continue;
      hists.emplace_back(hist, definition->second.get());
    }
    return hists;
  };

  // functions given as formula span the widened frame along the axis of their own (pad coordinates), the reference function always
  auto stretchFunctions = [&](char axisLabel, double_t newMin, double_t newMax) {
    const char funcLabel = histLabel(axisLabel);  // the swap is its own inverse
    for (TObject* obj : *pad->GetListOfPrimitives()) {
      auto* func = dynamic_cast<TF1*>(obj);
      auto definition = mDataDefinitions.find(obj);
      if (!func || definition == mDataDefinitions.end()) continue;
      if (definition->second.get() != refFunc && !spansFrame(*definition->second, funcLabel)) continue;
      if (func->GetNdim() == 1 && funcLabel == 'X') {
        func->SetRange(newMin, newMax);
      } else if (auto* func2d = dynamic_cast<TF2*>(func); func2d && func->GetNdim() == 2) {
        double_t x1{}, y1{}, x2{}, y2{};
        func2d->GetRange(x1, y1, x2, y2);
        ((funcLabel == 'X') ? x1 : y1) = newMin;
        ((funcLabel == 'X') ? x2 : y2) = newMax;
        func2d->SetRange(x1, y1, x2, y2);
      }
    }
  };

  for (const auto& room : rooms) {
    const char axisLabel = room.axisLabel;
    const bool isBinned = (axisLabel != valueAxis);
    const auto cutHists = (isBinned) ? dataHists(axisLabel) : vector<std::pair<TH1*, const Plot::Pad::Data*>>{};
    for (auto& [hist, data] : cutHists) {
      TAxis* histAxis = (axisLabel == 'X') ? hist->GetXaxis() : hist->GetYaxis();
      histAxis->SetRange();
      if (axisLabel == 'X') {
        SetBinRange(histAxis, data->GetMinRangeX(), data->GetMaxRangeX());
      } else {
        SetBinRange(histAxis, data->GetMinRangeY(), data->GetMaxRangeY());
      }
    }
    auto cutAtFrame = make_scope_guard([&, axisLabel]() {
      for (auto& [hist, data] : cutHists) {
        TAxis* histAxis = (axisLabel == 'X') ? hist->GetXaxis() : hist->GetYaxis();
        LimitToFrame(histAxis, (axisLabel == 'X') ? frame->GetXaxis() : frame->GetYaxis());
        // the setup of the pad restores the range of data with a range of its own after an interactive zoom: with the range cut at this frame
        const bool hasRange = (axisLabel == 'X') ? (data->GetMinRangeX() || data->GetMaxRangeX()) : (data->GetMinRangeY() || data->GetMaxRangeY());
        if (!hasRange) continue;
        padSetup += HistCommand(hist->GetName(), fmt::format(" h->Get{}axis()->SetRange({}, {});", axisLabel, histAxis->GetFirst(), histAxis->GetLast()));
      }
    });
    const auto extent = dataExtent(axisLabel);
    if (!extent) continue;  // nothing drawn to leave room around
    const bool isLog = isLogAxis(axisLabel);
    auto toFrame = [isLog](double_t value) { return (isLog) ? std::log10(value) : value; };
    const double_t curMin = (axisLabel == 'X') ? xMin : ((axisLabel == 'Y') ? yMin : frame->GetMinimum());
    const double_t curMax = (axisLabel == 'X') ? xMax : ((axisLabel == 'Y') ? yMax : frame->GetMaximum());
    if (isLog && !(curMin > 0. && curMax > 0.)) continue;

    // the data take up the rest of the frame: (max - dataMax) = above * (max - min), (dataMin - min) = below * (max - min);
    // a side without room keeps its value
    const double_t dataMin = toFrame(extent->first);
    const double_t dataMax = toFrame(extent->second);
    double_t frameMin = toFrame(curMin);
    double_t frameMax = toFrame(curMax);
    if (room.below && room.above) {
      const double_t length = (dataMax - dataMin) / (1. - *room.below - *room.above);
      frameMin = dataMin - *room.below * length;
      frameMax = dataMax + *room.above * length;
    } else if (room.above) {
      frameMax = (dataMax - *room.above * frameMin) / (1. - *room.above);
    } else {
      frameMin = (dataMin - *room.below * frameMax) / (1. - *room.below);
    }
    if (!(frameMin < frameMax)) {
      logger::warning("Room on the {} axis of pad {} ignored: the drawn data leave no valid range.", histLabel(axisLabel), padID);
      continue;
    }
    const double_t newMin = FromPad(frameMin, isLog);
    const double_t newMax = FromPad(frameMax, isLog);

    if (axisLabel == valueAxis) {
      if (!is2d) {
        // 1d histograms are drawn from the lower edge of the frame (their stored minimum): it moves along
        for (TObject* obj : *pad->GetListOfPrimitives()) {
          auto* hist = dynamic_cast<TH1*>(obj);
          if (hist && hist != frame && hist->GetDimension() == 1 && TMath::AreEqualRel(hist->GetMinimumStored(), curMin, 1e-9)) hist->SetMinimum(newMin);
        }
        yMin = newMin;
        yMax = newMax;
      }
      frame->SetMinimum(newMin);
      frame->SetMaximum(newMax);
      if (hbar) stretchFunctions(axisLabel, newMin, newMax);
      continue;
    }
    // a binned axis: the contents of the frame only matter for the automatic range of a 1d frame, which is kept as drawn;
    // its bins are then replaced by bins spanning the new range (data drawn on top are placed by the coordinates of the pad, not by these bins)
    if (!is2d) {
      if (frame->GetMinimumStored() == -1111) frame->SetMinimum(yMin);
      if (frame->GetMaximumStored() == -1111) frame->SetMaximum(yMax);
    }
    TAxis* axis = (axisLabel == 'X') ? frame->GetXaxis() : frame->GetYaxis();
    axis->Set(axis->GetNbins(), newMin, newMax);
    axis->SetRange();
    ((axisLabel == 'X') ? xMin : yMin) = newMin;
    ((axisLabel == 'X') ? xMax : yMax) = newMax;
    stretchFunctions(axisLabel, newMin, newMax);
  }
}

//**************************************************************************************************
/**
 * Collects the colour indices used by an object and everything it contains, except the colours predefined by ROOT.
 */
//**************************************************************************************************
void PlotPainter::CollectCustomColors(TObject* obj, std::set<int32_t>& colors)
{
  if (!obj) return;
  auto add = [&colors](int32_t color) {
    const bool isPredefined = (color >= 0 && color <= 50) || (color >= 300 && color <= kGray + 3);  // basic colours and the colour wheel
    if (!isPredefined && gROOT->GetColor(color)) colors.insert(color);
  };
  auto addList = [&](TList* list) {
    if (!list) return;
    TIter next(list);
    while (TObject* item = next())
      CollectCustomColors(item, colors);
  };
  if (auto line = dynamic_cast<TAttLine*>(obj)) add(line->GetLineColor());
  if (auto fill = dynamic_cast<TAttFill*>(obj)) add(fill->GetFillColor());
  if (auto marker = dynamic_cast<TAttMarker*>(obj)) add(marker->GetMarkerColor());
  if (auto text = dynamic_cast<TAttText*>(obj)) add(text->GetTextColor());
  if (auto pad = dynamic_cast<TPad*>(obj)) {
    add(pad->GetFrameFillColor());
    add(pad->GetFrameLineColor());
    addList(pad->GetListOfPrimitives());
  }
  if (auto hist = dynamic_cast<TH1*>(obj)) {
    for (TAxis* axis : {hist->GetXaxis(), hist->GetYaxis(), hist->GetZaxis()}) {
      add(axis->GetAxisColor());
      add(axis->GetLabelColor());
      add(axis->GetTitleColor());
    }
    addList(hist->GetListOfFunctions());
  }
  if (auto graph = dynamic_cast<TGraph*>(obj)) addList(graph->GetListOfFunctions());
  if (auto multiGraph = dynamic_cast<TMultiGraph*>(obj)) addList(multiGraph->GetListOfGraphs());
  if (auto stack = dynamic_cast<THStack*>(obj)) addList(stack->GetHists());
  if (auto legend = dynamic_cast<TLegend*>(obj)) addList(legend->GetListOfPrimitives());
  if (auto paveText = dynamic_cast<TPaveText*>(obj)) addList(paveText->GetListOfLines());
}

//**************************************************************************************************
/**
 * Interpreter command that defines the given colours by index with the values they have now (an existing colour is updated).
 */
//**************************************************************************************************
string PlotPainter::ColorDefinitionCommand(const std::set<int32_t>& colors)
{
  string command;
  for (const int32_t index : colors) {
    const TColor* color = gROOT->GetColor(index);
    if (!color) continue;
    command += fmt::format(" {{ TColor* c = gROOT->GetColor({idx}); if (c) {{ c->SetRGB({r}, {g}, {b}); c->SetAlpha({a}); }} else {{ new TColor({idx}, {r}, {g}, {b}, \"{name}\", {a}); }} }}",
                           fmt::arg("idx", index), fmt::arg("r", color->GetRed()), fmt::arg("g", color->GetGreen()), fmt::arg("b", color->GetBlue()), fmt::arg("a", color->GetAlpha()), fmt::arg("name", color->GetName()));
  }
  return command;
}

//**************************************************************************************************
/**
 * Interpreter command that sets a colour palette interpolated linearly between the given rgb endpoints (with their stops),
 * with the end colours before the first and after the last stop.
 * The colours are looked up by value (TColor::GetColor), so repeated execution does not create new colours.
 */
//**************************************************************************************************
string PlotPainter::GradientPaletteCommand(int32_t nColors, const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, float_t alpha)
{
  if (rgbEndpoints.size() < 2 || nColors < 1) {
    logger::error("A colour gradient needs at least two endpoints and one colour (got {} and {}).", rgbEndpoints.size(), nColors);
    return fmt::format(" gStyle->SetPalette({});", static_cast<int32_t>(kBird));
  }
  string red, green, blue, stops;
  for (const auto& [r, g, b, stop] : rgbEndpoints) {
    red += fmt::format("{},", r);
    green += fmt::format("{},", g);
    blue += fmt::format("{},", b);
    stops += fmt::format("{},", stop);
  }
  return fmt::format(
    " {{ const Int_t n = {n}; const Double_t s[] = {{{s}}}; const Double_t r[] = {{{r}}}; const Double_t g[] = {{{g}}}; const Double_t b[] = {{{b}}}; Int_t pal[{n}];"
    " for (Int_t i = 0; i < n; ++i) {{ const Double_t x = (n > 1) ? Double_t(i) / (n - 1) : 0.; Int_t k = 0; while (k < {kmax} && x > s[k + 1]) ++k;"
    " const Double_t t = (s[k + 1] > s[k]) ? std::min(1., std::max(0., (x - s[k]) / (s[k + 1] - s[k]))) : 0.;"
    " pal[i] = TColor::GetColor(Float_t(r[k] + t * (r[k + 1] - r[k])), Float_t(g[k] + t * (g[k + 1] - g[k])), Float_t(b[k] + t * (b[k + 1] - b[k])), Float_t({a})); }}"
    " gStyle->SetPalette(n, pal); }}",
    fmt::arg("n", nColors), fmt::arg("s", stops), fmt::arg("r", red), fmt::arg("g", green), fmt::arg("b", blue), fmt::arg("kmax", rgbEndpoints.size() - 2), fmt::arg("a", alpha));
}

//**************************************************************************************************
/**
 * Scales a line width by the scale factor. ROOT line widths are integers, and graphs encode an
 * exclusion zone as 100 x zone size + line width (see TGraphPainter); only the line width is scaled.
 */
//**************************************************************************************************
Width_t PlotPainter::ScaleLineWidth(Width_t width) const
{
  const int32_t sign = (width < 0) ? -1 : 1;
  const int32_t exclusionZone = std::abs(width) / 100 * 100;
  const int32_t lineWidth = std::abs(width) % 100;
  if (lineWidth == 0) return width;
  const int32_t scaled = std::clamp(static_cast<int32_t>(std::lround(lineWidth * mScale)), 1, 99);
  return static_cast<Width_t>(sign * (exclusionZone + scaled));
}

//**************************************************************************************************
/**
 * Scales the outline width of a marker by the scale factor. ROOT has no line width for markers: the markers with an outline
 * (open markers, +, x, * and alike) encode it in the style number, as 50 + i + 18 x (width - 2) for the i-th of the 18 styles
 * with an outline (see TAttMarker::GetMarkerLineWidth); filled markers are returned unchanged.
 */
//**************************************************************************************************
Style_t PlotPainter::ScaleMarkerStyle(Style_t style) const
{
  if (mScale == 1.) return style;
  constexpr int32_t firstEncoded = 50;
  constexpr int32_t nOutlineStyles = 18;
  const Style_t base = TAttMarker::GetMarkerStyleBase(style);
  for (int32_t i = 0; i < nOutlineStyles; ++i) {
    if (TAttMarker::GetMarkerStyleBase(static_cast<Style_t>(firstEncoded + i)) != base) continue;
    const int32_t width = std::max(1, static_cast<int32_t>(std::lround(TAttMarker::GetMarkerLineWidth(style) * mScale)));
    return (width == 1) ? base : static_cast<Style_t>(firstEncoded + i + nOutlineStyles * (width - 2));
  }
  return style;
}

//**************************************************************************************************
/**
 * On screen, the user-defined style that stands in for line style 2, 3 or 4 (see GeneratePlot); every other style is returned unchanged.
 */
//**************************************************************************************************
Style_t PlotPainter::ScreenLineStyle(Style_t style) const
{
  if (style < 2 || style > 4) return style;
  const Style_t screenStyle = mScreenDashStyles[style - 2];
  return (screenStyle) ? screenStyle : style;
}

//**************************************************************************************************
/**
 * Scales all absolute sizes of a drawn object (and the objects it contains) by the scale factor:
 * line widths, marker sizes, border sizes and text sizes of pixel fonts (precision 3).
 * The draw option of the object is needed for ROOT features that use these sizes for other purposes.
 * All other sizes are defined relative to the pad and therefore already scale with the canvas.
 */
//**************************************************************************************************
void PlotPainter::ApplyScale(TObject* obj, std::set<TObject*>& done, const std::string& drawOption)
{
  if (!obj || !done.insert(obj).second) return;
  auto applyToList = [&](TCollection* list) {
    if (!list) return;
    TIter next(list);
    while (TObject* entry = next()) {
      ApplyScale(entry, done, next.GetOption());
    }
  };
  // ROOT moves filled histograms away from the frame by half of the frame line width (integer division) and then also draws their outer
  // vertical edges (see TGraphPainter::PaintGrapHist): a frame line of width 1 has to stay 1, otherwise these edges would appear
  auto scaleFrameLineWidth = [&](Width_t width) { return (width <= 1) ? width : ScaleLineWidth(width); };
  if (auto frame = dynamic_cast<TFrame*>(obj)) {
    frame->SetLineWidth(scaleFrameLineWidth(frame->GetLineWidth()));
  } else if (auto line = dynamic_cast<TAttLine*>(obj)) {
    line->SetLineWidth(ScaleLineWidth(line->GetLineWidth()));
  }
  if (auto line = dynamic_cast<TAttLine*>(obj)) line->SetLineStyle(ScreenLineStyle(line->GetLineStyle()));
  // histograms drawn with option TEXT use their marker size as relative text size of the bin contents (see THistPainter::PaintText)
  const bool isHistText = dynamic_cast<TH1*>(obj) && TString(drawOption).Contains("TEXT", TString::kIgnoreCase);
  auto marker = dynamic_cast<TAttMarker*>(obj);
  if (marker && !isHistText) {
    marker->SetMarkerSize(marker->GetMarkerSize() * mScale);
    marker->SetMarkerStyle(ScaleMarkerStyle(marker->GetMarkerStyle()));
  }
  auto text = dynamic_cast<TAttText*>(obj);
  if (text && text->GetTextFont() % 10 == 3) text->SetTextSize(text->GetTextSize() * mScale);
  // the border size of a box is no width: 0 means no outline, 1 an outline (its width is the line width scaled above),
  // from 2 on an outline plus a shadow of that many pixels (see TPave::PaintPave): only the shadow is scaled
  if (auto pave = dynamic_cast<TPave*>(obj); pave && pave->GetBorderSize() > 1) {
    pave->SetBorderSize(std::max(2, static_cast<Int_t>(std::lround(pave->GetBorderSize() * mScale))));
  }
  if (auto pad = dynamic_cast<TPad*>(obj)) {
    pad->SetFrameLineWidth(scaleFrameLineWidth(pad->GetFrameLineWidth()));
    pad->SetFrameLineStyle(ScreenLineStyle(pad->GetFrameLineStyle()));
    pad->SetBorderSize(static_cast<Short_t>(std::lround(pad->GetBorderSize() * mScale)));
    applyToList(pad->GetListOfPrimitives());
  }
  if (auto hist = dynamic_cast<TH1*>(obj)) {
    for (TAxis* axis : {hist->GetXaxis(), hist->GetYaxis(), hist->GetZaxis()}) {
      if (axis->GetLabelFont() % 10 == 3) axis->SetLabelSize(axis->GetLabelSize() * mScale);
      if (axis->GetTitleFont() % 10 == 3) axis->SetTitleSize(axis->GetTitleSize() * mScale);
    }
    applyToList(hist->GetListOfFunctions());
  }
  if (auto gaxis = dynamic_cast<TGaxis*>(obj)) {  // e.g. the radial axis of a polar drawing: label and title size are its own members
    if (gaxis->GetLabelFont() % 10 == 3) gaxis->SetLabelSize(gaxis->GetLabelSize() * mScale);
    if (gaxis->GetTextFont() % 10 == 3) gaxis->SetTitleSize(gaxis->GetTitleSize() * mScale);
  }
  if (auto grid = dynamic_cast<TGraphPolargram*>(obj)) {  // the polar grid of polar graphs: the label sizes are its own members
    if (grid->GetPolarLabelFont() % 10 == 3) grid->SetPolarLabelSize(grid->GetPolarLabelSize() * mScale);
    if (grid->GetRadialLabelFont() % 10 == 3) grid->SetRadialLabelSize(grid->GetRadialLabelSize() * mScale);
  }
  if (auto graph = dynamic_cast<TGraph*>(obj)) applyToList(graph->GetListOfFunctions());
  if (auto multiGraph = dynamic_cast<TMultiGraph*>(obj)) applyToList(multiGraph->GetListOfGraphs());
  if (auto stack = dynamic_cast<THStack*>(obj)) applyToList(stack->GetHists());
  if (auto legend = dynamic_cast<TLegend*>(obj)) applyToList(legend->GetListOfPrimitives());
  if (auto paveText = dynamic_cast<TPaveText*>(obj)) applyToList(paveText->GetListOfLines());
}

//**************************************************************************************************
/**
 * Returns a copy of the frame histogram with the given axis extended to newMin and / or newMax (bin contents and errors copied).
 * A newMin inside the axis comes from a log axis extended below its first positive edge: the bins with edges at or below zero, which
 * a log axis cannot show, are then left out. The lengths of the other axes and the attributes are kept; the result replaces the frame
 * in the pad and has to be drawn by the caller.
 */
//**************************************************************************************************
TH1* PlotPainter::ExtendFrameAxis(const TH1* frame, char axisLabel, optional<double_t> newMin, optional<double_t> newMax,
                                  optional<double_t> rangeMin, optional<double_t> rangeMax)
{
  const int32_t dim = frame->GetDimension();
  const int32_t extendedDim = (axisLabel == 'X') ? 0 : 1;
  const array<const TAxis*, 3> axes = {frame->GetXaxis(), frame->GetYaxis(), frame->GetZaxis()};
  // a range wholly outside the frame (20 to 30 for data from 0 to 10) also needs an edge at its inner end: a range is shown
  // in whole bins, so with the one added bin from 10 to 30 the axis would start at 10
  const TAxis* extendedAxis = axes[extendedDim];
  int32_t droppedBins = 0;
  while (newMin && *newMin >= extendedAxis->GetXmin() && droppedBins < extendedAxis->GetNbins() && extendedAxis->GetBinLowEdge(droppedBins + 1) <= 0.)
    ++droppedBins;
  const double_t keptMin = extendedAxis->GetBinLowEdge(droppedBins + 1);
  const bool innerLow = newMin && rangeMax && *rangeMax > *newMin && *rangeMax < keptMin;
  const bool innerHigh = newMax && rangeMin && *rangeMin < *newMax && *rangeMin > extendedAxis->GetXmax();
  const int32_t shift = (newMin ? 1 : 0) + (innerLow ? 1 : 0) - droppedBins;  // the bins move by the number of bins added (dropped) below
  // the edges added to the extended axis, collected before the loop (GCC 13 reports optionals read in it as maybe uninitialized)
  vector<double_t> below, above;
  if (newMin) below.push_back(*newMin);
  if (innerLow) below.push_back(*rangeMax);
  if (innerHigh) above.push_back(*rangeMin);
  if (newMax) above.push_back(*newMax);
  array<vector<double_t>, 3> edges;
  for (int32_t i = 0; i < dim; ++i) {
    if (i == extendedDim) edges[i] = below;
    for (int32_t bin = (i == extendedDim) ? droppedBins + 1 : 1; bin <= axes[i]->GetNbins() + 1; ++bin)
      edges[i].push_back(axes[i]->GetBinLowEdge(bin));
    if (i == extendedDim) edges[i].insert(edges[i].end(), above.begin(), above.end());
  }
  TH1* extended{nullptr};
  if (dim == 1) {
    extended = new TH1D(frame->GetName(), frame->GetTitle(), edges[0].size() - 1, edges[0].data());
  } else if (dim == 2) {
    extended = new TH2D(frame->GetName(), frame->GetTitle(), edges[0].size() - 1, edges[0].data(), edges[1].size() - 1, edges[1].data());
  } else {
    extended = new TH3D(frame->GetName(), frame->GetTitle(), edges[0].size() - 1, edges[0].data(), edges[1].size() - 1, edges[1].data(), edges[2].size() - 1, edges[2].data());
  }
  extended->SetDirectory(nullptr);
  extended->SetBit(kCanDelete);
  const int32_t shiftX = (extendedDim == 0) ? shift : 0;
  const int32_t shiftY = (extendedDim == 1) ? shift : 0;
  for (int32_t binZ = 1; binZ <= axes[2]->GetNbins(); ++binZ) {
    for (int32_t binY = 1; binY <= axes[1]->GetNbins(); ++binY) {
      for (int32_t binX = 1; binX <= axes[0]->GetNbins(); ++binX) {
        if (binX + shiftX < 1 || binY + shiftY < 1) continue;  // a dropped bin
        const int32_t bin = frame->GetBin(binX, binY, binZ);
        const int32_t newBin = extended->GetBin(binX + shiftX, binY + shiftY, binZ);
        extended->SetBinContent(newBin, frame->GetBinContent(bin));
        extended->SetBinError(newBin, frame->GetBinError(bin));
      }
    }
  }
  extended->SetEntries(frame->GetEntries());
  extended->SetMinimum(frame->GetMinimumStored());
  extended->SetMaximum(frame->GetMaximumStored());
  CopyStyle(*frame, *extended);
  // the axes are copied completely (attributes, labels, time format, bits, a selected range) and the extended one gets its new binning
  const array<TAxis*, 3> newAxes = {extended->GetXaxis(), extended->GetYaxis(), extended->GetZaxis()};
  for (int32_t i = 0; i < 3; ++i) {
    const bool hadRange = axes[i]->TestBit(TAxis::kAxisRange);
    axes[i]->Copy(*newAxes[i]);
    newAxes[i]->SetParent(extended);
    if (i != extendedDim) continue;
    newAxes[i]->Set(edges[i].size() - 1, edges[i].data());
    newAxes[i]->SetCanExtend(false);  // a copied labelled axis can extend: ROOT would shrink it back to the labelled bins when painting
    if (hadRange) {
      newAxes[i]->SetRange(std::max(axes[i]->GetFirst() + shift, 1), axes[i]->GetLast() + shift);
    } else {
      newAxes[i]->SetRange(0, 0);  // full new axis
    }
    if (shift && newAxes[i]->GetLabels()) {
      TIter next(newAxes[i]->GetLabels());
      while (TObject* label = next())
        label->SetUniqueID(label->GetUniqueID() + shift);
    }
  }
  return extended;
}

//**************************************************************************************************
/**
 * Function to warn user if incompatible text font and size were used.
 */
//**************************************************************************************************
void PlotPainter::CheckFontSizes(TList* list)
{
  auto compatible = [](int16_t textFont, float_t textSize, TObject* obj) {
    if (!textSize) return true;
    if (textFont % 10 <= 2 && textSize >= 1.f) {
      logger::warning("Though text font {} implies a relative text size it is set to {} for {}.", textFont, textSize, obj->GetName());
      return false;
    }
    if (textFont % 10 > 2 && textSize <= 1.f) {
      logger::warning("Though text font {} implies a text size in pixel it is set to {} for {}.", textFont, textSize, obj->GetName());
      return false;
    }
    return true;
  };
  // stops at the first incompatible setting (one wrong default would otherwise warn for every text)
  std::function<bool(TList*)> check = [&](TList* primitives) {
    if (!primitives) return true;
    for (TObject* obj : *primitives) {
      auto* pad = dynamic_cast<TPad*>(obj);
      auto* leg = dynamic_cast<TLegend*>(obj);
      auto* txt = dynamic_cast<TAttText*>(obj);
      auto* h = dynamic_cast<TH1*>(obj);
      if (pad && !check(pad->GetListOfPrimitives())) return false;
      if (leg && !check(leg->GetListOfPrimitives())) return false;
      if (txt && !compatible(txt->GetTextFont(), txt->GetTextSize(), obj)) return false;
      if (h && TString(h->GetName()).BeginsWith("axis_hist")) {
        for (TAxis* ax : {h->GetXaxis(), h->GetYaxis(), h->GetZaxis()}) {
          if (!compatible(ax->GetLabelFont(), ax->GetLabelSize(), ax) || !compatible(ax->GetTitleFont(), ax->GetTitleSize(), ax)) return false;
        }
      }
    }
    return true;
  };
  check(list);
}

//**************************************************************************************************
/**
 * Whether data has errors as ROOT draws them by default: a histogram with stored errors of which at least one is not zero
 * (a profile always computes them), a graph with a y error that is not zero. With countErrors, also a histogram without stored
 * errors whose contents give it errors (the square roots of the contents, which ROOT draws only when asked to).
 */
//**************************************************************************************************
bool PlotPainter::HasErrors(const TObject* obj, bool countErrors)
{
  if (auto hist = dynamic_cast<const TH1*>(obj)) {
    if (hist->GetSumw2N() == 0 && !hist->InheritsFrom(TProfile::Class()) && !countErrors) return false;
    for (int32_t bin = 1; bin <= hist->GetNbinsX(); ++bin) {
      if (hist->GetBinError(bin) != 0.) return true;
    }
    return false;
  }
  if (auto graph = dynamic_cast<const TGraph*>(obj)) {
    for (int32_t i = 0; i < graph->GetN(); ++i) {
      if (graph->GetErrorYlow(i) > 0. || graph->GetErrorYhigh(i) > 0.) return true;
    }
  }
  return false;
}

//**************************************************************************************************
/**
 * Determines which of the legend symbols (P marker, L line, F fill, E error bar) show what ROOT actually draws for an object with the given
 * drawing option and its attributes, and how the outline of a fill symbol must look (TLegend surrounds an F without L by the entry's line).
 * (verified with ROOT 6.40 by comparing the painted plot and legend for all aliases and the common ROOT options:
 * lines of width 0 are not drawn; hollow fill areas are outlined thin in the fill colour (as TLegend does), hollow boxes and bars
 * thin and solid in the line colour (not with line width 0), filled ones have no outline except graph 5 boxes (thin);
 * histograms with Sumw2 or E draw the errors with markers (skipping markers of size 0) and no line unless HIST is given; if all errors are zero,
 * ROOT drops them and draws the histogram as steps unless P, L, C or * is given; graphs draw error boxes and bands only with errors;
 * graph brackets mark x and y errors; polar graphs draw error bars only with E;
 * 2d drawings coloured by the palette have no symbol, surfaces their mesh lines)
 */
//**************************************************************************************************
PlotPainter::LegendSymbol PlotPainter::GetLegendSymbol(TObject* obj, string option)
{
  option = str_upper(option);
  auto takeToken = [&option](const string& token) {
    const size_t pos = option.find(token);
    if (pos == string::npos) return false;
    option.erase(pos, token.size());
    return true;
  };
  for (const char* ignored : kNonSymbolTokens)
    takeToken(ignored);
  auto has = [&option](char c) { return option.find(c) != string::npos; };
  auto fill = dynamic_cast<TAttFill*>(obj);
  auto line = dynamic_cast<TAttLine*>(obj);
  auto marker = dynamic_cast<TAttMarker*>(obj);
  const bool hasFill = fill && fill->GetFillStyle() != 0 && fill->GetFillColor() != 0;
  const bool hasLine = line && line->GetLineWidth() > 0;
  const string L = (hasLine) ? "L" : "";
  const string E = (hasLine) ? "E" : "";
  // a fill area (a hollow one is outlined thin in the fill colour, as the symbol of TLegend) and a box (a hollow one in the line colour);
  // hollow ones are not outlined with line width 0
  auto area = [&](Outline outline) -> LegendSymbol {
    if (!hasFill) return (hasLine) ? LegendSymbol{"F", Outline::none} : LegendSymbol{};
    return {"F", outline};
  };
  auto box = [&](Outline outline = Outline::none) -> LegendSymbol {
    if (!hasFill) return (hasLine) ? LegendSymbol{"F", Outline::thin} : LegendSymbol{};
    return {"F", outline};
  };
  auto join = [](LegendSymbol symbol, const string& more) {
    symbol.elements += more;
    return symbol;
  };

  const bool isGraph2D = obj->InheritsFrom(TGraph2D::Class());
  const bool isFunction2D = obj->InheritsFrom(TF2::Class());
  if (isGraph2D || isFunction2D || obj->InheritsFrom(TH2::Class()) || obj->InheritsFrom(TH3::Class())) {
    takeToken("POL");  // polar coordinates, not a symbol
    // coloured by the palette (also LEGO2, SURF1-4, the filled contours, the boxes of 3d histograms coloured by the content and the
    // coloured triangles of 2d graphs)
    for (const char* token : {"COL", "Z", "LEGO2", "SURF1", "SURF2", "SURF3", "SURF4", "CONT0", "CONT1", "CONT4", "BOX2", "ISO", "TRI1", "TRI2"}) {
      if (str_contains(option, token)) return {};
    }
    if (isGraph2D) {
      if (str_contains(option, "TRIW") || str_contains(option, "LINE")) return {L};  // a wire mesh, a line through the points
      if (str_contains(option, "TRI")) return area(Outline::line);                   // triangles in the fill colour, edged in the line colour
      if (has('P')) return {"P"};
    }
    if (isFunction2D) return (takeToken("FC") && hasFill) ? LegendSymbol{"F", Outline::none} : LegendSymbol{L};
    if (obj->InheritsFrom(TH2Poly::Class()) && !str_contains(option, "LEGO") && !str_contains(option, "SURF")) {
      // ROOT draws polygon bins only filled, outlined or as markers (THistPainter::PaintTH2PolyBins), not as boxes or contours
      LegendSymbol symbol = (has('F') && hasFill) ? LegendSymbol{"F", Outline::none} : LegendSymbol{};
      if (has('L')) symbol.elements += L;
      if (has('P')) symbol.elements += "P";
      return symbol;
    }
    if (str_contains(option, "CANDLE") || str_contains(option, "VIOLIN")) return (hasFill) ? area(Outline::line) : LegendSymbol{L};
    if (str_contains(option, "BOX")) return box();
    if (str_contains(option, "CONT") || str_contains(option, "SURF")) return {L};  // contour lines, mesh
    return (hasFill) ? area(Outline::line) : LegendSymbol{L};
  }
  if (obj->InheritsFrom(TF1::Class())) {
    // a function is drawn with markers at its sampling points with P, otherwise as a curve that a fill colours down to zero
    if (has('P')) return {"P"};
    return (hasFill) ? area(Outline::line) : LegendSymbol{L};
  }
  // the symbol of a graph painted without its errors (TGraphPainter::PaintGraphSimple) with the letters of an option
  auto simpleGraph = [&](const string& letters) {
    auto in = [&letters](char c) { return letters.find(c) != string::npos; };
    const bool lineDrawn = in('L') || in('C');
    LegendSymbol symbol;
    if (in('F')) symbol = area((lineDrawn) ? Outline::line : Outline::none);
    if (in('B')) symbol = join(box(), symbol.elements);
    if (in('P') || in('*')) symbol.elements += "P";
    if (lineDrawn) symbol.elements += L;
    return symbol;
  };
  if (auto multi = dynamic_cast<TGraphMultiErrors*>(obj); multi && multi->GetNYErrors() > 0 && HasErrors(multi)) {
    // the option split at ';' as ROOT does (TGraphPainter::PaintGraphMultiErrors): the graph itself, then one part per set of y errors
    // (with fewer parts the first is also that of the first set, sets without a part are drawn as error bars); the graph is painted only
    // where a set is drawn as error bars, or where no errors are drawn at all
    const size_t nSets = static_cast<size_t>(multi->GetNYErrors());
    std::vector<string> parts(nSets + 1);
    string rest = option;
    size_t filled = 0;
    if (static_cast<size_t>(std::count(rest.begin(), rest.end(), ';')) < nSets) parts[filled++] = rest.substr(0, rest.find(';'));
    for (size_t pos = rest.find(';'); pos != string::npos && filled <= nSets; pos = rest.find(';')) {
      parts[filled++] = rest.substr(0, pos);
      rest.erase(0, pos + 1);
    }
    if (filled <= nSets) parts[filled] = rest;
    auto in = [](const string& part, const char* token) { return part.find(token) != string::npos; };
    string global = parts[0];
    for (const char* ignored : {"X0", "Y0"}) {
      for (size_t pos = global.find(ignored); pos != string::npos; pos = global.find(ignored))
        global.erase(pos, 2);
    }
    LegendSymbol symbol;
    bool anyErrors = false, graphPainted = false, allBoxesOrBands = true;
    for (size_t set = 1; set <= nSets; ++set) {
      const string& part = parts[set];
      const bool band = in(part, "3") || in(part, "4");
      const bool boxes = in(part, "2") || in(part, "5");
      graphPainted |= !band && !boxes && !in(part, "[]") && !in(part, "||");
      allBoxesOrBands &= band || boxes;
      if (in(part, "X")) continue;  // this set is not drawn
      anyErrors = true;
      if (band || boxes) {
        if (!str_contains(symbol.elements, "F")) symbol = join((band) ? area(Outline::none) : box((in(part, "5")) ? Outline::thin : Outline::none), symbol.elements);
      } else if (!str_contains(symbol.elements, "E")) {
        symbol.elements += E;
      }
    }
    if (!anyErrors && in(global, "X")) return simpleGraph(global);
    // the x errors, left out where every set is drawn as boxes or bands (they show the x errors themselves)
    bool xErrors = false;
    for (int32_t i = 0; i < multi->GetN() && !xErrors; ++i)
      xErrors = multi->GetErrorXlow(i) > 0. || multi->GetErrorXhigh(i) > 0.;
    if (xErrors && !in(global, "X") && !allBoxesOrBands && !str_contains(symbol.elements, "E")) symbol.elements += E;
    if (graphPainted) {
      for (char element : simpleGraph(global).elements) {
        if (!str_contains(symbol.elements, string(1, element))) symbol.elements += element;
      }
    }
    return symbol;
  }
  if (auto graph = dynamic_cast<TGraph*>(obj)) {
    // a polar graph draws its error bars only with E (TGraphPainter::PaintGraphPolar)
    const bool hasErrors = HasErrors(graph) && (has('E') || !graph->InheritsFrom(TGraphPolar::Class()));
    const bool lineDrawn = has('L') || has('C');
    LegendSymbol symbol;
    if (str_contains(option, "[]")) {  // brackets at the ends of the x and y errors
      for (int32_t i = 0; i < graph->GetN(); ++i) {
        if (graph->GetErrorXlow(i) > 0. || graph->GetErrorXhigh(i) > 0. || graph->GetErrorYlow(i) > 0. || graph->GetErrorYhigh(i) > 0.) {
          symbol.elements = E;
          break;
        }
      }
      return symbol;
    }
    if (has('2') || has('3') || has('4') || has('5')) {  // error boxes and bands, drawn only with errors
      if (hasErrors) symbol = (has('3') || has('4')) ? area(Outline::none) : box((has('5')) ? Outline::thin : Outline::none);
    } else if (hasErrors && !has('X')) {
      symbol.elements = E;
    }
    if (has('F')) symbol = join(area((lineDrawn) ? Outline::line : Outline::none), symbol.elements);
    if (has('B')) symbol = join(box(), symbol.elements);
    // ROOT paints line and markers before the error band (TGraphPainter::PaintGraphErrors): an opaque band hides them
    const TColor* fillColor = (hasFill) ? gROOT->GetColor(fill->GetFillColor()) : nullptr;
    const bool hiddenByBand = hasErrors && (has('3') || has('4')) && fill->GetFillStyle() == 1001 && fillColor && fillColor->GetAlpha() == 1.f;
    if (hiddenByBand) return symbol;
    if (has('P') || has('*')) symbol.elements += "P";
    if (lineDrawn) symbol.elements += L;
    return symbol;
  }
  if (auto hist = dynamic_cast<TH1*>(obj)) {
    if (str_contains(option, "HBAR")) return box();  // bars only, without the errors or markers of other options
    const bool noErrors = takeToken("HIST");
    // E0 - E6 (the digits of other options, as in LF2, are no error types)
    char errorType = '\0';
    for (size_t pos = option.find('E'); pos != string::npos; pos = option.find('E', pos + 1)) {
      if (pos + 1 < option.size() && std::isdigit(static_cast<unsigned char>(option[pos + 1]))) errorType = option[pos + 1];
    }
    bool nonZeroErrors = false;
    for (int32_t bin = 1; bin <= hist->GetNbinsX() && !nonZeroErrors; ++bin)
      nonZeroErrors = hist->GetBinError(bin) > 0.;
    const bool markers = has('P') || has('*');
    const bool lineDrawn = has('L') || has('C');
    // ROOT draws the errors with Sumw2 (a profile has them) or E; if all are zero it drops them and draws steps unless P, L, C or * is given
    // an E after HIST asks for the errors again, and the steps are then drawn over them (THistPainter::Paint)
    const bool errorsAsked = has('E') || (!noErrors && (hist->GetSumw2N() > 0 || hist->InheritsFrom(TProfile::Class())));
    if (errorsAsked && nonZeroErrors) {  // markers with the errors (no line, also with L or C), markers of size 0 are skipped
      const string P = (marker && marker->GetMarkerSize() > 0) ? "P" : "";
      LegendSymbol symbol{E + P};
      if (errorType >= '3' && errorType <= '6') symbol = join(area(Outline::none), P);                  // fill areas
      if (errorType == '2') symbol = join(box(), P);                                                    // boxes
      if (noErrors) symbol = (hasFill) ? join(area(Outline::line), symbol.elements) : join(symbol, L);  // the steps are painted over the errors
      return symbol;
    }
    const bool drawnAsSteps = errorsAsked && !markers && !lineDrawn;
    if (!drawnAsSteps && markers && !lineDrawn && !has('B') && !has('F')) return {"P"};  // markers only
    if (has('B')) return box();                                                          // bars
    LegendSymbol symbol = (hasFill) ? area(Outline::line) : LegendSymbol{L};             // steps, lines or curves (HIST L and C fill the histogram if it has a fill colour)
    if (markers) symbol.elements += "P";
    return symbol;
  }
  return {E + "P"};
}

//**************************************************************************************************
/**
 * Draw the shapes of a pad that belong to one layer, with the final frame (after the axis room): the kinds over the whole frame
 * take its edges, regions in axis values are cut at it, lines and arrows between two points are not cut (as TLine).
 * insertAfter: put the shapes behind this primitive (the frame histogram for the layer behind the data) instead of at the end.
 */
//**************************************************************************************************
void PlotPainter::DrawShapes(TPad* pad, const Plot::Pad& padSettings, const Plot::Pad& padDefaults, Plot::Pad::Shape::layer_t layer, TObject* insertAfter, uint8_t padID)
{
  using Shape = Plot::Pad::Shape;
  using kind_t = Shape::kind_t;
  // TLine, TArrow, TBox and TEllipse take axis values (they convert them to pad coordinates, log10 on a log axis, when painted)
  const double_t xMin = FromPad(pad->GetUxmin(), pad->GetLogx()), xMax = FromPad(pad->GetUxmax(), pad->GetLogx());
  const double_t yMin = FromPad(pad->GetUymin(), pad->GetLogy()), yMax = FromPad(pad->GetUymax(), pad->GetLogy());
  const double_t nan = std::numeric_limits<double_t>::quiet_NaN();
  uint8_t shapeIndex{};
  std::map<kind_t, uint16_t> numberOfKind;  // over all layers: the nth shape of a kind is the nth call that added one
  for (const auto& shape : const_cast<Plot::Pad&>(padSettings).GetShapes()) {
    ++shapeIndex;
    const kind_t kind = shape->GetKind();
    const uint16_t number = ++numberOfKind[kind];
    if (shape->GetLayer() != layer) continue;
    const auto& c = shape->GetCoordinates();
    const coordinates_t coordinates = get_first_or(user_coordinates, shape->GetCoordinateSystem(), padSettings.GetDefaultShapeCoordinates(), padDefaults.GetDefaultShapeCoordinates());
    const bool isUser = (coordinates == user_coordinates);
    const bool cutAtFrame = (coordinates != pad_coordinates);  // in fractions of the pad a shape may lie outside of the frame on purpose
    // fractions of the frame or the pad: the part of the pad they span
    const std::array<double_t, 4> span = (coordinates == frame_coordinates) ? FrameInPad(pad) : std::array<double_t, 4>{0., 0., 1., 1.};  // x1, y1, x2, y2
    const size_t nExpected = (kind == kind_t::vertical_line || kind == kind_t::horizontal_line) ? 1u : ((kind == kind_t::region_x || kind == kind_t::region_y) ? 2u : 4u);
    // a polygon with too few points and positions that are not finite numbers were reported when the shape was added
    if ((kind == kind_t::polygon) ? (c.size() < 6 || c.size() % 2) : (c.size() != nExpected)) continue;
    const bool finite = std::all_of(c.begin(), c.end(), [](double_t value) { return std::isfinite(value); });
    if (!finite) continue;
    const bool isLine = kind == kind_t::line || kind == kind_t::vertical_line || kind == kind_t::horizontal_line;
    if (isLine && (shape->GetFillColor() || shape->GetFillStyle() || shape->GetFillAlpha())) {
      ReportIgnored(fmt::format("The fill of shape {} ({}) of pad {}", shapeIndex, Shape::KindName(kind), padID), "a line has no area (an arrow fills its head)");
    }
    // positions as axis values: given so, or fractions of the frame or the pad converted; a log axis cannot show values that are not positive
    auto valueX = [&](double_t x) { return (!isUser) ? FromPad(pad->GetX1() + (span[0] + x * (span[2] - span[0])) * (pad->GetX2() - pad->GetX1()), pad->GetLogx()) : ((pad->GetLogx() && x <= 0.) ? nan : x); };
    auto valueY = [&](double_t y) { return (!isUser) ? FromPad(pad->GetY1() + (span[1] + y * (span[3] - span[1])) * (pad->GetY2() - pad->GetY1()), pad->GetLogy()) : ((pad->GetLogy() && y <= 0.) ? nan : y); };
    auto skip = [&](const char* reason) { logger::warning("Shape {} ({}) of pad {} is not drawn: {}.", shapeIndex, Shape::KindName(kind), padID, reason); };

    TObject* object{};
    TObject* outline{};  // drawn after the fill: the outline of a polygon
    bool isRegion{};
    switch (kind) {
      case kind_t::line:
      case kind_t::arrow: {
        const double_t x1 = valueX(c[0]), y1 = valueY(c[1]), x2 = valueX(c[2]), y2 = valueY(c[3]);
        if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2)) {
          skip("a position is not positive on a log axis");
          continue;
        }
        if (kind == kind_t::arrow) {
          object = new TArrow(x1, y1, x2, y2, shape->GetArrowSize().value_or(0.03f), shape->GetArrowHead().value_or("|>").data());
        } else {
          object = new TLine(x1, y1, x2, y2);
        }
        break;
      }
      case kind_t::vertical_line:
      case kind_t::horizontal_line: {
        const bool isVertical = (kind == kind_t::vertical_line);
        const double_t pos = (isVertical) ? valueX(c[0]) : valueY(c[0]);
        // compared in pad coordinates: 10^log10(limit) is not always the limit, so a line at a limit of a log axis would count as outside
        const bool logAxis = (isVertical) ? pad->GetLogx() : pad->GetLogy();
        const double_t padPos = (!logAxis) ? pos : ((pos > 0.) ? std::log10(pos) : nan);
        if (!std::isfinite(padPos) || padPos < ((isVertical) ? pad->GetUxmin() : pad->GetUymin()) || padPos > ((isVertical) ? pad->GetUxmax() : pad->GetUymax())) {
          skip("its position is outside of the frame");
          continue;
        }
        object = (isVertical) ? new TLine(pos, yMin, pos, yMax) : new TLine(xMin, pos, xMax, pos);
        break;
      }
      case kind_t::region:
      case kind_t::region_x:
      case kind_t::region_y: {
        double_t x1 = xMin, x2 = xMax, y1 = yMin, y2 = yMax;
        if (kind == kind_t::region) {
          x1 = valueX(c[0]), y1 = valueY(c[1]), x2 = valueX(c[2]), y2 = valueY(c[3]);
        } else if (kind == kind_t::region_x) {
          x1 = valueX(c[0]), x2 = valueX(c[1]);
        } else {
          y1 = valueY(c[0]), y2 = valueY(c[1]);
        }
        // on a log axis a region reaching below 0 starts at the frame edge
        if (!std::isfinite(x1)) x1 = xMin;
        if (!std::isfinite(y1)) y1 = yMin;
        if (x1 > x2) std::swap(x1, x2);
        if (y1 > y2) std::swap(y1, y2);
        if (cutAtFrame) {
          x1 = std::max(x1, xMin), x2 = std::min(x2, xMax), y1 = std::max(y1, yMin), y2 = std::min(y2, yMax);
        }
        if (!std::isfinite(x2) || !std::isfinite(y2) || x1 >= x2 || y1 >= y2) {
          skip("it is outside of the frame");
          continue;
        }
        object = new TBox(x1, y1, x2, y2);
        isRegion = true;
        break;
      }
      case kind_t::ellipse: {
        if (c[2] <= 0. || c[3] <= 0.) continue;  // reported when it was added
        const double_t x = valueX(c[0]), y = valueY(c[1]);
        if (!std::isfinite(x) || !std::isfinite(y)) {
          skip("its centre is not positive on a log axis");
          continue;
        }
        if (isUser && (pad->GetLogx() || pad->GetLogy())) {
          logger::warning("Shape {} (ellipse) of pad {} is drawn distorted: TEllipse does not convert its radii on a log axis.", shapeIndex, padID);
        }
        // the radii are in pad coordinates (TEllipse converts only the centre)
        const double_t rx = (isUser) ? c[2] : c[2] * (span[2] - span[0]) * (pad->GetX2() - pad->GetX1());
        const double_t ry = (isUser) ? c[3] : c[3] * (span[3] - span[1]) * (pad->GetY2() - pad->GetY1());
        const auto arc = shape->GetArc().value_or(std::pair<double_t, double_t>{0., 360.});
        object = new TEllipse(x, y, rx, ry, arc.first, arc.second, shape->GetRotation().value_or(0.));
        break;
      }
      case kind_t::polygon: {
        // cut at the frame (Sutherland-Hodgman, edge by edge) in pad coordinates, where the edges are straight also on log axes
        auto padCoord = [](double_t value, bool isLog) { return (isLog) ? std::log10(value) : value; };
        vector<std::pair<double_t, double_t>> points;
        bool valid = true;
        for (size_t i = 0; i + 1 < c.size(); i += 2) {
          const double_t x = valueX(c[i]), y = valueY(c[i + 1]);
          valid = valid && std::isfinite(x) && std::isfinite(y);
          points.emplace_back(padCoord(x, pad->GetLogx()), padCoord(y, pad->GetLogy()));
        }
        if (!valid) {
          skip("a point is not positive on a log axis");
          continue;
        }
        if (cutAtFrame) {
          // inside: of the four frame edges (axis, limit, sign: lower or upper limit)
          for (auto [axis, limit, sign] : {std::tuple{0, pad->GetUxmin(), 1.}, std::tuple{0, pad->GetUxmax(), -1.}, std::tuple{1, pad->GetUymin(), 1.}, std::tuple{1, pad->GetUymax(), -1.}}) {
            auto coord = [axis = axis](const std::pair<double_t, double_t>& p) { return (axis == 0) ? p.first : p.second; };
            auto inside = [&, limit = limit, sign = sign](const std::pair<double_t, double_t>& p) { return sign * (coord(p) - limit) >= 0.; };
            vector<std::pair<double_t, double_t>> cut;
            for (size_t i = 0; i < points.size(); ++i) {
              const auto& a = points[i];
              const auto& b = points[(i + 1) % points.size()];
              if (inside(a)) cut.push_back(a);
              if (inside(a) != inside(b)) {
                const double_t t = (limit - coord(a)) / (coord(b) - coord(a));
                cut.emplace_back(a.first + t * (b.first - a.first), a.second + t * (b.second - a.second));
              }
            }
            points = cut;
            if (points.empty()) break;
          }
        }
        if (points.size() < 3) {
          skip("it is outside of the frame");
          continue;
        }
        points.push_back(points.front());  // closed
        vector<double_t> x, y;
        for (const auto& [px, py] : points) {
          x.push_back(FromPad(px, pad->GetLogx()));
          y.push_back(FromPad(py, pad->GetLogy()));
        }
        object = new TPolyLine(static_cast<int32_t>(x.size()), x.data(), y.data(), "f");
        if (shape->GetLineColor() || shape->GetLineStyle() || shape->GetLineWidth() || shape->GetLineAlpha()) {
          outline = new TPolyLine(static_cast<int32_t>(x.size()), x.data(), y.data());
        }
        isRegion = true;  // filled like a region
        break;
      }
    }
    if (!object) continue;

    // line: black, the default line width of the pad; fill: regions light grey and translucent, arrow heads in the line colour, ellipses open
    auto* line = dynamic_cast<TAttLine*>(object);
    auto* fill = dynamic_cast<TAttFill*>(object);
    const color_t lineColor = shape->GetLineColor().value_or(color_t(kBlack));
    line->SetLineColor(lineColor.ToIndex());
    if (shape->GetLineAlpha()) line->SetLineColor(TColor::GetColorTransparent(line->GetLineColor(), *shape->GetLineAlpha()));
    line->SetLineStyle(shape->GetLineStyle().value_or(kSolid));
    if (auto width = get_first(shape->GetLineWidth(), padSettings.GetDefaultLineWidth(), padDefaults.GetDefaultLineWidth())) line->SetLineWidth(static_cast<Width_t>(*width));
    if (fill) {
      const bool hasFill = shape->GetFillColor() || shape->GetFillStyle();
      if (isRegion && !hasFill) {
        fill->SetFillColor(TColor::GetColorTransparent(kGray, 0.3f));
        fill->SetFillStyle(1001);
      } else if (kind == kind_t::arrow && !hasFill) {
        fill->SetFillColor(line->GetLineColor());
        fill->SetFillStyle(1001);
      } else if (kind == kind_t::ellipse && !hasFill) {
        fill->SetFillStyle(0);
      } else {
        fill->SetFillColor(shape->GetFillColor().value_or(color_t(kGray)).ToIndex());
        fill->SetFillStyle(shape->GetFillStyle().value_or(1001));
      }
      if (shape->GetFillAlpha()) fill->SetFillColor(TColor::GetColorTransparent(fill->GetFillColor(), *shape->GetFillAlpha()));
    }
    // a region gets its outline only when a line is set for it (a box with option l, a polygon as second, open polyline)
    const bool regionOutline = kind != kind_t::polygon && isRegion && (shape->GetLineColor() || shape->GetLineStyle() || shape->GetLineWidth() || shape->GetLineAlpha());
    if (outline) line->Copy(*dynamic_cast<TAttLine*>(outline));
    const uint32_t drawnID = Shape::EncodeDrawnID({kind, number, coordinates});
    for (TObject* drawn : {object, outline}) {
      if (!drawn) continue;
      drawn->SetUniqueID(drawnID);  // for the position reported by a double-click in show mode
      drawn->SetBit(kCanDelete);    // owned by the pad
      const char* option = (drawn == object && regionOutline) ? "l" : ((drawn == object && kind == kind_t::polygon) ? "f" : "");
      if (insertAfter) {
        pad->GetListOfPrimitives()->AddAfter(insertAfter, drawn);
        insertAfter = drawn;
        for (auto* link = pad->GetListOfPrimitives()->FirstLink(); link && *option; link = link->Next()) {  // AddAfter takes no option
          if (link->GetObject() == drawn) link->SetOption(option);
        }
      } else {
        pad->GetListOfPrimitives()->Add(drawn, option);
      }
    }
  }
}

//**************************************************************************************************
/**
 * Draws a legend or text box into the pad, with the text defaults of the pad where the box sets none; returns false for an empty box.
 */
//**************************************************************************************************
bool PlotPainter::DrawBox(variant<shared_ptr<Plot::Pad::LegendBox>, shared_ptr<Plot::Pad::TextBox>> boxVariant, const Plot::Pad& pad,
                          const Plot::Pad& padDefaults, TPad* pad_ptr, const string& name)
{
  auto generate = [&](auto& box) -> TPave* {
    if (!box->GetTextFont()) {
      if (auto font = get_first(pad.GetDefaultTextFont(), padDefaults.GetDefaultTextFont())) box->SetTextFont(*font);
    }
    if (!box->GetTextSize()) {
      if (auto size = get_first(pad.GetDefaultTextSize(), padDefaults.GetDefaultTextSize())) box->SetTextSize(*size);
    }
    if (!box->GetTextColor()) {
      if (auto color = get_first(pad.GetDefaultTextColor(), padDefaults.GetDefaultTextColor())) box->SetTextColor(*color);
    }
    if (!box->GetTextAlpha()) {
      if (auto alpha = get_first(pad.GetDefaultTextAlpha(), padDefaults.GetDefaultTextAlpha())) box->SetTextAlpha(*alpha);
    }
    const coordinates_t coordinates = get_first_or(frame_coordinates, box->GetCoordinateSystem(), pad.GetDefaultBoxCoordinates(), padDefaults.GetDefaultBoxCoordinates());
    TPave* generated = GenerateBox(box, coordinates, pad_ptr);
    if (generated) generated->SetUniqueID(coordinates);  // a double-click in show mode reports the position as given
    return generated;
  };
  TPave* pave = std::visit(generate, boxVariant);
  if (!pave) return false;
  pave->SetName(name.data());
  pave->SetBit(kCanDelete);  // owned by the pad
  pave->Draw("SAME");
  pave->ConvertNDCtoPad();  // ROOT sets the NDC coordinates of a box only when it is painted: the placement of the next boxes needs them
  return true;
}

//**************************************************************************************************
/**
 * Function to generate a legend or text box.
 */
//**************************************************************************************************
TPave* PlotPainter::GenerateBox(variant<shared_ptr<Plot::Pad::LegendBox>, shared_ptr<Plot::Pad::TextBox>> boxVariant, coordinates_t coordinates, TPad* pad)
{
  TPave* returnBox{nullptr};

  auto processBox = [&](auto&& box) {
    using BoxType = std::decay_t<decltype(*box)>;
    constexpr bool isLegend = std::is_same_v<BoxType, Plot::Pad::LegendBox>;

    const float_t textSize = box->GetTextSize().value_or(24.f);
    const int16_t textFont = box->GetTextFont().value_or(43);

    // the drawn data a legend row refers to (named "<number of the data in the pad>:<name>" when drawn), in this pad or in another one
    // (with the option it is drawn with, which TObject::GetDrawOption finds only in the current pad)
    auto findDrawn = [this, pad](uint16_t dataID, const optional<uint8_t>& padID) -> std::pair<TObject*, string> {
      TPad* source = (padID) ? dynamic_cast<TPad*>(pad->GetCanvas()->GetPrimitive(("Pad_" + std::to_string(*padID)).data())) : pad;
      if (!source) return {nullptr, ""};
      const string prefix = std::to_string(dataID) + ":";
      for (TObjLink* link = source->GetListOfPrimitives()->FirstLink(); link; link = link->Next()) {
        if (str_starts_with(link->GetObject()->GetName(), prefix)) return {link->GetObject(), link->GetOption()};
      }
      if (auto notPainted = mNotPainted.find({source, dataID}); notPainted != mNotPainted.end()) {
        return notPainted->second;
      }
      return {nullptr, ""};
    };
    auto findData = [&findDrawn](uint16_t dataID, const optional<uint8_t>& padID) { return findDrawn(dataID, padID).first; };
    // the data a placeholder of a text names (a histogram drawn as graph: the histogram)
    auto findTextData = [&](uint16_t dataID, const optional<uint8_t>& padID) -> TNamed* {
      TObject* data_ptr = findData(dataID, padID);
      if (auto source = mSourceHists.find(data_ptr); data_ptr && source != mSourceHists.end()) {
        data_ptr = source->second.get();
      }
      return dynamic_cast<TNamed*>(data_ptr);
    };

    vector<string> lines;
    if constexpr (isLegend) {
      for (const auto& entry : box->GetEntries()) {
        lines.push_back(entry.GetLabel().value_or(""));
      }
    } else {
      lines = box->GetLines();
      if (lines.size() == 1 && lines[0].empty()) lines.clear();  // an empty text: no box
    }

    uint8_t nColumns{1u};
    bool orderTopDown{false};
    if constexpr (isLegend) {
      if (box->GetNumColumns()) nColumns = *box->GetNumColumns();
      orderTopDown = box->GetOrderTopDown().value_or(false);
    }
    uint16_t nLines = lines.size();
    if (nLines < 1) return;
    // TLegend fills its columns row by row: for the order top down the entries are rearranged when they are added; columns that
    // would stay empty are dropped (top down: 5 entries in 4 columns are 2 rows, so 3 columns; row by row: fewer entries than columns)
    const uint16_t nRows = (nLines + nColumns - 1) / nColumns;
    if (orderTopDown || nLines < nColumns) nColumns = (nLines + nRows - 1) / nRows;
    auto columnOf = [&](uint16_t lineID) { return (orderTopDown) ? lineID / nRows : lineID % nColumns; };

    const auto [padWidthPixel, padHeightPixel] = TextPadSizePixel(pad);

    // determine max width and height of legend entries
    double_t contentWidthPixel{};
    double_t titleWidthPixel{};
    vector<uint32_t> contentWidthPixelPerColumn(nColumns, 1);
    vector<double_t> layoutWidthPerColumn(nColumns, 0.);  // the widths TLegend shares its columns out by (TLatex::GetXsize)

    const double_t textHeightPixel = TextHeightPixel(textFont, textSize, pad);  // lines with sub- or superscripts are higher
    double_t lineHeightPixel = textHeightPixel;

    // font and size of an entry that sets one of them: the other one taken from the box, a size inherited across font precisions
    // converted (pixels for precision 3, a fraction of the pad otherwise), so that both are given to the entry and scale together
    auto entryText = [&](const auto& entry) -> std::pair<int16_t, float_t> {
      const int16_t font = entry.GetTextFont().value_or(textFont);
      if (entry.GetTextSize()) return {font, *entry.GetTextSize()};
      if ((font % 10 > 2) == (textFont % 10 > 2)) return {font, textSize};
      const double_t heightPixel = TextHeightPixel(textFont, textSize, pad);
      const auto [width, height] = TextPadSizePixel(pad);
      return {font, static_cast<float_t>((font % 10 > 2) ? heightPixel / mScale : heightPixel / std::min(width, height))};
    };

    struct LineExtent {
      double_t over;
      double_t under;
    };
    vector<LineExtent> lineExtents;  // legend rows: how far they reach above and below their baseline
    uint16_t lineID{};
    for (auto& line : lines) {
      auto lineTextFont = textFont;
      auto lineTextSize = textSize;
      if constexpr (isLegend) {
        const auto& entry = box->GetEntries()[lineID];
        if (entry.GetTextFont() || entry.GetTextSize()) std::tie(lineTextFont, lineTextSize) = entryText(entry);
        if (entry.GetRefDataID()) {
          if (TObject* data_ptr = findData(*entry.GetRefDataID(), entry.GetRefPadID())) {
            if (auto source = mSourceHists.find(data_ptr); source != mSourceHists.end()) {
              data_ptr = source->second.get();  // a histogram drawn as graph
            }
            ReplacePlaceholders(line, static_cast<TNamed*>(data_ptr));
          } else {
            logger::warning("Legend entry '{}' refers to data {}, which is not drawn in {}.", line, *entry.GetRefDataID(), (entry.GetRefPadID()) ? "pad " + std::to_string(*entry.GetRefPadID()) : "this pad");
          }
        }
      } else if (line.find('<') != string::npos) {
        ReplacePlaceholders(line, nullptr, findTextData);
      }
      DropEmptyGroups(line);

      // determine width and height of line to find max width and height (per column)
      TLatex textLine(0, 0, line.data());
      textLine.SetTextFont(lineTextFont);
      textLine.SetTextSize(lineTextSize);
      auto [width, height] = GetTextDimensions(textLine, pad);
      if (height > lineHeightPixel) lineHeightPixel = height;
      if constexpr (isLegend) {
        const auto [over, under] = GetTextExtentPixel(textLine, pad);
        lineExtents.push_back({over, under});
      }
      if (auto& columnWidth = contentWidthPixelPerColumn[columnOf(lineID)]; width > columnWidth) {
        columnWidth = width;
      }
      if (nColumns > 1) {
        // with the size the legend is painted with (ApplyScale): a text in TMathText syntax does not keep its proportion to others across sizes
        auto& layoutWidth = layoutWidthPerColumn[columnOf(lineID)];
        TLatex painted{textLine};
        if (lineTextFont % 10 == 3) painted.SetTextSize(lineTextSize * mScale);
        layoutWidth = std::max(layoutWidth, painted.GetXsize());
      }
      ++lineID;
    }
    for (const auto& length : contentWidthPixelPerColumn) {
      contentWidthPixel += length;
    }
    if (nColumns > 1) {
      // TLegend gives each column a share of the width for the labels in proportion to their widths as TLatex lays them out, which
      // differ from the painted ones for drawn parts (#frac, #sqrt): the labels get the width that gives every column its painted width
      double_t layoutTotal{};
      for (const auto& length : layoutWidthPerColumn) {
        layoutTotal += length;
      }
      for (uint8_t column = 0; column < nColumns; ++column) {
        if (layoutWidthPerColumn[column] > 0.) contentWidthPixel = std::max(contentWidthPixel, contentWidthPixelPerColumn[column] * layoutTotal / layoutWidthPerColumn[column]);
      }
    }
    uint32_t symbolColWidthPixel{0};
    if constexpr (isLegend) {
      TLatex markerDummy(0, 0, "-+-");  // defines width of marker
      markerDummy.SetTextFont(textFont);
      markerDummy.SetTextSize(textSize);
      auto [w, h] = GetTextDimensions(markerDummy, pad);
      symbolColWidthPixel = box->GetSymbolColScale().value_or(1.) * w;

      if (auto title = box->GetTitle()) {
        DropEmptyGroups(*title);
        TLatex textLine(0, 0, (*title).data());
        textLine.SetTextFont(textFont);
        textLine.SetTextSize(textSize);
        auto [width, height] = GetTextDimensions(textLine, pad);
        titleWidthPixel = width;
        if (height > lineHeightPixel) lineHeightPixel = height;
      }
    }

    float_t marginNDC = box->GetMargin().value_or(0.01);
    double_t lineSpacing = box->GetLineSpacing().value_or(0.3);
    if constexpr (isLegend) {
      // TLegend centres a row in its place (by its drawn extent) only if half a place is at least the text size of the legend, read
      // as a fraction of the pad height also when it is relative to the smaller pad side (so not in pads taller than wide); otherwise
      // it puts the baseline 0.45 of half a place below the centre (TLegend::PaintPrimitives). Rows reaching further below (#frac) or
      // above get higher places, whichever way TLegend places them, so that the box does not depend on the shape of the pad.
      const double_t gap = mScale;
      for (const auto& extent : lineExtents) {
        const double_t needed = std::max((extent.under + gap) / 0.55, (extent.over + gap) / 1.45);
        lineHeightPixel = std::max(lineHeightPixel, needed / (0.5 * (1 + lineSpacing)));
      }
    }

    double_t contentWidthNDC = contentWidthPixel / padWidthPixel;
    double_t lineHeightNDC = lineHeightPixel / padHeightPixel;
    double_t symbolColWidthNDC = symbolColWidthPixel / padWidthPixel;
    double_t titleWidthNDC = titleWidthPixel / padWidthPixel;
    double_t borderWidthNDC = mScale * box->GetBorderWidth().value_or(0.) / padWidthPixel;

    double_t totalWidthNDC{};
    double_t totalHeightNDC{};

    double_t totalMarginWidthNDC = (0.5 * borderWidthNDC + marginNDC + symbolColWidthNDC);

    if constexpr (isLegend) {
      totalWidthNDC = borderWidthNDC + 2 * marginNDC + symbolColWidthNDC + contentWidthNDC;
      totalHeightNDC = (1 + lineSpacing) * lineHeightNDC * std::ceil(static_cast<double_t>(nLines) / nColumns);
      if (box->GetTitle()) {
        if (titleWidthNDC > totalWidthNDC) {
          totalWidthNDC = borderWidthNDC + titleWidthNDC + 0.1 * totalMarginWidthNDC;  // root places header at margin/10
        }
        totalHeightNDC += (1 + lineSpacing) * lineHeightNDC;
      }
    } else {
      totalWidthNDC = borderWidthNDC + 2 * marginNDC + contentWidthNDC;
      totalHeightNDC = (1 + lineSpacing) * nLines * lineHeightNDC;
    }
    const auto [unitX, unitY] = SizeUnit(coordinates, pad);
    if (box->GetWidth()) {
      totalWidthNDC = *box->GetWidth() * unitX;
    }
    if (box->GetHeight()) {
      totalHeightNDC = *box->GetHeight() * unitY;
    }

    double_t relMarginWidth = totalMarginWidthNDC / totalWidthNDC;
    double_t relColumnSeparation{};
    if constexpr (isLegend) {
      if (nColumns > 1) {
        // TLegend gives every column the symbol space (its margin) and a share of the remaining width in proportion to the widest label
        // of the column, and puts the column separation between the columns (TLegend::PaintPrimitives): with the width they need,
        // the columns are as wide as their labels and the spacing is the same between all of them
        const double_t columnSpacingNDC = box->GetColumnSpacing().value_or(1.f) * textHeightPixel / padWidthPixel;
        const double_t neededWidthNDC = 0.5 * borderWidthNDC + nColumns * totalMarginWidthNDC + contentWidthNDC + (nColumns - 1) * columnSpacingNDC + marginNDC;
        if (!box->GetWidth()) totalWidthNDC = std::max(neededWidthNDC, totalWidthNDC);
        relMarginWidth = nColumns * totalMarginWidthNDC / totalWidthNDC;
        relColumnSeparation = (nColumns - 1) * columnSpacingNDC / totalWidthNDC;
      }
    }

    const double_t borderX = mScale * box->GetBorderWidth().value_or(0.) / padWidthPixel;
    const double_t borderY = mScale * box->GetBorderWidth().value_or(0.) / padHeightPixel;
    auto [upperLeftX, upperLeftY] = PlaceBox(*box, coordinates, pad, totalWidthNDC, totalHeightNDC, borderX, borderY, (isLegend) ? "legend" : "text");
    if constexpr (!isLegend) {
      // a single line placed at a position is aligned at it as ROOT aligns a TLatex at its point (the tens 1, 2, 3: left, centre, right;
      // the units 1, 2, 3: bottom, centre, top), otherwise the position is the upper left corner of the box
      if (const auto& align = box->GetTextAlign(); align && nLines == 1 && !box->IsAutoPlacement()) {
        upperLeftX -= 0.5 * (std::clamp(*align / 10, 1, 3) - 1) * totalWidthNDC;
        upperLeftY += 0.5 * (3 - std::clamp(*align % 10, 1, 3)) * totalHeightNDC;
      }
    }
    if (!box->IsAutoPlacement()) WarnIfBeyondPad((isLegend) ? "legend" : "text", upperLeftX, upperLeftY, totalWidthNDC, totalHeightNDC);

    if constexpr (isLegend) {
      TLegend* legend = new TLegend(upperLeftX, upperLeftY - totalHeightNDC, upperLeftX + totalWidthNDC, upperLeftY, "", "NDC NB");
      legend->SetMargin(relMarginWidth);
      legend->SetTextAlign(kHAlignLeft + kVAlignCenter);
      legend->SetNColumns(nColumns);
      legend->SetColumnSeparation(static_cast<float_t>(relColumnSeparation));
      legend->SetTextFont(textFont);
      legend->SetTextSize(textSize);
      if (const auto& textColor = box->GetTextColor()) legend->SetTextColor(textColor->ToIndex());
      if (const auto& textAlpha = box->GetTextAlpha()) legend->SetTextColor(TColor::GetColorTransparent(legend->GetTextColor(), *textAlpha));

      if (auto title = box->GetTitle()) {
        DropEmptyGroups(*title);
        string center = (nColumns > 1) ? "c" : "";
        legend->SetHeader((*title).data(), center.data());
      }

      const uint16_t nPlaces = (orderTopDown) ? nRows * nColumns : nLines;
      for (uint16_t place = 0; place < nPlaces; ++place) {
        // the entry at this place of TLegend's row-by-row filling
        const uint16_t i = (orderTopDown) ? (place % nColumns) * nRows + place / nColumns : place;
        if (i >= nLines) {
          legend->AddEntry(static_cast<TObject*>(nullptr), "", "");  // a free place at the bottom of the last column
          continue;
        }
        const auto& entry = box->GetEntries()[i];
        string label = lines[i];

        // user-defined symbol elements for single entry or all entries
        string symbolElements = entry.GetSymbolElements() ? *entry.GetSymbolElements() : ((box->GetDefaultSymbolElements()) ? *box->GetDefaultSymbolElements() : "");

        TLegendEntry* curEntry = nullptr;
        TAttMarker markerAttr;
        TAttLine lineAttr;
        TAttFill fillAttr;
        // the symbol a drawn data gives the row (elements: those set for the row, else what the data draws) in the look of the data
        struct SymbolPart {
          uint16_t dataID;
          string elements;
          TAttMarker marker;
          TAttLine line;
          TAttFill fill;
        };
        auto symbolPart = [](uint16_t dataID, TObject* data_ptr, const string& drawnWith, const string& elements) {
          const LegendSymbol drawn = GetLegendSymbol(data_ptr, drawnWith);
          SymbolPart part{dataID, str_upper((elements.empty()) ? drawn.elements : elements), {}, {}, {}};
          if (auto ptr = dynamic_cast<TAttMarker*>(data_ptr)) ptr->Copy(part.marker);
          if (auto ptr = dynamic_cast<TAttLine*>(data_ptr)) ptr->Copy(part.line);
          if (auto ptr = dynamic_cast<TAttFill*>(data_ptr)) ptr->Copy(part.fill);
          // what is not drawn is not shown (TLegend would draw lines of width 0 as hairlines, hollow fills as outlines)
          auto drop = [&part](char element) { part.elements.erase(std::remove(part.elements.begin(), part.elements.end(), element), part.elements.end()); };
          if (part.line.GetLineWidth() <= 0) {
            drop('L');
            drop('E');
          }
          const bool hollow = part.fill.GetFillStyle() == 0 || part.fill.GetFillColor() == 0;
          // the outline TLegend draws around a fill symbol without line
          if (str_contains(part.elements, "F") && !str_contains(part.elements, "L")) {
            if (drawn.outline == Outline::thin && part.line.GetLineWidth() > 0) {  // outlined thin and solid in the line colour
              part.line.SetLineWidth(1);
              part.line.SetLineStyle(kSolid);
              if (hollow) part.fill.SetFillColor(part.line.GetLineColor());
            }
            if (drawn.outline == Outline::none || part.line.GetLineWidth() <= 0) part.line.SetLineColor(TColor::GetColorTransparent(part.line.GetLineColor(), 0.f));
          }
          return part;
        };
        // the parts of the row's data and of the data added with AddSymbolOf, in the order they are drawn
        std::vector<SymbolPart> parts;
        const auto [data_ptr, drawnWith] = (entry.GetRefDataID()) ? findDrawn(*entry.GetRefDataID(), entry.GetRefPadID()) : std::pair<TObject*, string>{nullptr, ""};
        if (data_ptr) parts.push_back(symbolPart(*entry.GetRefDataID(), data_ptr, drawnWith, symbolElements));
        for (uint16_t dataID : entry.GetSymbolOf()) {
          const auto [other_ptr, otherDrawnWith] = findDrawn(dataID, entry.GetRefPadID());
          if (!other_ptr) {
            logger::warning("Legend row \"{}\": there is no data {} whose symbol could be added.", label, dataID);
            continue;
          }
          parts.push_back(symbolPart(dataID, other_ptr, otherDrawnWith, ""));
        }
        std::stable_sort(parts.begin(), parts.end(), [](const SymbolPart& a, const SymbolPart& b) { return a.dataID < b.dataID; });
        if (!parts.empty()) {
          // each element in the look of the data giving it, the one drawn later where two give the same (as in the plot); the line of the row
          // is that of the L element, without one the outline of the fill (TLegend draws no error bar with a fill), else that of the error bars
          symbolElements.clear();
          const SymbolPart* lineFrom = nullptr;
          const SymbolPart* outlineFrom = nullptr;
          const SymbolPart* errorsFrom = nullptr;
          for (const auto& part : parts) {
            for (char element : part.elements) {
              if (!str_contains(symbolElements, string(1, element))) symbolElements += element;
              if (element == 'P') part.marker.Copy(markerAttr);
              if (element == 'F') {
                part.fill.Copy(fillAttr);
                outlineFrom = &part;
              }
              if (element == 'L') lineFrom = &part;
              if (element == 'E') errorsFrom = &part;
            }
          }
          if (const SymbolPart* from = (lineFrom) ? lineFrom : ((outlineFrom) ? outlineFrom : errorsFrom)) from->line.Copy(lineAttr);
        }
        curEntry = legend->AddEntry(static_cast<TObject*>(nullptr), label.data(), symbolElements.data());

        // the attributes of the data, overridden by the defaults of the box and by those of the entry
        curEntry->SetMarkerColor(markerAttr.GetMarkerColor());
        curEntry->SetMarkerStyle(markerAttr.GetMarkerStyle());
        curEntry->SetMarkerSize(markerAttr.GetMarkerSize());
        curEntry->SetLineColor(lineAttr.GetLineColor());
        curEntry->SetLineStyle(lineAttr.GetLineStyle());
        curEntry->SetLineWidth(lineAttr.GetLineWidth());
        curEntry->SetFillColor(fillAttr.GetFillColor());
        curEntry->SetFillStyle(fillAttr.GetFillStyle());

        if (box->GetDefaultMarkerColor()) curEntry->SetMarkerColor(box->GetDefaultMarkerColor()->ToIndex());
        if (box->GetDefaultMarkerAlpha()) curEntry->SetMarkerColor(TColor::GetColorTransparent(curEntry->GetMarkerColor(), *box->GetDefaultMarkerAlpha()));
        if (box->GetDefaultMarkerStyle()) curEntry->SetMarkerStyle(*box->GetDefaultMarkerStyle());
        if (box->GetDefaultMarkerSize()) curEntry->SetMarkerSize(*box->GetDefaultMarkerSize());

        if (box->GetDefaultLineColor()) curEntry->SetLineColor(box->GetDefaultLineColor()->ToIndex());
        if (box->GetDefaultLineAlpha()) curEntry->SetLineColor(TColor::GetColorTransparent(curEntry->GetLineColor(), *box->GetDefaultLineAlpha()));
        if (box->GetDefaultLineStyle()) curEntry->SetLineStyle(*box->GetDefaultLineStyle());
        if (box->GetDefaultLineWidth()) curEntry->SetLineWidth(*box->GetDefaultLineWidth());

        if (box->GetDefaultFillColor()) curEntry->SetFillColor(box->GetDefaultFillColor()->ToIndex());
        if (box->GetDefaultFillAlpha()) curEntry->SetFillColor(TColor::GetColorTransparent(curEntry->GetFillColor(), *box->GetDefaultFillAlpha()));
        if (box->GetDefaultFillStyle()) curEntry->SetFillStyle(*box->GetDefaultFillStyle());

        if (entry.GetMarkerColor()) curEntry->SetMarkerColor(entry.GetMarkerColor()->ToIndex());
        if (entry.GetMarkerAlpha()) curEntry->SetMarkerColor(TColor::GetColorTransparent(curEntry->GetMarkerColor(), *entry.GetMarkerAlpha()));
        if (entry.GetMarkerStyle()) curEntry->SetMarkerStyle(*entry.GetMarkerStyle());
        if (entry.GetMarkerSize()) curEntry->SetMarkerSize(*entry.GetMarkerSize());

        if (entry.GetLineColor()) curEntry->SetLineColor(entry.GetLineColor()->ToIndex());
        if (entry.GetLineAlpha()) curEntry->SetLineColor(TColor::GetColorTransparent(curEntry->GetLineColor(), *entry.GetLineAlpha()));
        if (entry.GetLineStyle()) curEntry->SetLineStyle(*entry.GetLineStyle());
        if (entry.GetLineWidth()) curEntry->SetLineWidth(*entry.GetLineWidth());

        if (entry.GetFillColor()) curEntry->SetFillColor(entry.GetFillColor()->ToIndex());
        if (entry.GetFillAlpha()) curEntry->SetFillColor(TColor::GetColorTransparent(curEntry->GetFillColor(), *entry.GetFillAlpha()));
        if (entry.GetFillStyle()) curEntry->SetFillStyle(*entry.GetFillStyle());

        if (entry.GetTextColor()) curEntry->SetTextColor(entry.GetTextColor()->ToIndex());
        if (entry.GetTextAlpha()) curEntry->SetTextColor(TColor::GetColorTransparent(curEntry->GetTextColor(), *entry.GetTextAlpha()));
        if (entry.GetTextFont() || entry.GetTextSize()) {
          const auto [font, size] = entryText(entry);
          curEntry->SetTextFont(font);
          curEntry->SetTextSize(size);
        }
      }
      returnBox = legend;
    } else {
      TPaveText* paveText = new TPaveText(upperLeftX, upperLeftY - totalHeightNDC, upperLeftX + totalWidthNDC, upperLeftY, "NDC NB");
      paveText->SetMargin(relMarginWidth);
      paveText->SetTextAlign(box->GetTextAlign().value_or(kHAlignLeft + kVAlignCenter));
      paveText->SetBorderSize(1);
      paveText->SetTextFont(textFont);
      paveText->SetTextSize(textSize);
      if (const auto& textColor = box->GetTextColor()) paveText->SetTextColor(textColor->ToIndex());
      if (const auto& textAlpha = box->GetTextAlpha()) paveText->SetTextColor(TColor::GetColorTransparent(paveText->GetTextColor(), *textAlpha));

      for (const auto& line : lines) {
        TText* text = paveText->AddText(line.data());
        text->SetTextFont(textFont);
        text->SetTextSize(textSize);
      }
      returnBox = paveText;
    }

    if (returnBox) {
      returnBox->SetLineWidth(0);
      returnBox->SetFillStyle(0);
      ApplyBoxStyle(*box, returnBox);
      // without a border also no border size: TLegend draws the outline of the first hollow fill symbol with the line width of the box
      // (0, invisible), so the row of a band with a transparent line had no symbol
      if (returnBox->GetLineWidth() == 0) returnBox->SetBorderSize(0);
    }
  };
  std::visit(processBox, boxVariant);
  return returnBox;
}

//**************************************************************************************************
/**
 * Draws an image in a transparent sub-pad of the pad, at its position or placed automatically like the other boxes.
 * The image itself is stored with the canvas (Plots.root then opens without the file); a size not given keeps its aspect ratio.
 */
//**************************************************************************************************
void PlotPainter::DrawImage(const Plot::Pad::ImageBox& box, coordinates_t coordinates, TPad* pad, const string& name)
{
  const auto path = expand_path(box.GetPath());
  if (!path) return;
  std::error_code existsError;
  if (!std::filesystem::exists(*path, existsError)) {
    logger::warning("Image {} does not exist.", *path);
    return;
  }
  TImage* image = TImage::Open(path->data());
  if (!image || !image->IsValid()) {
    delete image;
    // without asimage ROOT has no image class at all
    std::unique_ptr<TImage> any(TImage::Create());
    if (any) {
      logger::warning("Image {} could not be read: it is not an image ROOT can read (png, jpg, gif, bmp, xpm, tiff, …).", *path);
    } else {
      logger::warning("Image {} could not be read (ROOT needs to be built with asimage for images).", *path);
    }
    return;
  }
  const auto [padWidthPixel, padHeightPixel] = PadSizePixel(pad);
  const double_t aspect = static_cast<double_t>(image->GetHeight()) / image->GetWidth() * padWidthPixel / padHeightPixel;  // height / width in NDC
  const auto [unitX, unitY] = SizeUnit(coordinates, pad);
  double_t widthNDC = box.GetWidth().value_or(0.2) * unitX;
  double_t heightNDC = (box.GetHeight()) ? *box.GetHeight() * unitY : widthNDC * aspect;
  if (box.GetHeight() && !box.GetWidth()) widthNDC = heightNDC / aspect;

  const auto [upperLeftX, upperLeftY] = PlaceBox(box, coordinates, pad, widthNDC, heightNDC, 0., 0., "image");
  constexpr double_t tolerance = 1e-9;
  if (widthNDC <= 0. || heightNDC <= 0. || upperLeftX < -tolerance || upperLeftY - heightNDC < -tolerance || upperLeftX + widthNDC > 1. + tolerance || upperLeftY > 1. + tolerance) {
    logger::warning("Image {} is not drawn: it does not fit into its pad (upper left corner ({:.3g}, {:.3g}), size {:.3g} x {:.3g}).", name.substr(name.find('_') + 1), upperLeftX, upperLeftY, widthNDC, heightNDC);
    delete image;
    return;
  }
  pad->cd();
  auto* imagePad = new TPad(name.data(), path->data(), std::max(upperLeftX, 0.), std::max(upperLeftY - heightNDC, 0.), std::min(upperLeftX + widthNDC, 1.), std::min(upperLeftY, 1.));
  imagePad->SetFillStyle(0);
  imagePad->SetBorderMode(0);
  imagePad->SetBorderSize(0);
  imagePad->SetMargin(0., 0., 0., 0.);
  imagePad->SetBit(kCanDelete);  // owned by the pad
  // a double-click in show mode reports the position as given
  imagePad->SetUniqueID(coordinates);
  imagePad->Draw();
  imagePad->cd();
  image->SetEditable(true);  // events go to the pad (otherwise ROOT zooms into the image on a click)
  image->SetBit(kCanDelete);
  image->Draw("X");  // fills the sub-pad, whose aspect ratio is set above
  // ROOT's bitmap output (TImageDump) draws all texts of the canvas wrongly (no minus signs, shifted indices) when an image is the last
  // thing painted before a text: an empty text after it avoids that
  auto* textAfterImage = new TText(0., 0., "");
  textAfterImage->SetBit(kCanDelete);
  textAfterImage->Draw();
  pad->cd();
}

//**************************************************************************************************
/**
 * The frame of a pad as fractions of the pad (x1, y1, x2, y2); a 3d view has none: the pad within its margins; a pad without data (an
 * empty layout cell) keeps ROOT's range 0 to 1 for the pad and the frame: the whole pad.
 */
//**************************************************************************************************
std::array<double_t, 4> PlotPainter::FrameInPad(const TVirtualPad* pad)
{
  if (pad->GetView()) return {pad->GetLeftMargin(), pad->GetBottomMargin(), 1. - pad->GetRightMargin(), 1. - pad->GetTopMargin()};
  const double_t width = pad->GetX2() - pad->GetX1();
  const double_t height = pad->GetY2() - pad->GetY1();
  return {(pad->GetUxmin() - pad->GetX1()) / width, (pad->GetUymin() - pad->GetY1()) / height, (pad->GetUxmax() - pad->GetX1()) / width, (pad->GetUymax() - pad->GetY1()) / height};
}

//**************************************************************************************************
/**
 * The NDC of a width and a height of 1 set for a box (SetSize, SetWidth, SetHeight): the frame in frame coordinates, the pad otherwise.
 */
//**************************************************************************************************
std::pair<double_t, double_t> PlotPainter::SizeUnit(coordinates_t coordinates, const TVirtualPad* pad)
{
  if (coordinates != frame_coordinates) return {1., 1.};
  const auto [x1, y1, x2, y2] = FrameInPad(pad);
  return {x2 - x1, y2 - y1};
}

//**************************************************************************************************
/**
 * Upper left corner (in NDC) of a box of the given size: at the position set for it (converted from frame or user coordinates)
 * or placed automatically within the frame of the pad (borderX, borderY: width of its border, half of which sticks out on each side).
 */
//**************************************************************************************************
template <typename BoxType>
std::pair<double_t, double_t> PlotPainter::PlaceBox(const BoxType& box, coordinates_t coordinates, TPad* pad, double_t widthNDC, double_t heightNDC, double_t borderX, double_t borderY, const string& boxType)
{
  if (box.IsAutoPlacement()) {
    double_t lowerLeftX{};
    double_t lowerLeftY{};
    FindBoxPosition(pad, widthNDC + borderX, heightNDC + borderY, *box.GetPlacement(), boxType, lowerLeftX, lowerLeftY);
    return {lowerLeftX + 0.5 * borderX, lowerLeftY + 0.5 * borderY + heightNDC};
  }
  if (coordinates == frame_coordinates) {
    const auto [x1, y1, x2, y2] = FrameInPad(pad);
    return {x1 + box.GetXPosition() * (x2 - x1), y1 + box.GetYPosition() * (y2 - y1)};
  }
  if (coordinates == user_coordinates) {
    // pad coordinates are log10 of the axis values on a log axis (XtoPad/YtoPad, as ROOT places objects in user coordinates);
    // a value at or below zero has none: ROOT puts it at the frame edge
    if ((pad->GetLogx() && box.GetXPosition() <= 0.) || (pad->GetLogy() && box.GetYPosition() <= 0.)) {
      logger::warning("The {} at ({}, {}) in user coordinates is put at the edge of the frame: a log axis has no position for values at or below zero.", boxType, box.GetXPosition(), box.GetYPosition());
    }
    return {(pad->XtoPad(box.GetXPosition()) - pad->GetX1()) / (pad->GetX2() - pad->GetX1()), (pad->YtoPad(box.GetYPosition()) - pad->GetY1()) / (pad->GetY2() - pad->GetY1())};
  }
  return {box.GetXPosition(), box.GetYPosition()};
}

//**************************************************************************************************
/**
 * Warns about a box at a position set for it that reaches beyond its pad (it is drawn: the part outside is cut off).
 */
//**************************************************************************************************
void PlotPainter::WarnIfBeyondPad(const string& boxType, double_t upperLeftX, double_t upperLeftY, double_t widthNDC, double_t heightNDC)
{
  constexpr double_t tolerance = 1e-9;
  if (upperLeftX < -tolerance || upperLeftY - heightNDC < -tolerance || upperLeftX + widthNDC > 1. + tolerance || upperLeftY > 1. + tolerance) {
    logger::warning("The {} reaches beyond its pad (upper left corner ({:.3g}, {:.3g}), size {:.3g} x {:.3g}, in units of the pad): the part outside of it is cut off.", boxType, upperLeftX, upperLeftY, widthNDC, heightNDC);
  }
}

//**************************************************************************************************
/**
 * Applies the border and fill set for a box (the ones not set keep the values the box has).
 */
//**************************************************************************************************
template <typename BoxType>
void PlotPainter::ApplyBoxStyle(const BoxType& box, TPave* pave)
{
  if (box.GetBorderStyle()) pave->SetLineStyle(*box.GetBorderStyle());
  if (box.GetBorderColor()) pave->SetLineColor(box.GetBorderColor()->ToIndex());
  if (box.GetBorderAlpha()) pave->SetLineColor(TColor::GetColorTransparent(pave->GetLineColor(), *box.GetBorderAlpha()));
  if (box.GetBorderWidth()) pave->SetLineWidth(*box.GetBorderWidth());
  if (box.GetFillStyle()) pave->SetFillStyle(*box.GetFillStyle());
  if (box.GetFillColor()) pave->SetFillColor(box.GetFillColor()->ToIndex());
  if (box.GetFillAlpha()) pave->SetFillColor(TColor::GetColorTransparent(pave->GetFillColor(), *box.GetFillAlpha()));
}

//**************************************************************************************************
/**
 * Position of a box of the given size (in NDC, including its border) that is placed automatically within the frame of the pad.
 * Without enough free space, the box is placed beside the other boxes on top of the data, or if there is no such room in the top left
 * corner of the frame (with a warning).
 * Returns the lower left corner of the box in NDC.
 */
//**************************************************************************************************
void PlotPainter::FindBoxPosition(TPad* pad, double_t width, double_t height, box_placement_t placement, const string& boxType, double_t& lowerLeftX, double_t& lowerLeftY)
{
  pad->cd();

  // area available for the box: the frame without the ticks of the axes and a small gap (in NDC)
  constexpr double_t kDistanceToFrame{1.4};  // in units of the tick length
  TH1* axisHist{};
  bool polarGrid{};  // the polar grid of polar graphs fills the pad, which has no frame and no ticks at its edges
  for (auto* obj : *pad->GetListOfPrimitives()) {
    if (obj->InheritsFrom(TH1::Class()) && TString(obj->GetName()).BeginsWith("axis_hist")) {
      axisHist = static_cast<TH1*>(obj);
    } else if (obj->InheritsFrom(TGraphPolargram::Class())) {
      polarGrid = true;
    }
  }
  const double_t tickLengthX = (axisHist) ? axisHist->GetXaxis()->GetTickLength() : gStyle->GetTickLength("X");  // ticks on the x axis (vertical)
  const double_t tickLengthY = (axisHist) ? axisHist->GetYaxis()->GetTickLength() : gStyle->GetTickLength("Y");  // ticks on the y axis (horizontal)
  const auto [frameX1, frameY1, frameX2, frameY2] = FrameInPad(pad);
  constexpr double_t kDistanceToPadEdge{0.015};  // without ticks: a small gap to the edge of the pad
  const double_t distanceX = (polarGrid) ? kDistanceToPadEdge : kDistanceToFrame * tickLengthY * (frameX2 - frameX1);
  const double_t distanceY = (polarGrid) ? kDistanceToPadEdge : kDistanceToFrame * tickLengthX * (frameY2 - frameY1);
  const std::array<double_t, 4> freeArea{frameX1 + distanceX, frameY1 + distanceY, frameX2 - distanceX, frameY2 - distanceY};

  // find box position that does not collide with any of the drawn objects
  const double_t freeWidthNDC = freeArea[2] - freeArea[0];
  const double_t freeHeightNDC = freeArea[3] - freeArea[1];
  const bool fitsIntoFrame = (width <= freeWidthNDC && height <= freeHeightNDC);
  bool coversData{};
  const bool found = fitsIntoFrame && FindFreeSpace(pad, freeArea, width, height, placement, lowerLeftX, lowerLeftY, coversData);
  if (found && !coversData) return;
  if (fitsIntoFrame) {
    logger::warning("Could not find enough space to place the {} properly.", boxType);
    if (found) return;
  } else {
    logger::warning("The {} ({:.2f} x {:.2f}) is larger than the space inside the frame ({:.2f} x {:.2f}, in units of the pad size): use fewer lines, more columns or a smaller text size.",
                    boxType, width, height, freeWidthNDC, freeHeightNDC);
  }
  // just place it in the top left corner of the frame
  lowerLeftX = freeArea[0];
  lowerLeftY = freeArea[3] - height;
}

//**************************************************************************************************
/**
 * Find a position for a box of the given size (in NDC) within the free area (x1, y1, x2, y2 in NDC) that does not overlap with anything drawn.
 * Replaces TPad::PlaceBox, which considers all bins of a histogram (also those outside of the visible range),
 * does not clip to the pad (absurd values like huge error bars then take forever and block the whole pad)
 * and samples graphs only coarsely.
 * The box is placed at the free position closest to the given corner of the frame
 * or (for best_corner) closest to the corner where it fits best.
 * Without such a position, the box goes on top of maps (filled bins of 2d data) and then on top of anything but the other boxes
 * (coversData set).
 * Returns the lower left corner of the box in NDC.
 */
//**************************************************************************************************
bool PlotPainter::FindFreeSpace(TPad* pad, const std::array<double_t, 4>& freeArea, double_t width, double_t height, box_placement_t placement, double_t& lowerLeftX, double_t& lowerLeftY, bool& coversData)
{
  const double_t cellSize = 10. * mScale;  // grid cell size in pixels (as in TPad::PlaceBox), scaled so that boxes are placed the same way at any scale
  // size of the pad in pixels (GetWw and GetWh alone are the size of the whole canvas)
  const double_t padWidthPixel = PadSizePixel(pad).first;  // no structured binding: captured below
  const double_t padHeightPixel = PadSizePixel(pad).second;
  const int32_t nx = static_cast<int32_t>(padWidthPixel / cellSize);
  const int32_t ny = static_cast<int32_t>(padHeightPixel / cellSize);
  if (nx <= 0 || ny <= 0) return false;
  // cells occupied by drawn objects: bit 1 for objects, bit 2 for filled bins of maps (a box goes on top of a map only if there is no other room),
  // bit 3 for boxes placed before (avoided also by a box that has to go on top of the data)
  constexpr uint8_t kObject{1u}, kMap{2u}, kBox{4u};
  coversData = false;
  vector<uint8_t> blocked(static_cast<size_t>(nx) * ny, 0u);

  // conversion from pad coordinates to (fractional) grid cells
  const double_t cellsPerX = nx / (pad->GetX2() - pad->GetX1());
  const double_t cellsPerY = ny / (pad->GetY2() - pad->GetY1());
  auto cellX = [&](double_t x) { return (x - pad->GetX1()) * cellsPerX; };
  auto cellY = [&](double_t y) { return (y - pad->GetY1()) * cellsPerY; };
  // conversion from user coordinates to pad coordinates (on a log axis non-positive values are moved to the pad edge)
  auto padX = [&](double_t x) { return (!pad->GetLogx()) ? x : ((x > 0.) ? std::log10(x) : pad->GetX1()); };
  auto padY = [&](double_t y) { return (!pad->GetLogy()) ? y : ((y > 0.) ? std::log10(y) : pad->GetY1()); };

  auto setRect = [&](double_t x1, double_t y1, double_t x2, double_t y2, uint8_t value) {  // in cells, inclusive (value: kObject, kMap or kBox)
    if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2)) return;
    if (x1 > x2) std::swap(x1, x2);
    if (y1 > y2) std::swap(y1, y2);
    if (x2 < 0. || y2 < 0. || x1 >= nx || y1 >= ny) return;  // completely outside of the grid
    // clamp to the grid before casting (values far outside of the pad do not fit into an integer)
    const int32_t i1 = static_cast<int32_t>(std::floor(std::max(x1, 0.)));
    const int32_t i2 = static_cast<int32_t>(std::floor(std::min(x2, nx - 1.)));
    const int32_t j1 = static_cast<int32_t>(std::floor(std::max(y1, 0.)));
    const int32_t j2 = static_cast<int32_t>(std::floor(std::min(y2, ny - 1.)));
    for (int32_t i = i1; i <= i2; ++i) {
      for (int32_t j = j1; j <= j2; ++j) {
        blocked[i + static_cast<size_t>(j) * nx] |= value;
      }
    }
  };
  // a point, with a marker of that size: ROOT draws 8 pixels per unit of the size, times mScale (ApplyScale, after the placement)
  auto markPoint = [&](double_t x, double_t y, float_t markerSize = 0.f) {
    const double_t halfCells = 4. * markerSize * mScale / cellSize;
    setRect(cellX(x) - halfCells, cellY(y) - halfCells, cellX(x) + halfCells, cellY(y) + halfCells, kObject);
  };
  auto markLine = [&](double_t x1, double_t y1, double_t x2, double_t y2) {  // in pad coordinates
    double_t cx1 = cellX(x1), cy1 = cellY(y1), cx2 = cellX(x2), cy2 = cellY(y2);
    if (!std::isfinite(cx1) || !std::isfinite(cy1) || !std::isfinite(cx2) || !std::isfinite(cy2)) return;
    // clip the line to the grid (Liang-Barsky) so lines far outside of the pad cost nothing
    double_t t0 = 0., t1 = 1.;
    const double_t dx = cx2 - cx1, dy = cy2 - cy1;
    for (auto [p, q] : {std::pair{-dx, cx1}, std::pair{dx, nx - cx1}, std::pair{-dy, cy1}, std::pair{dy, ny - cy1}}) {
      if (p == 0.) {
        if (q < 0.) return;
      } else {
        const double_t t = q / p;
        if (p < 0.) {
          t0 = std::max(t0, t);
        } else {
          t1 = std::min(t1, t);
        }
      }
    }
    if (t0 > t1) return;
    const int32_t nSteps = static_cast<int32_t>(std::ceil(2. * std::max(std::abs(dx), std::abs(dy)) * (t1 - t0))) + 1;
    for (int32_t step = 0; step <= nSteps; ++step) {
      const double_t t = t0 + (t1 - t0) * step / nSteps;
      setRect(cx1 + t * dx, cy1 + t * dy, cx1 + t * dx, cy1 + t * dy, kObject);
    }
  };

  // a filled polygon (pad coordinates): the cells whose centre row crosses its inside (scanline; the outline is marked separately)
  auto markPolygon = [&](const vector<std::pair<double_t, double_t>>& points) {
    if (points.size() < 3) return;
    double_t yMin = std::numeric_limits<double_t>::infinity(), yMax = -yMin;
    for (const auto& [x, y] : points) {
      yMin = std::min(yMin, cellY(y));
      yMax = std::max(yMax, cellY(y));
    }
    if (!std::isfinite(yMin) || !std::isfinite(yMax)) return;
    vector<double_t> crossings;
    for (int32_t j = std::max(0, static_cast<int32_t>(std::floor(yMin))); j < std::min(ny, static_cast<int32_t>(std::ceil(yMax)) + 1); ++j) {
      const double_t yc = j + 0.5;
      crossings.clear();
      for (size_t k = 0; k < points.size(); ++k) {
        const auto& [xa, ya] = points[k];
        const auto& [xb, yb] = points[(k + 1) % points.size()];
        const double_t ca = cellY(ya), cb = cellY(yb);
        if ((ca <= yc && yc < cb) || (cb <= yc && yc < ca)) crossings.push_back(cellX(xa) + (yc - ca) / (cb - ca) * (cellX(xb) - cellX(xa)));
      }
      std::sort(crossings.begin(), crossings.end());
      for (size_t k = 0; k + 1 < crossings.size(); k += 2) {
        setRect(crossings[k], j, crossings[k + 1], j, kObject);
      }
    }
  };
  // the part of a band (pad coordinates) from (xa, lowA to highA) to (xb, lowB to highB), column by column: in each column from the lowest to
  // the highest edge within it (at its two sides)
  auto markBand = [&](double_t xa, double_t lowA, double_t highA, double_t xb, double_t lowB, double_t highB) {
    const double_t ca = cellX(xa), cb = cellX(xb);
    if (!std::isfinite(ca) || !std::isfinite(cb)) return;
    const double_t c1 = std::max(std::min(ca, cb), 0.), c2 = std::min(std::max(ca, cb), static_cast<double_t>(nx));
    auto at = [&](double_t c) { return std::clamp((cb == ca) ? 0. : (c - ca) / (cb - ca), 0., 1.); };
    for (double_t c = std::floor(c1); c <= c2; c += 1.) {
      const double_t ta = at(c), tb = at(c + 1.);
      const double_t bottom = std::min(lowA + ta * (lowB - lowA), lowA + tb * (lowB - lowA));
      const double_t top = std::max(highA + ta * (highB - highA), highA + tb * (highB - highA));
      setRect(c, cellY(bottom), c, cellY(top), kObject);
    }
  };
  // a filled histogram or curve starts at 0, or at the frame edge if 0 is outside of the frame; on a log axis at the lower frame edge
  // (TGraphPainter::PaintGrapHist and Smooth)
  auto fillBaseY = [&]() { return (pad->GetLogy()) ? pad->GetUymin() : std::clamp(0., pad->GetUymin(), pad->GetUymax()); };

  // contour lines of 2d data (CONT, CONT1 to CONT3; CONT0 and CONT4 fill, COL is a map): only the cells a line passes through, i.e. where a
  // level lies between the values at the corners of the cell (widened by half a cell for the width of the lines); the levels are those of
  // the painted histogram (that of a 2d function is made when it is painted); without levels the whole range counts as before
  auto isLineContour = [](TObject* obj) {
    TString option = obj->GetDrawOption();
    option.ToUpper();
    return option.Contains("CONT") && !option.Contains("CONT0") && !option.Contains("CONT4") && !option.Contains("COL");
  };
  auto markContourLines = [&](TH1* levelsFrom, const std::function<double_t(double_t, double_t)>& valueAt, double_t xMin, double_t yMin, double_t xMax, double_t yMax) {
    const int32_t nLevels = (levelsFrom) ? levelsFrom->GetContour() : 0;
    if (nLevels <= 0) return false;
    const int32_t i1 = std::max(0, static_cast<int32_t>(std::floor(cellX(padX(xMin)))));
    const int32_t i2 = std::min(nx - 1, static_cast<int32_t>(std::floor(cellX(padX(xMax)))));
    const int32_t j1 = std::max(0, static_cast<int32_t>(std::floor(cellY(padY(yMin)))));
    const int32_t j2 = std::min(ny - 1, static_cast<int32_t>(std::floor(cellY(padY(yMax)))));
    if (i1 > i2 || j1 > j2) return true;
    vector<double_t> levels(nLevels);
    for (int32_t k = 0; k < nLevels; ++k)
      levels[k] = levelsFrom->GetContourLevel(k);
    std::sort(levels.begin(), levels.end());
    // the values on the corners of half cells, from which each widened cell takes its 4 x 4 corners
    const auto user = [](double_t padValue, bool isLog) { return (isLog) ? std::pow(10., padValue) : padValue; };
    const int32_t nCornersX = 2 * (i2 - i1 + 1) + 3;
    const int32_t nCornersY = 2 * (j2 - j1 + 1) + 3;
    vector<double_t> corners(static_cast<size_t>(nCornersX) * nCornersY);
    for (int32_t a = 0; a < nCornersX; ++a) {
      const double_t x = std::clamp(user(pad->GetX1() + (i1 - 0.5 + 0.5 * a) / cellsPerX, pad->GetLogx()), xMin, xMax);
      for (int32_t b = 0; b < nCornersY; ++b) {
        const double_t y = std::clamp(user(pad->GetY1() + (j1 - 0.5 + 0.5 * b) / cellsPerY, pad->GetLogy()), yMin, yMax);
        corners[a + static_cast<size_t>(b) * nCornersX] = valueAt(x, y);
      }
    }
    for (int32_t i = i1; i <= i2; ++i) {
      for (int32_t j = j1; j <= j2; ++j) {
        double_t low = std::numeric_limits<double_t>::infinity();
        double_t high = -low;
        for (int32_t a = 2 * (i - i1); a <= 2 * (i - i1) + 3; ++a) {
          for (int32_t b = 2 * (j - j1); b <= 2 * (j - j1) + 3; ++b) {
            const double_t value = corners[a + static_cast<size_t>(b) * nCornersX];
            if (!std::isfinite(value)) continue;
            low = std::min(low, value);
            high = std::max(high, value);
          }
        }
        if (!(low < high)) continue;
        const auto level = std::upper_bound(levels.begin(), levels.end(), low);  // a level above the lowest corner and not above the highest
        if (level != levels.end() && *level <= high) setRect(i, j, i, j, kMap);
      }
    }
    return true;
  };

  auto markHist = [&](TH1* hist) {
    if (hist->GetDimension() == 2 && !pad->GetView()) {
      if (!dynamic_cast<TH2Poly*>(hist) && isLineContour(hist)) {
        TAxis* xAxis = hist->GetXaxis();
        TAxis* yAxis = hist->GetYaxis();
        auto* map = static_cast<TH2*>(hist);
        // between the outer bin centres, where ROOT draws the contours of a histogram (Interpolate refuses points beyond them)
        const double_t xLow = xAxis->GetBinCenter(xAxis->GetFirst()), xHigh = xAxis->GetBinCenter(xAxis->GetLast());
        const double_t yLow = yAxis->GetBinCenter(yAxis->GetFirst()), yHigh = yAxis->GetBinCenter(yAxis->GetLast());
        const auto value = [map, xLow, xHigh, yLow, yHigh](double_t x, double_t y) { return map->Interpolate(std::clamp(x, xLow, xHigh), std::clamp(y, yLow, yHigh)); };
        if (markContourLines(hist, value, xAxis->GetBinLowEdge(xAxis->GetFirst()), yAxis->GetBinLowEdge(yAxis->GetFirst()), xAxis->GetBinUpEdge(xAxis->GetLast()), yAxis->GetBinUpEdge(yAxis->GetLast()))) return;
      }
      // maps drawn in the frame (col, box, text, ...): the filled bins of the visible range; ROOT leaves empty bins blank
      if (auto* poly = dynamic_cast<TH2Poly*>(hist)) {
        for (auto* obj : *poly->GetBins()) {
          auto* bin = static_cast<TH2PolyBin*>(obj);
          if (bin->GetContent() != 0.) setRect(cellX(padX(bin->GetXMin())), cellY(padY(bin->GetYMin())), cellX(padX(bin->GetXMax())), cellY(padY(bin->GetYMax())), kMap);
        }
        return;
      }
      TAxis* xAxis = hist->GetXaxis();
      TAxis* yAxis = hist->GetYaxis();
      for (int32_t binX = xAxis->GetFirst(); binX <= xAxis->GetLast(); ++binX) {
        for (int32_t binY = yAxis->GetFirst(); binY <= yAxis->GetLast(); ++binY) {
          if (hist->GetBinContent(binX, binY) == 0.) continue;
          setRect(cellX(padX(xAxis->GetBinLowEdge(binX))), cellY(padY(yAxis->GetBinLowEdge(binY))), cellX(padX(xAxis->GetBinUpEdge(binX))), cellY(padY(yAxis->GetBinUpEdge(binY))), kMap);
        }
      }
      return;
    }
    if (hist->GetDimension() != 1) return;
    // error bars as ROOT draws them (THistPainter::MakeChopt): with an E in the option, also after HIST, or with stored errors unless HIST
    // is given; SAME, TEXT etc. contain an E but are no error option
    TString option = hist->GetDrawOption();
    option.ToUpper();
    for (const char* ignored : kNonSymbolTokens)
      option.ReplaceAll(ignored, "");
    TAxis* axis = hist->GetXaxis();
    // bar charts (THistPainter::PaintBar and PaintBarH): boxes from the frame edge to the content, also without fill style
    if (const bool isHorizontal = option.Contains("HBAR"); isHorizontal || option.Contains("BAR")) {
      for (int32_t bin = axis->GetFirst(); bin <= axis->GetLast(); ++bin) {
        double_t low = (isHorizontal) ? padY(axis->GetBinLowEdge(bin)) : padX(axis->GetBinLowEdge(bin));
        const double_t up = (isHorizontal) ? padY(axis->GetBinUpEdge(bin)) : padX(axis->GetBinUpEdge(bin));
        const double_t width = (up - low) * hist->GetBarWidth();
        low += hist->GetBarOffset() * (up - low);
        if (isHorizontal) {
          setRect(cellX(pad->GetUxmin()), cellY(low), cellX(padX(hist->GetBinContent(bin))), cellY(low + width), kObject);
        } else {
          setRect(cellX(low), cellY(pad->GetUymin()), cellX(low + width), cellY(padY(hist->GetBinContent(bin))), kObject);
        }
      }
      return;
    }
    // an error option without any error to draw (all zero, as ROOT computes them: the square root of the content without Sumw2): ROOT
    // draws the steps instead (or the markers)
    auto anyError = [hist, axis]() {
      for (int32_t bin = 1; bin <= axis->GetNbins(); ++bin) {
        if (hist->GetBinError(bin) != 0.) return true;
      }
      return false;
    };
    const bool hasErrors = (option.Contains("E") || (!option.Contains("HIST") && HasErrors(hist))) && anyError();
    const bool hasErrorBand = hasErrors && (option.Contains("E3") || option.Contains("E4") || option.Contains("E5") || option.Contains("E6"));  // through the bin centres
    const bool hasErrorArea = hasErrorBand || (hasErrors && option.Contains("E2"));
    const bool hasLine = !hasErrors && (option.Contains("L") || option.Contains("C"));  // a line or curve through the bin centres instead of steps
    const bool hasMarkers = option.Contains("P") || option.Contains("*");
    // steps without markers, line or curve: markers (P, also with HIST, or E P without errors) are drawn alone, as in the legend rows
    const bool hasSteps = !hasErrors && !hasLine && !hasMarkers;
    const bool showEmpty = option.Contains("E0");
    const float_t markerSize = (hasMarkers || (hasErrors && !hasErrorArea)) ? hist->GetMarkerSize() : 0.f;
    // the fill of a histogram drawn as steps, line or curve (THistPainter::PaintHist: a fill colour and style are needed):
    // steps are filled per bin, a line with option F2 down to the lower frame edge, a smoothed curve down to the base of the fill
    if ((hasSteps || hasLine) && hist->GetFillColor() != 0 && hist->GetFillStyle() != 0) {
      const bool isCurve = option.Contains("C");
      const bool isLineArea = option.Contains("F2");
      if (isCurve || isLineArea) {
        const double_t base = (isLineArea) ? pad->GetUymin() : fillBaseY();
        vector<std::pair<double_t, double_t>> polygon;
        for (int32_t bin = axis->GetFirst(); bin <= axis->GetLast(); ++bin) {
          polygon.emplace_back(padX(axis->GetBinCenter(bin)), padY(hist->GetBinContent(bin)));
        }
        if (!polygon.empty()) {
          polygon.emplace_back(polygon.back().first, base);
          polygon.emplace_back(polygon.front().first, base);
        }
        markPolygon(polygon);
      } else {
        const double_t base = fillBaseY();
        for (int32_t bin = axis->GetFirst(); bin <= axis->GetLast(); ++bin) {
          setRect(cellX(padX(axis->GetBinLowEdge(bin))), cellY(base), cellX(padX(axis->GetBinUpEdge(bin))), cellY(padY(hist->GetBinContent(bin))), kObject);
        }
      }
    }
    // the last bin drawn, for lines and bands through the bin centres
    int32_t previous = 0;
    for (int32_t bin = axis->GetFirst(); bin <= axis->GetLast(); ++bin) {  // only the visible range
      const double_t content = hist->GetBinContent(bin);
      if (hasErrors && !showEmpty && content == 0.) continue;  // not drawn by ROOT
      const double_t xLow = padX(axis->GetBinLowEdge(bin));
      const double_t xUp = padX(axis->GetBinUpEdge(bin));
      const double_t xCenter = padX(axis->GetBinCenter(bin));
      const double_t y = padY(content);
      const double_t low = padY(content - hist->GetBinErrorLow(bin)), high = padY(content + hist->GetBinErrorUp(bin));
      if (hasLine) {
        if (previous) markLine(padX(axis->GetBinCenter(previous)), padY(hist->GetBinContent(previous)), xCenter, y);
      } else if (hasSteps || hasErrors) {
        markLine(xLow, y, xUp, y);  // the step, or the error bar in x
      }
      markPoint(xCenter, y, markerSize);
      if (hasErrorBand && previous) {
        const double_t before = hist->GetBinContent(previous);
        markBand(padX(axis->GetBinCenter(previous)), padY(before - hist->GetBinErrorLow(previous)), padY(before + hist->GetBinErrorUp(previous)), xCenter, low, high);
      }
      previous = bin;
      if (hasErrorBand) {
        setRect(cellX(xCenter), cellY(low), cellX(xCenter), cellY(high), kObject);
      } else if (hasErrorArea) {
        setRect(cellX(xLow), cellY(low), cellX(xUp), cellY(high), kObject);
      } else if (hasErrors) {
        markLine(xCenter, low, xCenter, high);
      } else if (hasSteps && bin < axis->GetLast()) {
        markLine(xUp, y, xUp, padY(hist->GetBinContent(bin + 1)));  // step to the next bin
      }
    }
  };
  auto markGraph = [&](TGraph* graph, const TObject* drawn = nullptr) {  // drawn: the object in the pad whose drawing option applies, if not the graph
    TString option = (drawn ? drawn : graph)->GetDrawOption();
    option.ToLower();
    for (const char* ignored : {"same", "pfc", "plc", "pmc"})
      option.ReplaceAll(ignored, "");
    const bool hasErrorBoxes = option.Contains("2") || option.Contains("5");
    const bool hasErrorBand = option.Contains("3") || option.Contains("4");
    // lines between consecutive points only with L or C, or without any option (TGraphPainter::PaintGraph): markers alone leave the space between them free
    const bool hasLine = option.Contains("l") || option.Contains("c") || option.Strip(TString::kBoth).IsNull();
    const float_t markerSize = (option.Contains("p") || option.Contains("*")) ? graph->GetMarkerSize() : 0.f;
    const int32_t n = graph->GetN();
    // the errors of point i (a graph without errors gives -1)
    auto errorXlow = [graph](int32_t i) { return std::max(graph->GetErrorXlow(i), 0.); };
    auto errorXhigh = [graph](int32_t i) { return std::max(graph->GetErrorXhigh(i), 0.); };
    // of a graph with several sets of y errors the largest (each set is drawn)
    auto* multi = dynamic_cast<TGraphMultiErrors*>(graph);
    auto errorYlow = [graph, multi](int32_t i) {
      double_t error = std::max(graph->GetErrorYlow(i), 0.);
      for (int32_t set = 0; multi && set < multi->GetNYErrors(); ++set)
        error = std::max(error, multi->GetErrorYlow(i, set));
      return error;
    };
    auto errorYhigh = [graph, multi](int32_t i) {
      double_t error = std::max(graph->GetErrorYhigh(i), 0.);
      for (int32_t set = 0; multi && set < multi->GetNYErrors(); ++set)
        error = std::max(error, multi->GetErrorYhigh(i, set));
      return error;
    };
    if (n > 0 && option.Contains("b")) {
      // bar chart (TGraphPainter::PaintGraph): boxes of a fraction of the mean point distance around each point, from the base of a fill
      const auto [xMin, xMax] = std::minmax_element(graph->GetX(), graph->GetX() + n);
      const double_t halfWidth = 0.5 * (*xMax - *xMin) / n * gStyle->GetBarWidth();
      for (int32_t i = 0; i < n; ++i) {
        setRect(cellX(padX(graph->GetPointX(i) - halfWidth)), cellY(fillBaseY()), cellX(padX(graph->GetPointX(i) + halfWidth)), cellY(padY(graph->GetPointY(i))), kObject);
      }
    }
    if (option.Contains("f") && graph->GetFillColor() != 0 && graph->GetFillStyle() != 0) {
      // filled area: the polygon through the points (closed from the last point back to the first)
      vector<std::pair<double_t, double_t>> polygon;
      polygon.reserve(n);
      for (int32_t i = 0; i < n; ++i) {
        polygon.emplace_back(padX(graph->GetPointX(i)), padY(graph->GetPointY(i)));
      }
      markPolygon(polygon);
    }
    for (int32_t i = 0; i < n; ++i) {
      const double_t xUser = graph->GetPointX(i), yUser = graph->GetPointY(i);
      const double_t x = padX(xUser), y = padY(yUser);
      markPoint(x, y, markerSize);
      if (hasLine && i > 0) markLine(padX(graph->GetPointX(i - 1)), padY(graph->GetPointY(i - 1)), x, y);
      const double_t exLow = errorXlow(i), exHigh = errorXhigh(i);
      const double_t eyLow = errorYlow(i), eyHigh = errorYhigh(i);
      if (hasErrorBoxes) {
        setRect(cellX(padX(xUser - exLow)), cellY(padY(yUser - eyLow)), cellX(padX(xUser + exHigh)), cellY(padY(yUser + eyHigh)), kObject);
        continue;
      }
      if (exLow > 0. || exHigh > 0.) markLine(padX(xUser - exLow), y, padX(xUser + exHigh), y);
      if (eyLow > 0. || eyHigh > 0.) markLine(x, padY(yUser - eyLow), x, padY(yUser + eyHigh));
      if (hasErrorBand && i > 0) {
        // the band from the previous point to this one
        const double_t yPrev = graph->GetPointY(i - 1);
        markBand(padX(graph->GetPointX(i - 1)), padY(yPrev - errorYlow(i - 1)), padY(yPrev + errorYhigh(i - 1)), x, padY(yUser - eyLow), padY(yUser + eyHigh));
      }
    }
  };
  auto markFunc = [&](TF1* func) {
    if (func->GetNdim() == 2 && !pad->GetView()) {
      // a 2d function drawn into the frame (colours, contours): a map over its range, contour lines where they pass
      double_t xMin{}, yMin{}, xMax{}, yMax{};
      func->GetRange(xMin, yMin, xMax, yMax);
      const auto value = [func](double_t x, double_t y) { return func->Eval(x, y); };
      if (isLineContour(func) && markContourLines(func->GetHistogram(), value, xMin, yMin, xMax, yMax)) return;
      setRect(cellX(padX(xMin)), cellY(padY(yMin)), cellX(padX(xMax)), cellY(padY(yMax)), kMap);
      return;
    }
    if (func->GetNdim() != 1) return;
    double_t xMin{}, xMax{};
    func->GetRange(xMin, xMax);
    const int32_t nPoints = std::max(func->GetNpx(), 2);
    // with a fill colour and style ROOT fills the curve down to the base of a fill (as a histogram drawn as curve)
    const bool isFilled = func->GetFillColor() != 0 && func->GetFillStyle() != 0;
    vector<std::pair<double_t, double_t>> polygon;
    double_t xPrev{}, yPrev{};
    for (int32_t i = 0; i < nPoints; ++i) {
      const double_t xUser = xMin + (xMax - xMin) * i / (nPoints - 1);
      const double_t x = padX(xUser);
      const double_t y = padY(func->Eval(xUser));
      if (i > 0) markLine(xPrev, yPrev, x, y);
      if (isFilled) polygon.emplace_back(x, y);
      xPrev = x;
      yPrev = y;
    }
    if (isFilled) {
      polygon.emplace_back(padX(xMax), fillBaseY());
      polygon.emplace_back(padX(xMin), fillBaseY());
      markPolygon(polygon);
    }
  };

  for (auto* obj : *pad->GetListOfPrimitives()) {
    if (obj->InheritsFrom(TFrame::Class())) {
      // only the area within the frame is considered anyway
      continue;
    } else if (auto* pave = dynamic_cast<TPave*>(obj)) {
      // legends and texts placed before (coordinates in NDC)
      setRect(pave->GetX1NDC() * nx, pave->GetY1NDC() * ny, pave->GetX2NDC() * nx, pave->GetY2NDC() * ny, kBox);
    } else if (auto* box = dynamic_cast<TBox*>(obj)) {  // regions (shapes), in axis values like lines and ellipses
      setRect(cellX(padX(box->GetX1())), cellY(padY(box->GetY1())), cellX(padX(box->GetX2())), cellY(padY(box->GetY2())), kObject);
    } else if (auto* line = dynamic_cast<TLine*>(obj)) {
      if (!line->TestBit(TLine::kLineNDC)) markLine(padX(line->GetX1()), padY(line->GetY1()), padX(line->GetX2()), padY(line->GetY2()));  // lines and arrows
    } else if (auto* polyline = dynamic_cast<TPolyLine*>(obj)) {                                                                          // polygons (shapes): the outline, and with a fill the inside
      vector<std::pair<double_t, double_t>> points;
      for (int32_t i = 0; i < polyline->GetN(); ++i) {
        points.emplace_back(padX(polyline->GetX()[i]), padY(polyline->GetY()[i]));
      }
      for (size_t i = 0; i + 1 < points.size(); ++i) {
        markLine(points[i].first, points[i].second, points[i + 1].first, points[i + 1].second);
      }
      if (polyline->GetFillStyle() != 0 && TString(polyline->GetOption()).Contains("f")) markPolygon(points);
    } else if (auto* ellipse = dynamic_cast<TEllipse*>(obj)) {
      // the outline, and with a fill the inside (as a polygon of the drawn curve)
      vector<std::pair<double_t, double_t>> points;
      const double_t phiMin = ellipse->GetPhimin(), phiMax = ellipse->GetPhimax(), theta = ellipse->GetTheta() * TMath::DegToRad();
      for (int32_t i = 0; i <= 72; ++i) {
        const double_t phi = (phiMin + (phiMax - phiMin) * i / 72.) * TMath::DegToRad();
        const double_t dx = ellipse->GetR1() * std::cos(phi), dy = ellipse->GetR2() * std::sin(phi);
        points.emplace_back(padX(ellipse->GetX1()) + dx * std::cos(theta) - dy * std::sin(theta), padY(ellipse->GetY1()) + dx * std::sin(theta) + dy * std::cos(theta));
      }
      for (size_t i = 0; i + 1 < points.size(); ++i) {
        markLine(points[i].first, points[i].second, points[i + 1].first, points[i + 1].second);
      }
      if (ellipse->GetFillStyle() != 0) markPolygon(points);
    } else if (auto* subPad = dynamic_cast<TPad*>(obj); subPad && TString(subPad->GetName()).BeginsWith("ImageBox_")) {
      // images placed before (position in NDC of this pad)
      setRect(subPad->GetXlowNDC() * nx, subPad->GetYlowNDC() * ny, (subPad->GetXlowNDC() + subPad->GetWNDC()) * nx, (subPad->GetYlowNDC() + subPad->GetHNDC()) * ny, kBox);
    } else if (auto* hist = dynamic_cast<TH1*>(obj)) {
      if (!TString(hist->GetName()).BeginsWith("axis_hist") && !TString(hist->GetName()).Contains("hframe")) markHist(hist);
    } else if (auto* grid = dynamic_cast<TGraphPolargram*>(obj)) {
      // the polar grid (TGraphPolargram::PaintPolarDivisions): a disc of radius 1 around the origin of the pad, and the label of each major
      // division of the angle at radius 1 + offset, aligned so that it reaches away from the disc (FindAlign); the labels are made and
      // measured as ROOT writes them (fractions of pi in radian mode, else the angle with %5.3g, own labels set with SetPolarLabel unknown)
      vector<std::pair<double_t, double_t>> disc;
      for (int32_t i = 0; i < 72; ++i) {
        const double_t phi = TMath::TwoPi() * i / 72.;
        disc.emplace_back(std::cos(phi), std::sin(phi));
      }
      markPolygon(disc);
      const double_t userPerPixelX = (pad->GetX2() - pad->GetX1()) / padWidthPixel;
      const double_t userPerPixelY = (pad->GetY2() - pad->GetY1()) / padHeightPixel;
      const bool ortho = grid->TestBit(TGraphPolargram::kLabelOrtho);
      auto markLabel = [&](double_t x1, double_t y1, double_t x2, double_t y2) { setRect(cellX(padX(x1)), cellY(padY(y1)), cellX(padX(x2)), cellY(padY(y2)), kObject); };
      TLatex measure;
      measure.SetTextFont(grid->GetPolarLabelFont());
      measure.SetTextSize(grid->GetPolarLabelSize());
      TIter link(pad->GetListOfPrimitives());  // the option of the grid in the pad: N leaves out the labels
      while (link() != grid) {
      }
      const int32_t nLabels = TString(link.GetOption()).Contains("n", TString::kIgnoreCase) ? 0 : grid->GetNdivPolar() % 100;
      for (int32_t i = 0; i < nLabels; ++i) {
        const double_t theta = i * 2 * TMath::Pi() / nLabels;  // as ROOT computes it: the alignment compares it with pi/2, pi and 3 pi/2
        string label;
        if (grid->IsRadian()) {
          const int32_t divisor = std::gcd(2 * i, nLabels);
          const int32_t num = 2 * i / divisor, den = nLabels / divisor;
          if (num == 0) {
            label = "0";
          } else if (den == 1) {
            label = (num == 1) ? "#pi" : std::to_string(num) + "#pi";
          } else {
            label = "#frac{" + ((num == 1) ? string{} : std::to_string(num)) + "#pi}{" + std::to_string(den) + "}";
          }
        } else {
          label = fmt::format("{:.3g}", grid->GetTMin() + i * (grid->GetTMax() - grid->GetTMin()) / nLabels);
        }
        measure.SetTitle(label.data());
        UInt_t wPixel{}, hPixel{};
        measure.GetBoundingBox(wPixel, hPixel);
        const double_t w = wPixel * userPerPixelX, h = hPixel * userPerPixelY;
        double_t x = (1. + grid->GetPolarOffset()) * std::cos(theta);
        double_t y = (1. + grid->GetPolarOffset()) * std::sin(theta);
        double_t x1{}, y1{};
        if (ortho) {
          // turned along the radius: a square of the label length around its middle
          const double_t length = std::max(wPixel, hPixel);
          x += 0.5 * length * userPerPixelX * std::cos(theta);
          y += 0.5 * length * userPerPixelY * std::sin(theta);
          x1 = x - 0.5 * length * userPerPixelX;
          y1 = y - 0.5 * length * userPerPixelY;
          markLabel(x1, y1, x1 + length * userPerPixelX, y1 + length * userPerPixelY);
          continue;
        }
        y += (theta >= 3 * TMath::Pi() / 12. && theta < 2 * TMath::Pi() / 3.) ? 0.04 : 0.01;  // ROOT lifts the labels a bit
        // TGraphPolargram::FindAlign (private): horizontal 1 left, 2 centred, 3 right; vertical 1 bottom, 2 centred, 3 top
        const double_t pi = TMath::Pi();
        int32_t align = 12;
        if (theta > 0. && theta < pi / 2) {
          align = 11;
        } else if (theta == pi / 2) {
          align = 21;
        } else if (theta > pi / 2 && theta < pi) {
          align = 31;
        } else if (theta == pi) {
          align = 32;
        } else if (theta > pi && theta < 3 * pi / 2) {
          align = 33;
        } else if (theta == 3 * pi / 2) {
          align = 23;
        } else if (theta > 3 * pi / 2) {
          align = 13;
        }
        x1 = x - 0.5 * (align / 10 - 1) * w;
        y1 = y - 0.5 * (align % 10 - 1) * h;
        markLabel(x1, y1, x1 + w, y1 + h);
      }
    } else if (auto* polar = dynamic_cast<TGraphPolar*>(obj)) {
      // its points in the coordinates of the pad, as painting placed them on the polar grid
      TGraph painted(polar->GetN(), polar->GetXpol(), polar->GetYpol());
      CopyStyle(*polar, painted);
      markGraph(&painted, polar);
    } else if (auto* graph = dynamic_cast<TGraph*>(obj)) {
      markGraph(graph);
    } else if (auto* graph2d = dynamic_cast<TGraph2D*>(obj); graph2d && !pad->GetView() && graph2d->GetN() > 0) {
      // a 2d graph drawn into the frame (colours, contours): a map over the range of its points
      setRect(cellX(padX(graph2d->GetXmin())), cellY(padY(graph2d->GetYmin())), cellX(padX(graph2d->GetXmax())), cellY(padY(graph2d->GetYmax())), kMap);
    } else if (auto* multiGraph = dynamic_cast<TMultiGraph*>(obj)) {
      for (auto* subGraph : *multiGraph->GetListOfGraphs()) {
        markGraph(static_cast<TGraph*>(subGraph));
      }
    } else if (auto* stack = dynamic_cast<THStack*>(obj)) {
      for (auto* subHist : *stack->GetHists()) {
        markHist(static_cast<TH1*>(subHist));
      }
    } else if (auto* func = dynamic_cast<TF1*>(obj)) {
      markFunc(func);
    }
  }

  // summed area table to check a whole rectangle at once (counting the cells blocked by the given kinds)
  vector<int32_t> sum(static_cast<size_t>(nx + 1) * (ny + 1), 0);
  auto fillSum = [&](uint8_t mask) {
    for (int32_t i = 0; i < nx; ++i) {
      for (int32_t j = 0; j < ny; ++j) {
        sum[(i + 1) + static_cast<size_t>(j + 1) * (nx + 1)] = ((blocked[i + static_cast<size_t>(j) * nx] & mask) != 0) + sum[i + static_cast<size_t>(j + 1) * (nx + 1)] + sum[(i + 1) + static_cast<size_t>(j) * (nx + 1)] - sum[i + static_cast<size_t>(j) * (nx + 1)];
      }
    }
  };
  // whether a box at this position (lower left corner in NDC) overlaps with any cell occupied by drawn objects
  auto isBlocked = [&](double_t x, double_t y) {
    const int32_t i1 = std::clamp(static_cast<int32_t>(std::floor(x * nx)), 0, nx);
    const int32_t i2 = std::clamp(static_cast<int32_t>(std::ceil((x + width) * nx)), 0, nx);
    const int32_t j1 = std::clamp(static_cast<int32_t>(std::floor(y * ny)), 0, ny);
    const int32_t j2 = std::clamp(static_cast<int32_t>(std::ceil((y + height) * ny)), 0, ny);
    auto at = [&](int32_t a, int32_t b) { return sum[a + static_cast<size_t>(b) * (nx + 1)]; };
    return (at(i2, j2) - at(i1, j2) - at(i2, j1) + at(i1, j1)) != 0;
  };
  // candidate positions: both edges of the free area and all cell boundaries in between
  if (freeArea[2] - width < freeArea[0] || freeArea[3] - height < freeArea[1]) return false;
  auto candidates = [](double_t min, double_t max, int32_t nCells) {
    vector<double_t> positions{min};
    for (int32_t k = static_cast<int32_t>(std::floor(min * nCells)) + 1; k < max * nCells; ++k) {
      positions.push_back(static_cast<double_t>(k) / nCells);
    }
    if (max > min) positions.push_back(max);
    return positions;
  };
  const auto xPositions = candidates(freeArea[0], freeArea[2] - width, nx);
  const auto yPositions = candidates(freeArea[1], freeArea[3] - height, ny);

  // distance (in pixels) of the box at this position to the given corner of the free area
  auto distanceToCorner = [&](box_placement_t boxCorner, double_t x, double_t y) {
    const bool isLeft = (boxCorner == bottom_left || boxCorner == top_left);
    const bool isBottom = (boxCorner == bottom_left || boxCorner == bottom_right);
    const double_t dx = ((isLeft) ? x - freeArea[0] : freeArea[2] - (x + width)) * padWidthPixel;
    const double_t dy = ((isBottom) ? y - freeArea[1] : freeArea[3] - (y + height)) * padHeightPixel;
    return dx * dx + dy * dy;
  };
  // for best_corner the one the box gets closest to is used (in case of a tie in this order of preference)
  const vector<box_placement_t> corners = (placement == best_corner) ? vector<box_placement_t>{top_left, top_right, bottom_right, bottom_left} : vector<box_placement_t>{placement};
  bool found = false;
  double_t minDistance{};
  size_t minRank{};  // position of the corner in the order of preference
  auto search = [&]() {
    for (const double_t x : xPositions) {
      for (const double_t y : yPositions) {
        if (isBlocked(x, y)) continue;
        for (size_t rank = 0; rank < corners.size(); ++rank) {
          const double_t distance = distanceToCorner(corners[rank], x, y);
          if (found && (distance > minDistance || (distance == minDistance && rank >= minRank))) continue;
          found = true;
          minDistance = distance;
          minRank = rank;
          lowerLeftX = x;
          lowerLeftY = y;
        }
      }
    }
  };
  const bool hasMaps = std::any_of(blocked.begin(), blocked.end(), [](uint8_t cell) { return cell & kMap; });
  fillSum(kObject | kMap | kBox);
  search();
  if (!found && hasMaps) {
    // a map filling the frame: on top of it, avoiding only the other objects
    fillSum(kObject | kBox);
    search();
  }
  if (!found) {
    // no room beside the data: on top of it, avoiding only the other boxes
    fillSum(kBox);
    search();
    coversData = found;
  }
  return found;
}

//**************************************************************************************************
/**
 * Functions to retrieve a copy or projection of the stored data properly casted it to its actual ROOT type.
 */
//**************************************************************************************************
optional<data_ptr_t> PlotPainter::GetDataClone(TObject* obj, const optional<Plot::Pad::Data::proj_info_t>& projInfo, bool keepStoredRanges)
{
  if (obj) {
    bool addDirStatus = TH1::AddDirectoryStatus();
    TH1::AddDirectory(false);
    auto addDirGuard = make_scope_guard([addDirStatus]() { TH1::AddDirectory(addDirStatus); });

    if (projInfo) {
      string name = obj->GetName();
      name += projInfo->GetNameSuffix();
      auto returnPointer = GetProjection(obj, *projInfo);
      if (returnPointer &&
          std::visit([](auto&& ptr) { return ptr != nullptr; }, *returnPointer)) {
        std::visit([&name](auto&& ptr) { ptr->SetName(name.data()); ptr->SetBit(kCanDelete); }, *returnPointer);
        // the axis titles of the input, for the <projection> and <selection> placeholders of the projection
        vector<string> axisTitles;
        if (obj->InheritsFrom(THnBase::Class())) {
          auto* histPtr = static_cast<THnBase*>(obj);
          for (int32_t i = 0; i < histPtr->GetNdimensions(); ++i)
            axisTitles.emplace_back(histPtr->GetAxis(i)->GetTitle());
        } else if (obj->InheritsFrom(TH1::Class())) {
          auto* histPtr = static_cast<TH1*>(obj);
          for (TAxis* axis : {histPtr->GetXaxis(), histPtr->GetYaxis(), histPtr->GetZaxis()})
            axisTitles.emplace_back(axis->GetTitle());
        }
        mProjectionAxisTitles[std::visit([](auto&& ptr) -> const TObject* { return ptr; }, *returnPointer)] = std::move(axisTitles);
        return returnPointer;
      } else {
        logger::error("Projection failed for {}.", obj->GetName());
      }
    } else {
      // TProfile2D is TH2, TH2 is TH1, TH3 is TH1, TProfile is TH1, TF3 is TF2, TF2 is TF1
      if (auto returnPointer = GetDataClone<TProfile2D, TH2, TH3, TProfile, TH1, TGraph2D, TGraph, TF3, TF2, TF1, TEfficiency>(obj)) {
        std::visit([](auto&& ptr) { ptr->SetBit(kCanDelete); }, *returnPointer);
        // axis ranges, minimum and maximum stored with the input: except for a frame, the plot definition alone sets what is drawn
        // (SetRangeX etc.); a discarded range is visible in the plot, since the data is drawn beyond it
        if (!keepStoredRanges) {
          std::visit(
            [](auto* ptr) {
              using T = std::remove_pointer_t<decltype(ptr)>;
              if constexpr (std::is_base_of_v<TH1, T>) {
                ptr->GetXaxis()->SetRange();  // SetRange(1, nBins) would leave kAxisRange set
                if (ptr->GetDimension() > 1) ptr->GetYaxis()->SetRange();
                if (ptr->GetDimension() > 2) ptr->GetZaxis()->SetRange();
                ptr->SetMinimum();
                ptr->SetMaximum();
              } else if constexpr (std::is_base_of_v<TGraph, T>) {
                // the axis histogram of a graph is stored with it once it was drawn or got axis titles (its limits are recomputed when the graph is drawn);
                // made here from points that are not finite (removed later), ROOT reports its infinite axis limits
                if (ptr->GetN() > 0) {
                  auto quiet = ignore_root_messages_below(kFatal);
                  ptr->GetHistogram()->GetXaxis()->SetRange();
                }
                ptr->SetMinimum();
                ptr->SetMaximum();
              } else if constexpr (std::is_base_of_v<TGraph2D, T> || std::is_base_of_v<TF1, T>) {
                ptr->SetMinimum();
                ptr->SetMaximum();
              }
            },
            *returnPointer);
        }
        return returnPointer;
      } else {
        logger::error("Input data {} is of unsupported type {}.", obj->GetName(), obj->ClassName());
      }
    }
  }
  return nullopt;
}

template <typename T>
optional<data_ptr_t> PlotPainter::GetDataClone(TObject* obj)
{
  if (obj && obj->InheritsFrom(T::Class())) {
    if constexpr (std::is_same_v<T, TEfficiency>) {
      auto teff = static_cast<TEfficiency*>(obj);
      int dim = teff->GetDimension();
      // a TEfficiency carries ROOT's fill defaults (colour 19, style 1001; TH1 and TGraph start with colour 0) and copies them to the created object:
      // they are not a fill set by the user, so the default drawing rule must not take them as one (points, not an area, with SetKeepInputStyle())
      // the created object is named by ROOT (eff_graph, eff_histo): it is drawn under the name of the input
      auto dropDefaultFill = [&](auto* created) {
        if (created->GetFillColor() == 19 && created->GetFillStyle() == 1001) created->SetFillStyle(0);
        created->SetName(teff->GetName());
        return created;
      };
      if (dim == 1) {
        return static_cast<TGraph*>(dropDefaultFill(teff->CreateGraph()));
      } else if (dim == 2) {
        return static_cast<TH2*>(dropDefaultFill(teff->CreateHistogram()));
      }
    } else {
      return static_cast<T*>(obj->Clone());
    }
  }
  return nullopt;
}

template <typename T, typename Next, typename... Rest>
optional<data_ptr_t> PlotPainter::GetDataClone(TObject* obj)
{
  if (auto returnPointer = GetDataClone<T>(obj)) return returnPointer;
  return GetDataClone<Next, Rest...>(obj);
}

optional<data_ptr_t> PlotPainter::GetProjection(TObject* obj, Plot::Pad::Data::proj_info_t projInfo)
{
  const bool isProfile = projInfo.isProfile && *projInfo.isProfile;
  // only 1d and 2d histograms are valid outputs! (could be extended to 3d if there is a way to plot this)
  if (projInfo.dims.size() == 0 || projInfo.dims.size() > 2) {
    logger::error("Invalid number of dimensions specified for projection of histogram {}.", obj->GetName());
    return nullopt;
  }
  int32_t nDims{};
  if (obj->InheritsFrom(THnBase::Class())) {
    nDims = static_cast<THnBase*>(obj)->GetNdimensions();
  } else if (obj->InheritsFrom(TH1::Class())) {
    nDims = static_cast<TH1*>(obj)->GetDimension();
  } else {
    logger::error("Cannot do {} for type {} ({}).", (isProfile) ? "profiles" : "projections", obj->ClassName(), obj->GetName());
    return nullopt;
  }
  for (auto dim : projInfo.dims) {
    if (dim >= nDims) {
      logger::error("Cannot project {} onto dimension {}: it only has {}.", obj->GetName(), dim, plural(nDims, "dimension"));
      return nullopt;
    }
  }
  if (projInfo.dims.size() == 2 && projInfo.dims[0] == projInfo.dims[1]) {
    logger::error("Cannot project {} onto the same dimension twice.", obj->GetName());
    return nullopt;
  }
  // the axis a profile averages: the named one, or else the only axis besides the kept ones (TH2 and TH3 profile it themselves below)
  std::optional<uint8_t> averagedAxis = projInfo.averagedAxis;
  const bool isInputProfile = obj->InheritsFrom(TProfile2D::Class()) || obj->InheritsFrom(TProfile3D::Class());
  if (isProfile && averagedAxis && isInputProfile) {
    logger::error("Cannot average axis {} of {}: it is a profile already, Profile(dims) gives the mean of its values.", *averagedAxis, obj->GetName());
    return nullopt;
  }
  if (isProfile && averagedAxis && *averagedAxis >= nDims) {
    logger::error("Cannot average axis {} of {}: it only has {}.", *averagedAxis, obj->GetName(), plural(nDims, "dimension"));
    return nullopt;
  }
  const bool isTHn = obj->InheritsFrom(THnBase::Class());
  if (isProfile && !averagedAxis && isTHn) {
    const int32_t nLeft = nDims - static_cast<int32_t>(projInfo.dims.size());
    if (nLeft != 1) {
      logger::error((nLeft == 0) ? "Cannot profile {} along all of its axes: none is left to average." : "Cannot profile {}: {} axes besides the kept ones are left, name the one to average with Profile(averagedAxis, dims).",
                    obj->GetName(), nLeft);
      return nullopt;
    }
    for (uint8_t axis = 0; !averagedAxis; ++axis) {
      if (std::find(projInfo.dims.begin(), projInfo.dims.end(), axis) == projInfo.dims.end()) averagedAxis = axis;
    }
  }
  // the ranges of the projection are set on the input itself (read once for all plots): its own ranges are restored afterwards
  vector<TAxis*> axes;
  if (isTHn) {
    auto* histPtr = static_cast<THnBase*>(obj);
    for (int32_t i = 0; i < histPtr->GetNdimensions(); ++i)
      axes.push_back(histPtr->GetAxis(i));
  } else {
    auto* histPtr = static_cast<TH1*>(obj);
    for (int16_t i = 0; i < histPtr->GetDimension(); ++i)
      axes.push_back(GetAxis(histPtr, i));
  }
  // a range stored with the input is not applied (as for drawn data, the definition sets the ranges): it is visible in the result
  // for an axis that is kept, but not for one that is integrated out, so that case is reported
  vector<std::pair<int32_t, int32_t>> storedRanges;  // (first, last) bin; (0, 0) for none
  for (size_t i = 0; i < axes.size(); ++i) {
    TAxis* axis = axes[i];
    const bool hasRange = axis->TestBit(TAxis::kAxisRange);
    storedRanges.emplace_back((hasRange) ? std::make_pair(axis->GetFirst(), axis->GetLast()) : std::make_pair(0, 0));
    if (!hasRange) continue;
    const bool isRestricted = axis->GetFirst() > 1 || axis->GetLast() < axis->GetNbins();
    const bool isKept = std::find(projInfo.dims.begin(), projInfo.dims.end(), i) != projInfo.dims.end();
    // ProjectX() etc. without arguments pass (0, -1), ROOT's "all bins": only a range with start <= end is set by the definition
    const bool isSetByDefinition = std::any_of(projInfo.ranges.begin(), projInfo.ranges.end(), [i](const auto& range) { return std::get<0>(range) == i && std::get<1>(range) <= std::get<2>(range); });
    // reported once per plot (the frame of a pad is a second copy of its data)
    if (isRestricted && !isKept && !isSetByDefinition && mReportedStoredRanges.emplace(obj, i).second) {
      logger::warning("The range [{:g}, {:g}] stored on axis {} of {} is not applied to its {}: set it in the plot definition if wanted.", axis->GetBinLowEdge(axis->GetFirst()), axis->GetBinUpEdge(axis->GetLast()), i,
                      obj->GetName(), (isProfile) ? "profile" : "projection");
    }
    axis->SetRange();
  }
  auto rangeGuard = make_scope_guard([&axes, &storedRanges]() {
    for (size_t i = 0; i < axes.size(); ++i) {
      if (storedRanges[i].first == 0 && storedRanges[i].second == 0) {
        axes[i]->SetRange();
      } else {
        axes[i]->SetRange(storedRanges[i].first, storedRanges[i].second);
      }
    }
  });
  // in user coordinates the bins are selected as in TAxis::SetRangeUser: a bin is included if the range overlaps it,
  // so an end on a bin edge does not include the neighbouring bin (FindFixBin, since FindBin would extend an extendable axis);
  // "on a bin edge" with a tolerance, since ROOT computes the edges of fixed bins (e.g. 3 * 0.1 = 0.30000000000000004)
  const bool isUserCoord = projInfo.isUserCoord && *projInfo.isUserCoord;
  // bin numbers follow ROOT (e.g. 0, -1 for the whole axis); a range in axis units outside of the axis or reversed would make ROOT
  // reset it to the whole axis: the data is then left out
  auto setRange = [isUserCoord, obj](TAxis* axis, double_t start, double_t end) {
    if (!isUserCoord) {
      axis->SetRange(static_cast<int32_t>(start), static_cast<int32_t>(end));
      return true;
    }
    constexpr double_t relTol = 1e-9;
    int32_t first = axis->FindFixBin(start);
    int32_t last = axis->FindFixBin(end);
    if (axis->GetBinUpEdge(first) - relTol * axis->GetBinWidth(first) <= start) ++first;
    if (axis->GetBinLowEdge(last) + relTol * axis->GetBinWidth(last) >= end && last > first) --last;  // a range of zero width keeps the bin containing it
    if (first > last || last < 1 || first > axis->GetNbins()) {
      // the projection fails, and with it the plot (as for missing data)
      logger::error("The projection range [{}, {}] of {} selects no bins of an axis with {} bins from {} to {}.", start, end, obj->GetName(),
                    axis->GetNbins(), axis->GetXmin(), axis->GetXmax());
      return false;
    }
    axis->SetRange(first, last);
    return true;
  };

  // TProfile2D::ProfileX/ProfileY and TProfile3D::Project3DProfile give a result without axis titles (ROOT bug 67): those of the input
  auto withTitles = [&axes, &projInfo](auto* result) {
    result->GetXaxis()->SetTitle(axes[projInfo.dims[0]]->GetTitle());
    if (projInfo.dims.size() == 2) result->GetYaxis()->SetTitle(axes[projInfo.dims[1]]->GetTitle());
    return result;
  };
  // a profile ROOT cannot make directly (none of a THn, none of a TH3 onto one axis): the projection onto the kept axes and the averaged one
  // (last, without its range), profiled as a TH2 or TH3 with the range of the averaged axis, so flows and ranges are those of TH2 and TH3
  if (isProfile && averagedAxis && (isTHn || (obj->InheritsFrom(TH3::Class()) && projInfo.dims.size() == 1))) {
    vector<std::tuple<uint8_t, double_t, double_t>> averagedRanges;
    for (const auto& rangeTuple : projInfo.ranges) {
      const uint8_t rangeDim = std::get<0>(rangeTuple);
      if (rangeDim >= nDims) {
        logger::error("Invalid dimension specified for setting ranges of histogram {}.", obj->GetName());
        return nullopt;
      }
      if (rangeDim == *averagedAxis) {
        averagedRanges.push_back(rangeTuple);
      } else if (!setRange(axes[rangeDim], std::get<1>(rangeTuple), std::get<2>(rangeTuple))) {
        return nullopt;
      }
    }
    std::unique_ptr<TH1> projected;
    if (isTHn) {
      auto* histPtr = static_cast<THnBase*>(obj);
      if (projInfo.dims.size() == 1) {
        projected.reset(histPtr->Projection(*averagedAxis, projInfo.dims[0]));  // ROOT's order: y first
      } else {
        projected.reset(histPtr->Projection(projInfo.dims[0], projInfo.dims[1], *averagedAxis));
      }
    } else {
      projected.reset(static_cast<TH3*>(obj)->Project3D((GetAxisStr(*averagedAxis) + GetAxisStr(projInfo.dims[0])).data()));
    }
    projected->SetDirectory(nullptr);
    TAxis* averaged = GetAxis(projected.get(), static_cast<int16_t>(projInfo.dims.size()));
    for (const auto& rangeTuple : averagedRanges) {
      if (!setRange(averaged, std::get<1>(rangeTuple), std::get<2>(rangeTuple))) return nullopt;
    }
    if (projInfo.dims.size() == 1) return static_cast<TH2*>(projected.get())->ProfileX("_px");
    return static_cast<TH3*>(projected.get())->Project3DProfile("yx");
  }

  if (isTHn && !isProfile) {
    THnBase* histPtr = static_cast<THnBase*>(obj);
    for (const auto& rangeTuple : projInfo.ranges) {
      int32_t rangeDim = std::get<0>(rangeTuple);
      if (rangeDim >= histPtr->GetNdimensions()) {
        logger::error("Invalid dimension specified for setting ranges of histogram {}.", obj->GetName());
        return nullopt;
      }
      if (!setRange(histPtr->GetAxis(rangeDim), std::get<1>(rangeTuple), std::get<2>(rangeTuple))) return nullopt;
    }
    if (projInfo.dims.size() == 2) {
      return histPtr->Projection(projInfo.dims[1], projInfo.dims[0]);
    } else if (projInfo.dims.size() == 1) {
      return histPtr->Projection(projInfo.dims[0]);
    }
  } else if (obj->InheritsFrom(TH3::Class())) {
    TH3* histPtr = static_cast<TH3*>(obj);
    for (const auto& rangeTuple : projInfo.ranges) {
      int32_t rangeDim = std::get<0>(rangeTuple);
      if (rangeDim >= 3) {
        logger::error("Invalid dimension specified for setting ranges of histogram {}.", obj->GetName());
        return nullopt;
      }
      if (!setRange(GetAxis(histPtr, rangeDim), std::get<1>(rangeTuple), std::get<2>(rangeTuple))) return nullopt;
    }
    if (auto* profile = dynamic_cast<TProfile3D*>(histPtr)) {
      // a projection of a profile is the profile of the merged bins (TH3::Project3D would add up their means); Profile alike
      const string target = GetAxisStr(projInfo.dims[0]);
      if (projInfo.dims.size() == 2) return withTitles(profile->Project3DProfile((GetAxisStr(projInfo.dims[1]) + target).data()));
      const string other = (projInfo.dims[0] == 0) ? "y" : "x";
      std::unique_ptr<TProfile2D> merged(profile->Project3DProfile((other + target).data()));  // target horizontal
      merged->SetDirectory(nullptr);
      return withTitles(merged->ProfileX((string(profile->GetName()) + "_p" + target).data()));
    }
    if (projInfo.dims.size() == 2) {
      // get string if it is "xy" or "yx" or "zx"...
      if (isProfile) {
        return histPtr->Project3DProfile((GetAxisStr(projInfo.dims[1]) + GetAxisStr(projInfo.dims[0])).data());
      } else {
        return static_cast<TH2*>(histPtr->Project3D((GetAxisStr(projInfo.dims[1]) + GetAxisStr(projInfo.dims[0])).data()));
      }
    } else if (projInfo.dims.size() == 1) {
      if (isProfile) {  // Project3D would give the counts; the mean of which of the other two axes is not defined
        logger::error("Cannot profile 3d histogram {} onto one axis without the axis to average: name it with Profile(averagedAxis, dims), or give two axes (the mean of the third).", obj->GetName());
        return nullopt;
      }
      return histPtr->Project3D(GetAxisStr(projInfo.dims[0]).data());
    }
  } else if (obj->InheritsFrom(TH2Poly::Class())) {
    // its bins are polygons, not cells along the axes: the projections of TH2 would give its empty cell array
    logger::error("Cannot do {} of TH2Poly {}: its bins are polygons, not rows and columns along the axes.", (isProfile) ? "profiles" : "projections", obj->GetName());
    return nullopt;
  } else if (obj->InheritsFrom(TH2::Class())) {
    TH2* histPtr = static_cast<TH2*>(obj);
    if (projInfo.dims.size() > 1) {
      logger::error("Invalid dimension specified for projecting histogram {}.", obj->GetName());
      return nullopt;
    }
    for (const auto& rangeTuple : projInfo.ranges) {
      int32_t rangeDim = std::get<0>(rangeTuple);
      if (rangeDim >= 2) {
        logger::error("Invalid dimension specified for setting ranges of histogram {}.", obj->GetName());
        return nullopt;
      }
      if (!setRange(GetAxis(histPtr, rangeDim), std::get<1>(rangeTuple), std::get<2>(rangeTuple))) return nullopt;
    }
    if (auto* profile = dynamic_cast<TProfile2D*>(histPtr)) {
      // a projection of a profile is the profile of the merged bins (TH2::ProjectionX would add up their means); Profile alike
      if (projInfo.dims[0] == 0) return withTitles(profile->ProfileX("_px"));
      if (projInfo.dims[0] == 1) return withTitles(profile->ProfileY("_py"));
    }
    if (projInfo.dims[0] == 0) {
      if (isProfile) {
        return histPtr->ProfileX("_px");
      } else {
        return histPtr->ProjectionX("_px");
      }
    } else if (projInfo.dims[0] == 1) {
      if (isProfile) {
        return histPtr->ProfileY("_py");
      } else {
        return histPtr->ProjectionY("_py");
      }
    } else {
      logger::error("Invalid dimension specified for {} from {} ({}).", (isProfile) ? "profile" : "projection", obj->GetName(), obj->ClassName());
    }
  } else {
    logger::error("Cannot do {} for type {} ({}).", (isProfile) ? "profiles" : "projections", obj->ClassName(), obj->GetName());
  }
  return nullopt;
}

template <typename T>
TAxis* PlotPainter::GetAxis(T* histPtr, int16_t i)
{
  switch (i) {
    case 0:
      return histPtr->GetXaxis();
    case 1:
      return histPtr->GetYaxis();
    case 2:
      return histPtr->GetZaxis();
    default:
      return nullptr;
  }
}

string PlotPainter::GetAxisStr(int16_t i)
{
  switch (i) {
    case 0:
      return "x";
    case 1:
      return "y";
    case 2:
      return "z";
    default:
      return "";
  }
}

//**************************************************************************************************
/**
 * Helper-functions for (interpolated) division of various root data types.
 * Return false if the division is not possible.
 */
//**************************************************************************************************
bool PlotPainter::Divide(TGraph* numerator, TGraph* denominator, bool binomialErrors, bool verbose, const TH1* denominatorBins)
{
  // the empty bins of a histogram denominator (no content and no error, also those that held NaN) have no value: interpolated, they are
  // no knots (a knot at zero made the ratio swing around it), and the points of the numerator inside them are left out
  vector<bool> inEmptyBin;
  if (denominatorBins) {
    const TAxis* axis = denominatorBins->GetXaxis();
    auto isEmpty = [&](int32_t bin) { return bin >= 1 && bin <= axis->GetNbins() && !denominatorBins->GetBinContent(bin) && !denominatorBins->GetBinError(bin); };
    for (int32_t i = 0; i < numerator->GetN(); ++i)
      inEmptyBin.push_back(isEmpty(axis->FindFixBin(numerator->GetX()[i])));
  }
  int32_t numN = numerator->GetN();
  double_t* numX = numerator->GetX();
  double_t* numY = numerator->GetY();
  double_t* numEy = numerator->GetEY();
  // asymmetric errors through TGraph's virtual getters (TGraphAsymmErrors, TGraphBentErrors; null for the others)
  double_t* numEyLow = numerator->GetEYlow();
  double_t* numEyHigh = numerator->GetEYhigh();
  int32_t denomN = denominator->GetN();
  double_t* denomX = denominator->GetX();
  double_t* denomY = denominator->GetY();
  double_t* denomEy = denominator->GetEY();
  double_t* denomEyLow = denominator->GetEYlow();
  double_t* denomEyHigh = denominator->GetEYhigh();
  auto multiErrors = MultiErrorSums(numerator);

  bool doInterpol = false;
  if (numN > denomN || (numEy && (denomEyLow && denomEyHigh)) || ((numEyLow && numEyHigh) && denomEy)) {
    doInterpol = true;
  } else {
    // check if graphs have the same x values
    for (int32_t i = 0; i < numN; ++i) {
      if (numX[i] != denomX[i]) {
        doInterpol = true;
        break;
      }
    }
  }

  bool deleteDenom = false;
  std::unique_ptr<TGraph> filledBins;
  if (doInterpol) {
    if (denominatorBins) {
      auto* filled = new TGraphErrors();
      filledBins.reset(filled);
      filled->SetName(denominator->GetName());  // used in messages
      for (int32_t i = 0; i < denomN; ++i) {
        if (!denomY[i] && !(denominator->GetErrorY(i) > 0.)) continue;
        const int32_t n = filled->GetN();
        filled->SetPoint(n, denomX[i], denomY[i]);
        filled->SetPointError(n, denominator->GetErrorX(i), denominator->GetErrorY(i));
      }
      if (!filled->GetN()) {
        if (verbose && numN) logger::warning("{} is zero at every point of {}: the ratio has no values.", denominator->GetName(), numerator->GetName());
        for (int32_t i = numN - 1; i >= 0; --i)
          RemoveGraphPoint(numerator, i);
        return true;
      }
      denominator = filled;
      denomN = filled->GetN();
      denomX = filled->GetX();
      denomY = filled->GetY();
      denomEy = filled->GetEY();
      denomEyLow = filled->GetEYlow();
      denomEyHigh = filled->GetEYhigh();
    }
    if (denomN < 2) {
      logger::error("Cannot interpolate {} for the division: it has {}, at least 2 are needed.", denominator->GetName(), plural(denomN, (denominatorBins) ? "filled bin" : "point"));
      return false;
    }
    // spline interpolation requires sorted values
    if (!denominator->TestBit(TGraph::kIsSortedX)) {
      denominator = static_cast<TGraph*>(denominator->Clone(denominator->GetName()));  // keeps the name for messages
      SortGraph(denominator);
      denomX = denominator->GetX();
      denomY = denominator->GetY();
      denomEy = denominator->GetEY();
      denomEyLow = denominator->GetEYlow();
      denomEyHigh = denominator->GetEYhigh();
      deleteDenom = true;
    }
    // a spline needs strictly increasing x: with a repeated x (steps, bands) every interpolated value is NaN and the ratio empty
    for (int32_t i = 1; i < denomN; ++i) {
      if (denomX[i] == denomX[i - 1]) {
        logger::error("Cannot divide {} by graph {}: it has two points at x = {}, so it cannot be interpolated at the x values of {}.", numerator->GetName(), denominator->GetName(), denomX[i], numerator->GetName());
        if (deleteDenom) delete denominator;
        return false;
      }
    }

    // create splines for values and error envelope of denominator
    vector<double_t> denomShiftLow(denomN);
    vector<double_t> denomShiftHigh(denomN);
    bool errorlessDenom = false;
    for (int32_t i = 0; i < denomN; ++i) {
      if (denomEy) {
        denomShiftLow[i] = denomY[i] - denomEy[i];
        denomShiftHigh[i] = denomY[i] + denomEy[i];
      } else if (denomEyLow && denomEyHigh) {
        denomShiftLow[i] = denomY[i] - denomEyLow[i];
        denomShiftHigh[i] = denomY[i] + denomEyHigh[i];
      } else {
        errorlessDenom = true;
        break;
      }
    }
    TSpline3 denomSpline("denomSpline", denominator);
    optional<TSpline3> denomSplineLow;
    optional<TSpline3> denomSplineHigh;
    if (!errorlessDenom) {
      denomSplineLow.emplace("denomSplineLow", denomX, denomShiftLow.data(), denomN);
      denomSplineHigh.emplace("denomSplineHigh", denomX, denomShiftHigh.data(), denomN);
    }

    // reset denominator pointers
    denomY = nullptr;
    denomEy = nullptr;
    denomEyLow = nullptr;
    denomEyHigh = nullptr;

    // create arrays of interpolated points and errors at numerator positions
    denomY = new double_t[numN];
    if (!errorlessDenom) {
      if (numEy) {
        denomEy = new double_t[numN];
      } else if (numEyLow && numEyHigh) {
        denomEyLow = new double_t[numN];
        denomEyHigh = new double_t[numN];
      }
    }
    // outside the denominator's range the spline extrapolates: warn with the distance, so the user can judge if that is reasonable
    const double_t xMin = denomX[0] - std::max(0., denominator->GetErrorXlow(0));
    const double_t xMax = denomX[denomN - 1] + std::max(0., denominator->GetErrorXhigh(denomN - 1));
    int32_t nOutside = 0;
    double_t maxDistance = 0.;
    for (int32_t i = 0; i < numN; ++i) {
      if (const double_t distance = std::max(xMin - numX[i], numX[i] - xMax); distance > 0. && !(denominatorBins && inEmptyBin[i])) {
        ++nOutside;
        maxDistance = std::max(maxDistance, distance);
      }
      denomY[i] = denomSpline.Eval(numX[i]);
      if (denomEy) {
        denomEy[i] = std::sqrt(0.5 * (std::pow(denomY[i] - denomSplineLow->Eval(numX[i]), 2) + std::pow(denomSplineHigh->Eval(numX[i]) - denomY[i], 2)));
      } else if (denomEyLow && denomEyHigh) {
        denomEyLow[i] = std::abs(denomY[i] - denomSplineLow->Eval(numX[i]));
        denomEyHigh[i] = std::abs(denomSplineHigh->Eval(numX[i]) - denomY[i]);
      }
    }
    if (nOutside) {
      if (verbose) logger::warning("{} of {} points of {} are outside the range [{}, {}] of {} (up to {} beyond); the denominator is extrapolated there.", nOutside, numN, numerator->GetName(), xMin, xMax, denominator->GetName(), maxDistance);
    }
    // the cubic spline can overshoot between steep points: a denominator of one sign interpolated to zero or the other sign
    const double_t* knotY = denominator->GetY();
    const bool allPositive = std::all_of(knotY, knotY + denomN, [](double_t y) { return y > 0.; });
    const bool allNegative = !allPositive && std::all_of(knotY, knotY + denomN, [](double_t y) { return y < 0.; });
    if (verbose && (allPositive || allNegative)) {
      int32_t nFlipped{};
      for (int32_t i = 0; i < numN; ++i) {
        if (!(denominatorBins && inEmptyBin[i]) && ((allPositive) ? denomY[i] <= 0. : denomY[i] >= 0.)) ++nFlipped;
      }
      if (nFlipped) {
        logger::warning("The interpolation of {} for {} of {} points of {} has the wrong sign (the spline overshoots): the ratio there is not meaningful.",
                        denominator->GetName(), nFlipped, numN, numerator->GetName());
      }
    }
  }

  // compute ratio and errors
  vector<int32_t> undefined;  // points where the denominator is zero: removed, as ROOT does not draw the empty bins of TH1::Divide
  for (int32_t i = 0; i < numN; ++i) {
    if (!denomY[i] || (denominatorBins && inEmptyBin[i])) {
      undefined.push_back(i);
    } else {
      // the ratio decreases with the denominator if numerator and denominator have the same sign: the lower error of the numerator
      // then goes with the upper error of the denominator (before the swap for a negative denominator below), else with the lower one
      const bool sameSign = (numY[i] > 0.) == (denomY[i] > 0.);
      double_t* denomForLow = (sameSign) ? denomEyHigh : denomEyLow;
      double_t* denomForHigh = (sameSign) ? denomEyLow : denomEyHigh;
      for (auto [errNum, errDenom] : vector<tuple<double_t*, double_t*>>{{numEy, denomEy}, {numEyLow, denomForLow}, {numEyHigh, denomForHigh}}) {
        if (errNum) {
          double_t denomError = (errDenom) ? errDenom[i] : 0.;
          if (binomialErrors) {
            // binomial error propagation (as implemented in root)
            errNum[i] = (numY[i] == denomY[i]) ? 0. : std::sqrt(std::abs(((1. - 2. * numY[i] / denomY[i]) * std::pow(errNum[i], 2) + std::pow(numY[i], 2) * std::pow(denomError, 2) / std::pow(denomY[i], 2)) / std::pow(denomY[i], 2)));
          } else {
            // gaussian error propagation
            errNum[i] = std::sqrt(std::pow(errNum[i] / denomY[i], 2) + std::pow(denomError * numY[i] / std::pow(denomY[i], 2), 2));
          }
        }
      }
      numY[i] = numY[i] / denomY[i];
      if (denomY[i] < 0. && numEyLow && numEyHigh) {
        std::swap(numEyLow[i], numEyHigh[i]);  // a negative denominator turns the ratio upside down
        SwapMultiErrors(numerator, multiErrors, i);
      }
    }
  }
  ScaleMultiErrors(numerator, multiErrors);
  if (verbose && !undefined.empty() && static_cast<int32_t>(undefined.size()) == numN) {
    logger::warning("{} is zero at every point of {}: the ratio has no values.", denominator->GetName(), numerator->GetName());
  }
  for (auto it = undefined.rbegin(); it != undefined.rend(); ++it) {
    RemoveGraphPoint(numerator, *it);
  }
  if (doInterpol) {
    if (denomY) delete[] denomY;
    if (denomEy) delete[] denomEy;
    if (denomEyLow) delete[] denomEyLow;
    if (denomEyHigh) delete[] denomEyHigh;
  }
  if (deleteDenom) {
    delete denominator;
  }
  return true;
}
bool PlotPainter::Divide(TH1* numerator, TGraph* denominator, bool binomialErrors, bool verbose, const TH1* denominatorBins)
{
  if (numerator->GetDimension() != 1) {
    logger::error("Cannot divide {} by graph {}: only a 1d histogram can be divided by a graph.", numerator->GetName(), denominator->GetName());
    return false;
  }
  if (numerator->GetXaxis()->IsAlphanumeric()) {
    logger::error("Cannot divide {} by graph {}: its x axis is alphanumeric.", numerator->GetName(), denominator->GetName());
    return false;
  }
  TGraphErrors numeratorGraph(numerator);
  numeratorGraph.SetName(numerator->GetName());  // used in messages
  if (!Divide(&numeratorGraph, denominator, binomialErrors, verbose, denominatorBins)) return false;
  const double_t entries = numerator->GetEntries();  // SetBinContent counts one entry per bin
  numerator->Reset();
  // by position: the points of bins with a zero denominator were removed, those bins stay empty
  for (int32_t i = 0; i < numeratorGraph.GetN(); ++i) {
    const int32_t bin = numerator->FindFixBin(numeratorGraph.GetX()[i]);
    numerator->SetBinContent(bin, numeratorGraph.GetY()[i]);
    numerator->SetBinError(bin, numeratorGraph.GetEY()[i]);
  }
  numerator->SetEntries(entries);
  return true;
}
bool PlotPainter::Divide(TGraph* numerator, TH1* denominator, bool binomialErrors, bool verbose)
{
  if (denominator->GetDimension() != 1) {
    logger::error("Cannot divide graph {} by {}: only a 1d histogram can divide a graph.", numerator->GetName(), denominator->GetName());
    return false;
  }
  if (denominator->GetXaxis()->IsAlphanumeric()) {
    logger::error("Cannot divide graph {} by {}: the x axis of {} is alphanumeric.", numerator->GetName(), denominator->GetName(), denominator->GetName());
    return false;
  }
  TGraphErrors denominatorGraph(denominator);
  denominatorGraph.SetName(denominator->GetName());  // used in messages
  return Divide(numerator, &denominatorGraph, binomialErrors, verbose, denominator);
}
bool PlotPainter::Divide(TH1* numerator, TH1* denominator, bool binomialErrors, bool verbose)
{
  if (verbose) {
    bool empty = true;
    for (int32_t bin = 0; empty && bin < denominator->GetNcells(); ++bin) {
      empty = !denominator->GetBinContent(bin);
    }
    if (empty) logger::warning("{} is empty: the ratio {} has no values.", denominator->GetName(), numerator->GetName());
  }
  // edges are compared with the tolerance of ROOT's own consistency check in TH1::Divide (TH1::CheckBinLimits)
  auto sameAxisBinning = [](TAxis* a, TAxis* b) -> bool {
    if (a->GetNbins() != b->GetNbins()) return false;
    if (a->IsAlphanumeric() || b->IsAlphanumeric()) return true;
    for (int32_t i = 1; i <= a->GetNbins() + 1; ++i) {
      if (!TMath::AreEqualAbs(a->GetBinLowEdge(i), b->GetBinLowEdge(i), a->GetBinWidth(i) * 1e-10)) return false;
    }
    return true;
  };
  if (numerator->GetDimension() != denominator->GetDimension()) {
    logger::error("Cannot divide {} ({}d) by {} ({}d): the dimensions differ.", numerator->GetName(), numerator->GetDimension(), denominator->GetName(), denominator->GetDimension());
    return false;
  }
  auto* numeratorPoly = dynamic_cast<TH2Poly*>(numerator);
  auto* denominatorPoly = dynamic_cast<TH2Poly*>(denominator);
  if (numeratorPoly || denominatorPoly) {
    // TH1::Divide pairs the cells by number: they must be the same polygons (compared by bounding box and area)
    bool samePolygons = numeratorPoly && denominatorPoly && numeratorPoly->GetNumberOfBins() == denominatorPoly->GetNumberOfBins();
    for (int32_t bin = 1; samePolygons && bin <= numeratorPoly->GetNumberOfBins(); ++bin) {
      auto* a = static_cast<TH2PolyBin*>(numeratorPoly->GetBins()->At(bin - 1));
      auto* b = static_cast<TH2PolyBin*>(denominatorPoly->GetBins()->At(bin - 1));
      const double_t tolerance = 1e-10 * std::max(a->GetXMax() - a->GetXMin(), a->GetYMax() - a->GetYMin());
      samePolygons = TMath::AreEqualAbs(a->GetXMin(), b->GetXMin(), tolerance) && TMath::AreEqualAbs(a->GetXMax(), b->GetXMax(), tolerance) &&
                     TMath::AreEqualAbs(a->GetYMin(), b->GetYMin(), tolerance) && TMath::AreEqualAbs(a->GetYMax(), b->GetYMax(), tolerance) &&
                     TMath::AreEqualRel(a->GetArea(), b->GetArea(), 1e-10);
    }
    if (!samePolygons) {
      logger::error("Cannot divide {} by {}: a TH2Poly can only be divided by a TH2Poly with the same polygons in the same order.", numerator->GetName(), denominator->GetName());
      return false;
    }
    // the polygons divided as TH1::Divide does: it ends with ResetStats, which reads past the arrays of a TH2Poly (ROOT bug)
    const double_t entries = (binomialErrors) ? denominator->GetEntries() : numerator->GetEntries();
    for (int32_t bin = 1; bin <= numeratorPoly->GetNumberOfBins(); ++bin) {
      const double_t b1 = numerator->GetBinContent(bin);
      const double_t b2 = denominator->GetBinContent(bin);
      const double_t e1sq = std::pow(numerator->GetBinError(bin), 2);
      const double_t e2sq = std::pow(denominator->GetBinError(bin), 2);
      double_t errorSq{};
      if (b2 && binomialErrors) {
        errorSq = (b1 == b2) ? 0. : std::abs(((1. - 2. * b1 / b2) * e1sq + b1 * b1 * e2sq / (b2 * b2)) / (b2 * b2));
      } else if (b2) {
        errorSq = (e1sq * b2 * b2 + e2sq * b1 * b1) / std::pow(b2, 4);
      }
      numerator->SetBinContent(bin, (b2) ? b1 / b2 : 0.);
      numerator->SetBinError(bin, std::sqrt(errorSq));
    }
    numerator->SetEntries(entries);
    return true;
  }
  bool sameBinning = sameAxisBinning(numerator->GetXaxis(), denominator->GetXaxis());
  if (numerator->GetDimension() >= 2) sameBinning = sameBinning && sameAxisBinning(numerator->GetYaxis(), denominator->GetYaxis());
  if (numerator->GetDimension() >= 3) sameBinning = sameBinning && sameAxisBinning(numerator->GetZaxis(), denominator->GetZaxis());
  if (sameBinning) {
    // without stored errors TH1::Divide takes the content, not its absolute value, as squared error: a negative bin gets a NaN error
    for (TH1* operand : {numerator, denominator}) {
      if (!operand->GetSumw2N()) operand->Sumw2();
    }
    return numerator->Divide(numerator, denominator, 1., 1., (binomialErrors) ? "B" : "");
  }
  for (TAxis* axis : {numerator->GetXaxis(), numerator->GetYaxis(), numerator->GetZaxis(), denominator->GetXaxis(), denominator->GetYaxis(), denominator->GetZaxis()}) {
    if (axis && axis->IsAlphanumeric()) {
      logger::error("Cannot divide {} by {}: their binnings differ and interpolation needs numeric axes.", numerator->GetName(), denominator->GetName());
      return false;
    }
  }
  if (numerator->GetDimension() == 1 && denominator->GetDimension() == 1) {
    TGraphErrors denominatorGraph(denominator);
    denominatorGraph.SetName(denominator->GetName());  // used in messages
    return Divide(numerator, &denominatorGraph, binomialErrors, verbose, denominator);
  }
  logger::error("Cannot divide {} by {}: their binnings differ (interpolated division of {}d histograms is not supported).", numerator->GetName(), denominator->GetName(), numerator->GetDimension());
  return false;
}
bool PlotPainter::Divide(TH1* numerator, TF1* denominator, bool binomialErrors, bool verbose)
{
  if (denominator->GetNdim() > numerator->GetDimension()) {
    logger::error("Cannot divide {} by the higher dimensional function {}.", numerator->GetName(), denominator->GetName());
    return false;
  }
  // bins outside the range of the function are left out of the ratio, as points of graphs are: set to 0 and, along the axes, cut off the drawn range
  // (TH1::Divide would leave them as they are); the position of a bin is its centre; underflow and overflow are always left out
  if (!numerator->GetSumw2N()) numerator->Sumw2();
  const double_t entries = numerator->GetEntries();
  int32_t nOutside{};
  auto divideBin = [&](int32_t bin, double_t* xyz, bool underOrOverflow) {
    const double_t content = numerator->GetBinContent(bin);
    const double_t error = numerator->GetBinError(bin);
    const bool inside = !underOrOverflow && denominator->IsInside(xyz);
    const double_t denom = (inside) ? denominator->EvalPar(xyz) : 0.;
    if (!inside && !underOrOverflow && content != 0.) ++nOutside;
    numerator->SetBinContent(bin, (denom) ? content / denom : 0.);
    numerator->SetBinError(bin, (denom) ? error / std::abs(denom) : 0.);
    return inside;
  };
  if (auto* poly = dynamic_cast<TH2Poly*>(numerator)) {
    // the cells of a TH2Poly are its polygons (the centre of their bounding box); its sea bins (negative numbers) have no position
    for (TObject* obj : *poly->GetBins()) {
      auto* polyBin = static_cast<TH2PolyBin*>(obj);
      double_t xyz[3] = {0.5 * (polyBin->GetXMin() + polyBin->GetXMax()), 0.5 * (polyBin->GetYMin() + polyBin->GetYMax()), 0.};
      divideBin(polyBin->GetBinNumber(), xyz, false);
    }
  } else {
    std::array<TAxis*, 3> axes{numerator->GetXaxis(), numerator->GetYaxis(), numerator->GetZaxis()};
    std::array<int32_t, 3> firstInside{INT32_MAX, INT32_MAX, INT32_MAX}, lastInside{INT32_MIN, INT32_MIN, INT32_MIN};
    for (int32_t bin = 0; bin < numerator->GetNcells(); ++bin) {
      std::array<int32_t, 3> binXYZ{};
      numerator->GetBinXYZ(bin, binXYZ[0], binXYZ[1], binXYZ[2]);
      double_t xyz[3] = {axes[0]->GetBinCenter(binXYZ[0]), axes[1]->GetBinCenter(binXYZ[1]), axes[2]->GetBinCenter(binXYZ[2])};
      const bool underOrOverflow = numerator->IsBinUnderflow(bin) || numerator->IsBinOverflow(bin);
      if (divideBin(bin, xyz, underOrOverflow)) {
        for (size_t i = 0; i < 3; ++i) {
          firstInside[i] = std::min(firstInside[i], binXYZ[i]);
          lastInside[i] = std::max(lastInside[i], binXYZ[i]);
        }
      }
    }
    // the bins along an axis outside the range of the function are not drawn (a range of the axis set already is kept where it is narrower)
    for (int32_t i = 0; i < numerator->GetDimension() && firstInside[i] <= lastInside[i]; ++i) {
      const int32_t first = std::max(firstInside[i], axes[i]->GetFirst());
      const int32_t last = std::min(lastInside[i], axes[i]->GetLast());
      if (first != axes[i]->GetFirst() || last != axes[i]->GetLast()) axes[i]->SetRange(first, last);
    }
    numerator->ResetStats();  // not for a TH2Poly: it reads past its arrays (ROOT bug)
  }
  numerator->SetEntries(entries);
  if (nOutside && verbose) {
    logger::warning("{} bins of {} lie outside the range of function {} and are left out of the ratio.", nOutside, numerator->GetName(), denominator->GetName());
  }
  if (binomialErrors && verbose) {
    logger::warning("Binomial errors are not supported for the division of {} by function {}.", numerator->GetName(), denominator->GetName());
  }
  return true;
}
bool PlotPainter::Divide(TGraph* numerator, TF1* denominator, bool binomialErrors, bool verbose)
{
  if (denominator->GetNdim() > 1) {
    logger::error("Cannot divide graph {} by the higher dimensional function {}.", numerator->GetName(), denominator->GetName());
    return false;
  }
  // the points outside the range of the function are left out
  const int32_t nPoints = numerator->GetN();
  RemoveGraphPointsOutside(numerator, denominator->GetXmin(), denominator->GetXmax(), nullopt, nullopt);
  if (numerator->GetN() < nPoints && verbose) {
    logger::warning("{} points of {} lie outside the range of function {} and are left out of the ratio.", nPoints - numerator->GetN(), numerator->GetName(), denominator->GetName());
  }
  double_t* x = numerator->GetX();
  double_t* y = numerator->GetY();
  double_t* ey = numerator->GetEY();
  double_t* eyLow = numerator->GetEYlow();    // asymmetric and bent errors
  double_t* eyHigh = numerator->GetEYhigh();  // (nullptr for symmetric errors)
  auto multiErrors = MultiErrorSums(numerator);
  vector<int32_t> undefined;  // points where the function is zero: removed (see Divide(TGraph*, TGraph*))
  for (int32_t i = 0; i < numerator->GetN(); ++i) {
    const double_t denom = denominator->Eval(x[i]);
    if (!denom) undefined.push_back(i);
    const double_t absDenom = std::abs(denom);  // errors must stay positive
    y[i] = (denom) ? y[i] / denom : 0.;
    if (ey) {
      ey[i] = (denom) ? ey[i] / absDenom : 0.;
    } else if (eyLow && eyHigh) {
      eyLow[i] = (denom) ? eyLow[i] / absDenom : 0.;
      eyHigh[i] = (denom) ? eyHigh[i] / absDenom : 0.;
      if (denom < 0.) {
        std::swap(eyLow[i], eyHigh[i]);  // a negative denominator turns the ratio upside down
        SwapMultiErrors(numerator, multiErrors, i);
      }
    }
  }
  ScaleMultiErrors(numerator, multiErrors);
  for (auto it = undefined.rbegin(); it != undefined.rend(); ++it) {
    RemoveGraphPoint(numerator, *it);
  }
  if (binomialErrors && verbose) {
    logger::warning("Binomial errors are not supported for the division of {} by function {}.", numerator->GetName(), denominator->GetName());
  }
  return true;
}

//**************************************************************************************************
/**
 * Removes point i of a graph. TGraph::RemovePoint leaves the y errors of a TGraphMultiErrors where they are (ROOT bug) and crashes on one
 * made by its streamer (no sums of the errors yet): the sums are made first and the error sets moved here.
 */
//**************************************************************************************************
void PlotPainter::RemoveGraphPoint(TGraph* graph, int32_t i)
{
  auto* multi = dynamic_cast<TGraphMultiErrors*>(graph);
  if (!multi) {
    graph->RemovePoint(i);
    return;
  }
  multi->GetEYlow();
  multi->GetEYhigh();
  multi->RemovePoint(i);
  for (int32_t e = 0; e < multi->GetNYErrors(); ++e) {
    const double_t* low = multi->GetEYlow(e);
    const double_t* high = multi->GetEYhigh(e);
    for (int32_t k = i; k < multi->GetN(); ++k)
      multi->SetPointEY(k, e, low[k + 1], high[k + 1]);
  }
}

//**************************************************************************************************
/**
 * Sorts the points of a graph in x. TGraphMultiErrors::Sort leaves the y errors where they are (ROOT bug): its points are put in order
 * here, each with its errors.
 */
//**************************************************************************************************
void PlotPainter::SortGraph(TGraph* graph)
{
  auto* multi = dynamic_cast<TGraphMultiErrors*>(graph);
  if (!multi) {
    graph->Sort();
    return;
  }
  const int32_t n = multi->GetN();
  multi->GetEYlow();  // a graph made by the streamer has no sums of its errors yet (see RemoveGraphPoint)
  multi->GetEYhigh();
  vector<int32_t> order(n);
  std::iota(order.begin(), order.end(), 0);
  std::stable_sort(order.begin(), order.end(), [&](int32_t a, int32_t b) { return multi->GetX()[a] < multi->GetX()[b]; });
  auto reordered = [&](const double_t* values) {
    vector<double_t> result(n);
    for (int32_t i = 0; i < n; ++i)
      result[i] = values[order[i]];
    return result;
  };
  const vector<double_t> x = reordered(multi->GetX()), y = reordered(multi->GetY());
  const vector<double_t> exLow = reordered(multi->GetEXlow()), exHigh = reordered(multi->GetEXhigh());
  vector<vector<double_t>> eyLow, eyHigh;
  for (int32_t e = 0; e < multi->GetNYErrors(); ++e) {
    eyLow.push_back(reordered(multi->GetEYlow(e)));
    eyHigh.push_back(reordered(multi->GetEYhigh(e)));
  }
  for (int32_t i = 0; i < n; ++i) {
    multi->SetPoint(i, x[i], y[i]);
    multi->SetPointEX(i, exLow[i], exHigh[i]);
    for (int32_t e = 0; e < multi->GetNYErrors(); ++e)
      multi->SetPointEY(i, e, eyLow[e][i], eyHigh[e][i]);
  }
  multi->SetBit(TGraph::kIsSortedX);
}

//**************************************************************************************************
/**
 * Removes point i of a 2d graph: RemovePoint is not virtual, the one of TGraph2D leaves the errors where they are.
 */
//**************************************************************************************************
void PlotPainter::RemoveGraphPoint(TGraph2D* graph, int32_t i)
{
  if (auto* errors = dynamic_cast<TGraph2DErrors*>(graph)) {
    errors->RemovePoint(i);
  } else if (auto* asymmErrors = dynamic_cast<TGraph2DAsymmErrors*>(graph)) {
    asymmErrors->RemovePoint(i);
  } else {
    graph->RemovePoint(i);
  }
}

//**************************************************************************************************
/**
 * Removes the points of a graph outside of the given x and y limits. The order of the points is kept (it matters for lines).
 */
//**************************************************************************************************
void PlotPainter::RemoveGraphPointsOutside(TGraph* graph, optional<double_t> minX, optional<double_t> maxX, optional<double_t> minY, optional<double_t> maxY)
{
  if (!minX && !maxX && !minY && !maxY) return;
  for (int32_t i = graph->GetN() - 1; i >= 0; --i) {  // from the back, so that the indices of the remaining points stay valid
    const double_t x = graph->GetPointX(i);
    const double_t y = graph->GetPointY(i);
    if ((minX && x < *minX) || (maxX && x > *maxX) || (minY && y < *minY) || (maxY && y > *maxY)) RemoveGraphPoint(graph, i);
  }
}

//**************************************************************************************************
/**
 * Scales histogram axis by a constant value.
 */
//**************************************************************************************************
void PlotPainter::ScaleAxis(TH1* hist, int16_t axisIndex, double_t scaleFactor)
{
  TAxis* axis = GetAxis(hist, axisIndex);
  if (axis->GetXbins()->GetSize() > 0) {
    // variable bin widths
    vector<double_t> newBinEdges;
    auto* edges = axis->GetXbins()->GetArray();
    newBinEdges.reserve(axis->GetXbins()->GetSize());
    for (int32_t i = 0; i < axis->GetXbins()->GetSize(); ++i) {
      newBinEdges.push_back(edges[i] * scaleFactor);
    }
    axis->Set(axis->GetNbins(), newBinEdges.data());
  } else {
    // fixed bin width
    axis->Set(axis->GetNbins(), axis->GetXmin() * scaleFactor, axis->GetXmax() * scaleFactor);
  }
  hist->ResetStats();
}

//**************************************************************************************************
/**
 * Rescales the point coordinates (and associated errors) of a graph along one axis by a constant factor.
 */
//**************************************************************************************************
void PlotPainter::ScaleGraphAxis(TGraph* graph, int16_t axis, double_t scaleFactor)
{
  double_t* values = (axis == 0) ? graph->GetX() : graph->GetY();
  double_t* errors = (axis == 0) ? graph->GetEX() : graph->GetEY();
  double_t* errorsLow = (axis == 0) ? graph->GetEXlow() : graph->GetEYlow();     // asymmetric and bent errors
  double_t* errorsHigh = (axis == 0) ? graph->GetEXhigh() : graph->GetEYhigh();  // (nullptr for symmetric errors)
  auto multiErrors = (axis == 1) ? MultiErrorSums(graph) : nullopt;
  // a negative factor (Normalize of values summing up below zero) turns the values upside down: errors stay positive, low and high swap
  const double_t errorFactor = std::abs(scaleFactor);
  for (int32_t i{}; i < graph->GetN(); ++i) {
    values[i] *= scaleFactor;
    if (errors) errors[i] *= errorFactor;
    if (errorsLow) errorsLow[i] *= errorFactor;
    if (errorsHigh) errorsHigh[i] *= errorFactor;
    if (scaleFactor < 0. && errorsLow && errorsHigh) {
      std::swap(errorsLow[i], errorsHigh[i]);
      SwapMultiErrors(graph, multiErrors, i);
    }
  }
  ScaleMultiErrors(graph, multiErrors);
}

//**************************************************************************************************
/**
 * A TGraphMultiErrors draws its y errors per dimension, GetEYlow()/GetEYhigh() are only their cached sums.
 * MultiErrorSums stores the sums before they are changed, ScaleMultiErrors then scales the dimensions of each point to the new sums
 * (consistent with every mode of summing them), SwapMultiErrors swaps low and high of a point as the sums were swapped.
 */
//**************************************************************************************************
optional<PlotPainter::multi_error_sums_t> PlotPainter::MultiErrorSums(TGraph* graph)
{
  if (!dynamic_cast<TGraphMultiErrors*>(graph)) return nullopt;
  const int32_t n = graph->GetN();
  return multi_error_sums_t{{graph->GetEYlow(), graph->GetEYlow() + n}, {graph->GetEYhigh(), graph->GetEYhigh() + n}};
}
void PlotPainter::SwapMultiErrors(TGraph* graph, optional<multi_error_sums_t>& sums, int32_t i)
{
  if (!sums) return;
  auto* multi = static_cast<TGraphMultiErrors*>(graph);
  std::swap(sums->first[i], sums->second[i]);
  for (int32_t e = 0; e < multi->GetNYErrors(); ++e) {
    std::swap(multi->GetEYlow(e)[i], multi->GetEYhigh(e)[i]);
  }
}
void PlotPainter::ScaleMultiErrors(TGraph* graph, const optional<multi_error_sums_t>& sums)
{
  if (!sums) return;
  auto* multi = static_cast<TGraphMultiErrors*>(graph);
  for (auto [newSums, oldSums, isLow] : {tuple{multi->GetEYlow(), &sums->first, true}, tuple{multi->GetEYhigh(), &sums->second, false}}) {
    for (int32_t i = 0; i < multi->GetN(); ++i) {
      const double_t oldSum = (*oldSums)[i];
      for (int32_t e = 0; e < multi->GetNYErrors(); ++e) {
        double_t& error = (isLow) ? multi->GetEYlow(e)[i] : multi->GetEYhigh(e)[i];
        // without errors before (an error of the denominator was added), the first dimension takes it
        error = (oldSum) ? error * newSums[i] / oldSum : ((e == 0) ? newSums[i] : 0.);
      }
    }
  }
}

void PlotPainter::ScaleGraphAxis(TGraph2D* graph, int16_t axis, double_t scaleFactor)
{
  double_t* values{};
  double_t* errors{};
  double_t* errorsLow{};
  double_t* errorsHigh{};
  switch (axis) {
    case 0:
      values = graph->GetX();
      errors = graph->GetEX();
      errorsLow = graph->GetEXlow();
      errorsHigh = graph->GetEXhigh();
      break;
    case 1:
      values = graph->GetY();
      errors = graph->GetEY();
      errorsLow = graph->GetEYlow();
      errorsHigh = graph->GetEYhigh();
      break;
    case 2:
      values = graph->GetZ();
      errors = graph->GetEZ();
      errorsLow = graph->GetEZlow();
      errorsHigh = graph->GetEZhigh();
      break;
    default:
      return;
  }
  const double_t errorFactor = std::abs(scaleFactor);  // as for 1d graphs
  for (int32_t i{}; i < graph->GetN(); ++i) {
    values[i] *= scaleFactor;
    if (errors) errors[i] *= errorFactor;
    if (errorsLow) errorsLow[i] *= errorFactor;
    if (errorsHigh) errorsHigh[i] *= errorFactor;
    if (scaleFactor < 0. && errorsLow && errorsHigh) std::swap(errorsLow[i], errorsHigh[i]);
  }
}

//**************************************************************************************************
/**
 * Rescales functions or function arguments by a constant factor.
 */
//**************************************************************************************************
TF1* PlotPainter::ScaleFunc(TF1* func, double_t domainFactorX, double_t contentFactor)
{
  func->AddToGlobalList(false);
  auto origFunc = std::shared_ptr<TF1>(func);
  auto* newFunc = new TF1(
    origFunc->GetName(),
    [origFunc, domainFactorX, contentFactor](double_t* x, double_t*) { return contentFactor * origFunc->Eval(x[0] / domainFactorX); },
    origFunc->GetXmin() * domainFactorX, origFunc->GetXmax() * domainFactorX, 0, 1, TF1::EAddToList::kNo);
  CopyFuncAttributes(origFunc.get(), newFunc);
  return newFunc;
}

TF2* PlotPainter::ScaleFunc(TF2* func, double_t domainFactorX, double_t domainFactorY, double_t contentFactor)
{
  func->AddToGlobalList(false);
  auto origFunc = std::shared_ptr<TF2>(func);
  auto* newFunc = new TF2(
    origFunc->GetName(),
    [origFunc, domainFactorX, domainFactorY, contentFactor](double_t* x, double_t*) {
      return contentFactor * origFunc->Eval(x[0] / domainFactorX, x[1] / domainFactorY);
    },
    origFunc->GetXmin() * domainFactorX, origFunc->GetXmax() * domainFactorX,
    origFunc->GetYmin() * domainFactorY, origFunc->GetYmax() * domainFactorY, 0, 2, TF1::EAddToList::kNo);
  CopyFuncAttributes(origFunc.get(), newFunc);
  return newFunc;
}

TF3* PlotPainter::ScaleFunc(TF3* func, double_t domainFactorX, double_t domainFactorY, double_t domainFactorZ, double_t contentFactor)
{
  func->AddToGlobalList(false);
  auto origFunc = std::shared_ptr<TF3>(func);
  auto* newFunc = new TF3(
    origFunc->GetName(),
    [origFunc, domainFactorX, domainFactorY, domainFactorZ, contentFactor](double_t* x, double_t*) {
      return contentFactor * origFunc->Eval(x[0] / domainFactorX, x[1] / domainFactorY, x[2] / domainFactorZ);
    },
    origFunc->GetXmin() * domainFactorX, origFunc->GetXmax() * domainFactorX,
    origFunc->GetYmin() * domainFactorY, origFunc->GetYmax() * domainFactorY,
    origFunc->GetZmin() * domainFactorZ, origFunc->GetZmax() * domainFactorZ, 0, 3, TF1::EAddToList::kNo);
  CopyFuncAttributes(origFunc.get(), newFunc);
  return newFunc;
}

//**************************************************************************************************
/**
 * Returns actual dimensions in pixel of the text with latex formatting.
 */
//**************************************************************************************************
tuple<uint32_t, uint32_t> PlotPainter::GetTextDimensions(TLatex& text, TPad* pad)
{
  // measured in the mode the plot is painted in: in batch mode the bounding box is the one of the rendered text
  // (switching batch mode off for the measurement, with a canvas created in batch mode, gives nothing or, for texts
  // with drawn parts such as #sqrt, a few pixels of garbage)
  uint32_t width{};
  uint32_t height{};
  int16_t font{text.GetTextFont()};
  if (font % 10 <= 2) {
    text.GetBoundingBox(width, height);
  } else {
    // measured as the same text with a relative size: ROOT converts a relative size to pixels with the smaller of
    // the pad's width and height (as GetTextSizePixel), so the pixel size is divided by that one
    TLatex textBox{text};
    textBox.SetTextFont(font - 1);
    const auto [padWidthPixel, padHeightPixel] = TextPadSizePixel(pad);
    textBox.SetTextSize(text.GetTextSize() * mScale / std::min(padWidthPixel, padHeightPixel));
    textBox.GetBoundingBox(width, height);
  }
  return {width, height};
}

//**************************************************************************************************
/**
 * Returns how far in pixel the text with latex formatting reaches above and below its baseline, as TLatex lays it out (its sizes
 * are protected: read in a class of its own, as TLatex::GetYsize does). A text in TMathText syntax counts as all above.
 */
//**************************************************************************************************
std::pair<double_t, double_t> PlotPainter::GetTextExtentPixel(TLatex& text, TPad* pad)
{
  struct Extent : public TLatex {
    std::pair<double_t, double_t> OverUnder()
    {
      TString title = GetTitle();
      if (title.Length() == 0 || title.Contains("\\") || CheckLatexSyntax(title)) return {0., 0.};
      fError = nullptr;
      const Double_t angle = fTextAngle;
      const TLatexFormSize form = FirstParse(0, GetTextSize(), title.Data());
      fTextAngle = angle;
      fTabSize.clear();
      return {form.Over(), form.Under()};
    }
  };
  Extent extent;
  text.Copy(extent);
  // a size in pixels measured as the same size relative to the smaller pad side (see GetTextDimensions)
  if (const int16_t font = text.GetTextFont(); font % 10 > 2) {
    extent.SetTextFont(font - 1);
    const auto [padWidthPixel, padHeightPixel] = TextPadSizePixel(pad);
    extent.SetTextSize(text.GetTextSize() * mScale / std::min(padWidthPixel, padHeightPixel));
  }
  return extent.OverUnder();
}

//**************************************************************************************************
/**
 * Hides the labels of an axis whose text would reach beyond the pad (Axis::SetHideClippedLabels), e.g. the top label of a
 * ratio pad at the edge shared with the pad above. The label positions are those TGaxis will draw (THLimitsFinder::Optimize
 * for a linear axis with optimized divisions, the decades of a log axis, equidistant for a negative number of divisions).
 * Called once the pad is updated, so that its ranges are final.
 */
//**************************************************************************************************
void PlotPainter::HideClippedLabels(TPad* pad, TAxis* axis, char axisLabel)
{
  if (axis->GetLabelSize() <= 0.f || axis->GetTimeDisplay() || axis->GetLabels()) return;
  const bool isX = (axisLabel == 'X');
  const bool isLog = isX ? pad->GetLogx() : pad->GetLogy();
  const double_t wmin = FromPad(isX ? pad->GetUxmin() : pad->GetUymin(), isLog);
  const double_t wmax = FromPad(isX ? pad->GetUxmax() : pad->GetUymax(), isLog);
  const vector<double_t> labelValues = LabelValues(pad, axis, axisLabel);

  // extent of a label around its tick: half its height (y axis) or half its width (x axis), in absolute pixels
  const auto [padWidthPixel, padHeightPixel] = PadSizePixel(pad);
  const double_t labelSizePixel = (axis->GetLabelFont() % 10 == 3) ? axis->GetLabelSize() * mScale : axis->GetLabelSize() * std::min(padWidthPixel, padHeightPixel);
  const double_t padLow = isX ? pad->XtoAbsPixel(pad->GetX1()) : pad->YtoAbsPixel(pad->GetY2());  // pixel rows grow downwards
  const double_t padHigh = isX ? pad->XtoAbsPixel(pad->GetX2()) : pad->YtoAbsPixel(pad->GetY1());
  int32_t labelNumber{};  // TGaxis numbers the labels it draws from the low end
  for (double_t value : labelValues) {
    if (value < wmin - 1e-9 * (wmax - wmin) || value > wmax + 1e-9 * (wmax - wmin)) continue;
    ++labelNumber;
    const double_t padCoord = isLog ? TMath::Log10(value) : value;
    const double_t tickPixel = isX ? pad->XtoAbsPixel(padCoord) : pad->YtoAbsPixel(padCoord);
    double_t halfExtent = 0.5 * labelSizePixel;
    if (isX) {
      TLatex label(0, 0, fmt::format("{:g}", value).data());
      label.SetTextFont(axis->GetLabelFont());
      label.SetTextSize(axis->GetLabelSize());
      halfExtent = 0.5 * std::get<0>(GetTextDimensions(label, pad));
    }
    if (tickPixel - halfExtent < padLow || tickPixel + halfExtent > padHigh) {
      // a linear axis is addressed by label number: TGaxis matches a value against the label as drawn, i.e. after taking out
      // the exponent of the axis (500 is drawn as 0.5 next to x10^3); the decades of a log axis are drawn as they are
      if (isLog) {
        axis->ChangeLabelByValue(value, -1., 0.);
      } else {
        axis->ChangeLabel(labelNumber, -1., 0.);
      }
    }
  }
}

//**************************************************************************************************
/**
 * The values TGaxis writes labels at along the x or y axis of a pad (the decades of a log axis), as far as they can be told
 * without painting: the primary divisions optimised as by TGaxis.
 */
//**************************************************************************************************
vector<double_t> PlotPainter::LabelValues(TPad* pad, const TAxis* axis, char axisLabel)
{
  const bool isX = (axisLabel == 'X');
  // THistPainter::PaintAxis scales the primary divisions of an x axis whose setting carries a max digits part (> 1000) with
  // the width of the pad relative to the canvas, so that a narrow pad gets fewer labels
  return LabelValues(axis, isX ? pad->GetUxmin() : pad->GetUymin(), isX ? pad->GetUxmax() : pad->GetUymax(), isX ? pad->GetLogx() : pad->GetLogy(),
                     (isX && axis->GetNdivisions() > 1000) ? pad->GetAbsWNDC() : 1.);
}

//**************************************************************************************************
/**
 * The values TGaxis writes labels at along an axis from padMin to padMax (pad coordinates: log10 for a log axis), the primary divisions
 * multiplied by divisionScale.
 */
//**************************************************************************************************
vector<double_t> PlotPainter::LabelValues(const TAxis* axis, double_t padMin, double_t padMax, bool isLog, double_t divisionScale)
{
  const double_t wmin = FromPad(padMin, isLog);
  const double_t wmax = FromPad(padMax, isLog);
  vector<double_t> labelValues;
  if (!(wmax > wmin)) return labelValues;
  if (isLog) {
    for (int32_t decade = static_cast<int32_t>(std::ceil(padMin - 1e-9)); decade <= static_cast<int32_t>(std::floor(padMax + 1e-9)); ++decade) {
      labelValues.push_back(TMath::Power(10., decade));
    }
    return labelValues;
  }
  const int32_t nDiv = axis->GetNdivisions();
  int32_t nPrimary = std::abs(nDiv) % 100;
  if (divisionScale != 1.) nPrimary = static_cast<int32_t>(static_cast<float>(std::max(1, nPrimary)) * divisionScale);
  if (nPrimary == 0) return labelValues;
  double_t binLow{wmin}, binHigh{wmax}, binWidth{(wmax - wmin) / nPrimary};
  int32_t nBins{nPrimary};
  if (nDiv > 0 && nPrimary > 1) {  // TGaxis: no optimisation for a single division
    THLimitsFinder::Optimize(wmin, wmax, nPrimary, binLow, binHigh, nBins, binWidth, "");
    // the labels lie within the axis; if at most one is left, TGaxis divides the axis as it is (from its minimum, without optimisation)
    const double_t epsilon = 1e-9 * (wmax - wmin);
    if (wmin - binLow > epsilon) {
      binLow += binWidth;
      --nBins;
    }
    if (binHigh - wmax > epsilon) {
      binHigh -= binWidth;
      --nBins;
    }
    if (nBins < 1) {
      binLow = wmin;
      binWidth = (wmax - wmin) / nPrimary;
      nBins = nPrimary;
    }
  }
  if (!(binWidth > 0.)) return labelValues;
  for (int32_t i = 0; i <= nBins; ++i) {
    const double_t value = binLow + i * binWidth;
    labelValues.push_back((std::abs(value) < 1e-9 * binWidth) ? 0. : value);  // TGaxis writes 0 where the steps miss it by rounding (-1 + 5 * 0.2)
  }
  return labelValues;
}

//**************************************************************************************************
/**
 * Width in pixels of the widest label of the y axis of a pad, as needed for the automatic placement of its title (TGaxis puts a
 * title with offset 0 next to the widest label): the labels as TGaxis writes them in the usual cases, decades of a log axis as
 * 1, 10 and 10^{n}, values beyond the maximum number of digits with the common power of ten taken out. Labels of the pad that
 * HideClippedLabels dropped still count, as they do for TGaxis.
 */
//**************************************************************************************************
double_t PlotPainter::MaxLabelWidth(TPad* pad, const TAxis* axis, char axisLabel)
{
  return MaxLabelWidth(pad, axis, LabelValues(pad, axis, axisLabel), (axisLabel == 'X') ? pad->GetLogx() : pad->GetLogy());
}

//**************************************************************************************************
/**
 * Width in pixels of the widest of the labels at the given values (as TGaxis writes them, see above).
 */
//**************************************************************************************************
double_t PlotPainter::MaxLabelWidth(TPad* pad, const TAxis* axis, const vector<double_t>& values, bool isLog)
{
  if (axis->GetLabelSize() <= 0.f) return 0.;
  pad->cd();  // texts are measured in the current pad
  double_t largest{};
  for (double_t value : values) {
    largest = std::max(largest, std::abs(value));
  }
  const int32_t maxDigits = (axis->GetNdivisions() / 1000000 > 0) ? axis->GetNdivisions() / 1000000 : TGaxis::GetMaxDigits();
  // the power of ten TGaxis takes out of the labels (a multiple of 3, TGaxis::PaintAxis): for labels finer than maxDigits decimals below
  // 1, for more digits than maxDigits the smallest one leaving at most maxDigits - 1 digits, for fewer than -maxDigits the largest one
  // leaving at least 10^(2 - maxDigits)
  int32_t exponent{};
  if (!isLog && !axis->GetNoExponent() && largest > 0.) {
    constexpr double_t epsilon = 1e-5;
    const double_t step = (values.size() > 1) ? std::abs(values[1] - values[0]) : 0.;
    if (step > 0. && step < TMath::Power(10., -maxDigits) && std::log10(largest) + epsilon < 0.) {
      int32_t digits = std::abs(static_cast<int32_t>(std::log10(largest) + epsilon));
      if (digits % 3 == 1) {
        digits += 2;
      } else if (digits % 3 == 2) {
        digits += 1;
      }
      exponent = -digits;
    } else {
      const int32_t nDigits = static_cast<int32_t>(((largest >= 1.) ? std::log10(largest) : std::log10(largest * 0.0001)) + epsilon) + 1;
      double_t shown = largest;
      if (nDigits > maxDigits) {
        do {
          ++exponent;
          shown /= 10.;
        } while (exponent % 3 != 0 || shown > TMath::Power(10., maxDigits - 1));
      } else if (nDigits < -maxDigits) {
        do {
          --exponent;
          shown *= 10.;
        } while (exponent % 3 != 0 || shown < TMath::Power(10., 2 - maxDigits));
      }
    }
  }
  const double_t factor = TMath::Power(10., exponent);
  double_t width{};
  for (double_t value : values) {
    string text;
    if (isLog) {
      const auto decade = static_cast<int32_t>(std::lround(std::log10(value)));
      text = (decade == 0) ? "1" : ((decade == 1) ? "10" : fmt::format("10^{{{}}}", decade));
    } else {
      const double_t shown = value / factor;
      text = fmt::format("{:g}", (std::abs(shown) < 1e-9 * largest / factor) ? 0. : shown);
    }
    TLatex label(0, 0, TString(text).ReplaceAll("-", "#minus").Data());  // as TGaxis writes a minus sign
    label.SetTextFont(axis->GetLabelFont());
    label.SetTextSize(axis->GetLabelSize());
    width = std::max(width, static_cast<double_t>(std::get<0>(GetTextDimensions(label, pad))));
  }
  return width;
}

//**************************************************************************************************
/**
 * A value a PanelLayout set for a pad (see Plot::Pad::mLayoutValues): the pad's own, else that of pad 0.
 */
//**************************************************************************************************
optional<float_t> PlotPainter::LayoutValue(const Plot::Pad& pad, const Plot::Pad& padDefaults, const string& key)
{
  for (const Plot::Pad* curPad : {&pad, &padDefaults}) {
    if (auto value = curPad->mLayoutValues.find(key); value != curPad->mLayoutValues.end()) {
      return value->second;
    }
  }
  return std::nullopt;
}

//**************************************************************************************************
/**
 * The axis titles of the input of a projection (for <projection>) go with the object that replaces it.
 */
//**************************************************************************************************
void PlotPainter::MoveProjectionTitles(const TObject* from, const TObject* to)
{
  if (auto node = mProjectionAxisTitles.extract(from)) {
    node.key() = to;
    mProjectionAxisTitles.insert(std::move(node));
  }
}

//**************************************************************************************************
/**
 * Replaces a profile by the histogram of its means (errors as set by its error option); returns whether data was a profile.
 */
//**************************************************************************************************
bool PlotPainter::ProfileMeans(data_ptr_t& data)
{
  auto replace = [&](auto* profile, auto* means) {
    means->SetDirectory(nullptr);
    means->SetName(profile->GetName());
    CopyStyle(*profile, *means);  // the projections take labels and titles, not the style
    MoveProjectionTitles(profile, means);
    delete profile;
    data = means;
    return true;
  };
  if (auto* profile = std::get_if<TProfile*>(&data)) return replace(*profile, static_cast<TH1*>((*profile)->ProjectionX("means")));
  if (auto* profile = std::get_if<TProfile2D*>(&data)) return replace(*profile, static_cast<TH2*>((*profile)->ProjectionXY("means")));
  if (auto* hist = std::get_if<TH3*>(&data)) {
    if (auto* profile = dynamic_cast<TProfile3D*>(*hist)) return replace(profile, static_cast<TH3*>(profile->ProjectionXYZ("means")));
  }
  return false;
}

//**************************************************************************************************
/**
 * The settings of an axis in pad 0 and in the pad, in this order (those of the pad override): only the ones that exist.
 */
//**************************************************************************************************
vector<const Plot::Pad::Axis*> PlotPainter::AxisSettings(const Plot::Pad& pad, const Plot::Pad& padDefaults, char axisLabel)
{
  vector<const Plot::Pad::Axis*> settings;
  for (const Plot::Pad* curPad : {&padDefaults, &pad}) {
    if (auto axis = curPad->GetAxes().find(axisLabel); axis != curPad->GetAxes().end()) {
      settings.push_back(&axis->second);
    }
  }
  return settings;
}

//**************************************************************************************************
/**
 * The lower end of the range of an axis as the frame uses it: none if it is at or below zero on a log axis, which cannot show it.
 */
//**************************************************************************************************
optional<double_t> PlotPainter::MinRange(const Plot::Pad& pad, const Plot::Pad& padDefaults, char axisLabel)
{
  optional<double_t> min;
  bool log{};
  for (const auto* axis : AxisSettings(pad, padDefaults, axisLabel)) {
    if (axis->GetMinRange()) min = axis->GetMinRange();
    if (axis->GetLog()) log = *axis->GetLog();
  }
  return (log && min && *min <= 0.) ? nullopt : min;
}

//**************************************************************************************************
/**
 * Places a z title with offset 0 next to the widest label of its axis, one title height beyond it: the vertical z axis of a 3D view
 * (labels left of it) and a colour scale (labels right of it). TGaxis places such titles itself only on the side of an ordinary y axis
 * (TGaxis::PaintAxis adds the label width without the side, so the title of a colour scale lands on its labels), so the offset is set
 * from the widest label of the given ranges (minimum, maximum, log: a 3d histogram has its binned z axis and its contents on the scale,
 * which share the title offset). Called once the pad is painted, when the ranges are final. Returns whether the offset was set.
 */
//**************************************************************************************************
bool PlotPainter::PlaceZTitle(TPad* pad, TAxis* zAxis, const vector<label_range_t>& ranges)
{
  if (zAxis->GetTitleOffset() != 0.f || TString(zAxis->GetTitle()).IsWhitespace() || zAxis->GetTitleSize() <= 0.f) return false;
  const auto [padWidthPixel, padHeightPixel] = PadSizePixel(pad);
  if (padWidthPixel <= 0. || padHeightPixel <= 0.) return false;
  const bool pixelFont = zAxis->GetTitleFont() % 10 == 3;
  // TGaxis puts the title of a vertical axis at 1.6 * offset * title size from it, the size read as a fraction of the pad width
  const double_t unit = 1.6 * (pixelFont ? zAxis->GetTitleSize() * mScale : zAxis->GetTitleSize() * padWidthPixel);
  const double_t titleHeight = pixelFont ? zAxis->GetTitleSize() * mScale : zAxis->GetTitleSize() * std::min(padWidthPixel, padHeightPixel);
  double_t labels{};
  for (const auto& [min, max, isLog] : ranges) {
    if (isLog && !(min > 0.)) continue;
    const double_t padMin = isLog ? std::log10(min) : min;
    const double_t padMax = isLog ? std::log10(max) : max;
    if (padMax > padMin) labels = std::max(labels, MaxLabelWidth(pad, zAxis, LabelValues(zAxis, padMin, padMax, isLog), isLog));
  }
  zAxis->SetTitleOffset(static_cast<float_t>((labels + titleHeight) / unit));
  return true;
}

//**************************************************************************************************
/**
 * The data of a pad that defines its axis frame (without a frame input): the most recently added data marked SetDefinesFrame, else the
 * first; an empty graph is not drawn and has no ranges, so the frame then comes from the first data that is not empty (if there is any).
 */
//**************************************************************************************************
size_t PlotPainter::FrameDataIndex(const vector<shared_ptr<Plot::Pad::Data>>& data, const data_buffer_t& dataBuffer)
{
  auto framePos = std::find_if(data.rbegin(), data.rend(), [](const auto& curData) { return curData->GetDefinesFrame(); });
  size_t frameDataID = (framePos != data.rend()) ? static_cast<size_t>((framePos.base() - 1) - data.begin()) : 0u;
  auto isEmptyGraph = [&](size_t dataID) {
    const auto& curData = data[dataID];
    if (curData->GetType() == "ratio") return false;
    TObject* input = Input(*curData, dataBuffer);
    if (auto* graph2d = dynamic_cast<TGraph2D*>(input)) return graph2d->GetN() == 0;
    auto* graph = dynamic_cast<TGraph*>(input);
    return graph && graph->GetN() == 0 && !dynamic_cast<TGraphPolar*>(graph);
  };
  if (isEmptyGraph(frameDataID)) {
    for (size_t dataID = 0; dataID < data.size(); ++dataID) {
      if (!isEmptyGraph(dataID)) return dataID;
    }
  }
  return frameDataID;
}

//**************************************************************************************************
/**
 * The input of data from the buffer of the inputs (nullptr if it is not there).
 */
//**************************************************************************************************
TObject* PlotPainter::Input(const Plot::Pad::Data& data, const data_buffer_t& dataBuffer)
{
  auto source = dataBuffer.find(data.GetDataSource());
  if (source == dataBuffer.end()) return nullptr;
  auto item = source->second.find(data.GetName() + data.GetDataInfo().GetNameSuffix());
  return (item == source->second.end()) ? nullptr : item->second.get();
}

//**************************************************************************************************
/**
 * Whether the colour axis of a pad is the contents of a 3d histogram (C), not its z axis: its frame is a 3d histogram (or profile),
 * not projected. Known before the pad is painted, for the z groups (see PlotManager::GeneratePlot); GetContentScales gives the pads
 * as painted.
 */
//**************************************************************************************************
bool PlotPainter::IsContentScale(const Plot::Pad& pad, const data_buffer_t& dataBuffer)
{
  const auto& data = pad.GetData();
  const Plot::Pad::Data* frame = (pad.GetFrame()) ? pad.GetFrame().get() : ((data.empty()) ? nullptr : data[FrameDataIndex(data, dataBuffer)].get());
  if (!frame || frame->GetProjInfo()) return false;
  const TObject* input = Input(*frame, dataBuffer);
  return input && input->InheritsFrom(TH3::Class());
}

//**************************************************************************************************
/**
 * Draws a ColorScale() cell of a layout: the colour scale of the z group of the panel it shows, a copy of the scale of the first panel
 * of the group (its range, contours, palette and log setting, which the group shares), along the frames the cell spans.
 */
//**************************************************************************************************
void PlotPainter::DrawColorScaleCell(TCanvas* canvas, Plot& plot, uint8_t padID)
{
  Plot::Pad& cell = plot[padID];
  const uint8_t shown = *cell.GetColorScaleOf();
  if (shown == 0) {
    logger::warning("The colour scale cell (pad {}) stays empty: no panel left of it, name one with ColorScale(padID).", padID);
    return;
  }
  const vector<uint8_t> alone{shown};  // a panel in no z group: the cell shows its own scale
  const vector<uint8_t>* group{&alone};
  for (const auto& members : mSharedAxes['Z']) {
    if (std::find(members.begin(), members.end(), shown) != members.end()) group = &members;
  }
  TPad* memberPad{nullptr};
  TH1* source{nullptr};
  for (uint8_t member : *group) {
    auto frame = mScaleHists.find(member);
    if (frame == mScaleHists.end()) continue;
    memberPad = dynamic_cast<TPad*>(canvas->GetPrimitive(("Pad_" + std::to_string(member)).data()));
    source = frame->second;
    if (memberPad) break;
  }
  if (!memberPad) {
    logger::warning("The colour scale cell (pad {}) stays empty: no panel of the z group of pad {} draws a 2d histogram, graph or function, or a 3d histogram.", padID, shown);
    return;
  }
  const Plot::Pad& defaults = plot[0];
  canvas->cd();
  auto* cellPad = new TPad(("Pad_" + std::to_string(padID)).data(), "", cell.GetXLow().value_or(0.), cell.GetYLow().value_or(0.), cell.GetXUp().value_or(1.), cell.GetYUp().value_or(1.));
  cellPad->SetNumber(padID);
  cellPad->SetFillStyle(0);
  cellPad->SetFrameFillStyle(0);
  cellPad->SetFrameLineColor(TColor::GetColorTransparent(kBlack, 0.f));
  cellPad->SetFrameLineWidth(0);
  cellPad->SetTopMargin(get_first_or(0.1f, cell.GetMarginTop(), defaults.GetMarginTop()));
  cellPad->SetBottomMargin(get_first_or(0.1f, cell.GetMarginBottom(), defaults.GetMarginBottom()));
  // a frame of one pixel at the left edge (ROOT needs one to draw the histogram the scale belongs to)
  const double_t cellWidthPixel = PadSizePixel(cellPad).first;
  cellPad->SetLeftMargin(0.);
  cellPad->SetRightMargin(1. - 1. / std::max(cellWidthPixel, 2.));
  const bool contentScale = mContentScales.count(static_cast<uint8_t>(memberPad->GetNumber())) > 0;
  cellPad->SetLogz(!contentScale && memberPad->GetLogz());  // ROOT colours the boxes of a 3d histogram linearly (the log z of its pad is the binned z)
  cellPad->Draw();
  cellPad->cd();
  // the setup of the panel (its palette): put in front of the scale once it is drawn, since drawing it without SAME clears the pad
  TExec* cellSetup{nullptr};
  if (auto* setup = dynamic_cast<TExec*>(memberPad->GetListOfPrimitives()->FindObject(("setup_pad_" + std::to_string(memberPad->GetNumber())).data()))) {
    cellSetup = new TExec(("setup_pad_" + std::to_string(padID)).data(), setup->GetTitle());
    cellSetup->SetBit(kCanDelete);
    cellSetup->Exec();
  }
  // the histogram of the scale has no cells to paint: its x axis lies beyond the frame (only its z axis is shown); the scale of a 3d histogram
  // (its contents) is that of a 2d histogram with its range, colour levels and the look of its scale (the title after ';' of its z title)
  TH1* scale{};
  const string scaleName = fmt::format("color_scale_pad_{}", padID);
  if (contentScale) {
    scale = new TH2D(scaleName.data(), "", 1, 0., 1., 1, 0., 1.);
    scale->SetDirectory(nullptr);
    scale->SetStats(false);
    scale->SetMinimum(source->GetMinimum());
    scale->SetMaximum(source->GetMaximum());
    if (source->TestBit(TH1::kUserContour)) {
      vector<double_t> levels(static_cast<size_t>(source->GetContour()));
      source->GetContour(levels.data());
      scale->SetContour(static_cast<int32_t>(levels.size()), levels.data());
    } else {
      scale->SetContour(source->GetContour());
    }
    source->GetZaxis()->TAttAxis::Copy(*scale->GetZaxis());
    for (const auto* content : AxisSettings(plot[static_cast<uint8_t>(memberPad->GetNumber())], defaults, 'C')) {
      if (content->GetTitleFont()) scale->GetZaxis()->SetTitleFont(*content->GetTitleFont());
      if (content->GetTitleColor()) scale->GetZaxis()->SetTitleColor(content->GetTitleColor()->ToIndex());
      if (content->GetLabelFont()) scale->GetZaxis()->SetLabelFont(*content->GetLabelFont());
      if (content->GetLabelColor()) scale->GetZaxis()->SetLabelColor(content->GetLabelColor()->ToIndex());
    }
    TString title = source->GetZaxis()->GetTitle();
    title.Remove(0, (title.Index(";") >= 0) ? title.Index(";") + 1 : title.Length());
    scale->GetZaxis()->SetTitle(title);
  } else {
    scale = static_cast<TH1*>(source->Clone(scaleName.data()));
  }
  scale->SetDirectory(nullptr);
  scale->SetBit(kCanDelete);
  delete scale->GetListOfFunctions()->Remove(scale->GetListOfFunctions()->FindObject("palette"));
  scale->GetXaxis()->Set(scale->GetNbinsX(), 1e300, 2e300);
  for (TAxis* axis : {scale->GetXaxis(), scale->GetYaxis()}) {
    axis->SetLabelSize(0.f);
    axis->SetTickLength(0.f);
    axis->SetTitle("");
    axis->SetAxisColor(TColor::GetColorTransparent(kBlack, 0.f));
  }
  // the sizes of the scale are those of the cell (relative to its pad), unless the panel has pixel fonts
  TAxis* zAxis = scale->GetZaxis();
  optional<float_t> titleSize, labelSize, titleOffset, labelOffset, scaleOffset, scaleWidth;
  for (const auto* axis : AxisSettings(cell, defaults, 'Z')) {
    if (axis->GetTitleSize()) titleSize = axis->GetTitleSize();
    if (axis->GetLabelSize()) labelSize = axis->GetLabelSize();
    if (axis->GetTitleOffset()) titleOffset = axis->GetTitleOffset();
    if (axis->GetLabelOffset()) labelOffset = axis->GetLabelOffset();
    if (axis->GetColorScaleOffset()) scaleOffset = axis->GetColorScaleOffset();
    if (axis->GetColorScaleWidth()) scaleWidth = axis->GetColorScaleWidth();
  }
  if (!labelSize) labelSize = get_first(cell.GetDefaultTextSize(), defaults.GetDefaultTextSize());
  if (zAxis->GetTitleFont() % 10 != 3) {
    if (titleSize) zAxis->SetTitleSize(*titleSize);
    if (titleOffset) zAxis->SetTitleOffset(*titleOffset);
  }
  if (zAxis->GetLabelFont() % 10 != 3) {
    if (labelSize) zAxis->SetLabelSize(*labelSize);
    if (labelOffset) zAxis->SetLabelOffset(*labelOffset);
  }
  scale->Draw("COLZ");
  if (cellSetup) cellPad->GetListOfPrimitives()->AddFirst(cellSetup);
  cellPad->Update();
  PlaceZTitle(cellPad, zAxis, {{scale->GetMinimum(), scale->GetMaximum(), static_cast<bool>(cellPad->GetLogz())}});
  if (auto* palette = dynamic_cast<TPave*>(scale->GetListOfFunctions()->FindObject("palette"))) {
    palette->SetX1NDC(scaleOffset.value_or(0.005f));
    palette->SetX2NDC(scaleOffset.value_or(0.005f) + scaleWidth.value_or(0.045f));
    palette->ConvertNDCtoPad();
  }
  cellPad->Modified();
}

//**************************************************************************************************
/**
 * Draws the title of the x axis (y axis) once for a row (column) of touching panels, if all of them show the same one: along the
 * frames of all of them, aligned as TGaxis aligns the title of one axis (at its end, centred with SetTitleCenter) and at the
 * distance TGaxis keeps from the axis. The titles of the panels themselves are removed.
 */
//**************************************************************************************************
void PlotPainter::DrawCommonTitle(TCanvas* canvas, char axisLabel, const vector<uint8_t>& pads, size_t index)
{
  const bool isX = (axisLabel == 'X');
  vector<std::pair<TPad*, TAxis*>> members;
  for (uint8_t padID : pads) {
    auto frame = mFrameHists.find(padID);
    auto* pad = dynamic_cast<TPad*>(canvas->GetPrimitive(("Pad_" + std::to_string(padID)).data()));
    if (frame == mFrameHists.end() || !pad) return;  // a panel without flat frame: every panel keeps its title
    members.emplace_back(pad, (isX != IsHBarFrame(pad, frame->second)) ? frame->second->GetXaxis() : frame->second->GetYaxis());
  }
  const string title = members.front().second->GetTitle();
  for (const auto& [pad, axis] : members) {
    if (title.empty() || title != axis->GetTitle() || axis->GetTitleSize() <= 0.f) return;  // panels with titles of their own keep them
  }
  // the frames of the members in NDC of the canvas, and the distance of the title from the axis in pixels (see TGaxis::PaintAxis)
  double_t low = 1., high = 0., axisPosition = 1.;
  double_t distance{};
  for (const auto& [pad, axis] : members) {
    const double_t x0 = pad->GetAbsXlowNDC() + pad->GetLeftMargin() * pad->GetAbsWNDC();
    const double_t x1 = pad->GetAbsXlowNDC() + (1. - pad->GetRightMargin()) * pad->GetAbsWNDC();
    const double_t y0 = pad->GetAbsYlowNDC() + pad->GetBottomMargin() * pad->GetAbsHNDC();
    const double_t y1 = pad->GetAbsYlowNDC() + (1. - pad->GetTopMargin()) * pad->GetAbsHNDC();
    low = std::min(low, isX ? x0 : y0);
    high = std::max(high, isX ? x1 : y1);
    axisPosition = std::min(axisPosition, isX ? y0 : x0);
    const auto [padWidthPixel, padHeightPixel] = PadSizePixel(pad);
    const bool pixelFont = axis->GetTitleFont() % 10 == 3;
    const double_t charHeight = pixelFont ? axis->GetTitleSize() * mScale : axis->GetTitleSize() * (isX ? padHeightPixel : padWidthPixel);
    double_t offset = axis->GetTitleOffset();
    if (isX) {
      if (offset == 0.) offset = gStyle->GetTitleOffset("X");
      distance = std::max(distance, 1.6 * charHeight * offset);
    } else if (offset == 0.) {
      distance = std::max(distance, charHeight + MaxLabelWidth(pad, axis, axisLabel));
    } else {
      distance = std::max(distance, 1.6 * charHeight * offset);
    }
  }
  TAxis* first = members.front().second;
  TPad* firstPad = members.front().first;
  const bool center = first->TestBit(TAxis::kCenterTitle);
  const double_t along = center ? 0.5 * (low + high) : high;
  const double_t canvasWidthPixel = canvas->GetWw();
  const double_t canvasHeightPixel = canvas->GetWh();
  const double_t across = isX ? axisPosition - distance / canvasHeightPixel : axisPosition - distance / canvasWidthPixel;
  auto* text = new TLatex(isX ? along : across, isX ? across : along, title.data());
  text->SetName(fmt::format("common_title_{}_{}", axisLabel, index).data());
  text->SetNDC();
  // as a pixel font (unscaled, see ApplyScale), so that the size does not depend on the canvas
  const double_t sizePixel = (first->GetTitleFont() % 10 == 3) ? first->GetTitleSize() : first->GetTitleSize() * std::min(PadSizePixel(firstPad).first, PadSizePixel(firstPad).second) / mScale;
  text->SetTextFont(first->GetTitleFont() / 10 * 10 + 3);
  text->SetTextSize(static_cast<float_t>(sizePixel));
  text->SetTextColor(first->GetTitleColor());
  text->SetTextAlign(center ? 22 : 32);
  if (!isX) text->SetTextAngle(90.);
  text->SetBit(kCanDelete);
  canvas->cd();
  text->Draw();
  // the panels lose their own titles (also the copies of their frames that redraw the axes): on the frame the axis found above (with
  // HBAR the histogram's y axis is the horizontal one), on a copy the one holding the title (the copy of an HBAR frame has swapped axes)
  for (const auto& [pad, axis] : members) {
    const string frameName = "axis_hist_pad_" + std::to_string(pad->GetNumber());
    for (TObject* primitive : *pad->GetListOfPrimitives()) {
      auto* hist = dynamic_cast<TH1*>(primitive);
      if (!hist || frameName != hist->GetName()) continue;
      TAxis* own = (hist == axis->GetParent()) ? axis : (isX ? hist->GetXaxis() : hist->GetYaxis());
      TAxis* other = (own == hist->GetXaxis()) ? hist->GetYaxis() : hist->GetXaxis();
      ((hist == axis->GetParent() || title == own->GetTitle() || title != other->GetTitle()) ? own : other)->SetTitle("");
    }
    pad->Modified();
  }
}

//**************************************************************************************************
/**
 * Interpreter command for the setup of a pad (run at every paint): the commands on the histogram of that name in the pad, named h.
 */
//**************************************************************************************************
string PlotPainter::HistCommand(const string& histName, const string& commands)
{
  string name = histName;
  for (size_t pos = name.find_first_of("\\\""); pos != string::npos; pos = name.find_first_of("\\\"", pos + 2)) {
    name.insert(pos, 1, '\\');
  }
  return fmt::format(" if (auto* h = dynamic_cast<TH1*>(gPad->GetPrimitive(\"{}\"))) {{{} }}", name, commands);
}

//**************************************************************************************************
/**
 * Width and height of a pad in pixels.
 */
//**************************************************************************************************
std::pair<double_t, double_t> PlotPainter::PadSizePixel(const TVirtualPad* pad)
{
  return {pad->GetWw() * pad->GetAbsWNDC(), pad->GetWh() * pad->GetAbsHNDC()};
}

//**************************************************************************************************
/**
 * Width and height of a pad in whole pixels, as ROOT takes them to convert a text size relative to the pad (by the smaller of both).
 */
//**************************************************************************************************
std::pair<double_t, double_t> PlotPainter::TextPadSizePixel(const TVirtualPad* pad)
{
  return {pad->XtoPixel(pad->GetX2()), pad->YtoPixel(pad->GetY1())};
}

//**************************************************************************************************
/**
 * Height in pixels of a text in the pad: a pixel size (precision 3) is stored unscaled (ApplyScale multiplies it later),
 * a size relative to the pad already scales with it.
 */
//**************************************************************************************************
double_t PlotPainter::TextHeightPixel(int16_t textFont, float_t textSize, const TVirtualPad* pad) const
{
  if (textFont % 10 > 2) return mScale * textSize;
  const auto [width, height] = TextPadSizePixel(pad);
  return static_cast<float_t>(textSize * std::min(width, height));
}

//**************************************************************************************************
/**
 * Drops the empty groups of a text drawn by ROOT's TMathText (one with a backslash), on which it crashes.
 */
//**************************************************************************************************
void PlotPainter::DropEmptyGroups(string& text)
{
  if (text.find('\\') == string::npos || text.find('{') == string::npos) return;
  static const std::regex emptyGroup("\\{\\s*\\}");
  string result = text;
  while (std::regex_search(result, emptyGroup)) {
    result = std::regex_replace(result, emptyGroup, "");  // again for nested ones: {{}}
  }
  if (result == text) return;
  if (mReportedTexts.insert(text).second) {
    logger::warning("Text '{}' has an empty group {{}} and a backslash, on which ROOT crashes: drawn as '{}'.", text, result);
  }
  text = result;
}

//**************************************************************************************************
/**
 * Reports a setting that has no meaning where it is set, once per plot.
 */
//**************************************************************************************************
void PlotPainter::ReportIgnored(const string& setting, const char* reason)
{
  if (mReportedIgnored.insert(setting).second) logger::warning("{} is ignored: {}.", setting, reason);
}

//**************************************************************************************************
/**
 * Warns about 1d histograms and graphs that lie entirely beyond one edge of the frame of a flat pad (called once the pad is updated,
 * so that its ranges are final): every bin or point with its error bars, a filled histogram down to zero. Data beyond two edges is
 * not warned about, since its lines may cross the frame. One pass over the bins or points of each item.
 */
//**************************************************************************************************
void PlotPainter::WarnIfOutsideFrame(TPad* pad)
{
  const auto user = [](double_t value, bool isLog) { return (isLog) ? std::pow(10., value) : value; };
  const double_t xMin = user(pad->GetUxmin(), pad->GetLogx());
  const double_t xMax = user(pad->GetUxmax(), pad->GetLogx());
  const double_t yMin = user(pad->GetUymin(), pad->GetLogy());
  const double_t yMax = user(pad->GetUymax(), pad->GetLogy());
  for (TObject* obj : *pad->GetListOfPrimitives()) {
    // the data of the pad: named <number>:<name>
    const string name = obj->GetName();
    const size_t colon = name.find(':');
    const bool isData = colon != string::npos && colon != 0 && std::all_of(name.begin(), name.begin() + colon, [](char c) { return std::isdigit(static_cast<unsigned char>(c)); });
    if (!isData) continue;
    bool left{}, right{}, below{}, above{}, inside{};
    const auto add = [&](double_t x1, double_t x2, double_t y1, double_t y2) {
      if (x2 < xMin) {
        left = true;
      } else if (x1 > xMax) {
        right = true;
      } else if (y2 < yMin) {
        below = true;
      } else if (y1 > yMax) {
        above = true;
      } else {
        inside = true;
      }
    };
    if (auto* hist = dynamic_cast<TH1*>(obj); hist && hist->GetDimension() == 1) {
      const bool filled = hist->GetFillStyle() != 0;
      for (int32_t bin = 1; bin <= hist->GetNbinsX() && !inside; ++bin) {
        const double_t content = hist->GetBinContent(bin);
        if (!std::isfinite(content)) continue;
        double_t low = content - hist->GetBinErrorLow(bin);
        double_t high = content + hist->GetBinErrorUp(bin);
        if (filled) {
          low = std::min(low, 0.);
          high = std::max(high, 0.);
        }
        add(hist->GetXaxis()->GetBinLowEdge(bin), hist->GetXaxis()->GetBinUpEdge(bin), low, high);
      }
    } else if (auto* graph = dynamic_cast<TGraph*>(obj); graph && graph->GetFillStyle() == 0) {
      for (int32_t i = 0; i < graph->GetN() && !inside; ++i) {
        const double_t x = graph->GetPointX(i);
        const double_t y = graph->GetPointY(i);
        add(x - std::max(graph->GetErrorXlow(i), 0.), x + std::max(graph->GetErrorXhigh(i), 0.), y - std::max(graph->GetErrorYlow(i), 0.), y + std::max(graph->GetErrorYhigh(i), 0.));
      }
    } else {
      continue;
    }
    if (inside || (left + right + below + above) != 1) continue;
    const char* where = (left) ? "left of" : (right) ? "right of"
                                           : (below) ? "below"
                                                     : "above";
    logger::warning("Data {} lies entirely {} the frame of its pad (the first data, or the data set with SetDefinesFrame, decides the frame): nothing of it is visible.", name.substr(colon + 1), where);
  }
}

//**************************************************************************************************
/**
 * Function to replace placeholders in labels.
 */
//**************************************************************************************************
void PlotPainter::ReplacePlaceholders(string& str, TNamed* rowData, const std::function<TNamed*(uint16_t, const optional<uint8_t>&)>& findData)
{
  // <word>, with a format in brackets (which may contain '>', e.g. [>8.2f]) and a reference :data@pad, in either order
  static const std::regex wordsRegex("<(name|title|source|projection|selection|entries|integral|mean|stddev|maximum|minimum)(?:\\[[^\\]]*\\])?(?::\\d{1,9}(?:@\\d{1,9})?)?(?:\\[[^\\]]*\\])?>");
  static const std::regex formatRegex("\\[[^\\]]*\\]");
  static const std::regex refRegex(":(\\d{1,9})(?:@(\\d{1,9}))?>$");  // after the format is removed
  // what looks like a placeholder with a reference but is none, e.g. a typo as <meen:1> (a word with brackets alone, e.g. <x[cm]>, may be text)
  static const std::regex lookalikeRegex("<[A-Za-z]+(?:\\[[^\\]]*\\])?:\\d+(?:@\\d+)?(?:\\[[^\\]]*\\])?>");
  for (auto it = std::sregex_iterator(str.begin(), str.end(), lookalikeRegex); it != std::sregex_iterator(); ++it) {
    if (!std::regex_match(it->str(), wordsRegex)) logger::warning("Placeholder {} is not valid: known are <name>, <title>, <source>, <projection>, <selection>, <entries>, <integral>, <mean>, <stddev>, <maximum>, <minimum>.", it->str());
  }
  auto wordsBegin = std::sregex_iterator(str.begin(), str.end(), wordsRegex);
  auto wordsEnd = std::sregex_iterator();

  string result;
  size_t lastEnd = 0;  // offset into the original str, just past the previous match

  for (std::sregex_iterator match = wordsBegin; match != wordsEnd; ++match) {
    string matchStr = match->str();
    size_t matchPos = match->position();

    // copy the untouched text since the previous match, then compute the replacement
    result.append(str, lastEnd, matchPos - lastEnd);

    // the data: that of the legend row, or in a text the one named by number (<mean:2>: data 2 of the pad, <mean:1@4>: data 1 of pad 4)
    TNamed* data_ptr = rowData;
    const string withoutFormat = std::regex_replace(matchStr, formatRegex, "");
    if (std::smatch ref; std::regex_search(withoutFormat, ref, refRegex)) {
      data_ptr = nullptr;
      const auto dataID = std::stoul(ref[1].str());
      const auto padNumber = (ref[2].matched) ? std::stoul(ref[2].str()) : 0ul;
      if (!findData) {
        logger::warning("Placeholder {}: the data can be named in texts only, a legend label shows the data of its row.", matchStr);
      } else if (dataID <= std::numeric_limits<uint16_t>::max() && padNumber <= std::numeric_limits<uint8_t>::max()) {
        data_ptr = findData(static_cast<uint16_t>(dataID), (ref[2].matched) ? optional<uint8_t>(static_cast<uint8_t>(padNumber)) : nullopt);
      }
      if (findData && !data_ptr) logger::warning("Placeholder {}: there is no data {} drawn in {}.", matchStr, dataID, (ref[2].matched) ? "pad " + ref[2].str() : "this pad");
    } else if (!data_ptr) {
      logger::warning("Placeholder {} in a text needs the number of its data, e.g. <mean:1> for data 1 of the pad or <mean:1@2> for data 1 of pad 2.", matchStr);
    }
    if (!data_ptr) {
      result += matchStr;  // left as it is
      lastEnd = matchPos + matchStr.size();
      continue;
    }

    string format{};
    if (auto formatIt = std::sregex_iterator(matchStr.begin(), matchStr.end(), formatRegex); formatIt != std::sregex_iterator()) {
      format = formatIt->str();
      format = format.substr(1, format.size() - 2);
    }
    format.erase(remove(format.begin(), format.end(), '%'), format.end());
    format.erase(remove(format.begin(), format.end(), ' '), format.end());
    const bool integerFormat = !format.empty() && (format.back() == 'd' || format.back() == 'i');  // e.g. <entries[d]>
    if (!integerFormat && !(str_contains(format, "e") || str_contains(format, "f") || str_contains(format, "g") || str_contains(format, "E") || str_contains(format, "F") || str_contains(format, "G"))) {
      format = format + "g";
    }
    format = "{:" + format + "}";
    auto formatValue = [&](double_t value) {
      // all placeholders are formatted from a double; integer formats need an integer argument
      return integerFormat ? fmt::format(fmt::runtime(format), std::llround(value)) : fmt::format(fmt::runtime(format), value);
    };

    string replaceStr = matchStr;
    if (str_contains(matchStr, "name") || str_contains(matchStr, "source") || str_contains(matchStr, "projection") || str_contains(matchStr, "selection")) {
      // from the definition of the data (the drawn object is named "<index>:<name>[<request>]:<data source>[<projection>]"):
      // its name, data source, what is filled (variables in the notation of TTree::Draw, "y:x") and which entries or bins
      replaceStr.clear();
      auto definition = mDataDefinitions.find(data_ptr);
      if (definition == mDataDefinitions.end()) {
        // not drawn data: nothing to describe
      } else if (const auto& data = *definition->second; str_contains(matchStr, "name")) {
        replaceStr = data.GetName();
      } else if (str_contains(matchStr, "source")) {
        if (!is_user_defined(data.GetDataSource())) replaceStr = data.GetDataSource();
      } else {
        const auto& dataInfo = data.GetDataInfo();
        const auto& projInfo = data.GetProjInfo();
        // axes of the histogram before a projection: the variables of a tree request, else the axis titles of the input histogram,
        // and x, y, z and "axis 3", ... (THn) where there is no title
        auto titles = mProjectionAxisTitles.find(data_ptr);
        if (auto source = mSourceHists.find(data_ptr); titles == mProjectionAxisTitles.end() && source != mSourceHists.end()) {
          titles = mProjectionAxisTitles.find(source->second.get());
        }
        auto axisName = [&](uint8_t axis) -> string {
          if (axis < dataInfo.dataDims.size()) return dataInfo.dataDims[axis].var;
          if (titles != mProjectionAxisTitles.end() && axis < titles->second.size() && !titles->second[axis].empty()) return titles->second[axis];
          return (axis < 3) ? string(1, static_cast<char>('x' + axis)) : "axis " + std::to_string(axis);
        };
        if (str_contains(matchStr, "projection")) {
          if (projInfo && !projInfo->dims.empty()) {
            if (projInfo->isProfile && *projInfo->isProfile) replaceStr = "prof. ";
            for (auto axis = projInfo->dims.rbegin(); axis != projInfo->dims.rend(); ++axis) {
              replaceStr += ((axis == projInfo->dims.rbegin()) ? "" : ":") + axisName(*axis);
            }
          } else if (!dataInfo.dataDims.empty()) {
            if (dataInfo.isProfileNoScatter && *dataInfo.isProfileNoScatter) replaceStr = "prof. ";
            const bool isScatter = dataInfo.isProfileNoScatter && !*dataInfo.isProfileNoScatter;
            const size_t nDims = (isScatter && dataInfo.dataDims.size() != 3) ? 2 : dataInfo.dataDims.size();  // a scatter: x and y (and z), not their errors
            for (size_t dim = nDims; dim-- > 0;) {
              replaceStr += ((dim == nDims - 1) ? "" : ":") + dataInfo.dataDims[dim].var;
            }
          }
          if (dataInfo.weight && !replaceStr.empty()) replaceStr += " (" + *dataInfo.weight + ")";
        } else {
          auto join = [](const vector<string>& items, const string& separator) {
            string joined;
            for (const auto& item : items) {
              joined += ((joined.empty()) ? "" : separator) + item;
            }
            return joined;
          };
          vector<string> parts;
          if (dataInfo.filters) {
            for (const auto& filter : *dataInfo.filters) {
              parts.push_back((dataInfo.filters->size() > 1 && str_contains(filter, "||")) ? "(" + filter + ")" : filter);
            }
          }
          if (!parts.empty()) parts = {join(parts, " && ")};
          if (dataInfo.entries.max) parts.push_back(fmt::format("entries [{}, {})", dataInfo.entries.min.value_or(0), *dataInfo.entries.max));
          if (projInfo) {
            for (const auto& [axis, low, high] : projInfo->ranges) {
              if (high < low) continue;  // all bins (ProfileX() and the like)
              if (projInfo->isUserCoord && *projInfo->isUserCoord) {
                parts.push_back(fmt::format("{} < {} < {}", low, axisName(axis), high));
              } else {
                parts.push_back(fmt::format("bins {}-{} of {}", static_cast<int32_t>(low), static_cast<int32_t>(high), axisName(axis)));
              }
            }
          }
          replaceStr = join(parts, ", ");
        }
      }
    } else if (str_contains(matchStr, "title")) {
      replaceStr = data_ptr->GetTitle();
    } else if (data_ptr->InheritsFrom(TH1::Class())) {
      try {
        if (str_contains(matchStr, "entries")) {
          replaceStr = formatValue(static_cast<TH1*>(data_ptr)->GetEntries());
        } else if (str_contains(matchStr, "integral")) {
          replaceStr = formatValue(static_cast<TH1*>(data_ptr)->Integral());
        } else if (auto* poly = dynamic_cast<TH2Poly*>(data_ptr); poly && (str_contains(matchStr, "mean") || str_contains(matchStr, "stddev"))) {
          // from the polygons at their centres: ROOT's statistics of a TH2Poly read the cells of its TH2 grid, which hold nothing
          double_t sumW{}, sumWX{}, sumWX2{};
          for (int32_t bin = 1; bin <= poly->GetNumberOfBins(); ++bin) {
            auto* polyBin = static_cast<TH2PolyBin*>(poly->GetBins()->At(bin - 1));
            const double_t x = 0.5 * (polyBin->GetXMin() + polyBin->GetXMax());
            const double_t w = polyBin->GetContent();
            sumW += w;
            sumWX += w * x;
            sumWX2 += w * x * x;
          }
          const double_t mean = (sumW != 0.) ? sumWX / sumW : 0.;
          replaceStr = formatValue(str_contains(matchStr, "mean") ? mean : ((sumW != 0.) ? std::sqrt(std::max(sumWX2 / sumW - mean * mean, 0.)) : 0.));
        } else if (str_contains(matchStr, "mean")) {
          replaceStr = formatValue(static_cast<TH1*>(data_ptr)->GetMean());
        } else if (str_contains(matchStr, "stddev")) {
          replaceStr = formatValue(static_cast<TH1*>(data_ptr)->GetStdDev());
        } else if (str_contains(matchStr, "maximum") || str_contains(matchStr, "minimum")) {
          // from the bins: GetMaximum()/GetMinimum() would return the frame limits stored on the drawn histogram
          auto* hist = static_cast<TH1*>(data_ptr);
          const bool isMax = str_contains(matchStr, "maximum");
          if (auto* poly = dynamic_cast<TH2Poly*>(hist)) {
            // the polygons, not the cells of the TH2 grid (GetMaximumBin)
            double_t extreme = (isMax) ? -std::numeric_limits<double_t>::infinity() : std::numeric_limits<double_t>::infinity();
            for (int32_t bin = 1; bin <= poly->GetNumberOfBins(); ++bin) {
              extreme = (isMax) ? std::max(extreme, poly->GetBinContent(bin)) : std::min(extreme, poly->GetBinContent(bin));
            }
            replaceStr = formatValue(std::isfinite(extreme) ? extreme : 0.);
          } else {
            replaceStr = formatValue(hist->GetBinContent((isMax) ? hist->GetMaximumBin() : hist->GetMinimumBin()));
          }
        }
      } catch (const fmt::format_error& e) {
        logger::error("Incompatible format string in {}: {}.", matchStr, e.what());
        replaceStr = matchStr;
      }
    } else if (auto* graph = dynamic_cast<TGraph*>(data_ptr); graph && (str_contains(matchStr, "entries") || (graph->GetN() > 0 && !str_contains(matchStr, "integral") && !str_contains(matchStr, "mean") && !str_contains(matchStr, "stddev")))) {
      // for graphs only the unambiguous ones: number of points and smallest / largest y value
      try {
        if (str_contains(matchStr, "entries")) {
          replaceStr = formatValue(static_cast<double_t>(graph->GetN()));  // as double like the histogram entries, so the same formats apply
        } else if (str_contains(matchStr, "maximum") || str_contains(matchStr, "minimum")) {
          // the points inside the frame (outside of it they are not drawn)
          const bool isMax = str_contains(matchStr, "maximum");
          const auto range = mGraphFrameRangeX.find(graph);
          double_t extreme = (isMax) ? -std::numeric_limits<double_t>::infinity() : std::numeric_limits<double_t>::infinity();
          for (int32_t i = 0; i < graph->GetN(); ++i) {
            const double_t x = graph->GetX()[i];
            if (range != mGraphFrameRangeX.end() && (x < range->second.first || x > range->second.second)) continue;
            extreme = (isMax) ? std::max(extreme, graph->GetY()[i]) : std::min(extreme, graph->GetY()[i]);
          }
          replaceStr = formatValue(std::isfinite(extreme) ? extreme : 0.);
        }
      } catch (const fmt::format_error& e) {
        logger::error("Incompatible format string in {}: {}.", matchStr, e.what());
        replaceStr = matchStr;
      }
    } else if (auto* graph2d = dynamic_cast<TGraph2D*>(data_ptr); graph2d && (str_contains(matchStr, "entries") || (graph2d->GetN() > 0 && (str_contains(matchStr, "maximum") || str_contains(matchStr, "minimum"))))) {
      // as for graphs: the number of points and the smallest / largest z of the points inside the frame
      try {
        if (str_contains(matchStr, "entries")) {
          replaceStr = formatValue(static_cast<double_t>(graph2d->GetN()));
        } else {
          const bool isMax = str_contains(matchStr, "maximum");
          const auto range = mGraph2DFrameRange.find(graph2d);
          double_t extreme = (isMax) ? -std::numeric_limits<double_t>::infinity() : std::numeric_limits<double_t>::infinity();
          for (int32_t i = 0; i < graph2d->GetN(); ++i) {
            const double_t x = graph2d->GetX()[i];
            const double_t y = graph2d->GetY()[i];
            if (range != mGraph2DFrameRange.end() && (x < range->second[0] || x > range->second[1] || y < range->second[2] || y > range->second[3])) continue;
            extreme = (isMax) ? std::max(extreme, graph2d->GetZ()[i]) : std::min(extreme, graph2d->GetZ()[i]);
          }
          replaceStr = formatValue(std::isfinite(extreme) ? extreme : 0.);
        }
      } catch (const fmt::format_error& e) {
        logger::error("Incompatible format string in {}: {}.", matchStr, e.what());
        replaceStr = matchStr;
      }
    } else {
      const string name = data_ptr->GetName();
      logger::warning("Placeholder {} is not available for {} {}.", matchStr, data_ptr->ClassName(), name.substr(name.find(':') + 1));  // without the "<index>:" prefix
    }

    result += replaceStr;
    lastEnd = matchPos + matchStr.size();
    // a placeholder that resolves to nothing takes one separating space with it, so "<name> <projection> <selection>" ends without a space
    if (replaceStr.empty()) {
      if (!result.empty() && result.back() == ' ' && (lastEnd >= str.size() || str[lastEnd] == ' ')) {
        result.pop_back();
      } else if (result.empty() && lastEnd < str.size() && str[lastEnd] == ' ') {
        ++lastEnd;
      }
    }
  }
  result.append(str, lastEnd, string::npos);  // copy whatever remains after the last match

  str = std::move(result);
}

//**************************************************************************************************
/**
 * The red, green and blue of nColors spread evenly over a gradient (the same interpolation as in GradientPaletteCommand).
 */
//**************************************************************************************************
vector<tuple<double_t, double_t, double_t>> PlotPainter::GradientRGB(int32_t nColors, const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints)
{
  if (rgbEndpoints.size() < 2) {
    logger::error("A colour gradient needs at least two endpoints, got {}.", rgbEndpoints.size());
    return {};
  }
  if (nColors < 1) {
    logger::error("Number of gradient colours must be positive, got {}.", nColors);
    return {};
  }
  vector<tuple<double_t, double_t, double_t>> rgb;
  rgb.reserve(nColors);
  for (int32_t i = 0; i < nColors; ++i) {
    const double_t x = (nColors > 1) ? static_cast<double_t>(i) / (nColors - 1) : 0.;
    size_t k = 0;
    while (k < rgbEndpoints.size() - 2 && x > std::get<3>(rgbEndpoints[k + 1]))
      ++k;
    const auto& [r1, g1, b1, s1] = rgbEndpoints[k];
    const auto& [r2, g2, b2, s2] = rgbEndpoints[k + 1];
    const double_t t = (s2 > s1) ? std::clamp((x - s1) / (s2 - s1), 0., 1.) : 0.;  // the end colours beyond the first and last stop
    rgb.emplace_back(r1 + t * (r2 - r1), g1 + t * (g2 - g1), b1 + t * (b2 - b1));
  }
  return rgb;
}

//**************************************************************************************************
/**
 * Helper to generate nColors between specified rgb endpoints.
 */
//**************************************************************************************************
vector<int16_t> PlotPainter::GenerateGradientColors(int32_t nColors, const vector<tuple<float_t, float_t, float_t, float_t>>& rgbEndpoints, float_t alpha)
{
  // colours are looked up by value, so identical gradients share them
  vector<int16_t> gradientColors;
  for (const auto& [r, g, b] : GradientRGB(nColors, rgbEndpoints)) {
    gradientColors.push_back(static_cast<int16_t>(TColor::GetColor(static_cast<float_t>(r), static_cast<float_t>(g), static_cast<float_t>(b), alpha)));
  }
  return gradientColors;
}

}  // end namespace SciRooPlot
