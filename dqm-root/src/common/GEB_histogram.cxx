#include "VFAT_histogram.cxx"
#include "GEMClusterization/GEMStrip.h"
#include "GEMClusterization/GEMStripCollection.h"
#include "GEMClusterization/GEMClusterContainer.h"
#include "GEMClusterization/GEMClusterizer.h"
#include "TH1.h"
#define NETA 8
// #include "gem/readout/GEMslotContents.h"
// #include "db_interface.cxx"
//#include "plotter.cxx"
//!A class that creates histograms for GEB data
class GEB_histogram: public Hardware_histogram
{
public:
  //!Constructor calls base constructor of Hardware_histogram. Requires a string for filename, directory, and another string.
  GEB_histogram(const std::string & filename, TDirectory * dir, const std::string & hwid):Hardware_histogram(filename, dir, hwid){}//call base constructor

  //!Books histograms for GEB data
  /*!
    Books histograms for the following data: Zero Suppresion flags, GLIB input ID, VFAT word count (header), 
    Errors and Warnings (Thirteen Flags, InFIFO underflow flag, Stuck data flag), OH CRC, VFAT word count (trailer)
  */
  void bookHistograms()
  {
    m_dir->cd();
    //ZeroSup  = new TH1F("ZeroSup", "Zero Suppression", 0xffffff,  0x0 , 0xffffff);
    InputID  = new TH1F("InputID", "GLIB input ID", 31,  0x0 , 0b11111);      
    Vwh      = new TH1F("Vwh", "VFAT word count", 4095,  0x0 , 0xfff);
    // Assing custom bin labels
    const char *error_flags[5] = {"Event Size Overflow", "L1AFIFO Full", "InFIFO Full", "Evt FIFO Full","InFIFO Underflow"};
    Errors   = new TH1I("Errors", "Critical errors", 5,  0, 5);
    for (int i = 1; i<6; i++) Errors->GetXaxis()->SetBinLabel(i, error_flags[i-1]);
    const char *warning_flags[10] = {"BX AMC-OH Mismatch", "BX AMC-VFAT Mismatch", "OOS AMC OH", "OOS AMC VFAT","No VFAT Marker","Event Size Warn", "L1AFIFO Near Full", "InFIFO Near Full", "EvtFIFO Near Full", "Stuck Data"};
    Warnings = new TH1I("Warnings", "Warnings", 10,  0, 10);
    for (int i = 1; i<11; i++) Warnings->GetXaxis()->SetBinLabel(i, warning_flags[i-1]);
    //OHCRC    = new TH1F("OHCRC", "OH CRC", 0xffff,  0x0 , 0xffff);
    Vwt      = new TH1F("Vwt", "VFAT word count", 4095,  0x0 , 0xfff);
    BeamProfile      = new TH2I("BeamProfile", "2D Occupancy", 8, 0, 8, 384, 0, 384);
    ClusterMult      = new TH1I("ClusterMult", "Cluster multiplicity", 384,  0, 384 );
    ClusterSize      = new TH1I("ClusterSize", "Cluster size", 384,  0, 384 );
    for(int ie=0; ie < NETA; ie++){
      ClusterMultEta  [ie] = new TH1I(("ClusterMult"+std::to_string(static_cast <long long> (ie))).c_str(), "Cluster multiplicity", 384,  0, 384 );
      ClusterSizeEta  [ie] = new TH1I(("ClusterSize"+std::to_string(static_cast <long long> (ie))).c_str(), "Cluster size", 384,  0, 384 );
    }
    SlotN   = new TH1I("VFATSlots", "VFAT Slots", 24,  0, 24);
    Totalb1010 = new TH1F("Totalb1010", "Control Bit 1010", 15,  0x0 , 0xf);
    Totalb1100 = new TH1F("Totalb1100", "Control Bit 1100", 15,  0x0 , 0xf);
    Totalb1110 = new TH1F("Totalb1110", "Control Bit 1110", 15,  0x0 , 0xf);
    TotalFlag  = new TH1F("TotalFlag", "Control Flags", 15,  0x0 , 0xf);
    TotalCRC   = new TH1F("TotalCRC", "CRC Mismatches", 0xffff,-32768,32768);

    Integrity_canvas = newCanvas("GEBIntegrity",3,2,2400,1200);
    Occupancy_canvas = newCanvas("GEBOccupancy",3,3,1800,1800);
    ClusterSize_canvas = newCanvas("GEBClusterSize",3,3,1800,1800);
    ClusterMult_canvas = newCanvas("GEBClusterMult",3,3,1800,1800);
  }


  //!Fills histograms for GEB data
  /*!
    Fills the histograms for the following data: Zero Suppresion flags, GLIB input ID, VFAT word count (header), 
    Errors and Warnings (Thirteen Flags, InFIFO underflow flag, Stuck data flag), OH CRC, VFAT word count (trailer)
  */

