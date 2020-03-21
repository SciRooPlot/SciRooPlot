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

// std headers
#include <iostream>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>

// boost headers
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>

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

// some preprocessor macros for logging and debugging
#define LOG(s) cout << s << endl;
#define LOGF(s, ...) printf(s, __VA_ARGS__); cout << endl;
#define ERROR(s) cout << "ERROR: " << s << endl;
#define ERRORF(s, ...)  cout << "ERROR: "; printf(s, __VA_ARGS__); cout << endl;
#define HERE cout << "This is line " << __LINE__ << \
" in function " << __FUNCTION__ << " (" << __FILE__ << ")" << endl;


namespace PlottingFramework {


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

}

#endif /* PlottingFramework_h */
