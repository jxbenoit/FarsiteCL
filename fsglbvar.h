/*fsglbvar.h
  Header File for accessing global data for FARSITE interface.
  Copyright 1994, 1995
  Mark A. Finney, Systems for Environmental Management
  See LICENSE.TXT file for license information.
*/
#ifndef FSGLBVAR_H
#define FSGLBVAR_H

#include<stdlib.h>
#include<stdio.h>
#include"themes.h"
#include"fsxwatm.h"
#include"fsxw.h"
#include"fsx4.h"

#define NULLLOCATION       0
#define IGNITIONLOCATION   1
#define BARRIERLOCATION    2
#define FUELLOCATION       3
#define WEATHERLOCATION    4
#define ZOOMLOCATION       5
#define EDITLOCATION       6
#define DIRECTLOCATION     7
#define INDIRECTLOCATION   8
#define PARALLELLOCATION   9
#define AERIALLOCATION    10
#define RELOCATEDIRECT    11
#define RELOCATEINDIRECT  12
#define RELOCATEPARALLEL  13
#define RELOCATEAIRATTACK 14
#define ADDGROUNDATTACK   15
#define FIREINFORMATION   16
#define MOVEPERIMPOINTS   17
#define PANLANDSCAPE      18
#define STOPLOCATION      19
#define MEASURELANDSCAPE  20

#define WW_UNITS_METRIC     1
#define WW_UNITS_ENGLISH    0
#define WW_UNITS_UNDEFINED -1

#define RAST_ARRIVALTIME       20
#define RAST_FIREINTENSITY     21
#define RAST_SPREADRATE        22
#define RAST_FLAMELENGTH       23
#define RAST_HEATPERAREA       24
#define RAST_CROWNFIRE         25
#define RAST_FIREDIRECTION     26
#define RAST_REACTIONINTENSITY 27

#define E_DATA 0
#define S_DATA 1
#define A_DATA 2
#define F_DATA 3
#define C_DATA 4
#define H_DATA 5
#define B_DATA 6
#define P_DATA 7
#define D_DATA 8
#define W_DATA 9

#define FM_INTERVAL_TIME  0
#define FM_INTERVAL_ELEV  1
#define FM_INTERVAL_SLOPE 2
#define FM_INTERVAL_ASP   3
#define FM_INTERVAL_COV   4

typedef unsigned long COLORREF;

#define GROUND  1  //Attack resource type
#define AIR     2  //Attack resource type

double pow2( double base );    //Returns square of base
int GetFuelsAltered();  //Retrieve flag for altered fuel map
void SetFuelsAltered( int YesNo );  //Set flag for altered fuel map
long GetTheme_DistanceUnits();
short GetTheme_Units( short DataTheme );
short GetTheme_NumCategories( short DataTheme );
long GetTheme_HiValue( short DataTheme );
long GetTheme_LoValue( short DataTheme );
char *GetTheme_FileName( short DataTheme );
bool LinkDensityWithCrownCover( long TrueFalse );   //Returns true or false
int CheckCellResUnits();  //Returns 0 for metric 1 for English
double GetCellResolutionX();  //Return landscape X cell dimension
double GetCellResolutionY();  //Return landscape Y cell dimension
double MetricResolutionConvert();  // 3.28..for english, 1.0 for metric
long CanSetRasterResolution( long YesNo ); //Flag to allow setting of rast res
//Flag to reduce raster extent for viewport (below).
long SetRasterExtentToViewport( long YesNo );
void GetRastRes( double *XResol, double *YResol );  //Get raster output res
long GetInputMode();    //Gets input mode to the landscape window

long GetRastFormat();
long GetVectFormat();
void SetRastMake( bool YesNo );  //Make/don't make raster files
bool GetRastMake();
void SetVectMake( bool YesNo );  //Make/don't make vector files
bool GetVectMake();
long ExcludeBarriersFromVectorFiles( long YesNo );  //Include barriers or not
bool GetVectVisOnly();
void SetShapeMake( bool YesNo );
bool GetShapeMake();
long AccessDisplayUnits( long val );
long AccessOutputUnits( long val );

long OutputFireParam( long Param );  //Select fire chx for display

