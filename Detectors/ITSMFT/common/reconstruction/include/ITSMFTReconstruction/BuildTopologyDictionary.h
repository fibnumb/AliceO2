// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file BuildTopologyDictionary.h
/// \brief Definition of the BuildTopologyDictionary class.
///
/// \author Luca Barioglio, University and INFN of Torino
///
/// Short BuildTopologyDictionary descritpion
///
/// This class is used to build the dictionary of topologies, storing the information
/// concerning topologies with their own entry and groups of rare topologies
///

#ifndef ALICEO2_ITSMFT_BUILDTOPOLOGYDICTIONARY_H
#define ALICEO2_ITSMFT_BUILDTOPOLOGYDICTIONARY_H
#include <algorithm>
#include <array>
#include <map>
#include <unordered_map>
#include "ITSMFTBase/SegmentationAlpide.h"
#include "DataFormatsITSMFT/ClusterTopology.h"
#include "DataFormatsITSMFT/TopologyDictionary.h"

#define _HISTO_ // in order to have a histogram with the ditribution of groupIDs

#ifdef _HISTO_
#include "TH1F.h"
#endif

namespace o2
{
namespace ITSMFT
{
struct TopologyInfo {
  int mSizeX;
  int mSizeZ;
  float mCOGx;
  float mCOGz;
  float mXmean;
  float mXsigma2;
  float mZmean;
  float mZsigma2;
  int mNpixels;
};

class BuildTopologyDictionary
{
 public:
#ifdef _HISTO_
  TH1F mHdist; // Distribution of groupIDs
#endif

  BuildTopologyDictionary();

  void accountTopology(const ClusterTopology& cluster, float dX, float dZ);
  void setNGroups(unsigned int ngr); // set number of groups
  void setThreshold(double thr);
  void setThresholdCumulative(double cumulative); // Considering the integral
  void groupRareTopologies();
  friend std::ostream& operator<<(std::ostream& os, const BuildTopologyDictionary& BD);
  void printDictionary(std::string fname);
  void printDictionaryBinary(std::string fname);

  int getTotClusters() const { return mTotClusters; }
  int getNotInGroups() const { return mNotInGroups; }
  int getNGroups() const { return mNumberOfGroups; }

 private:
  TopologyDictionary mDictionary; ///< Dictionary of topologies
  std::map<unsigned long, std::pair<ClusterTopology, unsigned long>>
    mTopologyMap; ///< Temporary map of type <hash,<topology,counts>>
  std::vector<std::pair<unsigned long, unsigned long>> mTopologyFrequency; ///< <freq,hash>, needed to define threshold
  int mTotClusters;
  int mNumberOfGroups;
  int mNotInGroups;
  double mFrequencyThreshold;

  std::unordered_map<long unsigned, TopologyInfo> mMapInfo;

  ClassDefNV(BuildTopologyDictionary, 1);
};
} // namespace ITSMFT
} // namespace o2
#endif
