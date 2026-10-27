"""Samples 17 colours of each of ROOT's predefined palettes for the Styles page: [[name, number, [hex, ...]], ...].
Usage: extract_palettes.py > palettes.json"""
import json
import sys

import ROOT

NAMES = ("kDeepSea kGreyScale kDarkBodyRadiator kBlueYellow kRainBow kInvertedDarkBodyRadiator kBird kCubehelix kGreenRedViolet "
         "kBlueRedYellow kOcean kColorPrintableOnGrey kAlpine kAquamarine kArmy kAtlantic kAurora kAvocado kBeach kBlackBody "
         "kBlueGreenYellow kBrownCyan kCMYK kCandy kCherry kCoffee kDarkRainBow kDarkTerrain kFall kFruitPunch kFuchsia kGreyYellow "
         "kGreenBrownTerrain kGreenPink kIsland kLake kLightTemperature kLightTerrain kMint kNeon kPastel kPearl kPigeon kPlum kRedBlue "
         "kRose kRust kSandyTerrain kSienna kSolar kSouthWest kStarryNight kSunset kTemperatureMap kThermometer kValentine "
         "kVisibleSpectrum kWaterMelon kCool kCopper kGistEarth kViridis kCividis").split()
out = []
for name in NAMES:
    number = getattr(ROOT, name, None)
    if number is None:
        continue
    ROOT.gStyle.SetPalette(int(number))
    palette = ROOT.TColor.GetPalette()
    n = palette.GetSize()
    out.append([name, int(number), [ROOT.gROOT.GetColor(palette[min(n - 1, round(i * (n - 1) / 16))]).AsHexString() for i in range(17)]])
json.dump(out, sys.stdout, separators=(",", ":"))
