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


#include "MultDepSpec.h"

namespace MultDepSpec{

const double range::minPt = 0.5;
const double range::maxPt = 50.;
const int range::minMult = 0;
const int range::maxMult = 100;

dataStyle pp_2TeV
{
  {
  {data, {"pp_2TeV", colors::green, markers::diamond, 35}},
  {mc, {"pp_2TeV", colors::green, markers::circle_open, 35}},
  {pythia6, {"pp_2TeV", colors::green, markers::circle_open, 35}},
  {pythia, {"Models:pythia/pp_2TeV", colors::green, markers::circle_open, 35}},
  {epos_lhc, {"Models:epos_lhc/pp_2TeV", colors::green, markers::circle_open, 35}},
  {geom_scaling, {"Models:geom_scaling/pp_2TeV", colors::green, markers::circle_open, 35}},
  }
};
dataStyle pp_5TeV
{
  {
  {data, {"pp_5TeV", colors::blue, markers::cross, 60}},
  {mc, {"pp_5TeV", colors::blue, markers::circle_open, 60}},
  {pythia, {"Models:pythia/pp_5TeV", colors::blue, markers::circle_open, 60}},
  {epos_lhc, {"Models:epos_lhc/pp_5TeV", colors::blue, markers::circle_open, 60}},
  {epos, {"Models:epos/pp_5TeV", colors::blue, markers::circle_open, 60}},
  {geom_scaling, {"Models:geom_scaling/pp_5TeV", colors::blue, markers::circle_open, 60}},
  }
};
dataStyle pp_7TeV
{
  {
  {data, {"pp_7TeV", colors::magenta, markers::circle, 60}},
  {mc, {"pp_7TeV", colors::magenta, markers::circle_open, 60}},
  {pythia, {"Models:pythia/pp_7TeV", colors::magenta, markers::circle_open, 60}},
  {epos_lhc, {"Models:epos_lhc/pp_7TeV", colors::magenta, markers::circle_open, 60}},
  {geom_scaling, {"Models:geom_scaling/pp_7TeV", colors::magenta, markers::circle_open, 60}},
  }
};
dataStyle pp_13TeV
{
  {
  {data, {"pp_13TeV", colors::red, markers::square, 75}},
  {mc, {"pp_13TeV", colors::red, markers::circle_open, 75}},
  {pythia, {"Models:pythia/pp_13TeV", colors::red, markers::circle_open, 75}},
  {epos_lhc, {"Models:epos_lhc/pp_13TeV", colors::red, markers::circle_open, 75}},
  {geom_scaling, {"Models:geom_scaling/pp_13TeV", colors::red, markers::circle_open, 75}},
  }
};
dataStyle pPb_5TeV
{
  {
  {data, {"pPb_5TeV", colors::magenta, markers::circle, 100}},
  {mc, {"pPb_5TeV", colors::magenta, markers::circle_open, 100}},
  {dpmjet, {"pPb_5TeV", colors::magenta, markers::circle_open, 100}},
  {angantyr, {"Models:pythia_angantyr/pPb_5TeV", colors::magenta, markers::circle_open, 100}},
  {epos_lhc, {"Models:epos_lhc/pPb_5TeV", colors::magenta, markers::circle_open, 100}},
  {epos, {"Models:epos/pPb_5TeV", colors::magenta, markers::circle_open, 100}},
  {geom_scaling, {"Models:geom_scaling/pPb_5TeV", colors::magenta, markers::circle_open, 100}},
  }
};
dataStyle pPb_8TeV
{
  {
  {data, {"pPb_8TeV", colors::yellow, markers::circle, 90}},
  {mc, {"pPb_8TeV", colors::yellow, markers::circle_open, 90}},
  {epos_lhc, {"pPb_8TeV", colors::yellow, markers::circle_open, 90}},
  {geom_scaling, {"Models:geom_scaling/pPb_8TeV", colors::yellow, markers::circle_open, 90}},
  }
};
dataStyle PbPb_2TeV
{
  {
  }
};
dataStyle PbPb_5TeV
{
  {
  {data, {"PbPb_5TeV", colors::red, markers::cross, 3000}},
  {mc, {"PbPb_5TeV", colors::red, markers::circle_open, 3000}},
  {hijing, {"PbPb_5TeV", colors::red, markers::circle_open, 3000}},
  {angantyr, {"Models:pythia_angantyr/PbPb_5TeV", colors::red, markers::circle_open, 3000}},
  {epos_lhc, {"Models:epos_lhc/PbPb_5TeV", colors::red, markers::circle_open, 3000}},
  {epos, {"Models:epos/PbPb_5TeV", colors::red, markers::circle_open, 3000}},
  }
};
dataStyle XeXe_5TeV
{
  {
  {data, {"XeXe_5TeV", colors::green, markers::circle, 2000}},
  {mc, {"XeXe_5TeV", colors::green, markers::circle_open, 2000}},
  {hijing, {"XeXe_5TeV", colors::green, markers::circle_open, 2000}},
  }
};
} // end namespace MultDepSpec
