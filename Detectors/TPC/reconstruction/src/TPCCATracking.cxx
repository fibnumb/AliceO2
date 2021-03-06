// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file TPCCATracking.cxx
/// \author David Rohr

#include "TPCReconstruction/TPCCATracking.h"

#include "FairLogger.h"
#include "ReconstructionDataFormats/Track.h"
#include "SimulationDataFormat/MCCompLabel.h"
#include "SimulationDataFormat/MCTruthContainer.h"
#include "TChain.h"
#include "TClonesArray.h"
#include "TPCBase/Mapper.h"
#include "TPCBase/PadRegionInfo.h"
#include "TPCBase/ParameterDetector.h"
#include "TPCBase/ParameterElectronics.h"
#include "TPCBase/ParameterGas.h"
#include "TPCBase/Sector.h"

// The AliHLTTPCCAO2Interface.h needs certain macro definitions.
// The AliHLTTPCCAO2Interface will only be included once here, all O2 TPC tracking will run through this TPCCATracking
// class.
// Therefore, the macros are defined here and not globally, in order not to pollute the global namespace
#define HLTCA_STANDALONE
#define HLTCA_TPC_GEOMETRY_O2

// This class is only a wrapper for the actual tracking contained in the HLT O2 CA Tracking library.
#include "AliHLTTPCCAO2Interface.h"

using namespace o2::TPC;
using namespace o2;
using namespace o2::dataformats;

using MCLabelContainer = MCTruthContainer<MCCompLabel>;

TPCCATracking::TPCCATracking() : mTrackingCAO2Interface(), mClusterData_UPTR(), mClusterData(nullptr) {}
TPCCATracking::~TPCCATracking() { deinitialize(); }
int TPCCATracking::initialize(const char* options)
{
  mTrackingCAO2Interface.reset(new AliHLTTPCCAO2Interface);
  int retVal = mTrackingCAO2Interface->Initialize(options);
  if (retVal) {
    mTrackingCAO2Interface.reset();
  } else {
    mClusterData_UPTR.reset(new AliHLTTPCCAClusterData[Sector::MAXSECTOR]);
    mClusterData = mClusterData_UPTR.get();
  }
  return (retVal);
}

void TPCCATracking::deinitialize()
{
  mTrackingCAO2Interface.reset();
  mClusterData_UPTR.reset();
  mClusterData = nullptr;
}

int TPCCATracking::convertClusters(TChain* inputClustersChain, const std::vector<o2::TPC::Cluster>* inputClustersArray,
                                   ClusterNativeAccessFullTPC& outputClusters,
                                   std::unique_ptr<ClusterNative[]>& clusterMemory)
{
  if (inputClustersChain && inputClustersArray) {
    LOG(FATAL) << "Internal error, must not pass in both TChain and TClonesArray of clusters\n";
  }
  int numChunks;
  if (inputClustersChain) {
    inputClustersChain->SetBranchAddress("TPCClusterHW", &inputClustersArray);
    numChunks = inputClustersChain->GetEntries();
  } else {
    numChunks = 1;
  }

  Mapper& mapper = Mapper::instance();
  for (int iter = 0; iter < 2; iter++) {
    for (int i = 0; i < Sector::MAXSECTOR; i++) {
      for (int j = 0; j < Constants::MAXGLOBALPADROW; j++) {
        outputClusters.nClusters[i][j] = 0;
      }
    }

    unsigned int nClusters = 0;
    for (int iChunk = 0; iChunk < numChunks; iChunk++) {
      if (inputClustersChain) {
        inputClustersChain->GetEntry(iChunk);
      }
      for (int icluster = 0; icluster < inputClustersArray->size(); ++icluster) {
        const auto& cluster = (*inputClustersArray)[icluster];
        const CRU cru(cluster.getCRU());
        const Sector sector = cru.sector();
        const PadRegionInfo& region = mapper.getPadRegionInfo(cru.region());
        const int rowInSector = cluster.getRow() + region.getGlobalRowOffset();

        if (iter == 1) {
          ClusterNative& oCluster =
            outputClusters.clusters[sector][rowInSector][outputClusters.nClusters[sector][rowInSector]];
          oCluster.setTimeFlags(cluster.getTimeMean(), 0);
          oCluster.setPad(cluster.getPadMean());
          oCluster.setSigmaTime(cluster.getTimeSigma());
          oCluster.setSigmaPad(cluster.getPadSigma());
          oCluster.qTot = cluster.getQmax();
          oCluster.qMax = cluster.getQ();
        }
        outputClusters.nClusters[sector][rowInSector]++;
      }
      nClusters += inputClustersArray->size();
    }
    if (iter == 0) {
      clusterMemory.reset(new ClusterNative[nClusters]);
      unsigned int pos = 0;
      for (int i = 0; i < Sector::MAXSECTOR; i++) {
        for (int j = 0; j < Constants::MAXGLOBALPADROW; j++) {
          outputClusters.clusters[i][j] = &clusterMemory[pos];
          pos += outputClusters.nClusters[i][j];
        }
      }
    }
  }
  for (int i = 0; i < Sector::MAXSECTOR; i++) {
    for (int j = 0; j < Constants::MAXGLOBALPADROW; j++) {
      std::sort(outputClusters.clusters[i][j], outputClusters.clusters[i][j] + outputClusters.nClusters[i][j],
                ClusterNativeContainer::sortComparison);
    }
  }

  return (0);
}

