// Plotting Framework
//
// Copyright (C) 2019-2020  Mario Krüger
// Contact: mario.kruger@cern.ch
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

#ifndef PlottingFramework_h
#define PlottingFramework_h

#define DEBUG_LVL 2   // < 2: no debug, < 1: no warnings, < 0 no errors
#define COUT_LVL 2    // < 2: no log,   < 1: no info,     < 0 no print

// std headers
#include <cstdint>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <exception>
#include <regex>

// boost headers
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

// fmt headers
#include <fmt/core.h>

// root headers
#include "TH1.h"
#include "TH2.h"
#include "TProfile.h"
#include "TProfile2D.h"
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TGraph2D.h"
#include "TGraph2DErrors.h"
#include "TGraphSmooth.h"
#include "TF1.h"
#include "TF2.h"

#include "TFile.h"
#include "TCanvas.h"
#include "TFrame.h"
#include "TLegendEntry.h"
#include "TIterator.h"
#include "TKey.h"
#include "TObjArray.h"
#include "TObject.h"
#include "TStyle.h"
#include "TSystem.h"
#include <TROOT.h>
#include <TObjectTable.h>
#include "TExec.h"
#include "TTimeStamp.h"
#include "TLatex.h"
#include "TPaveText.h"
#include "TLegend.h"
#include "TText.h"
#include "TLatex.h"
#include "TError.h"
#include "TSpline.h"
#include "TPaletteAxis.h"
#include "TView.h"
#include "TApplication.h"
#include "TGWindow.h"
#include "TRootCanvas.h"


// some preprocessor macros for logging, printing and debugging
#define DEBUG(s, ...) {fmt::print(stderr, "\033[1;36m[ DEBU ]\033[0m "); fmt::print(stderr, s, ##__VA_ARGS__); fmt::print(stderr, "\n");}
#define WARNING(s, ...) {fmt::print(stderr, "\033[1;33m[ WARN ]\033[0m "); fmt::print(stderr, s, ##__VA_ARGS__); fmt::print(stderr, "\n");}
#define ERROR(s, ...) {fmt::print(stderr, "\033[1;31m[ ERR  ]\033[0m "); fmt::print(stderr, s, ##__VA_ARGS__); fmt::print(stderr, "\n");}

#define LOG(s, ...) {fmt::print("\033[1;32m[ LOG  ]\033[0m "); fmt::print(s, ##__VA_ARGS__); fmt::print("\n");}
#define INFO(s, ...) {fmt::print("\033[1;37m[ INFO ]\033[0m "); fmt::print(s, ##__VA_ARGS__); fmt::print("\n");}

#define PRINT(s, ...) {fmt::print("         "); fmt::print(s, ##__VA_ARGS__); fmt::print("\n");}
#define PRINT_INLINE(s, ...) {fmt::print(s, ##__VA_ARGS__);}
#define PRINT_SEPARATOR {PRINT(fmt::format("{:-<{}}", "-", 80));}
#define HERE {fmt::print("\033[1;33m[ ---> ]\033[0m Line {} in function {} ({})", __LINE__, __FUNCTION__, __FILE__); fmt::print("\n");}

// Debug suppression levels
#if DEBUG_LVL < 2
  #undef DEBUG
  #define DEBUG(s, ...) ;
#endif
#if DEBUG_LVL < 1
  #undef WARNING
  #define WARNING(s, ...) ;
#endif
#if DEBUG_LVL < 0
  #undef ERROR
  #define ERROR(s, ...) ;
#endif

// Output stream suppression levels
#if COUT_LVL < 2
  #undef LOG
  #define LOG(s, ...) ;
#endif
#if COUT_LVL < 1
  #undef INFO
  #define INFO(s, ...) ;
#endif
#if COUT_LVL < 0
  #undef PRINT
  #define PRINT(s, ...) ;
  #undef PRINT_INLINE
  #define PRINT_INLINE(s, ...) ;
#endif

namespace PlottingFramework
{

// aliases
using std::cout;
using std::endl;
using std::flush;
using std::string;
using std::vector;
using std::tuple;
using std::pair;
using std::map;
using std::set;
using std::array;
using std::shared_ptr;
using std::variant;
using std::optional;

// supported input data types
using data_ptr_t = variant<TH1*, TH2*, TGraph*, TGraph2D*, TProfile*, TProfile2D*, TF2*, TF1*>;

using data_ptr_t_1d = variant<TH1*, TGraph*, TProfile*, TF1*>;
using data_ptr_t_2d = variant<TH2*, TGraph2D*, TProfile2D*, TF2*>;
using data_ptr_t_hist = variant<TH1*, TH2*, TProfile*, TProfile2D*>;
using data_ptr_t_hist_1d = variant<TH1*, TProfile*>;
using data_ptr_t_hist_2d = variant<TH2*, TProfile2D*>;
using data_ptr_t_graph = variant<TGraph*, TGraph2D*>;
using data_ptr_t_graph_1d = variant<TGraph*>;
using data_ptr_t_graph_2d = variant<TGraph2D*>;
using data_ptr_t_func = variant<TF1*, TF2*>;
using data_ptr_t_func_1d = variant<TF1*>;
using data_ptr_t_func_2d = variant<TF2*>;


using boost::property_tree::ptree;
using boost::property_tree::write_json;
using boost::property_tree::read_json;
using boost::property_tree::write_xml;
using boost::property_tree::read_xml;

const string gNameGroupSeparator = "_IN_";

inline constexpr bool str_contains(const string &str1, const string &str2)
{
  return (str1.find(str2) != string::npos);
}


enum drawing_options_t : uint8_t
{
  // 1d options
  points = 0,
  points_xerr,
  points_endcaps,
  points_line,
  line,
  curve,
  band,
  band_line,
  hist,
  hist_no_borders,
  fit,
  bar,
  area,
  area_curve,
  area_line,
  boxes,
  boxes_only,
  stars,
  // 2d options
  colz,
  surf,
};


const map<drawing_options_t, string> defaultDrawingOpions_Hist
{
  { points, "X0 EP" },
  { points_xerr, "EP" },
  { points_endcaps, "E1" },
  { curve, "HIST C" },
  { line, "HIST L" },
  { bar, "HIST B" },
  { boxes, "E2" },
  { band, "E6" },
  { band_line, "E5" },
  { area, "HIST F" },
  { area_curve, "HIST CF" },
  { area_line, "HIST LF" },
  { hist, "HIST" },
  { bar, "HIST B" },
  { hist_no_borders, "HIST ][" },
  { stars, "*H" },
};

const map<drawing_options_t, string> defaultDrawingOpions_Graph
{
  { points, "P Z" },
  { points_line, "P Z L" },
  { points_endcaps, "P" },
  { curve, "X C" },
  { line, "X L" },
  { bar, "X B" },
  { boxes, "P2" },
  { band, "4" },
  { band_line, "3" },
  { area, "X CF" },
  { area_line, "X LC" },
  { boxes_only, "2" },
};

} // end namespace PlottingFramework
#endif /* PlottingFramework_h */
