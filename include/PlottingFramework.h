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

#ifndef PlottingFramework_h
#define PlottingFramework_h

#define DEBUG_LVL 2   // < 2: no debug, < 1: no warnings, < 0 no errors
#define COUT_LVL 2    // < 2: no log,   < 1: no info,     < 0 no print

// std headers
#include <iostream>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <exception>

// boost headers
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

// fmt headers
#include <fmt/core.h>

// root headers
#include "TCanvas.h"
#include "TLegendEntry.h"
#include "TFile.h"
#include "TIterator.h"
#include "TKey.h"
#include "TObjArray.h"
#include "TObject.h"
#include "TStyle.h"
#include "TGraph.h"
#include "TF1.h"
#include "TH1.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TH3D.h"
#include "THn.h"
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
#include "TGraph.h"
#include "TGraphErrors.h"
#include "TPaletteAxis.h"
#include "TView.h"
#include "TApplication.h"
#include "TGWindow.h"
#include "TRootCanvas.h"



// some preprocessor macros for logging, printing and debugging
#define DEBUG(s, ...) {fmt::print(stderr, "\033[1;36m[ DEBUG   ]\033[0m "); fmt::print(stderr, s, ##__VA_ARGS__); fmt::print(stderr, "\n");}
#define WARNING(s, ...) {fmt::print(stderr, "\033[1;33m[ WARNING ]\033[0m "); fmt::print(stderr, s, ##__VA_ARGS__); fmt::print(stderr, "\n");}
#define ERROR(s, ...) {fmt::print(stderr, "\033[1;31m[ ERROR   ]\033[0m "); fmt::print(stderr, s, ##__VA_ARGS__); fmt::print(stderr, "\n");}

#define LOG(s, ...) {fmt::print("\033[1;32m[ LOG     ]\033[0m "); fmt::print(s, ##__VA_ARGS__); fmt::print("\n");}
#define INFO(s, ...) {fmt::print("\033[1;37m[ INFO    ]\033[0m "); fmt::print(s, ##__VA_ARGS__); fmt::print("\n");}

#define PRINT(s, ...) {fmt::print(s, ##__VA_ARGS__); fmt::print("\n");}
#define PRINT_INLINE(s, ...) {fmt::print(s, ##__VA_ARGS__);}
#define PRINT_SEPARATOR {PRINT(fmt::format("{:-<{}}", "-", 100));}
#define HERE {fmt::print("\033[1;33m[  HERE   ]\033[0m Line {} in function {} ({})", __LINE__, __FUNCTION__, __FILE__); fmt::print("\n");}

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

using boost::property_tree::ptree;
using boost::property_tree::write_json;
using boost::property_tree::read_json;
using boost::property_tree::write_xml;
using boost::property_tree::read_xml;

const string gNameGroupSeparator = "_IN_";

} // end namespace PlottingFramework
#endif /* PlottingFramework_h */