void SetCanopyChx( double Height, double CrownBase, double BulkDensity,
                   double Diameter, long FoliarMoisture, long Tolerance,
                   long Species );
double GetDefaultCrownHeight();
double GetDefaultCrownBase();
double GetDefaultCrownBD( short cover );
double GetAverageDBH();
double GetFoliarMC();
long GetTolerance();
long GetCanopySpecies();

int GetFuelConversion( int fuel );
int SetFuelConversion( int fueltype, int fuelmodel );
double GetFuelDepth( int Number );
long AccessFuelModelUnits( long Val );

void SetTint( double tint );
double GetTint();
void SetMaxWindowDim( long Dim );
bool ViewPortStatus( long TrueFalse );  //See if view port has changed
bool ViewPortChanged( long TrueFalse );
void SetLandscapeViewPort( double north, double south, double east,
                           double west );
double GetViewNorth();
double GetViewSouth();
double GetViewEast();
double GetViewWest();
long GetNumViewEast();
long GetNumViewNorth();
double arp();
void BoundingBox();
void ReversePoints( int TYPE );

long AdjustIgnitionSpreadRates( long YesNoValue );
bool PreserveInactiveEnclaves( long YesNo );

void SetRasterFileName( const char *FileName );
char *GetRasterFileName( long Type );
void SetVectorFileName( const char *FileName );
char *GetVectorFileName();
void ResetVisPerimFile();
void InitSuppressionResources( const char *FileName ); //Added Aug30,2010

void SetFileOutputOptions( long FileType, bool YesNo );
bool GetFileOutputOptions( long FileType );

void  SetShapeFileChx( const char *ShapeFileName, long VisOnly, long Polygons,
                       long BarriersSep );
char* GetShapeFileChx( long *VisOnly, long *Polygons, long *BarriersSep );
void trim( char* totrim );
LandscapeTheme *GetLandscapeTheme();

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  long ID;
  RasterTheme *theme;
  void *last;
  void *next;
  long Changed;
  bool Selected;
} Themes;

Themes* GetRasterTheme( long Num );
Themes* AllocRasterTheme();
long GetNumRasterThemes();

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  long ID;
  VectorTheme theme;
  void *next;
  void *last;
} VectorStorage;

long GetNumVectorThemes();
long AllocVectorTheme();
VectorTheme* GetVectorTheme( long Num );
void FreeAllVectorThemes();

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  streamdata
  Structure for holding weather/wind stream data max and min values.
*/
typedef struct {
  int wtr;
  int wnd;
  int all;
} streamdata;

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  WindData
  Wind data structure.
*/
typedef struct {
  long   mo;
  long   dy;
  long   hr;
  double ws;  //Windspeed mph
  long   wd;  //Winddirection azimuth
  long   cl;  //Cloudiness
} WindData;

bool CalcFirstLastStreamData();  //Compute 1st & last valid dates for sim
void SetCorrectStreamNumbers();  //For Atm Grid to fool stream data
long GetOpenWindStation();
long AllocWindData( long StatNum, long NumObs );
void FreeWindData( long StationNumber );
long SetWindData( long StationNumber, long NumObs, long month, long day,
                  long hour, double windspd, long winddir, long cloudy );
long GetWindMonth( long StationNumber, long NumObs );
long GetWindDay( long StationNumber, long NumObs );
long GetWindHour( long StationNumber, long NumObs );
double GetWindSpeed( long StationNumber, long NumObs );
long GetWindDir( long StationNumber, long NumObs );
long GetWindCloud( long StationNumber, long NumObs );
long GetMaxWindObs( long StationNumber );
void SetWindUnits( long StationNumber, long Units );
void SetWeatherUnits( long StationNumber, long Units );
long GetWindUnits( long StationNumber );
long GetWeatherUnits( long StationNumber );

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  long   mo;
  long   dy;
  double rn;
  long   t1;
  long   t2;
  double T1;
  double T2;
  long   H1;
  long   H2;
  double el;
  long   tr1;
  long   tr2;
} WeatherData;

long GetOpenWeatherStation();
long AllocWeatherData( long StatNum, long NumObs );
void FreeWeatherData( long StationNumber );
long SetWeatherData( long StationNumber, long NumObs, long month, long day,
                     double rain, long time1, long time2, double temp1,
                     double temp2, long humid1, long humid2, double elevation,
                     long tr1, long tr2 );
