/*
 *  $Id: DmpSimAlg.cc, 2014-09-25 22:53:04 DAMPE $
 *  Author(s):
 *    Chi WANG (chiwang@mail.ustc.edu.cn) 10/06/2014
*/

#include <time.h>   // time_t

#include "DmpSimAlg.h"
#include "DmpSimRunManager.h"
#include "G4PhysListFactory.hh"
#include "DmpSimDetector.h"
#include "DmpSimPrimaryGeneratorAction.h"
#include "DmpSimTrackingAction.h"
#include "DmpCore.h"
#include "DmpRootIOSvc.h"
#include "G4UImanager.hh"
#ifdef G4UI_USE_QT
#include "G4UIExecutive.hh"
#endif
#ifdef G4VIS_USE_OPENGLQT
#include "G4VisExecutive.hh"
#endif

//-------------------------------------------------------------------
DmpSimAlg::DmpSimAlg()
 :DmpVAlg("Sim/Alg/RunManager"),
  fSimRunMgr(0),
  fPhyFactory(0),
  fVisMod(true),
  fPhyListName("QGSP_BIC"),
  fSeed(time((time_t*)NULL))
{
  fPhyFactory = new G4PhysListFactory();
  OptMap.insert(std::make_pair("Physics",0));
  OptMap.insert(std::make_pair("Gdml",1));
  OptMap.insert(std::make_pair("Nud/DeltaTime",2));
  OptMap.insert(std::make_pair("Seed",3));
  OptMap.insert(std::make_pair("BT/AuxOffsetX",4));
  OptMap.insert(std::make_pair("BT/AuxOffsetY",5));
  OptMap.insert(std::make_pair("BT/MagneticFieldValue",6));
  OptMap.insert(std::make_pair("BT/MagneticFieldPosZ",7));
  // mode check
  if(".mac" == gRootIOSvc->GetInputExtension()){
    fVisMod = false; // then will active batch mode
    gRootIOSvc->Set("Output/Key","sim");
  }else{
    if(gRootIOSvc->GetOutputStem() == ""){
      gRootIOSvc->Set("Output/FileName","DmpSimVis.root");
    }
  }
}

//-------------------------------------------------------------------
DmpSimAlg::~DmpSimAlg(){
  delete fPhyFactory;
  //delete fSimRunMgr;
}

//-------------------------------------------------------------------
#include <boost/lexical_cast.hpp>
//#include "DmpEvtMCNudBlock.h"
void DmpSimAlg::Set(const std::string &type,const std::string &argv){
  if(OptMap.find(type) == OptMap.end()){
    DmpLogError<<"[DmpSimAlg::Set] No argument type:\t"<<type<<DmpLogEndl;
    std::cout<<"\tPossible options are:"<<DmpLogEndl;
    for(std::map<std::string,short>::iterator anOpt= OptMap.begin();anOpt!=OptMap.end();anOpt++){
      std::cout<<"\t\t"<<anOpt->first<<DmpLogEndl;
    }
    throw;
  }
  switch (OptMap[type]){
    case 0: // Physics
    {
      fPhyListName = argv;
      break;
    }
    case 1: // Gdml
    {
      DmpSimDetector::SetGdml(argv);
      break;
    }
    case 2: // Nud/DeltaTime
    {
      //DmpEvtMCNudBlock::SetDeltaTime(boost::lexical_cast<short>(argv));
      break;
    }
    case 3: // Seed
    {
      fSeed = boost::lexical_cast<long>(argv);
      break;
    }
    case 4: // Auxiliary detector offset X
    {
      DmpSimDetector::SetAuxDetOffsetX(boost::lexical_cast<double>(argv));
      break;
    }
    case 5: // Auxiliary detector offset Y
    {
      DmpSimDetector::SetAuxDetOffsetY(boost::lexical_cast<double>(argv));
      break;
    }
    case 6: // Magnetic field value
    {
      DmpSimDetector::SetMagneticFieldValue(boost::lexical_cast<double>(argv));
      break;
    }
    case 7: // Magnetic field position z
    {
      DmpSimDetector::SetMagneticFieldPosition(boost::lexical_cast<double>(argv));
      break;
    }
  }
}

//-------------------------------------------------------------------
#include <stdlib.h>     // getenv()
bool DmpSimAlg::Initialize(){
// set seed
  std::cout<<"\tRandom seed: "<<fSeed<<DmpLogEndl;      // keep this information in any case
  CLHEP::HepRandom::setTheSeed(fSeed);
// set G4 kernel
  fSimRunMgr = new DmpSimRunManager();
  fSimRunMgr->SetUserInitialization(fPhyFactory->GetReferencePhysList(fPhyListName));
  fSimRunMgr->SetUserAction(new DmpSimPrimaryGeneratorAction());      // only Primary Generator is mandatory
  fSimRunMgr->SetUserInitialization(new DmpSimDetector());
  fSimRunMgr->SetUserAction(new DmpSimTrackingAction());
  fSimRunMgr->Initialize();
// boot simulation
  if(fVisMod){    // visualization mode
// *
// *  TODO:  if /control/execute particle.mac, when will G4RunMgr::RunInitialization() been invoked?
// *
    G4UImanager *uiMgr = G4UImanager::GetUIpointer();
#ifdef G4UI_USE_QT
    char *dummyargv[20]={"visual"};
    G4UIExecutive *ui = new G4UIExecutive(1,dummyargv);
#ifdef G4VIS_USE_OPENGLQT
    G4VisExecutive *vis = new G4VisExecutive();
    vis->Initialize();
    //G4String prefix = (G4String)getenv("DMPSWSYS")+"/share/Simulation/";
    G4String prefix = "./";
    uiMgr->ApplyCommand("/control/execute "+prefix+"DmpSimVis.mac");
#endif
    if (ui->IsGUI()){
      uiMgr->ApplyCommand("/control/execute "+prefix+"DmpSimGUI.mac");
    }
    ui->SessionStart();
    delete ui;
#endif
    std::cout<<"DEBUG: "<<__FILE__<<"("<<__LINE__<<")"<<std::endl;
  }else{    // batch mode
    // if not vis mode, do some prepare for this run. refer to G4RunManagr::BeamOn()
    if(fSimRunMgr->ConfirmBeamOnCondition()){
      fSimRunMgr->ConstructScoringWorlds();
      fSimRunMgr->RunInitialization();
      // *
      // *  TODO:  check G4RunManager::InitializeEventLoop(the third argument right?)
      // *
      fSimRunMgr->InitializeEventLoop(gCore->GetMaxEventNumber(),gRootIOSvc->GetInputFileName().c_str(),gCore->GetMaxEventNumber());
    }else{
      DmpLogError<<"G4RunManager::Initialize() failed"<<DmpLogEndl;
      return false;
    }
  }
    std::cout<<"DEBUG: "<<__FILE__<<"("<<__LINE__<<")"<<std::endl;
  return true;
}

//-------------------------------------------------------------------
bool DmpSimAlg::ProcessThisEvent(){
  if(fVisMod){
    return true;
  }
  if(fSimRunMgr->SimOneEvent(gCore->GetCurrentEventID())){
    return true;
  }
  return false;
}

//-------------------------------------------------------------------
bool DmpSimAlg::Finalize(){
  if(not fVisMod){
    fSimRunMgr->TerminateEventLoop();
    fSimRunMgr->RunTermination();
  }
  return true;
}
