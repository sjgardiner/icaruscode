////////////////////////////////////////////////////////////////////////
// Class:       PMTPulseAna
// Plugin Type: analyzer (art v3_05_00)
// File:        PMTPulseAna_module.cc
//
// Generated at Mon Sep 21 15:21:37 2020 by Andrea Scarpelli
//
//  mailto:ascarpell@bnl.gov
////////////////////////////////////////////////////////////////////////
#include "larcore/Geometry/Geometry.h"
#include "larcore/CoreUtils/ServiceUtil.h" // lar::providerFrom()

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/FileBlock.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"

#include "canvas/Utilities/Exception.h"

#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art_root_io/TFileService.h"
#include "lardataobj/RawData/OpDetWaveform.h"
#include "lardataobj/RecoBase/OpHit.h"

#include "sbndaq-artdaq-core/Overlays/ICARUS/ICARUSTriggerUDPFragment.hh"

#include "TTree.h"

namespace pmtcalo {
  class PMTPulseAna;
}


class pmtcalo::PMTPulseAna : public art::EDAnalyzer {

public:

  explicit PMTPulseAna(fhicl::ParameterSet const& pset);

  PMTPulseAna(PMTPulseAna const&) = delete;
  PMTPulseAna(PMTPulseAna&&) = delete;
  PMTPulseAna& operator=(PMTPulseAna const&) = delete;
  PMTPulseAna& operator=(PMTPulseAna&&) = delete;

  virtual void beginJob() override;

  void analyze(art::Event const& event) override;

  double _findMedian(std::vector<short int> a, size_t n);
  float _getTotalCharge( std::vector<short int> wfrm );
  void clean();

private:

  art::InputTag m_trigger_label;
  art::InputTag m_decode_label;
  art::InputTag m_ophit_label;

  fhicl::ParameterSet m_waveform_config;

  TTree *m_trigger_ttree;
  TTree *m_ophit_ttree; 
  TTree *m_geo_ttree;

  int m_run;
  int m_subrun;
  int m_event;
  int m_spexi_local_seconds = -999;
  int m_spexi_local_nseconds = -999;
  int m_wr_event = -999;
  int m_wr_seconds = -999;
  int m_wr_nseconds = -999;
  int m_gate_id = -999;
  int m_gate_type = -999;
  
  std::vector<int>   *m_channel_id = NULL;
  std::vector<float> *m_tstart = NULL;
  std::vector<float> *m_tmax = NULL;
  std::vector<float> *m_amplitude = NULL;
  std::vector<float> *m_integral = NULL;
  std::vector<float> *m_total_charge = NULL;

  art::ServiceHandle<art::TFileService> tfs;

};


//------------------------------------------------------------------------------


pmtcalo::PMTPulseAna::PMTPulseAna(fhicl::ParameterSet const& pset)
  : art::EDAnalyzer(pset)  // ,
{
   m_decode_label = pset.get<art::InputTag>("DecoderModule", "daqPMT");
   m_trigger_label = pset.get<art::InputTag>("TriggerFragment", "ICARUSTriggerUDP");
   m_ophit_label = pset.get<art::InputTag>("OpHitModule", "ophit");
}


//------------------------------------------------------------------------------


void pmtcalo::PMTPulseAna::beginJob()
{

  m_trigger_ttree = tfs->make<TTree>("triggertree", "Trigger metadata");

  m_trigger_ttree->Branch("run", &m_run, "run/I" );
  m_trigger_ttree->Branch("subrun", &m_subrun, "subrun/I" );
  m_trigger_ttree->Branch("event", &m_event, "event/I" );
  m_trigger_ttree->Branch("spexi_local_seconds", &m_spexi_local_seconds, "spexi_local_seconds/I");
  m_trigger_ttree->Branch("spexi_local_nseconds", &m_spexi_local_nseconds, "spexi_local_nseconds/I");
  m_trigger_ttree->Branch("wr_event", &m_wr_event, "wr_event/I");
  m_trigger_ttree->Branch("wr_seconds", &m_wr_seconds, "wr_seconds/I");
  m_trigger_ttree->Branch("wr_nseconds", &m_wr_nseconds, "wr_nseconds/I");
  m_trigger_ttree->Branch("gate_id", &m_gate_id, "gate_id/I");
  m_trigger_ttree->Branch("gate_type", &m_gate_type, "gate_type/I" );


  m_ophit_ttree = tfs->make<TTree>("ophittree","OpHit TTree");
  
  m_ophit_ttree->Branch("run", &m_run, "run/I" );
  m_ophit_ttree->Branch("subrun", &m_subrun, "subrun/I" );
  m_ophit_ttree->Branch("event", &m_event, "event/I" );
  m_ophit_ttree->Branch("channel_id", &m_channel_id);
  m_ophit_ttree->Branch("tstart", &m_tstart );
  m_ophit_ttree->Branch("tmax", &m_tmax );
  m_ophit_ttree->Branch("amplitude", &m_amplitude );
  m_ophit_ttree->Branch("integral", &m_integral );
  m_ophit_ttree->Branch("total_charge", &m_total_charge );

  //-----------------------------------------------------------------------------

  m_geo_ttree = tfs->make<TTree>("geotree","tree with detector geo info");

  std::vector<double> pmtX, pmtY, pmtZ;
  std::vector<double> minX, minY, minZ;
  std::vector<double> maxX, maxY, maxZ;
  
  auto const geop = lar::providerFrom<geo::Geometry>();
  double PMTxyz[3];
  for(size_t opch=0; opch<geop->NOpChannels(); ++opch) {
    geop->OpDetGeoFromOpChannel(opch).GetCenter(PMTxyz);
    pmtX.push_back(PMTxyz[0]);
    pmtY.push_back(PMTxyz[1]);
    pmtZ.push_back(PMTxyz[2]);
  }

  m_geo_ttree->Branch("pmtX",&pmtX);
  m_geo_ttree->Branch("pmtY",&pmtY);
  m_geo_ttree->Branch("pmtZ",&pmtZ);

  m_geo_ttree->Fill();
  
}


