/*fsxwatm.h
  See LICENSE.TXT file for license information.
*/
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<iostream>

#define ATMTEMP  0
#define ATMHUMID 1
#define ATMRAIN  2
#define ATMWSPD  3
#define ATMWDIR  4
#define ATMCLOUD 5

//============================================================================
struct Atmosphere
{ //Atmosphere
private:
  double North;
  double South;
  double East;
  double West;
  double ResolutionX;
  double ResolutionY;
  long   CellX;
  long   CellY;
  short* Value;

public:
  long XNumber;
  long YNumber;
  long NumCellsPerGrid;

  Atmosphere();
  ~Atmosphere();
  void SetHeaderInfo( double north, double south, double east, double west,
                      double resolutionx, double resolutiony );
  bool GetHeaderInfo( double* north, double* south, double* east,
                      double* west, double* resolutionx,
                      double* resolutiony );
  void GetResolution( double* resolutionx, double* resolutiony );
  bool CompareHeader( double north, double south, double east, double west,
                      double resolutionx, double resolutiony );
  bool AllocAtmGrid( long timeintervals );
  bool FreeAtmGrid();
  bool GetAtmValue( double xpt, double ypt, long time, short* value );
  bool SetAtmValue( long number, long time, short value );
};//Atmosphere

//============================================================================
//AtmosphereGrid
//Base class for access and loading of atmospheric variables.
class AtmosphereGrid
{ //AtmosphereGrid
  FILE* InputTable;
  FILE* ThisFile;
  long month, day, hour;
  long* Month;    //Array of pointers to longs that store dates
  long* Day;
  long* Hour;
  long NumGrids;
  long StartGrid;
  long TimeIntervals;
  long Metric;
  Atmosphere atmosphere[6];

public:
  bool AtmGridWTR;
  bool AtmGridWND;
  char ErrMsg[256];

  AtmosphereGrid( long numgrids ); //Will default to 6 if all themes included,
  ~AtmosphereGrid();               //and 3 if only wind spd dir & cloud %
  bool ReadInputTable( char* InputFileName );
  bool ReadHeaders( long FileNumber );
  bool CompareHeader( long FileNumber );
  bool SetAtmosphereValues( long timeinterval, long filenumber );
  bool GetAtmosphereValue( long FileNumber, double xpt, double ypt,
                           long time, short* value );
  void GetResolution( long FileNumber, double* resolutionx,
                      double* resolutiony );
  long GetAtmMonth( long count );
  long GetAtmDay( long count );
  long GetAtmHour( long count );
  long GetTimeIntervals();
  void FreeAtmData();
};//AtmosphereGrid