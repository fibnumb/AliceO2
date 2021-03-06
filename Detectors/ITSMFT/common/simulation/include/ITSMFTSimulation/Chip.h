// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

//
//  Chip.h
//  ALICEO2
//
//  Created by Markus Fasel on 23.07.15.
//  Adapted from AliITSUChip by Massimo Masera
//

#ifndef ALICEO2_ITSMFT_CHIP_
#define ALICEO2_ITSMFT_CHIP_

#include <ITSMFTBase/Digit.h>
#include <TObject.h> // for TObject
#include <exception>
#include <map>
#include <sstream>
#include <vector>
#include "MathUtils/Cartesian3D.h"
#include "SimulationDataFormat/MCCompLabel.h"

namespace o2
{
namespace ITSMFT
{
class Hit;
}
}

namespace o2
{
namespace ITSMFT
{
class Hit;
class DigiParams;

/// @class Chip
/// @brief Container for similated points connected to a given chip
///
/// This class contains all points of an event connected to a given
/// chip identified by the chip index.
class Chip
{
 public:
  using Label = o2::MCCompLabel;
  /// @class IndexException
  /// @brief Handling discrepancies between Chip index stored in the hit
  /// and Chip index stored in the chip
  class IndexException : public std::exception
  {
   public:
    /// Default constructor
    /// Initializes indices with -1. Not to be used when throwing the
    /// exception. Use other constructor instead
    IndexException() : mDetIdChip(-1), mDetIdHit(-1) {}
    /// Constructor
    /// Initializing indices from chip and from hit
    /// @param indexChip Chip index stored in chip itself
    /// @param indexHit Chip index stored in the hit
    IndexException(ULong_t indexChip, ULong_t indexHit) : mDetIdChip(indexChip), mDetIdHit(indexHit) {}
    /// Destructor
    ~IndexException() throw() override {}
    /// Build error message
    /// The error message contains the indices stored in the chip and in the hit
    /// @return Error message connected to this exception
    const char* what() const throw() override
    {
      std::stringstream message;
      message << "Chip ID " << mDetIdHit << " from hit different compared to this ID " << mDetIdChip;
      return message.str().c_str();
    }

    /// Get the chip index stored in the chip
    /// @return Chip index stored in the chip
    ULong_t GetChipIndexChip() const { return mDetIdChip; }
    /// Fet the chip index stored in the hit
    /// @return Chip index stored in the hit
    ULong_t GetChipIndexHit() const { return mDetIdHit; }
   private:
    ULong_t mDetIdChip; ///< Index of the chip stored in the chip
    ULong_t mDetIdHit;  ///< Index of the chip stored in the hit
  };

  /// Default constructor
  Chip() = default;

  /// Destructor
  ~Chip() = default;

  /// Main constructor
  /// @param chipindex Index of the chip
  /// @param mat Transformation matrix
  Chip(const DigiParams* par, Int_t index, const o2::Transform3D* mat);

  /// Copy constructor
  /// @param ref Reference for the copy
  Chip(const Chip& ref);

  /// Assignment operator
  /// @param ref Reference for assignment
  /// @return Chip after assignment
  Chip& operator=(const Chip& ref);

  /// Comparison operator, checking for equalness
  /// Comparison done on chip index
  /// @param other Chip to compare with
  /// @return True if chip indices are the same
  Bool_t operator==(const Chip& other) const;

  /// Coparison operator, checking for uneqalness
  /// Comparison done on chip index
  /// @param other Chip to compare with
  /// @return True if chip indices are the different
  Bool_t operator!=(const Chip& other) const;

  /// Comparison operator, checking whether this chip is
  /// smaller based on the chip index
  /// @param other Chip to compare with
  /// @return True if this chip index is smaller than the other chip index
  Bool_t operator<(const Chip& other) const;

  /// Empties the point container
  /// @param option unused
  void Clear();

  /// Change the chip index
  /// @param index New chip index
  void SetChipIndex(Int_t index) { mChipIndex = index; }
  void Init(Int_t index, const o2::Transform3D* mat)
  {
    mChipIndex = index;
    mMat = mat;
  }

  /// Get the chip index
  /// @return Index of the chip
  Int_t GetChipIndex() const { return mChipIndex; }
  /// Insert new ITSMFT point into the Chip
  /// @param p Hit to be added
  void InsertHit(const Hit* p);

  /// Get the number of point assigned to the chip
  /// @return Number of points assigned to the chip
  Int_t GetNumberOfHits() const { return mHits.size(); }
  /// reset points container
  void ClearHits() { mHits.clear(); }
  o2::ITSMFT::Digit* findDigit(ULong64_t key);

  /// Access Hit assigned to chip at a given index
  /// @param index Index of the point
  /// @return Hit at given index (nullptr if index is out of bounds)
  const Hit* GetHitAt(Int_t index) const;