long GetWeatherMonth( long StationNumber, long NumObs );
long GetWeatherDay( long StationNumber, long NumObs );
double GetWeatherRain( long StationNumber, long NumObs );
long GetWeatherTime1( long StationNumber, long NumObs );
long GetWeatherTime2( long StationNumber, long NumObs );
double GetWeatherTemp1( long StationNumber, long NumObs );
double GetWeatherTemp2( long StationNumber, long NumObs );
long GetWeatherHumid1( long StationNumber, long NumObs );
long GetWeatherHumid2( long StationNumber, long NumObs );
double GetWeatherElev( long StationNumber, long NumObs );
void GetWeatherRainTimes( long StationNumber, long NumObs, long *tr1,
                          long *tr2 );
long GetMaxWeatherObs( long StationNumber );

//============================================================================
struct StationGrid
{ //StationGrid
  long   XDim;    //Number of Easting cells
  long   YDim;    //Number of Northing cells
  double Width;    //Width of grid cell
  double Height;    //Height of grid cell
  long   *Grid;    //Holds weather/wind station numbers
  StationGrid();
  ~StationGrid();
};//StationGrid

void AllocStationGrid( long XDim, long YDim );  //Allocate memory for wx stns
long FreeStationGrid();                     //Free memory for weather stations
long GetStationNumber( double xpt, double ypt );  //Find stn number for coord
long SetStationNumber( long XPos, long YPos, long StationNumber );
void SetGridEastDimension( long XDim );
void SetGridNorthDimension( long YDim );
long GetGridEastDimension();
long GetGridNorthDimension();
long GridInitialized();
long GetNumStations();  //Retrieve number of weather/wind stations
void SetGridNorthOffset( double offset );  //Set offset for Atm Grid purposes
void SetGridEastOffset( double offset );

//============================================================================
struct FuelConversions
{ //FuelConversions
  int Type[257];        //Each fuel type contains a fuel model corresponding
  FuelConversions();    //load defaults
};//FuelConversions

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Header for landscape file.
typedef struct {
  long   CrownFuels;
  long   latitude;
  long   loeast;
  long   hieast;
  long   lonorth;
  long   hinorth;
  long   loelev;
  long   hielev;
  long   numeast;
  long   numnorth;
  double EastUtm;
  double WestUtm;
  double NorthUtm;
  double SouthUtm;
  long   GridUnits;        //0 for metric, 1 for English
  double XResol;
  double YResol;
  short  EUnits;
  short  SUnits;
  short  AUnits;
  short  FOptions;
  short  CUnits;
  short  HUnits;
  short  BUnits;
  short  PUnits;
} oldheaddata;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Header for landscape file.
typedef struct {
  long   CrownFuels;       //10 if no crown fuels, 11 if crown fuels exist
  long   latitude;
  double loeast;
  double hieast;
  double lonorth;
  double hinorth;
  long   loelev;
  long   hielev;
  long   numeast;
  long   numnorth;
  double EastUtm;
  double WestUtm;
  double NorthUtm;
  double SouthUtm;
  long   GridUnits;        //0 for metric, 1 for English
  double XResol;
  double YResol;
  short  EUnits;
  short  SUnits;
  short  AUnits;
  short  FOptions;
  short  CUnits;
  short  HUnits;
  short  BUnits;
  short  PUnits;
} headdata2;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Header for landscape file.
#ifndef OS_X
#pragma pack(push) //JWB 201004: Needs this (at least in VC6) so there's not a
#pragma pack(1)    //byte-boundary reading problem in fread() in ReadHeader().
#endif
typedef struct {
  long   CrownFuels;     //20 if no crown fuels, 21 if crown fuels exist
  long   GroundFuels;    //20 if no ground fuels, 21 if ground fuels exist
  long   latitude;
  double loeast;
  double hieast;
  double lonorth;
  double hinorth;
  long   loelev;
  long   hielev;
  long   numelev;    //-1 if more than 100 categories
  long   elevs[100];
  long   loslope;
  long   hislope;
  long   numslope;    //-1 if more than 100 categories
  long   slopes[100];
  long   loaspect;
  long   hiaspect;
  long   numaspect;    //-1 if more than 100 categories
  long   aspects[100];
  long   lofuel;
  long   hifuel;
  long   numfuel;    //-1 if more than 100 categories
  long   fuels[100];
  long   locover;
  long   hicover;
  long   numcover;    //-1 if more than 100 categories
  long   covers[100];
  long   loheight;
  long   hiheight;
  long   numheight;    //-1 if more than 100 categories
  long   heights[100];
  long   lobase;
  long   hibase;
  long   numbase;    //-1 if more than 100 categories
  long   bases[100];
  long   lodensity;
  long   hidensity;
  long   numdensity;    //-1 if more than 100 categories
  long   densities[100];
  long   loduff;
  long   hiduff;
  long   numduff;    //-1 if more than 100 categories
  long   duffs[100];
  long   lowoody;
  long   hiwoody;
  long   numwoody;    //-1 if more than 100 categories
  long   woodies[100];
  long   numeast;
  long   numnorth;
  double EastUtm;
  double WestUtm;
  double NorthUtm;
  double SouthUtm;
  long   GridUnits;        // 0 for metric, 1 for English
  double XResol;
  double YResol;
  short  EUnits;
  short  SUnits;
  short  AUnits;
  short  FOptions;
  short  CUnits;
  short  HUnits;
  short  BUnits;
  short  PUnits;
  short  DUnits;
  short  WOptions;
  char   ElevFile[256];
  char   SlopeFile[256];
  char   AspectFile[256];
  char   FuelFile[256];
  char   CoverFile[256];
  char   HeightFile[256];
  char   BaseFile[256];
  char   DensityFile[256];
  char   DuffFile[256];
  char   WoodyFile[256];
  char   Description[512];
} headdata;
#ifndef OS_X
#pragma pack()  //JWB 2010: See above
#endif