  void fillHistograms(GEBdata * geb, std::map<int,int> slot_map){
    //ZeroSup->Fill(geb->ZeroSup());
    InputID->Fill(geb->InputID());
    Vwh->Fill(geb->Vwh());
    Errors->Fill(geb->ErrorC());
    //OHCRC->Fill(geb->OHCRC());
    Vwt->Fill(geb->Vwt());
    //InFu->Fill(geb->InFu());
    //Stuckd->Fill(geb->Stuckd());
    uint8_t binFired = 0;
    // Fill Warnings histogram
    for (int bin = 0; bin < 9; bin++){
      binFired = ((geb->ErrorC() >> bin) & 0x1);
      if (binFired) Warnings->Fill(bin);
    }
    binFired = (geb->Stuckd() & 0x1);
    if (binFired) Warnings->Fill(9);
    // Fill Errors histogram
    for (int bin = 9; bin < 13; bin++){
      binFired = ((geb->ErrorC() >> bin) & 0x1);
      if (binFired) Errors->Fill(bin-9);
    }
    binFired = (geb->InFu() & 0x1);
    if (binFired) Warnings->Fill(4);
    v_vfat = geb->vfats();
    for (auto m_vfat = v_vfat.begin(); m_vfat!=v_vfat.end(); m_vfat++)
      {
        m_sn = slot_map.find(m_vfat->ChipID())->second;

        SlotN->Fill(m_sn);
        ofstream myfile;
        this->readMap(m_sn, m_strip_map);
        uint16_t chan0xf = 0;
        for (int chan = 0; chan < 128; ++chan) {
          if (chan < 64){
            chan0xf = ((m_vfat->lsData() >> chan) & 0x1);
            if(chan0xf) {
              int m_i = (int) m_sn%8;
              int m_j = 127 - m_strip_map[chan] + ((int) m_sn/8)*128;
              if (allstrips.find(m_i) == allstrips.end()){
                GEMStripCollection strips;
                allstrips[m_i]=strips;
              }
              // bx set to 0...
              GEMStrip s(m_j,0);
              allstrips[m_i].insert(s);
              BeamProfile->Fill(m_i,m_j);
            }
          } else {
            chan0xf = ((m_vfat->msData() >> (chan-64)) & 0x1);
            if(chan0xf) {
              int m_i = (int) m_sn%8;
              int m_j = 127 - m_strip_map[chan] + ((int) m_sn/8)*128;
              if (allstrips.find(m_i) == allstrips.end()){
                GEMStripCollection strips;
                allstrips[m_i]=strips;
              }
              // bx set to 0...
              GEMStrip s(m_j,0);
              allstrips[m_i].insert(s);
              BeamProfile->Fill(m_i,m_j);
            }
          }
        }
      }
    int ncl=0;
    int ncleta=0;
    for (std::map<int, GEMStripCollection>::iterator ieta=allstrips.begin(); ieta!= allstrips.end(); ieta++){
      ncleta=0;
      GEMClusterizer clizer;
      GEMClusterContainer cls = clizer.doAction(ieta->second);
      ncl+=cls.size();
      ncleta+=cls.size();
      for (GEMClusterContainer::iterator icl=cls.begin();icl!=cls.end();icl++){
        ClusterSize->Fill(icl->clusterSize());    
        ClusterSizeEta[NETA-1-ieta->first]->Fill(icl->clusterSize());   
      }
      ClusterMultEta[NETA-1-ieta->first]->Fill(ncleta);   
    }
    ClusterMult->Fill(ncl);
  }

#include "GEB_summaryCanvases.cxx"
  
  void fillSummaryCanvases(vector<VFAT_histogram> vfat_histograms)
  {
    for(int vfat_index=0;vfat_index<vfat_histograms.size();vfat_index++)
      {
  	VFAT_histogram current_vfatH = vfat_histograms[vfat_index];
	Totalb1010->Add(current_vfatH.getb1010());
	Totalb1100->Add(current_vfatH.getb1100());
	Totalb1110->Add(current_vfatH.getb1110());
	TotalFlag ->Add(current_vfatH.getFlag());
	TotalCRC  ->Add(current_vfatH.getCRC());
      }

    fillGEBCanvases();
  }

  

  
  
  //!Adds a VFAT_histogram object to the m_vfatH vector
  void addVFATH(VFAT_histogram* vfatH, int i){m_vfatsH[i]=vfatH;}
  //!Returns the m_vfatsH vector
  VFAT_histogram * vfatsH(int i){return m_vfatsH[i];}
private:
  VFAT_histogram *m_vfatsH[24];    ///<A vector of VFAT_histogram 
  //TH1F* ZeroSup;                           ///<Histogram for Zero Suppression flags
  TH1F* InputID;                           ///<Histogram for GLIB input ID
  TH1F* Vwh;                               ///<Histogram for VFAT word count (header)
  TH1I* Errors;                            ///<Histogram for thirteen flags in GEM Chamber Header
  TH1I* Warnings;                          ///<Histogram for Warnings (InFIFO underflow, Stuck Data)
  //TH1F* OHCRC;                             ///<Histogram for OH CRC
  TH1F* Vwt;                               ///<Histogram for VFAT word count (trailer)
  TH2I* BeamProfile;                       ///<Histogram for 2D BeamProfile
  TH1I* ClusterMult;                       ///<Histogram for GEB cluster multiplicity
  TH1I* ClusterSize;                       ///<Histogram for GEB cluster size
  TH1I* ClusterMultEta[NETA];              ///<Histogram for GEB eta cluster multiplicity
  TH1I* ClusterSizeEta[NETA];              ///<Histogram for GEB eta cluster size
  TH1I* SlotN;                             ///<Histogram for VFAT slots
  TH1F* Totalb1010;			     // Add all vfat control bit histograms
  TH1F* Totalb1100;
  TH1F* Totalb1110;
  TH1F* TotalFlag;
  TH1F* TotalCRC;
  TCanvas* Integrity_canvas;
  TCanvas* Occupancy_canvas;
  TCanvas* ClusterSize_canvas;
  TCanvas* ClusterMult_canvas;
  
  
  std::map<int, GEMStripCollection> allstrips;
  //VFATdata * m_vfat;
  std::vector<VFATdata> v_vfat;            ///Vector of VFATdata
  int m_sn;
  int m_strip_map[128];
};
