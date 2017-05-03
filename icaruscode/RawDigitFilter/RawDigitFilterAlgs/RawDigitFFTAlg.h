#ifndef RAWDIGITFFTALG_H
#define RAWDIGITFFTALG_H
////////////////////////////////////////////////////////////////////////
//
// Class:       RawDigitFFTAlg
// Module Type: producer
// File:        RawDigitFFTAlg.h
//
//              This module provides some basic Fast Fourier Transform
//              algorithms for operating on RawDigit waveforms
//
// Configuration parameters:
//
// FillHistograms        - Turn on histogram filling for diagnostics
// RunFFTInputWires      - FFT analyze the input RawDigits if true - diagnostics
// RunFFTCorrectedWires  - FFT analyze the output RawDigits if true - diagnostics
//
//
// Created by Tracy Usher (usher@slac.stanford.edu) on January 6, 2016
//
////////////////////////////////////////////////////////////////////////

#include "RawDigitNoiseFilterDefs.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"
#include "art/Framework/Services/Optional/TFileService.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"

#include "TProfile.h"

namespace icarus_tool
{
    class IWaveformTool;
}

namespace caldata
{
    
class RawDigitFFTAlg
{
public:

    // Copnstructors, destructor.
    RawDigitFFTAlg(fhicl::ParameterSet const & pset);
    ~RawDigitFFTAlg();

    // Provide for reinitialization if necessary
    void reconfigure(fhicl::ParameterSet const & pset);
    void initializeHists(art::ServiceHandle<art::TFileService>&);
    
    template <class T> void getFFTCorrection(std::vector<T>&, double) const;
    
    template <class T> void getFFTCorrection(std::vector<T>&, size_t) const;
    
    void filterFFT(std::vector<short>&, size_t, size_t, float pedestal=0.) const;
    
private:
    
 //   using PeakTuple    = std::tuple<size_t,size_t,size_t>; // first bin, peak bin, last bin
 //   using PeakTupleVec = std::vector<PeakTuple>;
    
 //   void triangleSmooth(std::vector<double>&, size_t = 0)               const;
 //   void firstDerivative(std::vector<double>&, std::vector<double>&)    const;
 //   void findPeaks(std::vector<double>&, PeakTupleVec&, double, size_t) const;
    
    std::vector<bool>      fTransformViewVec;      ///< apply FFT transform to this view
    bool                   fFillHistograms;        ///< if true then will fill diagnostic hists
    std::string            fHistDirName;           ///< If writing histograms, the folder name
    
    std::vector<TProfile*> fCorValHistVec;
    std::vector<TProfile*> fFFTPowerVec;
    std::vector<TProfile*> fFFTPowerDerivVec;
    std::vector<TProfile*> fFFTRealVec;
    std::vector<TProfile*> fFFTImaginaryVec;
    std::vector<TProfile*> fSmoothPowerVec;
    std::vector<TProfile*> fFFTCorValHistVec;
    
    std::unique_ptr<icarus_tool::IWaveformTool> fWaveformTool;
    
    // Useful services, keep copies for now (we can update during begin run periods)
    detinfo::DetectorProperties const* fDetectorProperties = lar::providerFrom<detinfo::DetectorPropertiesService>();   ///< Detector properties service
};
    
} // end caldata namespace

#endif