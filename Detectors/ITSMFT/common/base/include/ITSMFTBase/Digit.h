// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file Digi.h
/// \brief Definition of the ITSMFT digit
#ifndef ALICEO2_ITSMFT_DIGIT_H
#define ALICEO2_ITSMFT_DIGIT_H

#include "Rtypes.h" // for Double_t, ULong_t, etc
#include "SimulationDataFormat/MCCompLabel.h"
#include "CommonDataFormat/TimeStamp.h"

namespace o2
{

namespace ITSMFT
{
/// \class Digit
/// \brief Digit class for the ITS
///

using DigitBase = o2::dataformats::TimeStamp<double>;
class Digit : public DigitBase
{
  using Label = o2::MCCompLabel;

 public:

  static constexpr int maxLabels=3;
  
  /// Default constructor  
  Digit() = default;

  /// Constructor, initializing values for position, charge and time
  /// @param chipindex Global index of the pixel chip
  /// @param pixelindex Index of the pixel within the chip
  /// @param charge Accumulated charge of digit
  /// @param timestamp Time at which the digit was created
  Digit(UShort_t chipindex, UInt_t frame, UShort_t row, UShort_t col, Float_t charge, Double_t timestamp);

  /// Destructor
  ~Digit();

  /// Addition operator
  /// Adds the charge of 2 digits
  /// @param lhs First digit for addition (also serves as time stamp)
  /// @param rhs Second digit for addition
  /// @return Digit with the summed charge of two digits and the time stamp of the first one
  const Digit operator+(const Digit& other);

  /// Operator +=
  /// Adds charge in other digit to this digit
  /// @param other Digit added to this digit
  /// @return Digit after addition
  Digit& operator+=(const Digit& other);

  /// Get the index of the chip
  /// @return Index of the chip
  UShort_t getChipIndex() const { return mChipIndex; }
  /// Get the column of the pixel within the chip
  /// @return column of the pixel within the chip
  UShort_t getColumn() const { return mCol; }
  /// Get the row of the pixel within the chip
  /// @return row of the pixel within the chip
  UShort_t getRow() const { return mRow; }
  /// Get the accumulated charged of the digit
  /// @return charge of the digit
  Float_t getCharge() const { return mCharge; }
  /// Get the labels connected to this digit
  Label getLabel(Int_t idx) const { return mLabels[idx]; }
  /// Add Label to the list of Monte-Carlo labels
  void setLabel(Int_t idx, Label label) { mLabels[idx] = label; }
  void setLabel(Int_t idx, int tr, int ev, int src=0) { mLabels[idx].set(tr,ev,src); }
  /// Set the index of the chip
  /// @param index The chip index
  void setChipIndex(UShort_t index) { mChipIndex = index; }
  /// Set the index of the pixel within the chip
  /// @param index Index of the pixel within the chip
  void setPixelIndex(UShort_t row, UShort_t col)
  {
    mRow = row;
    mCol = col;
  }

  /// Set the charge of the digit
  /// @param charge The charge of the the digit
  void setCharge(Float_t charge) { mCharge = charge; }

  /// Add charge to the digit, registering the label if provided
  void addCharge(Float_t charge, Label lbl) {
    mCharge += charge;
    if ( lbl.isEmpty() ) return;
    for (int i=0;i<maxLabels;i++) {
      if ( mLabels[i] == lbl ) break; // label was already added
      if ( mLabels[i].isEmpty() ) {
	mLabels[i] = lbl;
      }
    }
  }

  /// Check whether digit is equal to other digit.
  /// Comparison is done based on the chip index and pixel index
  /// @param other The digit to compare with
  /// @return True if digits are equal, false otherwise

  /// Get the RO frame ID, stripping overflow part
  UInt_t getROFrame()         const {return mROFrame>>ROFrameOverFlowBits;}

  /// Get the number of extra RO frames the signal will propagate
  Int_t  getNOverflowFrames() const {return mROFrame & ROFrameOverFlowMask;}

  /// Set RO frame ID (no overflow recorded)
  void  setROFrame(UInt_t v)        {mROFrame = v<<ROFrameOverFlowBits;}

  /// Set number of overflow RO frames
  void  setNOverflowFrames(Int_t n)  {mROFrame &= ROFrameMask; mROFrame |= n&ROFrameOverFlowMask;}

  /// Get global ordering key made of readout frame, column and row
  static ULong64_t getOrderingKey(UInt_t roframe, UShort_t row, UShort_t col) {
    return (static_cast<ULong64_t>(roframe)<<(8*sizeof(UInt_t))) + (col<<(8*sizeof(Short_t))) + row;
  } 

  /// Get ROFrame from the ordering key
  static UInt_t key2ROFrame(ULong64_t key) {
    return static_cast<UInt_t>(key>>(8*(sizeof(UInt_t)+ROFrameOverFlowBits)));
  }
  
  /// Test if the current digit is lower than the other
  /// Comparison is done based on the chip index and pixel index. Two
  /// options are possible for true:
  /// -# Chip index of this digit smaller than chip index of other digit
  /// -# Chip indices are equal, but pixel index of this chip is lower
  /// @param other The digit to compare with
  /// @return True if this digit has a lower total index, false otherwise
  //
  virtual bool operator<(const Digit& other) const
  {
    /* if (mChipIndex < other.mChipIndex || */
    /*     (mChipIndex == other.mChipIndex && mCol < other.mCol)) { */
    /*       return true; */
    /* } */
    return false;
  }
  
  /// Print function: Print basic digit information on the  output stream
  /// @param output Stream to put the digit on
  /// @return The output stream
  std::ostream& print(std::ostream& output) const;

  /// Streaming operator for the digit class
  /// Using streaming functionality defined in function Print
  /// @param output The stream where the digit is written to
  /// @param digi The digit to put on the stream
  /// @return The output stream
  friend std::ostream& operator<<(std::ostream& output, const Digit& digi)
  {
    digi.print(output);
    return output;
  }

 private:
  UShort_t mChipIndex = 0; ///< Chip index
  UShort_t mRow = 0;       ///< Pixel index in X
  UShort_t mCol = 0;       ///< Pixel index in Z
  Float_t  mCharge = 0.f;    ///< Accumulated charge
  UInt_t   mROFrame = 0;   ///< readout frame ID + number of following frames the signal propagates 
  Label    mLabels[maxLabels];    ///< Particle labels associated to this digit

  static constexpr int    ROFrameOverFlowBits = 3;   ///< max bits occupied by ROFrame overflow record
  static constexpr UInt_t ROFrameOverFlowMask = (0x1<<ROFrameOverFlowBits)-1; //< mask for ROFrame overflow record
  static constexpr UInt_t ROFrameMask = ~ROFrameOverFlowMask; //< mask for ROFrame record

  ClassDefNV(Digit, 3);
};
}
}

#endif /* ALICEO2_ITSMFT_DIGIT_H */
