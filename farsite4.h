/*============================================================================
  farsite4.cpp

  RFL version  
  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  See LICENSE.TXT file for license information.
  ============================================================================
*/
#ifndef FARSITE4_H
#define FARSITE4_H

#include"fsxwignt.h"  //Ignition input from files
#include"fsxwinpt.h"
#include"fsxwattk.h"  //Attack functions and structures
#include"fsairatk.h"
#include"fsxwshap.h"  //Shapefile output
#include"rand2.h"
 
/*============================================================================
  TFarsiteInterface
*/
class TFarsiteInterface
{ //TFarsiteInterface
  bool   SIMULATE_GO, FARSITE_GO, StepThrough, IN_BURNPERIOD;
  char   IgFileName[256];
  char   FarsiteDirectory[256];
  double a1, p1, p2, sarea, firesizeh, firesizes; //For area & perimeter calcs
  double smolder, flaming;
  long   PrimaryVisEqual, PrimaryVisCount;
  long   SecondaryVisEqual, SecondaryVisCount;    //Visible timestep counters
  long   CurrentFire;
  long   NextFireAfterInterrupt;
  short  ProcNum;
  long   maximum, numspots, idum;//Max sim time, updated w/ global "numaximum"
  long   OutputVectsAsGrown;
  long   CountInwardFires, CountTotalFires, VectorBarriers;//For disp purposes
  double LastFMCalcTime;
  long   NumSimThreads;
  double TotalAreaBurned; //Added June 2009 (JWB)

  FarInputs Inputs;
  Attack  Atk;
  AirAttack AAtk;
  ShapeFileUtils shape;
  GroupAirAttack* gaat;
  FILE*  CurrentFile;

  struct WindGridData {
    double X;
    double Y;
    double WS;    // stores wind speed data used for wind maps
    double WD;    // stores wind direction data used for wind maps
  };
  WindGridData* WData;
  long NumWData, NumEDim, NumNDim;

  struct {
    double tws;
    double winddir;
  } WStat[5];

  Vectorize vect;  // object that writes vector files
  Burn  burn;            //Burn object contains burn,area,intersections,embers
  IgnitionFile Ignition;  // Ignition file inputs

  bool OpenLandscapeFile();
  void CheckSteps();        // check for changes in actual or visual timesteps

  bool WriteClocks();     // update the clocks if visible
  void WriteVectorOutputs(); //output fire perimeters to vector file & monitor
  void OpenFuelMods();       //opens fuel model file
  bool FarsiteProcess1();  //FARSITE process split to allow message processing
  bool FarsiteProcessSpots(); //FARSITE process split to allow mesg processing
  bool FarsiteProcess2();     //FARSITE process split to allow mesg processing
  void FarsiteProcess3();     //FARSITE process split to allow mesg processing
  void FarsiteSimulationLoop(); //calls FARSITE processes
  void WriteOutputs( int Type );//writes area & perim to arrays & draws graphs
  void CountFires();       // just counts fires for display
  void ElTimeConvert();    // convert time to Elapsed time
  void CurTimeConvert();   // convert time to Current Time
  void WriteGISLogFile( long LogType ); // write log file for GIS outputs
  void SaveIgnitions();    // for internal use with start/restart
  void CancelMoistureCalculation();
  bool PreCalculateFuelMoistures(); //do all fuel moisture calcs ahead of sim
  long InsertSpotFire( double xpt, double ypt );
  void SaveSettingsToFile( const char *fname );
  void ProcessNameValue( char* name, char* value );
  bool ValidateSettings();
  long ProcessAttackString( char *value, int ResourceType, long *ResourceNum,
                            int *AttackType, double *Coords );
  bool Execute_InitiateTerminate();

protected:
  void CheckStopLocations();

  long SimRequest;
  bool SIM_SUSPENDED, TerminateMoist, MxRecalculated;
  double MoistSimTime;

  void Execute_StartRestart();
  void Execute_ResumeSuspend();

  void StartMoistThread();
  unsigned RunMoistThread(void*);
  void MoistThread();

public:
  TFarsiteInterface( const char* directory );
  ~TFarsiteInterface();
  void FlatOpenProject();
  bool PreCalculateFuelMoisturesNoUI();
  void loadValues();
  bool SetInputsFromFile( char* settingsfilename );
};//TFarsiteInterface

#endif// FARSITE_H