bool TestForLCPVersion1();
bool TestForLCPVersion2();
bool ConvertLCPFileToVersion2();
bool ConvertLCPFile2ToVersion4();
void ReadHeader();    //Only for version 4
long HaveCrownFuels();
long HaveGroundFuels();
celldata CellData( double east, double north, celldata &cell,
                   crowndata &cfuel, grounddata &gfuel, long *posit );
void GetCellDataFromMemory( long posit, celldata &cell, crowndata &cfuel,
                            grounddata &gfuel );
bool GetBinaryCellData( unsigned long row, unsigned long col,
                        unsigned long num, short *data );
long GetCellPosition( double east, double north );
bool HaveCustomFuelModels();
bool HaveFuelConversions();
bool NeedCustFuelModels();
bool NeedConvFuelModels();
void SetCustFuelModelID( bool True_False );
void SetConvFuelModelID( bool True_False );
long DurationResetAtRestart( long YesNo );
long IgnitionResetAtRestart( long YesNo );
long RotateSensitiveIgnitions( long YesNo );
long ShowFiresAsGrown( long YesNo );
long GetVisibleTheme();

bool OpenLandFile();
void CloseLandFile();
void SetLandFileName( const char* FileName );
char* GetLandFileName();

double GetWestUtm();
double GetEastUtm();
double GetSouthUtm();
double GetNorthUtm();
double GetLoEast();
double GetHiEast();
double GetLoNorth();
double GetHiNorth();
long GetLoElev();
long GetHiElev();
long GetNumEast();
long GetNumNorth();
double ConvertEastingOffsetToUtm( double input );
double ConvertNorthingOffsetToUtm( double input );
double ConvertUtmToEastingOffset( double input );
double ConvertUtmToNorthingOffset( double input );

long GetConditMonth();
long GetConditDay();
long GetConditMinDeficit();
long GetLatitude();
long GetStartMonth();
long GetStartDay();
long GetStartHour();
long GetStartMin();
long GetStartDate();
void SetStartDate( long input );
long GetMaxMonth();
long GetMaxDay();
long GetMinMonth();
long GetMinDay();
long GetJulianDays( long Month );
double ConvertActualTimeToSimtime( long mo, long dy, long hr, long mn,
                                   bool FromCondit );
void ConvertSimtimeToActualTime( double SimTime, long *mo, long *dy, long *hr,
                                 long *mn, bool FromCondit );