int TPCCATracking::runTracking(const ClusterNativeAccessFullTPC& clusters, std::vector<TrackTPC>* outputTracks,
                               MCLabelContainer* outputTracksMCTruth)
{
  const static ParameterDetector& detParam = ParameterDetector::defaultInstance();
  const static ParameterGas& gasParam = ParameterGas::defaultInstance();
  const static ParameterElectronics& elParam = ParameterElectronics::defaultInstance();

  int nClusters[Sector::MAXSECTOR] = {};
  int nClustersConverted = 0;
  int nClustersTotal = 0;
  float vzbin = (elParam.getZBinWidth() * gasParam.getVdrift());
  float vzbinInv = 1.f / vzbin;

  Mapper& mapper = Mapper::instance();
  for (int i = 0; i < Sector::MAXSECTOR; i++) {
    for (int j = 0; j < Constants::MAXGLOBALPADROW; j++) {
      if (clusters.nClusters[i][j] > 0xFFFF) {
        LOG(ERROR) << "Number of clusters in sector " << i << " row " << j
                   << " exceeds 0xFFFF, which is currently a hard limit of the wrapper for the tracking!\n";
        return (1);
      }
      nClusters[i] += clusters.nClusters[i][j];
    }
    nClustersTotal += nClusters[i];

    mClusterData[i].StartReading(i, nClusters[i]);

    for (int j = 0; j < Constants::MAXGLOBALPADROW; j++) {
      Sector sector = i;
      int regionNumber = 0;
      while (j > mapper.getGlobalRowOffsetRegion(regionNumber) + mapper.getNumberOfRowsRegion(regionNumber))
        regionNumber++;
      CRU cru(sector, regionNumber);
      const PadRegionInfo& region = mapper.getPadRegionInfo(cru.region());
      for (int k = 0; k < clusters.nClusters[i][j]; k++) {
        const ClusterNative& cluster = clusters.clusters[i][j][k];
        AliHLTTPCCAClusterData& cd = mClusterData[i];
        AliHLTTPCCAClusterData::Data& hltCluster = cd.Clusters()[cd.NumberOfClusters()];

        const float padY = cluster.getPad();
        const int padNumber = int(padY);
        const GlobalPadNumber pad = mapper.globalPadNumber(PadPos(j, padNumber));
        const PadCentre& padCentre = mapper.padCentre(pad);
        const float localY = padCentre.Y() - (padY - padNumber) * region.getPadWidth();
        const float localYfactor = (cru.side() == Side::A) ? -1.f : 1.f;
        float zPositionAbs = cluster.getTime() * vzbin;
        if (!mTrackingCAO2Interface->GetParamContinuous())
          zPositionAbs = detParam.getTPClength() - zPositionAbs;
        else
          zPositionAbs = sContinuousTFReferenceLength * vzbin - zPositionAbs;

        // sanity checks
        if (zPositionAbs < 0 ||
            (!mTrackingCAO2Interface->GetParamContinuous() && zPositionAbs > detParam.getTPClength())) {
          LOG(INFO) << "Removing cluster " << i << " time: " << cluster.getTime() << ", abs. z: " << zPositionAbs
                    << "\n";
          continue;
        }

        hltCluster.fX = padCentre.X();
        hltCluster.fY = localY * (localYfactor);
        hltCluster.fZ = zPositionAbs * (-localYfactor);
        hltCluster.fRow = j;
        hltCluster.fAmp = cluster.qMax;
        hltCluster.fId = (i << 24) | (j << 16) | (k);

        cd.SetNumberOfClusters(cd.NumberOfClusters() + 1);
        nClustersConverted++;
      }
    }
  }

  if (nClustersTotal != nClustersConverted) {
    LOG(INFO) << "Passed " << nClustersConverted << " (out of " << nClustersTotal << ") clusters to CA tracker\n";
  }

  const AliHLTTPCGMMergedTrack* tracks;
  int nTracks;
  const AliHLTTPCGMMergedTrackHit* trackClusters;
  int retVal = mTrackingCAO2Interface->RunTracking(mClusterData, tracks, nTracks, trackClusters);
  if (retVal == 0) {
    std::vector<std::pair<int, float>> trackSort(nTracks);
    int tmp = 0, tmp2 = 0;
    for (char cside = 0; cside < 2; cside++) {
      for (int i = 0; i < nTracks; i++) {
        if (tracks[i].OK() && tracks[i].CSide() == cside)
          trackSort[tmp++] = { i, (cside == 0 ? 1.f : -1.f) * tracks[i].GetParam().GetZOffset() };
      }
      std::sort(trackSort.data() + tmp2, trackSort.data() + tmp,
                [](const auto& a, const auto& b) { return (a.second > b.second); });
      tmp2 = tmp;
      if (cside == 0)
        mNTracksASide = tmp;
    }
    nTracks = tmp;

    outputTracks->resize(nTracks);

    for (int iTmp = 0; iTmp < nTracks; iTmp++) {
      auto& oTrack = (*outputTracks)[iTmp];
      const int i = trackSort[iTmp].first;
      float time0 = 0.f, tFwd = 0.f, tBwd = 0.f;
      float zTrack = tracks[i].GetParam().GetZ();

      float zHigh = 0, zLow = 0;
      if (mTrackingCAO2Interface->GetParamContinuous()) {
        float zoffset = tracks[i].CSide() ? -tracks[i].GetParam().GetZOffset() : tracks[i].GetParam().GetZOffset();
        time0 = sContinuousTFReferenceLength - zoffset * vzbinInv;

        // estimate max/min time increments which still keep track in the physical limits of the TPC
        zHigh = trackClusters[tracks[i].FirstClusterRef()].fZ - tracks[i].GetParam().GetZOffset(); // high R cluster
        zLow = trackClusters[tracks[i].FirstClusterRef() + tracks[i].NClusters() - 1].fZ -
               tracks[i].GetParam().GetZOffset(); // low R cluster

        bool sideHighA = (trackClusters[tracks[i].FirstClusterRef()].fId >> 24) < Sector::MAXSECTOR / 2;
        bool sideLowA =
          (trackClusters[tracks[i].FirstClusterRef() + tracks[i].NClusters() - 1].fId >> 24) < Sector::MAXSECTOR / 2;

        // check if Z of edge clusters don't appear on the wrong side of the TPC
        if (sideHighA == sideLowA) {
          if ((sideHighA && zHigh < 0) || (!sideHighA && zHigh > 0)) {
            // need to add shift to bring track to the side corresponding to clusters
            float dz = sideHighA ? -std::min(zLow, zHigh) : -std::max(zLow, zHigh); // = -max(|zLow|,|zAbs|)
            zTrack += dz;
            time0 += dz * vzbinInv;
            zLow += dz;
            zHigh += dz;
          }
        } else {
          // at the moment we don't have tracks with clusters on different sides
        }
        // calculate time bracket
        float zLowAbs = zLow < 0.f ? -zLow : zLow;
        float zHighAbs = zHigh < 0.f ? -zHigh : zHigh;
        //
        // tFwd = (Lmax - max(|zLow|,|zAbs|))/vzbin  = drift time from cluster current Z till endcap
        // tBwd = min(|zLow|,|zAbs|))/vzbin          = drift time from CE till cluster current Z
        //
        if (zLowAbs < zHighAbs) {
          tFwd = (detParam.getTPClength() - zHighAbs) * vzbinInv;
          tBwd = zLowAbs * vzbinInv;
        } else {
          tFwd = (detParam.getTPClength() - zLowAbs) * vzbinInv;
          tBwd = zHighAbs * vzbinInv;
        }
      }

      oTrack =
        TrackTPC(tracks[i].GetParam().GetX(), tracks[i].GetAlpha(),
                 { tracks[i].GetParam().GetY(), zTrack, tracks[i].GetParam().GetSinPhi(),
                   tracks[i].GetParam().GetDzDs(), tracks[i].GetParam().GetQPt() },
                 { tracks[i].GetParam().GetCov(0), tracks[i].GetParam().GetCov(1), tracks[i].GetParam().GetCov(2),
                   tracks[i].GetParam().GetCov(3), tracks[i].GetParam().GetCov(4), tracks[i].GetParam().GetCov(5),
                   tracks[i].GetParam().GetCov(6), tracks[i].GetParam().GetCov(7), tracks[i].GetParam().GetCov(8),
                   tracks[i].GetParam().GetCov(9), tracks[i].GetParam().GetCov(10), tracks[i].GetParam().GetCov(11),
                   tracks[i].GetParam().GetCov(12), tracks[i].GetParam().GetCov(13), tracks[i].GetParam().GetCov(14) });
      oTrack.setTime0(time0);
      oTrack.setDeltaTBwd(tBwd);
      oTrack.setDeltaTFwd(tFwd);
      // RS: TODO: once the A/C merging will be implemented, both A and C side must be flagged for meged track
      if (tracks[i].CSide()) {
        oTrack.setHasCSideClusters();
      } else {
        oTrack.setHasASideClusters();
      }

      oTrack.setChi2(tracks[i].GetParam().GetChi2());
      auto& outerPar = tracks[i].GetParam().OuterParam();
      oTrack.setOuterParam(o2::track::TrackParCov(
        outerPar.fX, outerPar.fAlpha,
        { outerPar.fP[0], outerPar.fP[1], outerPar.fP[2], outerPar.fP[3], outerPar.fP[4] },
        { outerPar.fC[0], outerPar.fC[1], outerPar.fC[2], outerPar.fC[3], outerPar.fC[4], outerPar.fC[5],
          outerPar.fC[6], outerPar.fC[7], outerPar.fC[8], outerPar.fC[9], outerPar.fC[10], outerPar.fC[11],
          outerPar.fC[12], outerPar.fC[13], outerPar.fC[14] }));
      oTrack.resetClusterReferences(tracks[i].NClusters());
      std::vector<std::pair<MCCompLabel, unsigned int>> labels;
      for (int j = 0; j < tracks[i].NClusters(); j++) {
        int clusterId = trackClusters[tracks[i].FirstClusterRef() + j].fId;
        Sector sector = clusterId >> 24;
        int globalRow = (clusterId >> 16) & 0xFF;
        clusterId &= 0xFFFF;
        const ClusterNative& cl = clusters.clusters[sector][globalRow][clusterId];
        int regionNumber = 0;
        while (globalRow > mapper.getGlobalRowOffsetRegion(regionNumber) + mapper.getNumberOfRowsRegion(regionNumber))
          regionNumber++;
        CRU cru(sector, regionNumber);
        oTrack.addCluster(Cluster(cru, globalRow - mapper.getGlobalRowOffsetRegion(regionNumber), cl.qTot, cl.qMax,
                                  cl.getPad(), cl.getSigmaPad(), cl.getTime(), cl.getSigmaTime()));
        oTrack.setClusterReference(j, sector, globalRow, clusterId);
        if (outputTracksMCTruth) {
          for (const auto& element : clusters.clustersMCTruth[sector][globalRow]->getLabels(clusterId)) {
            bool found = false;
            for (int l = 0; l < labels.size(); l++) {
              if (labels[l].first == element) {
                labels[l].second++;
                found = true;
                break;
              }
            }
            if (!found)
              labels.emplace_back(element, 1);
          }
        }
      }
      if (outputTracksMCTruth) {
        int bestLabelNum = 0, bestLabelCount = 0;
        for (int j = 0; j < labels.size(); j++) {
          if (labels[j].second > bestLabelCount) {
            bestLabelNum = j;
            bestLabelCount = labels[j].second;
          }
        }
        MCCompLabel& bestLabel = labels[bestLabelNum].first;
        if (bestLabelCount < (1.f - sTrackMCMaxFake) * tracks[i].NClusters())
          bestLabel.set(-bestLabel.getTrackID(), bestLabel.getEventID(), bestLabel.getSourceID());
        outputTracksMCTruth->addElement(iTmp, bestLabel);
      }
      int lastSector = trackClusters[tracks[i].FirstClusterRef() + tracks[i].NClusters() - 1].fId >> 24;
    }
  }
  mTrackingCAO2Interface->Cleanup();
  return (retVal);
}

int TPCCATracking::runTracking(TChain* inputClustersChain, const std::vector<o2::TPC::Cluster>* inputClustersArray,
                               std::vector<TrackTPC>* outputTracks)
{
  if (mTrackingCAO2Interface == nullptr)
    return (1);
  int retVal = 0;

  std::unique_ptr<ClusterNative[]> clusterMemory;
  ClusterNativeAccessFullTPC clusters;
  retVal = convertClusters(inputClustersChain, inputClustersArray, clusters, clusterMemory);
  if (retVal)
    return (retVal);
  return (runTracking(clusters, outputTracks));
}

float TPCCATracking::getPseudoVDrift()
{
  const static ParameterGas& gasParam = ParameterGas::defaultInstance();
  const static ParameterElectronics& elParam = ParameterElectronics::defaultInstance();
  return (elParam.getZBinWidth() * gasParam.getVdrift());
}