  /// Get the line segment of a given point (from start to current position)
  /// in local coordinates.
  /// Function derived from AliITSUChip
  /// @param hitindex Index of the hit
  /// @param xstart X-coordinate of the start of the hit (in local coordinates)
  /// @param xpoint X-coordinate of the hit (in local coordinates)
  /// @param ystart Y-coordinate of the start of the hit (in local coordinates)
  /// @param ypoint Y-coordinate of the hit (in local coordinates)
  /// @param zstart Z-coordinate of the start of the hit (in local coordinates)
  /// @param zpoint Z-coordinate of the hit (in local coordinates)
  /// @param timestart Start time of the hit
  /// @param eloss Energy loss during the hit
  Bool_t LineSegmentLocal(Int_t hitindex, Double_t& xstart, Double_t& xpoint, Double_t& ystart, Double_t& ypoint,
                          Double_t& zstart, Double_t& zpoint, Double_t& timestart, Double_t& eloss) const;
  Bool_t LineSegmentLocal(const Hit* hit, Double_t& xstart, Double_t& xpoint, Double_t& ystart, Double_t& ypoint,
                          Double_t& zstart, Double_t& zpoint, Double_t& timestart, Double_t& eloss) const;

  /// Get the line segment of a given point (from start to current position)
  /// in global coordinates.
  /// Function derived from AliITSUChip
  /// @TODO: Change function to return a complete space point
  /// @param hitindex Index of the hit
  /// @param xstart X-coordinate of the start of the hit (in global coordinates)
  /// @param xpoint X-coordinate of the hit (in global coordinates)
  /// @param ystart Y-coordinate of the start of the hit (in global coordinates)
  /// @param ypoint Y-coordinate of the hit (in global coordinates)
  /// @param zstart Z-coordinate of the start of the hit (in global coordinates)
  /// @param zpoint Z-coordinate of the hit (in global coordinates)
  /// @param timestart Start time of the hit
  /// @param eloss Energy loss during the hit
  Bool_t LineSegmentGlobal(Int_t hitindex, Double_t& xstart, Double_t& xpoint, Double_t& ystart, Double_t& ypoint,
                           Double_t& zstart, Double_t& zpoint, Double_t& timestart, Double_t& eloss) const;
  Bool_t LineSegmentGlobal(const Hit* hit, Double_t& xstart, Double_t& xpoint, Double_t& ystart, Double_t& ypoint,
                           Double_t& zstart, Double_t& zpoint, Double_t& timestart, Double_t& eloss) const;

  /// Calculate median position of two hits
  /// @param p1 First point in the median calculation
  /// @param p2 Second point in the median calculation
  /// @param x Median x-position of the two hits in local coordinates
  /// @param y Median y-position of the two hits in local coordinates
  /// @param z Median z-position of the two hits in local coordinates
  void MedianHitLocal(const Hit* p1, const Hit* p2, Double_t& x, Double_t& y, Double_t& z) const;

  /// Calculate median positoin of two hits
  /// @param p1 First point in the median calculation
  /// @param p2 Second point in the median calculation
  /// @param x Median xposition of the two hits in global coordinates
  /// @param y Median xposition of the two hits in global coordinates
  /// @param z Median xposition of the two hits in global coordinates
  void MedianHitGlobal(const Hit* p1, const Hit* p2, Double_t& x, Double_t& y, Double_t& z) const;

  /// Calculate path length between two its points
  /// @param p1 First point for the path length calculation
  /// @param p2 Second point for the path length calculation
  /// @return path length between points
  Double_t PathLength(const Hit* p1, const Hit* p2) const;

  o2::ITSMFT::Digit* addDigit(UInt_t roframe, UShort_t row, UShort_t col, float charge, Label lbl, double timestamp);

  void fillOutputContainer(std::vector<Digit>* digits, UInt_t maxFrame);

 protected:
  Int_t mChipIndex = -1;                          ///< Chip ID
  const DigiParams* mParams = nullptr;            ///< externally set digitization parameters
  const o2::Transform3D* mMat = nullptr;          ///< Transformation matrix
  std::vector<const Hit*> mHits;                  ///< Hits connnected to the given chip
  std::map<ULong64_t, o2::ITSMFT::Digit> mDigits; ///< Map of fired pixels, possibly in multiple frames

  ClassDefNV(Chip, 1);
};

inline o2::ITSMFT::Digit* Chip::findDigit(ULong64_t key)
{
  // finds the digit corresponding to global key
  auto digitentry = mDigits.find(key);
  return digitentry != mDigits.end() ? &(digitentry->second) : nullptr;
}

//_______________________________________________________________________
inline Bool_t Chip::LineSegmentGlobal(Int_t hitindex, Double_t& xstart, Double_t& xpoint, Double_t& ystart,
                                      Double_t& ypoint, Double_t& zstart, Double_t& zpoint, Double_t& timestart,
                                      Double_t& eloss) const
{
  return (hitindex >= int(mHits.size())) ? kFALSE : LineSegmentGlobal(mHits[hitindex], xstart, xpoint, ystart, ypoint,
                                                                      zstart, zpoint, timestart, eloss);
}

//_______________________________________________________________________
inline Bool_t Chip::LineSegmentLocal(Int_t hitindex, Double_t& xstart, Double_t& xpoint, Double_t& ystart,
                                     Double_t& ypoint, Double_t& zstart, Double_t& zpoint, Double_t& timestart,
                                     Double_t& eloss) const
{
  return (hitindex >= int(mHits.size())) ? kFALSE : LineSegmentLocal(mHits[hitindex], xstart, xpoint, ystart, ypoint,
                                                                     zstart, zpoint, timestart, eloss);
}
}
}

#endif /* defined(ALICEO2_ITSMFT_CHIP_) */