void SetSimulationDuration( double minutes );
double GetSimulationDuration();
bool UseConditioningPeriod( long YesNo );
bool EnvironmentChanged( long Changed, long StationNumber, long FuelSize );
long GetMoistCalcInterval( long FM_SIZECLASS, long CATEGORY );

bool SetAtmosphereGrid( long NumGrids );
AtmosphereGrid* GetAtmosphereGrid();
long AtmosphereGridExists();

//----------------------------------------------------------
//Burn Period functions
//----------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  long Month;
  long Day;
  long Start;
  long End;
} AbsoluteBurnPeriod;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  double Start;
  double End;
} RelativeBurnPeriod;

void AddDownTime( double time );
double GetDownTime();
bool AllocBurnPeriod( long num );
void FreeBurnPeriod();
void SetBurnPeriod( long num, long mo, long dy, long start, long end );
bool InquireInBurnPeriod( double SimTime );
void ConvertAbsoluteToRelativeBurnPeriod();

//----------------------------------------------------------
//Coarse Woody Data Structures and Access Functions
//----------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Initial fuel moistures by fuel type.
typedef struct {
  bool FuelMoistureIsHere;
  long TL1;
  long TL10;
  long TL100;
  long TLLH;
  long TLLW;
} InitialFuelMoisture;

bool SetInitialFuelMoistures( long model, long t1, long t10, long t100,
                              long tlh, long tlw );
bool GetInitialFuelMoistures( long model, long *t1, long *t10, long *t100,
                              long *tlh, long *tlw );
long GetInitialFuelMoisture( long Model, long FuelClass );
bool InitialFuelMoistureIsHere( long Model );

//----------------------------------------------------------
//Coarse Woody Data Structures and Access Functions
//----------------------------------------------------------

#define MAXNUM_COARSEWOODY_MODELS 256
#define CWD_COMBINE_NEVER    1
#define CWD_COMBINE_ALWAYS   2
#define CWD_COMBINE_ABSENT   3

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  double SurfaceAreaToVolume;
  double Load;
  double HeatContent;
  double Density;
  double FuelMoisture;
} WoodyData;

//============================================================================
struct CoarseWoody
{ //CoarseWoody
  long   Units;
  double Depth;
  double TotalWeight;
  long   NumClasses;
  char   Description[64];
  WoodyData *wd;

  CoarseWoody();
};//CoarseWoody

bool AllocCoarseWoody( long ModelNumber, long NumClasses );
void FreeCoarseWoody( long ModelNumber );
void FreeAllCoarseWoody();
bool SetWoodyData( long ModelNumber, long ClassNumber, WoodyData *wd,
                   long Units );
bool SetWoodyDataDepth( long ModelNumber, double depth,
                        const char *Description );
bool SetNFFLWoody();
void GetWoodyData( long WoodyModelNumber, long SurfModelNumber,
                   long *NumClasses, WoodyData *woody, double *Depth,
                   double *TotalLoad );
long GetWoodyDataUnits( long ModelNumber, char *Description );
void GetCurrentFuelMoistures( long fuelmod, long woodymod, double *MxIn,
                              double *MxOut, long NumMx );
double WeightLossErrorTolerance( double value );
long WoodyCombineOptions( long Options );
double Get1000HrMoisture( long ModelNumber, bool Average );

//---------------------------------------------------------------
//Thread Functions
//---------------------------------------------------------------

long GetMaxThreads();

//-----------------------------------------------------------------
//New fuel model support
//-----------------------------------------------------------------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
typedef struct {
  long   number;
  char   code[8];
  double h1;
  double h10;
  double h100;
  double lh;
  double lw;
  long   dynamic;
  long   sav1;
  long   savlh;
  long   savlw;
  double depth;
  double xmext;
  double heatd;
  double heatl;
  char   desc[256];
} NewFuel;

void ResetNewFuels();
bool SetNewFuel( NewFuel *newfuel );
bool GetNewFuel( long number, NewFuel *newfuel );
void InitializeNewFuel();
bool IsNewFuelReserved( long number );

extern long   ConditMonth, ConditDay;
extern long   StartMonth, StartDay, StartHour, StartMin;
extern long   EndMonth, EndDay, EndHour, EndMin;
extern double SimulationDuration;
#endif //FSGLBVAR_H