//-----------------------------------------------------------------------------


double pmtcalo::PMTPulseAna::_findMedian(std::vector<short int> a, size_t n) 
{
  
  // First we sort the array
  sort(a.begin(), a.end());

  // check for even case
  if (n % 2 != 0)
    return (double)a[n / 2];

  return (double)(a[(n - 1) / 2] + a[n / 2]) / 2.0;

}


//----------------------------------------------------------------------------

float pmtcalo::PMTPulseAna::_getTotalCharge( std::vector<short int> wfrm )
{

  float sum=0.0;
  float median = _findMedian( wfrm, wfrm.size() );
  for( float adc : wfrm ){
    sum += (adc - median);
  }

  return sum;

}


//-----------------------------------------------------------------------------


void pmtcalo::PMTPulseAna::analyze(art::Event const& event)
{

  m_run = event.id().run();
  m_subrun = event.id().subRun();
  m_event = event.id().event();


  // Get the trigger metatada and protect runs without information
  try
  {
      // Recover the data fragments for the PMT 
      auto const& fragments = event.getByLabel<artdaq::Fragments>(m_trigger_label);
    
      // Make sure data available
      if (!fragments.empty())
      {
            for (auto const &rawFrag: fragments){

              icarus::ICARUSTriggerUDPFragment triggerFragment(rawFrag);
              m_spexi_local_seconds = triggerFragment.getSeconds();
              m_spexi_local_nseconds = triggerFragment.getNanoSeconds();
              m_wr_event = triggerFragment.getWREventNo();
              m_wr_seconds = triggerFragment.getWRSeconds();
              m_wr_nseconds = triggerFragment.getWRNanoSeconds();
              m_gate_id = triggerFragment.getGateID();
              m_gate_type = triggerFragment.getGateType();

            }
      }
  }
  catch(cet::exception const& e) {
        std::cout << "PMTPulseAna:Did not find daq data products to decode:"
          << "\n" << e.what() << std::endl;
  }


   // First thing we sort the ophit in their respective channels
   art::Handle< std::vector< recob::OpHit > > ophitHandle;
   event.getByLabel(m_ophit_label, ophitHandle);

   std::map<int, std::vector<recob::OpHit>> ophitch;
   for( auto const& ophit : (*ophitHandle) ){
    int ch = ophit.OpChannel();
    ophitch[ch].push_back( ophit );
   }

   // Now we loop over the raw waveforms and fill the ttree

   art::Handle< std::vector< raw::OpDetWaveform > > rawHandle;
   event.getByLabel(m_decode_label, rawHandle);

   // There is a valid handle per channel
   for( auto const& raw_waveform : (*rawHandle) )
   {

     int channel = raw_waveform.ChannelNumber();

     for( const recob::OpHit& ophit : ophitch[channel])
     {
      m_channel_id->push_back( channel );
      m_total_charge->push_back( _getTotalCharge( raw_waveform ) );
      m_tstart->push_back( ophit.PeakTimeAbs() );
      m_tmax->push_back( ophit.PeakTime() ); // << TODO FIXME
      m_integral->push_back( ophit.Area() );
      m_amplitude->push_back( ophit.Amplitude() );
    }

   } // end loop over pmt channels

   m_ophit_ttree->Fill();
   m_trigger_ttree->Fill();
   
   clean();

   ophitch.clear();
  
} // end analyze


//-----------------------------------------------------------------------------

void pmtcalo::PMTPulseAna::clean(){

  m_channel_id->clear();
  m_tstart->clear();
  m_tmax->clear();
  m_amplitude->clear();
  m_integral->clear();
  m_total_charge->clear();

}


DEFINE_ART_MODULE(pmtcalo::PMTPulseAna)
