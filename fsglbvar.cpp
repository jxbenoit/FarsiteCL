/*fsglbvar.cpp
  Global Variable and Functions for FARSITE
  Copyright 1994, 1995  Mark A. Finney, Systems for Environmental Management
  See LICENSE.TXT file for license information.
*/
#include<iostream>
#include<sys/stat.h>
#include"fsglbvar.h"
#include"fcopy.h"
#include"globals.h"
#include"portablestrings.h"
#include"fsxwattk.h"
#include"fsairatk.h"
#include"LCPAnalyzer.h"

using namespace std;

long ConditMonth = 0, ConditDay = 0;
long StartMonth = 0, StartDay = 0, StartHour = 0, StartMin = 0;
long EndMonth = 0, EndDay = 0, EndHour = 0, EndMin = 0;
double SimulationDuration = 0.0;

static long startdate = 0;
static double tint;   //Smallest interval of x & y extent of visible landscape
static long MaxWindowDim = 8000;  //Maximum farsite window dimension
static bool ViewPort_Changed = false;
static bool ViewPort_Status = false;
static double ViewPortNorth = 0;
static double ViewPortSouth = 0;
static double ViewPortEast = 0;
static double ViewPortWest = 0;
static long NumViewNorth;
static long NumViewEast;
static long AdjustIgSpreadRates = 0; //Can use init spread rate dlg for ignits
static int FuelsAltered = 0;
static FuelConversions fuelconversion;
static bool NEED_CUST_MODELS = false; //Custom fuel models
static bool HAVE_CUST_MODELS = false;
static bool NEED_CONV_MODELS = false;     //Fuel model conversions
static bool HAVE_CONV_MODELS = false;
static long CanSetRastRes = true;  //Flag to allow setting raster resolution
static long RasterVPExtent = false;
static double RasterCellResolutionX;
static double RasterCellResolutionY;
static bool CondPeriod = false;
static bool EnvtChanged[4][5] =
{
  {false, false, false, false, false}, {false, false, false, false, false},
  {false, false, false, false, false}, {false, false, false, false, false}
};

static char RasterArrivalTime[256];
static char RasterFireIntensity[256];
static char RasterFlameLength[256];
static char RasterSpreadRate[256];
static char RasterHeatPerArea[256];
static char RasterCrownFire[256];
static char RasterFireDirection[256];
static char RasterReactionIntensity[256];

static bool rast_arrivaltime = false;
static bool rast_fireintensity = false;
static bool rast_spreadrate = false;
static bool rast_flamelength = false;
static bool rast_heatperarea = false;
static bool rast_crownfire = false;
static bool rast_firedirection = false;
static bool rast_reactionintensity = false;

static char VectFName[256];
static char RastFName[256];
static char ShapeFileName[256] = "";
static long ShapeVisibleStepsOnly = 1;
static long ShapePolygonsNotLines = 1;
static long ShapeBarriersSeparately = 0;
static long VisibleThemeNumber = 3;  //Fuels default visible theme
static long LandscapeInputMode = NULLLOCATION;
//Crown bulk density is a function of canopy cover (below).
static bool CrownDensityLinkToCC = false;

static bool RastMake = false;
static bool VectMake = false;
static bool ShapeMake = false;
static bool VISONLY = false;    //Visible time step only for vector file
static long RastFormat = 1;
static long VectFormat = 0;
static char VisPerim[256] = "";//"c:\\visperim.vsp";
static WindData* wddt[5] = { 0, 0, 0, 0, 0 };
static WeatherData* wtrdt[5] = { 0, 0, 0, 0, 0 };
static long MaxWeatherObs[5] = { 0, 0, 0, 0, 0 };
static long MaxWindObs[5] = { 0, 0, 0, 0, 0 };
static long DurationReset = 0;    //Force DurationReset at ReStart
static long IgnitionReset = 0;
static long RotateIgnitions = 0;
static long ShowVectsAsGrown = 1;
static bool InactiveEnclaves = true;
static char getThemeFileName_name[256] = "";

//------------------ Display Fire Characteristics on 2-D and 3-D Landscapes
static long FireOutputParam = 4;
//----------------------------------------------------------------------------

static streamdata FirstMonth[5];
static streamdata FirstDay[5];
static streamdata FirstHour[5];
static streamdata LastMonth[5];
static streamdata LastDay[5];
static streamdata LastHour[5];

long MoistCalcInterval[4][5] =
{
  { 60, 200, 10, 45, 15},    // 1hr
  {120, 200, 10, 45, 15},    // 10hr
  {180, 400, 10, 45, 20},    // 100hr
  {240, 400, 15, 45, 30}     // 1000hr
};

NewFuel newfuels[257];    //Custom and standard fuel models

static long OldFilePosition = 0;
static headdata2 Header2;
static headdata Header;
static oldheaddata OldHeader;

//202003041717:
//Now setting headsize=0 to indicate it should be set from within
//either ReadHeader() or CellData(), using results from
//LCPAnalyzer.
//static size_t headsize = sizeof( Header );
static size_t headsize = 0;

static FILE* landfile = 0;
static char LandFName[256] = "";
static StationGrid grid;
static long NumWeatherStations = 0;
static long NumWindStations = 0;
static double NorthGridOffset = 0.0;
static double EastGridOffset = 0.0;

static short* landscape = 0;

static CanopyCharacteristics CanopyChx;

static long NumVectorThemes = 0;

static AtmosphereGrid* AtmGrid = 0;    //Pointer to AtmGrid;

/*============================================================================
  arp
  Calculates area and perimeter as a planimeter (with triangles).
*/
double arp()
{
/*
  long   count, count1=0, FireNum=GetNewFires()-1, numx;
  double xpt1, ypt1, xpt2, ypt2, aangle, zangle, DiffAngle;
  double newarea, area=0.0;
  numx=GetNumPoints( FireNum );
  if( numx < 3 ) return area;
  poly.startx = GetPerimeter1Value( FireNum, 0, 0 );
  poly.starty = GetPerimeter1Value( FireNum, 0, 1 );
  while( count1 < numx ) {
    count1++;
    xpt1 = GetPerimeter1Value( FireNum, 1, 0 );
    ypt1 = GetPerimeter1Value( FireNum, 1, 1 );
    zangle = poly.direction(xpt1, ypt1);
    if( zangle!=999.9 ) break;
  }
  count1++;
  for( count=count1; count < numx; count++ ) {
    xpt2 = GetPerimeter1Value( FireNum, count, 0 );
    ypt2 = GetPerimeter1Value( FireNum, count, 1 );
    newarea = .5*( poly.startx * ypt1 - xpt1 * poly.starty + xpt1 * ypt2 -
                   xpt2 * ypt1 + xpt2 * poly.starty - poly.startx * ypt2 );
    newarea = fabs( newarea );
    aangle = poly.direction( xpt2, ypt2 );
    if( aangle!=999.9 ) {
      DiffAngle = aangle - zangle;
      if( DiffAngle > PI ) DiffAngle = -( 2.0*PI-DiffAngle );
      else if( DiffAngle < -PI ) DiffAngle = 2.0 * PI + DiffAngle;
      if( DiffAngle > 0.0 ) area -= newarea;
      else if( DiffAngle < 0.0 ) area += newarea;
      zangle = aangle;
    }
    xpt1 = xpt2;
    ypt1 = ypt2;
  }
  return area;
*/
  return 0; //Delete when uncommenting
}

//============================================================================
void ReversePoints( int TYPE )
{ //ReversePoints
/*
  long   j, AFire = GetNewFires() - 1;
  long   count, BFire = GetNewFires();
  double fxc, fyc, fxc2, fyc2, RosI, RosI2;
  long   halfstop = GetNumPoints( AFire ) / 2;    //Truncated number of points

  switch( TYPE ) {
    case 0:  //Transfer points in reverse to next array, +1 fires
      AllocPerimeter1( BFire, GetNumPoints(AFire) );  //Allocate new array
      for( count=0; count < GetNumPoints(AFire); count++ ) {
        fxc = GetPerimeter1Value( AFire, count, 0 );
        fyc = GetPerimeter1Value( AFire, count, 1 );
        RosI = GetPerimeter1Value( AFire, count, 2 );
        j = GetNumPoints( AFire )- ( count + 1 );
        SetPerimeter1( BFire, j, fxc, fyc );  //Reverse points
        SetFireChx( BFire, j, RosI, 0 );  //In next available array
      }     //Set bounding box
      SetPerimeter1( BFire, count, GetPerimeter1Value(AFire, count, 0),
                     GetPerimeter1Value(AFire, count, 1) );    //Reverse points
      SetFireChx( BFire, count, GetPerimeter1Value(AFire, count, 2),
                  GetPerimeter1Value(AFire, count, 3) );  //In next avail array
      SetNumPoints( BFire, count );    //Same number of points in fire
      SetInout( BFire,1 );    //ID fire as outward burning
      IncNewFires( 1 );
      break;
    case 1:  //Reverse points in same array
      for( count=0; count < halfstop; count++ ) {
        j = GetNumPoints( AFire ) - count - 1;
        fxc = GetPerimeter1Value( AFire, count, 0 );
        fyc = GetPerimeter1Value( AFire, count, 1 );
        RosI = GetPerimeter1Value( AFire, count, 2 );
        fxc2 = GetPerimeter1Value( AFire, j, 0 );
        fyc2 = GetPerimeter1Value( AFire, j, 1 );
        RosI2 = GetPerimeter1Value( AFire, j, 2 );
        SetPerimeter1( AFire, count, fxc2, fyc2 );
        SetFireChx( AFire, count, RosI2, 0 );
        SetPerimeter1( AFire, j, fxc, fyc );
        SetFireChx( AFire, j, RosI, 0 );
      }
      break;
  }
*/
} //ReversePoints

//============================================================================
void BoundingBox()
{ //BoundingBox
/*
  double xpt, ypt, Xlo, Xhi, Ylo, Yhi;
  long   NumFire = GetNewFires() - 1;
  long   NumPoint = GetNumPoints( NumFire );

  Xlo = Xhi = GetPerimeter1Value( NumFire, 0, 0 );
  Ylo = Yhi = GetPerimeter1Value( NumFire, 0, 1 );
  for( int i = 1; i < NumPoint; i++ ) {
    xpt = GetPerimeter1Value( NumFire, i, 0 );
    ypt = GetPerimeter1Value( NumFire, i, 1 );
    if( xpt < Xlo ) Xlo = xpt;
    else {
      if( xpt > Xhi ) Xhi = xpt;
    }
    if( ypt < Ylo ) Ylo = ypt;
    else {
      if( ypt > Yhi ) Yhi = ypt;
    }
  }
  SetPerimeter1( NumFire, NumPoint, Xlo, Xhi );
  SetFireChx( NumFire, NumPoint, Ylo, Yhi );
*/
} //BoundingBox

/*============================================================================
  pow2
  Replaces pow(X,Y) where Y==2.
*/
double pow2( double base ) { return base * base; }

/*============================================================================
  SetTint
  Sets minimum display units for landcape window.
*/
void SetTint( double Tint ) { tint = Tint; }

/*============================================================================
  GetTint
  Retrieves minimum display unit for landscappe window.
*/
double GetTint() { return tint; }

/*============================================================================
  SetMaxWindowDim
  Sets dimension of landscape window.
*/
void SetMaxWindowDim( long Dim ) { MaxWindowDim = Dim; }

//============================================================================
bool ViewPortChanged( long TrueFalse )
{ //ViewPortChanged
  if( TrueFalse >= 0 ) ViewPort_Changed = TrueFalse ? true : false;

  return ViewPort_Changed;
} //ViewPortChanged

//============================================================================
bool ViewPortStatus( long TrueFalse )
{ //ViewPortStatus
  if( TrueFalse >= 0 ) {
    ViewPort_Status = TrueFalse ? true : false;

    if( TrueFalse == 0 ) {
      ViewPortNorth = Header.hinorth;
      ViewPortSouth = Header.lonorth;
      ViewPortEast = Header.hieast;
      ViewPortWest = Header.loeast;
    }
  }

  return ViewPort_Status;
} //ViewPortStatus

//============================================================================
void SetLandscapeViewPort( double north, double south, double east,
                           double west )
{ //SetLandscapeViewPort
  double cols, rows, f, ic, ir;

  ViewPortNorth = north;
  ViewPortSouth = south;
  ViewPortEast = east;
  ViewPortWest = west;
  rows = ( (ViewPortNorth - ViewPortSouth) / GetCellResolutionY() );
  f = modf( rows, &ir );
  NumViewNorth = (long) ir;
  if( f > 0.50 ) NumViewNorth++;
  cols = ( (ViewPortEast - ViewPortWest) / GetCellResolutionX() );
  f = modf( cols, &ic );
  NumViewEast = (long) ic;
  if( f > 0.5 ) NumViewEast++;
} //SetLandscapeViewPort

//============================================================================
double GetViewNorth() { return ViewPortNorth; }

//============================================================================
double GetViewSouth() { return ViewPortSouth; }

//============================================================================
double GetViewEast() { return ViewPortEast; }

//============================================================================
double GetViewWest() { return ViewPortWest; }

//============================================================================
long GetNumViewEast() { return NumViewEast; }

//============================================================================
long GetNumViewNorth() { return NumViewNorth; }

//============================================================================
long AdjustIgnitionSpreadRates( long YesNoValue )
{ //AdjustIgnitionSpreadRates
  if( YesNoValue > -1 ) AdjustIgSpreadRates = YesNoValue;

  return AdjustIgSpreadRates;
} //AdjustIgnitionSpreadRates

//============================================================================
CanopyCharacteristics::CanopyCharacteristics()
{ //CanopyCharacteristics::CanopyCharacteristics
  DefaultHeight = Height = 15.0;  //Need default for changing Variables in LCP
  DefaultBase = CrownBase = 4.0;
  DefaultDensity = BulkDensity = 0.20;
  Diameter = 20.0;
  FoliarMC = 100;
  Tolerance = 2;
  Species = 1;
} //CanopyCharacteristics::CanopyCharacteristics

//============================================================================
void SetCanopyChx( double Height, double CrownBase, double BulkDensity,
                   double Diameter, long FoliarMoisture, long Tolerance,
                   long Species )
{ //SetCanopyChx
  CanopyChx.Height = CanopyChx.DefaultHeight = Height;
  CanopyChx.CrownBase = CanopyChx.DefaultBase = CrownBase;
  CanopyChx.BulkDensity = CanopyChx.DefaultDensity = BulkDensity;
  CanopyChx.Diameter = Diameter;
  CanopyChx.FoliarMC = FoliarMoisture;
  CanopyChx.Tolerance = Tolerance;
  CanopyChx.Species = Species;
} //SetCanopyChx

//============================================================================
double GetDefaultCrownHeight() { return CanopyChx.Height; }

//============================================================================
double GetDefaultCrownBase() { return CanopyChx.CrownBase; }

//============================================================================
double GetDefaultCrownBD( short cover )
{ //GetDefaultCrownBD
  if( CrownDensityLinkToCC )
    return CanopyChx.BulkDensity * ( (double) cover ) / 100.0;

  return CanopyChx.BulkDensity;
} //GetDefaultCrownBD

//============================================================================
double GetAverageDBH() { return CanopyChx.Diameter; }

//============================================================================
double GetFoliarMC() { return CanopyChx.FoliarMC; }

//============================================================================
long GetTolerance() { return CanopyChx.Tolerance; }

//============================================================================
long GetCanopySpecies() { return CanopyChx.Species; }

/*============================================================================
  GetInputMode
  Gets input mode to the landscape window.
*/
long GetInputMode() { return LandscapeInputMode; }

//============================================================================
int GetFuelsAltered() { return FuelsAltered; }

//============================================================================
void SetFuelsAltered( int YesNo ) { FuelsAltered = YesNo; }

//============================================================================
FuelConversions::FuelConversions()
{ //FuelConversions::FuelConversions
  long i;

  for( i = 0; i < 257; i++ )    //Could also read default file here
    Type[i] = i;
  Type[99] = -1;
  Type[98] = -2;
  Type[97] = -3;
  Type[96] = -4;
  Type[95] = -5;
  Type[94] = -6;
  Type[93] = -7;
  Type[92] = -8;
  Type[91] = -9;
} //FuelConversions::FuelConversions

/*============================================================================
  GetFuelConversion
  Retrieve fuel model conversions.
*/
int  GetFuelConversion( int fuel )
{ //GetFuelConversion
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsglbvar:GetFuelConversion:1 fuel=%d\n",
            CallLevel, "", fuel );

  int cnv = -1;

  if( fuel >= 0 && fuel < 257 ) {  //Check fuel for valid array range
    cnv = fuelconversion.Type[fuel];   //Get new fuel
    if( Verbose >= CallLevel )
      printf( "%*sfsglbvar:GetFuelConversion:1a cnv=%d\n",
              CallLevel, "", cnv );

    if( cnv < 1 ) {
      if( cnv < -9 ) cnv = -1;
    }
    //Check to see if custom model required (below).
    else if( ! IsNewFuelReserved(cnv) ) {
      if( newfuels[cnv].number == 0 )  //Check to see if cust mod here
        cnv = -1;     //Return rock if no fuel model
    }
    else if( cnv > 256 )  //If new fuel too high
      cnv = -1;
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsglbvar:GetFuelConversion:2 cnv=%d\n",
            CallLevel, "", cnv );
  CallLevel--;

  return cnv;
} //GetFuelConversion

/*============================================================================
  SetFuelConversion
  Set fuel model conversions.
*/
int SetFuelConversion( int From, int To )
{ //SetFuelConversion
  if( From >= 0 && From < 257 && To < 257 && To >= 0 ) {
    if( To > 90 && To < 100 )
      To = To - 100;    //Make all negative for the 90's, indicate unburnable
    else if( To == 0 ) To = -1;
    fuelconversion.Type[From] = To;
  }
  else return false;

  return true;
} //SetFuelConversion

//============================================================================
long GetTheme_DistanceUnits() { return Header.GridUnits; }

//============================================================================
short GetTheme_Units( short DataTheme )
{ //GetTheme_Units
  short units;

  switch( DataTheme ) {
    case 0: units = Header.EUnits;  break;
    case 1: units = Header.SUnits;  break;
    case 2: units = Header.AUnits;  break;
    case 3: units = Header.FOptions;  break;
    case 4: units = Header.CUnits;  break;
    case 5: units = Header.HUnits;  break;
    case 6: units = Header.BUnits;  break;
    case 7: units = Header.PUnits;  break;
    case 8: units = Header.DUnits;  break;
    case 9: units = Header.WOptions;  break;
  }

  return units;
} //GetTheme_Units

//============================================================================
short GetTheme_NumCategories( short DataTheme )
{ //GetTheme_NumCategories
  long cats;

  switch( DataTheme ) {
    case 0: cats = Header.numelev;  break;
    case 1: cats = Header.numslope;  break;
    case 2: cats = Header.numaspect;  break;
    case 3: cats = Header.numfuel;  break;
    case 4: cats = Header.numcover;  break;
    case 5: cats = Header.numheight;  break;
    case 6: cats = Header.numbase;  break;
    case 7: cats = Header.numdensity;  break;
    case 8: cats = Header.numduff;  break;
    case 9: cats = Header.numwoody;  break;
  }

  return (short) cats;
} //GetTheme_NumCategories

//============================================================================
long GetTheme_HiValue( short DataTheme )
{ //GetTheme_HiValue
  long hi;

  switch( DataTheme ) {
    case 0: hi = Header.hielev;  break;
    case 1: hi = Header.hislope;  break;
    case 2: hi = Header.hiaspect;  break;
    case 3: hi = Header.hifuel;  break;
    case 4: hi = Header.hicover;  break;
    case 5: hi = Header.hiheight;  break;
    case 6: hi = Header.hibase;  break;
    case 7: hi = Header.hidensity;  break;
    case 8: hi = Header.hiduff;  break;
     case 9: hi = Header.hiwoody;  break;
  }

  return hi;
} //GetTheme_HiValue

//============================================================================
long GetTheme_LoValue( short DataTheme )
{ //GetTheme_LoValue
  long lo;

  switch( DataTheme ) {
    case 0: lo = Header.loelev;  break;
    case 1: lo = Header.loslope;  break;
    case 2: lo = Header.loaspect;  break;
    case 3: lo = Header.lofuel;  break;
    case 4: lo = Header.locover;  break;
    case 5: lo = Header.loheight;  break;
    case 6: lo = Header.lobase;  break;
    case 7: lo = Header.lodensity;  break;
    case 8: lo = Header.loduff;  break;
    case 9: lo = Header.lowoody;  break;
  }

  return lo;
} //GetTheme_LoValue

//============================================================================
char* GetTheme_FileName( short DataTheme )
{ //GetTheme_FileName
  switch( DataTheme ) {
    case 0: strcpy(getThemeFileName_name, Header.ElevFile);  break;
    case 1: strcpy(getThemeFileName_name, Header.SlopeFile);  break;
    case 2: strcpy(getThemeFileName_name, Header.AspectFile);  break;
    case 3: strcpy(getThemeFileName_name, Header.FuelFile);  break;
    case 4: strcpy(getThemeFileName_name, Header.CoverFile);  break;
    case 5: strcpy(getThemeFileName_name, Header.HeightFile);  break;
    case 6: strcpy(getThemeFileName_name, Header.BaseFile);  break;
    case 7: strcpy(getThemeFileName_name, Header.DensityFile);  break;
    case 8: strcpy(getThemeFileName_name, Header.DuffFile);  break;
    case 9: strcpy(getThemeFileName_name, Header.WoodyFile);  break;
  }

  return getThemeFileName_name;
} //GetTheme_FileName

//============================================================================
bool LinkDensityWithCrownCover( long TrueFalse )
{
  if( TrueFalse >= 0 ) CrownDensityLinkToCC = TrueFalse ? true : false;

  return CrownDensityLinkToCC;
};

//============================================================================
long HaveCrownFuels()
{ //HaveCrownFuels
  return Header.CrownFuels - 20;    //Subtract 10 to ID file as version 2.x
} //HaveCrownFuels

//============================================================================
long HaveGroundFuels() { return Header.GroundFuels - 20; }

//============================================================================
double GetCellResolutionX()
{ //GetCellResolutionX
  if( Header.GridUnits == 2 ) return Header.XResol * 1000.0;  //To kilometers

  return Header.XResol;
} //GetCellResolutionX

//============================================================================
double GetCellResolutionY()
{ //GetCellResolutionY
  if( Header.GridUnits == 2 ) return Header.YResol * 1000.0;

  return Header.YResol;
} //GetCellResolutionY

//============================================================================
double MetricResolutionConvert()
{ //MetricResolutionConvert
  if( Header.GridUnits == 1 ) return 3.280839895; //Metric conv to meters
  else return 1.0;
} //MetricResolutionConvert

//============================================================================
int CheckCellResUnits() { return Header.GridUnits; }

//============================================================================
void SetCustFuelModelID( bool True_False ) { HAVE_CUST_MODELS = True_False; }

//============================================================================
void SetConvFuelModelID( bool True_False ) { HAVE_CONV_MODELS = True_False; }

//============================================================================
bool NeedCustFuelModels() { return NEED_CUST_MODELS; }

//============================================================================
bool NeedConvFuelModels() { return NEED_CONV_MODELS; }

//============================================================================
bool HaveCustomFuelModels() { return HAVE_CUST_MODELS; }

//============================================================================
bool HaveFuelConversions() { return HAVE_CONV_MODELS; }

//============================================================================
long GetRastFormat() { return RastFormat; }

//============================================================================
long GetVectFormat() { return VectFormat; }

//============================================================================
long CanSetRasterResolution( long YesNo )
{ //CanSetRasterResolution
  if( YesNo > -1 ) CanSetRastRes = YesNo;

  return CanSetRastRes;
} //CanSetRasterResolution

//============================================================================
long SetRasterExtentToViewport( long YesNo )
{ //SetRasterExtentToViewport
  if( YesNo > -1 ) RasterVPExtent = YesNo;

  return RasterVPExtent;
} //SetRasterExtentToViewport

//============================================================================
void GetRastRes( double* XResol, double* YResol )
{ //GetRastRes
  //Raster output resolution.
  *XResol = RasterCellResolutionX;
  *YResol = RasterCellResolutionY;
} //GetRastRes

//============================================================================
void SetVectorFileName( const char* FileName )
{ //SetVectorFileName
  memset( VectFName, 0x0, sizeof(VectFName) );
  sprintf( VectFName, "%s", FileName );
  FILE* newfile;

  newfile = fopen( VectFName, "w" );
  if( newfile == NULL ) {
    SetFileMode( VectFName, FILE_MODE_READ | FILE_MODE_WRITE );
    newfile = fopen( VectFName, "w" );
  }
  fclose(newfile);
} //SetVectorFileName

//============================================================================
char* GetVectorFileName() { return VectFName; }

//============================================================================
void SetRasterFileName( const char* FileName )
{ //SetRasterFileName
  memset( RasterArrivalTime, 0x0, sizeof(RasterArrivalTime) );
  memset( RasterFireIntensity, 0x0, sizeof(RasterFireIntensity) );
  memset( RasterFlameLength, 0x0, sizeof(RasterFlameLength) );
  memset( RasterSpreadRate, 0x0, sizeof(RasterSpreadRate) );
  memset( RasterHeatPerArea, 0x0, sizeof(RasterHeatPerArea) );
  memset( RasterCrownFire, 0x0, sizeof(RasterCrownFire) );
  memset( RasterFireDirection, 0x0, sizeof(RasterFireDirection) );
  memset( RasterReactionIntensity, 0x0, sizeof(RasterReactionIntensity) );
  memset( RastFName, 0x0, sizeof(RastFName) );
  sprintf( RasterArrivalTime, "%s%s", FileName, ".toa" );
  sprintf( RasterFireIntensity, "%s%s", FileName, ".fli" );
  sprintf( RasterFlameLength, "%s%s", FileName, ".fml" );
  sprintf( RasterSpreadRate, "%s%s", FileName, ".ros" );
  sprintf( RasterHeatPerArea, "%s%s", FileName, ".hpa" );
  sprintf( RasterCrownFire, "%s%s", FileName, ".cfr" );
  sprintf( RasterFireDirection, "%s%s", FileName, ".sdr" );
  sprintf( RasterReactionIntensity, "%s%s", FileName, ".rci" );
  sprintf( RastFName, "%s%s", FileName, ".rst" );
} //SetRasterFileName

//============================================================================
char* GetRasterFileName( long Type )
{ //GetRasterFileName
  if( Type == 0 ) return RastFName;
  if( Type == RAST_ARRIVALTIME ) return RasterArrivalTime;
  if( Type == RAST_FIREINTENSITY ) return RasterFireIntensity;
  if( Type == RAST_FLAMELENGTH ) return RasterFlameLength;
  if( Type == RAST_SPREADRATE ) return RasterSpreadRate;
  if( Type == RAST_HEATPERAREA ) return RasterHeatPerArea;
  if( Type == RAST_CROWNFIRE ) return RasterCrownFire;
  if( Type == RAST_FIREDIRECTION ) return RasterFireDirection;
  if( Type == RAST_REACTIONINTENSITY ) return RasterReactionIntensity;

  return NULL;
} //GetRasterFileName

/*============================================================================
  InitSuppressionResources  -  Added Aug30,2010.
  Given the name of a file of suppression resource attributes, reads the file
  and sets up these resources.
  CrewName <name>
  CrewUnits <METERS_PER_MINUTE | FEET_PER_MINUTE | CHAINS_PER_HOUR>
  CrewFlameLimit <NUMBER>
  CrewLineRate <Fuel model> <rate>
  CrewCostPerHour <Cost>
  AircraftName <name>
  AircraftUnits <METERS | FEET>
  AircraftCoverage <Level> <Line length>
  AircraftReturnTime <minutes>
  AircraftCostPerDrop <cost>
*/
void InitSuppressionResources( const char* FileName )
{ //InitSuppressionResources
  FILE* IN = fopen( FileName, "r" );

  if( ! IN ) {
    printf( "## fsglbvar:InitSuppressionResources: Can't open Suppression "
            "Resources file %s! ##\n", FileName );
    return;
  }

  char s[80], key[80], value1[80], value2[80];
  int  i;
  do {
    fgets( s, 80, IN );
    strlwr( s );
    for( i = 0; s[i] != '#' && s[i] != 0 && i < 80; i++ );  //Trim comments
    if( s[i] == '#' ) s[i] = 0;
    for( i = 0; (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')
                && i < 80; i++ );  //Skip leading whitespace
    if( strlen(s) - i <= 1 ) continue; //Skip blank lines / comment lines

    key[0] = value1[0] = value2[0] = 0;
    sscanf( s, "%s %s %s", key, value1, value2 );
    if( ! feof(IN) && key[0] ) {
      if( strcmp( key, "crewname" ) == 0 ) {
        //Define a new crew.
        ++NumCrews;
        crew[NumCrews-1] = new Crew();
        strcpy( crew[NumCrews-1]->CrewName, value1 );
        //Note: Default values set in Crew constructor.
      }
      else if( strcmp( key, "crewunits" ) == 0 ) {
        if( NumCrews == 0 ) {
          printf( "## fsglbvar:InitSuppressionResources: CrewUnits keyword "
                  "appears before CrewName keyword has been set. CrewName "
                  "must be defined BEFORE setting Crew attributes. ##\n" );
          continue;
        }

        if( strncmp( value1, "meters", 6 ) )
          crew[NumCrews-1]->Units = METERS_PER_MINUTE;
        else if( strncmp( value1, "feet", 4 ) ) 
          crew[NumCrews-1]->Units = FEET_PER_MINUTE;
        else if( strncmp( value1, "chains", 6 ) ) 
          crew[NumCrews-1]->Units = CHAINS_PER_HOUR;
      }
      else if( strcmp( key, "crewflamelimit" ) == 0 ) {
        if( NumCrews == 0 ) {
          printf( "## fsglbvar:InitSuppressionResources: CrewFlameLimit "
                  "keyword appears before CrewName keyword has been set. "
                  "CrewName must be defined BEFORE setting Crew attributes. "
                  "##\n" );
          continue;
        }

        crew[NumCrews-1]->FlameLimit = atof( value1 );
      }
      else if( strcmp( key, "crewlinerate" ) == 0 ) {
        if( NumCrews == 0 ) {
          printf( "## fsglbvar:InitSuppressionResources: CrewLineRate "
                  "keyword appears before CrewName keyword has been set. "
                  "CrewName must be defined BEFORE setting Crew attributes. "
                  "##\n" );
          continue;
        }

        long fm = atoi( value1 );
        if( fm < 1 || fm > 51 ) {
          printf( "## fsglbvar:InitSuppressionResources: Invalid fuel model "
                  "number for CrewLineRate keyword: %s (should be between 1 "
                  "& 51) ##\n", value1 );
          continue;
        }
        crew[NumCrews-1]->LineProduction[fm-1] = atof( value2 );
      }
      else if( strcmp( key, "crewcostperhour" ) == 0 ) {
        if( NumCrews == 0 ) {
          printf( "## fsglbvar:InitSuppressionResources: CrewCostPerHour "
                  "keyword appears before CrewName keyword has been set. "
                  "CrewName must be defined BEFORE setting Crew attributes. "
                  "##\n" );
          continue;
        }

        crew[NumCrews-1]->Cost = atof( value1 );
      }
      else if( strcmp( key, "crewflatfee" ) == 0 ) {
        if( NumCrews == 0 ) {
          printf( "## fsglbvar:InitSuppressionResources: CrewFlatFee "
                  "keyword appears before CrewName keyword has been set. "
                  "CrewName must be defined BEFORE setting Crew attributes. "
                  "##\n" );
          continue;
        }

        crew[NumCrews-1]->FlatFee = atof( value1 );
      }
      else if( strcmp( key, "aircraftname" ) == 0 ) {
        //Define a new Aircraft.
        ++NumAircraft;
        aircraft[NumAircraft-1] = new AirCraft();
        strcpy( aircraft[NumAircraft-1]->AirCraftName, value1 );
        //Note: Default values set in Aircraft constructor.
      }
      else if( strcmp( key, "aircraftunits" ) == 0 ) {
        if( NumAircraft == 0 ) {
          printf( "## fsglbvar:InitSuppressionResources: AircraftUnits "
                  "keyword appears before AircraftName keyword has been set. "
                  "AircraftName must be defined BEFORE setting Aircraft "
                  "attributes. ##\n" );
          continue;
        }

        if( strncmp( value1, "meters", 6 ) )
          aircraft[NumAircraft-1]->Units = METERS;
        else if( strncmp( value1, "feet", 4 ) ) 
          aircraft[NumAircraft-1]->Units = FEET;
      }
      else if( strcmp( key, "aircraftcoverage" ) == 0 ) {
        if( NumAircraft == 0 ) {
          printf( "## fsglbvar:InitSuppressionResources: AircraftCoverage "
                  "keyword appears before AircraftName keyword has been set. "
                  "AircraftName must be defined BEFORE setting Aircraft "
                  "attributes. ##\n" );
          continue;
        }

        long level = atoi( value1 );
        long level_index = level;
        if( level_index == 6 ) level_index = 4;
        else if( level_index == 8 ) level_index = 5;
        else level_index--;  //Should be 1-4
        if( level_index < 0 || level_index > 6 ) {
          printf( "## fsglbvar:InitSuppressionResources: Invalid level for "
                  "AircraftCoverage keyword: %s (should be between 1, 2, 3, "
                  "4, 6, or 8 ) ##\n", value1 );
          continue;
        }
        aircraft[NumAircraft-1]->PatternLength[level_index] = atof( value2 );
      }
      else if( strcmp( key, "aircraftreturntime" ) == 0 ) {
        if( NumCrews == 0 ) {
          printf( "## fsglbvar:InitSuppressionResources: AircraftReturnTime "
                  "keyword appears before AircraftName keyword has been set. "
                  "AircraftName must be defined BEFORE setting Aircraft "
                  "attributes. ##\n" );
          continue;
        }

        aircraft[NumAircraft-1]->ReturnTime = atof( value1 );
      }
      else if( strcmp( key, "aircraftcostperdrop" ) == 0 ) {
        if( NumCrews == 0 ) {
          printf( "## fsglbvar:InitSuppressionResources: AircraftCostPerDrop "
                  "keyword appears before AircraftName keyword has been set. "
                  "AircraftName must be defined BEFORE setting Aircraft "
                  "attributes. ##\n" );
          continue;
        }

        aircraft[NumAircraft-1]->Cost = atof( value1 );
      }

    }
  } while( ! feof(IN) );

  fclose( IN );
} //InitSuppressionResources

//============================================================================
void SetFileOutputOptions( long FileType, bool YesNo )
{ //SetFileOutputOptions
  switch( FileType ) {
    case RAST_ARRIVALTIME:
      rast_arrivaltime = YesNo;
      break;
    case RAST_FIREINTENSITY:
      rast_fireintensity = YesNo;
      break;
    case RAST_SPREADRATE:
      rast_spreadrate = YesNo;
      break;
    case RAST_FLAMELENGTH:
      rast_flamelength = YesNo;
      break;
    case RAST_HEATPERAREA:
      rast_heatperarea = YesNo;
      break;
    case RAST_CROWNFIRE:
      rast_crownfire = YesNo;
      break;
    case RAST_FIREDIRECTION:
      rast_firedirection = YesNo;
      break;
    case RAST_REACTIONINTENSITY:
      rast_reactionintensity = YesNo;
      break;
  }
} //SetFileOutputOptions

//============================================================================
bool GetFileOutputOptions( long FileType )
{ //GetFileOutputOptions
  bool YesNo = false;

  switch( FileType ) {
    case RAST_ARRIVALTIME:
      YesNo = rast_arrivaltime;
      break;
    case RAST_FIREINTENSITY:
      YesNo = rast_fireintensity;
      break;
    case RAST_SPREADRATE:
      YesNo = rast_spreadrate;
      break;
    case RAST_FLAMELENGTH:
      YesNo = rast_flamelength;
      break;
    case RAST_HEATPERAREA:
      YesNo = rast_heatperarea;
      break;
    case RAST_CROWNFIRE:
      YesNo = rast_crownfire;
      break;
    case RAST_FIREDIRECTION:
      YesNo = rast_firedirection;
      break;
    case RAST_REACTIONINTENSITY:
      YesNo = rast_reactionintensity;
      break;
  }

  return YesNo;
} //GetFileOutputOptions

//============================================================================
void SetRastMake( bool YesNo ) { RastMake = YesNo; }

//============================================================================
bool GetRastMake() { return RastMake; }

static long DisplayUnits = 0;
static long OutputUnits = 0;

//============================================================================
long AccessDisplayUnits( long val )
{ //AccessDisplayUnits
  if( val >= 0 ) DisplayUnits = val;

  return DisplayUnits;
} //AccessDisplayUnits

//============================================================================
long AccessOutputUnits( long val )
{ //AccessOutputUnits
  if( val >= 0 ) OutputUnits = val;

  return OutputUnits;
} //AccessOutputUnits

//============================================================================
void SetVectMake( bool YesNo ) { VectMake = YesNo; }

//============================================================================
bool GetVectMake() { return VectMake; }

//============================================================================
void SetShapeMake( bool YesNo ) { ShapeMake = YesNo; }

//============================================================================
bool GetShapeMake() { return ShapeMake; }

//============================================================================
long ExcludeBarriersFromVectorFiles( long YesNo )
{ //ExcludeBarriersFromVectorFiles
  if( YesNo >= 0 ) ShapeBarriersSeparately = YesNo;

  return ShapeBarriersSeparately;
} //ExcludeBarriersFromVectorFiles

//============================================================================
bool GetVectVisOnly() { return VISONLY; }

//============================================================================
void SetShapeFileChx( const char* ShapeName, long VisOnly, long Polygons,
                      long BarriersSep )
{ //SetShapeFileChx
  memset( ShapeFileName, 0x0, sizeof(ShapeFileName) );
  strcpy( ShapeFileName, ShapeName );
  ShapeVisibleStepsOnly = VisOnly;
  ShapePolygonsNotLines = Polygons;
  ShapeBarriersSeparately = BarriersSep;
} //SetShapeFileChx

//============================================================================
char* GetShapeFileChx( long* VisOnly, long* Polygons, long* BarriersSep )
{ //GetShapeFileChx
  *VisOnly = ShapeVisibleStepsOnly;
  *Polygons = ShapePolygonsNotLines;
  *BarriersSep = ShapeBarriersSeparately;

  return ShapeFileName;
} //GetShapeFileChx

//----------------------------------------------------------------------------
//Vis Perim stuff
//----------------------------------------------------------------------------

static long i;

//============================================================================
void ResetVisPerimFile()
{ //ResetVisPerimFile
  if( Exists(VisPerim) ) {
    SetFileMode( VisPerim, FILE_MODE_READ | FILE_MODE_WRITE );
    remove( VisPerim );
  }
  memset( VisPerim, 0x0, sizeof VisPerim );
  GetCurDir( VisPerim, 255 );
  char RNum[16] = "";
  sprintf( RNum, "%d", rand() % 1000 );
  strcat( VisPerim, "\\visperim" );
  strcat( VisPerim, RNum );
  strcat( VisPerim, ".vsp" );
  if( Exists(VisPerim) ) {
    SetFileMode( VisPerim, FILE_MODE_READ | FILE_MODE_WRITE );
    remove( VisPerim );
  }
} //ResetVisPerimFile

//----------------------------------------------------------------------------
//Attached Vectors
//----------------------------------------------------------------------------
VectorStorage* CurVect = 0;
VectorStorage* FirstVect = 0;
VectorStorage* NextVect = 0;
VectorStorage* LastVect = 0;

//============================================================================
long GetNumVectorThemes() { return NumVectorThemes; }

//============================================================================
long AllocVectorTheme()
{ //AllocVectorTheme
  if( FirstVect == 0 ) {
    if( (FirstVect = new VectorStorage[1]) == NULL ) return -1;
    memset( FirstVect,0x0, sizeof(VectorStorage) );
    CurVect = FirstVect;
    CurVect->last = 0;
    CurVect->next = 0;
  }
  else {
    long i;

    CurVect = FirstVect;
    for( i = 0; i < NumVectorThemes; i++ ) {
      LastVect = CurVect;
      NextVect = (VectorStorage *) CurVect->next;
      CurVect = NextVect;
    }
    if( (CurVect = new VectorStorage[1]) == NULL ) return -1;
    memset( CurVect,0x0, sizeof(VectorStorage) );
    CurVect->last = (VectorStorage *) LastVect;
    if( LastVect ) LastVect->next = (VectorStorage *) CurVect;
    CurVect->next = 0;
  }
  NumVectorThemes++;

  return NumVectorThemes - 1;
} //AllocVectorTheme

//============================================================================
VectorTheme* GetVectorTheme( long FileNum )
{ //GetVectorTheme
  long i;

  CurVect = FirstVect;
  for( i = 0; i < NumVectorThemes; i++ ) {
    NextVect = (VectorStorage *) CurVect->next;
    if( i == FileNum ) break;
    CurVect = NextVect;
  }

  return &CurVect->theme;
} //GetVectorTheme

//============================================================================
void FreeAllVectorThemes()
{ //FreeAllVectorThemes
  CurVect = FirstVect;
  for( i = 0; i < NumVectorThemes; i++ ) {
    NextVect = (VectorStorage *) CurVect->next;
    if( CurVect ) delete[] CurVect;
    CurVect = NextVect;
  }
  if( CurVect ) delete[] CurVect;
  FirstVect = 0;
  CurVect = 0;
  NextVect = 0;
  NumVectorThemes = 0;
} //FreeAllVectorThemes

//----------------------------------------------------------------------------
//Manage weather/wind stream data
//----------------------------------------------------------------------------

//============================================================================
bool CalcFirstLastStreamData()
{ //CalcFirstLastStreamData
  bool     BadRange = false;
  long     sn;
  unsigned long  FirstWtr, FirstWnd, LastWtr, LastWnd;
  double   ipart, fract;

  for( sn = 0; sn < GetNumStations(); sn++ ) {
    if( AtmosphereGridExists() < 2 ) {
      if( ! wtrdt[sn] || (!wddt[sn] && AtmosphereGridExists() == 0) )
        continue;
    }

    FirstWtr = (unsigned long)( (GetJulianDays(FirstMonth[sn].wtr) +
                          FirstDay[sn].wtr) * 1440.0 );
    fract = modf( (double) FirstHour[sn].wnd / 100.0, &ipart );
    FirstWnd = (unsigned long)( (GetJulianDays(FirstMonth[sn].wnd) +
                          FirstDay[sn].wnd) * 1440.0 + ipart * 60.0 + fract );
    LastWtr = (unsigned long)( (GetJulianDays(LastMonth[sn].wtr) +
                          LastDay[sn].wtr) * 1440.0 );
    fract = modf( (double) LastHour[sn].wnd / 100.0, &ipart );
    LastWnd = (unsigned long)( (GetJulianDays(LastMonth[sn].wnd) +
                          LastDay[sn].wnd) * 1440.0 + ipart * 60.0 + fract );

    if( FirstWtr > LastWtr ) LastWtr += (unsigned long)( 365.0 * 1440.0 );
    if( FirstWnd > LastWnd ) LastWnd += (unsigned long)( 365.0 * 1440.0 );

    if( FirstWnd <= FirstWtr ) {
      if( LastWnd <= FirstWtr ) BadRange = true;
    }
    else if( FirstWnd > FirstWtr ) {
      if( FirstWnd > LastWtr ) BadRange = true;
    }
    if( BadRange ) {
      FirstMonth[sn].all = 0;
      FirstDay[sn].all = 0;
      FirstHour[sn].all = 0;

      return false;
    }

    if( FirstMonth[sn].wtr > FirstMonth[sn].wnd ) {
      FirstMonth[sn].all = FirstMonth[sn].wtr;
      FirstDay[sn].all = FirstDay[sn].wtr;
      FirstHour[sn].all = FirstHour[sn].wtr;
    }
    else if( FirstMonth[sn].wtr < FirstMonth[sn].wnd ) {
      FirstMonth[sn].all = FirstMonth[sn].wnd;
      FirstDay[sn].all = FirstDay[sn].wnd;
      FirstHour[sn].all = FirstHour[sn].wnd;
    }
    else {
      FirstMonth[sn].all = FirstMonth[sn].wtr;
      if( FirstDay[sn].wtr > FirstDay[sn].wnd ) {
        FirstDay[sn].all = FirstDay[sn].wtr;
        FirstHour[sn].all = FirstHour[sn].wtr;
      }
      else if( FirstDay[sn].wtr < FirstDay[sn].wnd ) {
        FirstDay[sn].all = FirstDay[sn].wnd;
        FirstHour[sn].all = FirstHour[sn].wnd;
      }
      else {
        FirstDay[sn].all = FirstDay[sn].wnd;
        if( FirstHour[sn].wtr > FirstHour[sn].wnd )
          FirstHour[sn].all = FirstHour[sn].wtr;
        else FirstHour[sn].all = FirstHour[sn].wnd;
      }
    }
    if( LastMonth[sn].wtr < LastMonth[sn].wnd ) {
      LastMonth[sn].all = LastMonth[sn].wtr;
      LastDay[sn].all = LastDay[sn].wtr;
      LastHour[sn].all = LastHour[sn].wtr;
    }
    else if( LastMonth[sn].wtr > LastMonth[sn].wnd ) {
      LastMonth[sn].all = LastMonth[sn].wnd;
      LastDay[sn].all = LastDay[sn].wnd;
      LastHour[sn].all = LastHour[sn].wnd;
    }
    else {
      LastMonth[sn].all = LastMonth[sn].wtr;
      if( LastDay[sn].wtr < LastDay[sn].wnd ) {
        LastDay[sn].all = LastDay[sn].wtr;
        LastHour[sn].all = LastHour[sn].wtr;
      }
      else if( LastDay[sn].wtr > LastDay[sn].wnd ) {
        LastDay[sn].all = LastDay[sn].wnd;
        LastHour[sn].all = LastHour[sn].wnd;
      }
      else {
        LastDay[sn].all = LastDay[sn].wnd;
        if( LastHour[sn].wtr < LastHour[sn].wnd )
          LastHour[sn].all = LastHour[sn].wtr;
        else LastHour[sn].all = LastHour[sn].wnd;
      }
    }
  }

  return true;
} //CalcFirstLastStreamData

//----------------------------------------------------------------------------
//Manage Atmosphere Grid with regular Weather/Wind Stream Data
//----------------------------------------------------------------------------

//============================================================================
void SetCorrectStreamNumbers()
{ //SetCorrectStreamNumbers
  long i;

  if( AtmosphereGridExists() == 1 ) {  //If wind grid only
    for( i = 0; i < 5; i++ ) FreeWindData( i );
    if( NumWeatherStations > 0 ) NumWindStations = NumWeatherStations;
    else NumWindStations = 1;
    for( i = 0; i < NumWindStations; i++ ) {
      FirstMonth[i].wnd = AtmGrid->GetAtmMonth( 0 );
      LastMonth[i].wnd =
                      AtmGrid->GetAtmMonth( AtmGrid->GetTimeIntervals() - 1 );
      FirstDay[i].wnd = AtmGrid->GetAtmDay( 0 );
      LastDay[i].wnd = AtmGrid->GetAtmDay( AtmGrid->GetTimeIntervals() - 1 );
      FirstHour[i].wnd = AtmGrid->GetAtmHour( 0 );
      LastHour[i].wnd =
                       AtmGrid->GetAtmHour( AtmGrid->GetTimeIntervals() - 1 );
    }
  }
  else if( AtmosphereGridExists() == 2 ) {  //If weather and wind grids
    for( i = 0; i < 5; i++ ) {
      FreeWindData( i );
      FreeWeatherData( i );
    }
    i = 0;
    NumWindStations = 1;
    NumWeatherStations = 1;
    FirstMonth[i].wnd = AtmGrid->GetAtmMonth( 0 );
    LastMonth[i].wnd =
                      AtmGrid->GetAtmMonth( AtmGrid->GetTimeIntervals() - 1 );
    FirstDay[i].wnd = AtmGrid->GetAtmDay( 0 );
    LastDay[i].wnd = AtmGrid->GetAtmDay( AtmGrid->GetTimeIntervals() - 1 );
    FirstHour[i].wnd = AtmGrid->GetAtmHour( 0 );
    LastHour[i].wnd = AtmGrid->GetAtmHour( AtmGrid->GetTimeIntervals() - 1 );
    FirstMonth[i].wtr = AtmGrid->GetAtmMonth( 0 );
    LastMonth[i].wtr =
                      AtmGrid->GetAtmMonth( AtmGrid->GetTimeIntervals() - 1 );
    FirstDay[i].wtr = AtmGrid->GetAtmDay( 0 );
    LastDay[i].wtr = AtmGrid->GetAtmDay( AtmGrid->GetTimeIntervals() - 1 );
    FirstHour[i].wtr = AtmGrid->GetAtmHour( 0 );
    LastHour[i].wtr = AtmGrid->GetAtmHour( AtmGrid->GetTimeIntervals() - 1 );
  }
} //SetCorrectStreamNumbers

//----------------------------------------------------------------------------
//Wind data structures
//----------------------------------------------------------------------------

//============================================================================
long GetOpenWindStation()
{ //GetOpenWindStation
  long i;

  for( i = 0; i < 5; i++ )
    if( ! wddt[i] ) break;

  return i;
} //GetOpenWindStation

//============================================================================
long AllocWindData( long StatNum, long NumObs )
{ //AllocWindData
  long StationNumber = StatNum;

  if( wddt[StationNumber] ) {
    delete[] wddt[StationNumber];
    wddt[StationNumber] = 0;
    MaxWindObs[StatNum] = 0;
  }
  else {
    StationNumber = GetOpenWindStation();
    if( StationNumber < 5 ) NumWindStations = StationNumber + 1;
  }
  if( StationNumber < 5 && NumObs > 0 ) {
    //Alloc 2* number needed.
    size_t nmemb = MaxWindObs[StationNumber] = NumObs * 2;
    if( (wddt[StationNumber] = new WindData[nmemb]) == NULL )
      StationNumber = -1;
  }
  else StationNumber = -1;

  return StationNumber;
} //AllocWindData

//============================================================================
void FreeWindData( long StationNumber )
{ //FreeWindData
  if( wddt[StationNumber] ) {
    delete[] wddt[StationNumber];
    NumWindStations--;
    MaxWindObs[StationNumber] = 0;
  }
  wddt[StationNumber] = 0;
  FirstMonth[StationNumber].wnd = 0;
  LastMonth[StationNumber].wnd = 0;
  FirstDay[StationNumber].wnd = 0;
  LastDay[StationNumber].wnd = 0;
  FirstHour[StationNumber].wnd = 0;
  LastHour[StationNumber].wnd = 0;
} //FreeWindData

//============================================================================
long SetWindData( long StationNumber, long NumObs, long month, long day,
                  long hour, double windspd, long winddir, long cloudy )
{ //SetWindData
  if( NumObs < MaxWindObs[StationNumber] ) {
    wddt[StationNumber][NumObs].mo = month;
    wddt[StationNumber][NumObs].dy = day;
    wddt[StationNumber][NumObs].hr = hour;
    wddt[StationNumber][NumObs].ws = windspd;
    wddt[StationNumber][NumObs].wd = winddir;
    wddt[StationNumber][NumObs].cl = cloudy;

    if( month == 13 ) {
      FirstMonth[StationNumber].wnd = wddt[StationNumber][0].mo;
      LastMonth[StationNumber].wnd = wddt[StationNumber][NumObs - 1].mo;
      FirstDay[StationNumber].wnd = wddt[StationNumber][0].dy;
      LastDay[StationNumber].wnd = wddt[StationNumber][NumObs - 1].dy;
      FirstHour[StationNumber].wnd = wddt[StationNumber][0].hr;
      LastHour[StationNumber].wnd = wddt[StationNumber][NumObs - 1].hr;
    }

    return 1;
  }

  return 0;
} //SetWindData

//============================================================================
long GetWindMonth( long StationNumber, long NumObs )
{ //GetWindMonth
  if( NumObs > MaxWindObs[StationNumber] - 1 )
    NumObs = MaxWindObs[StationNumber] - 1;

  return wddt[StationNumber][NumObs].mo;
} //GetWindMonth

//============================================================================
long GetWindDay( long StationNumber, long NumObs )
{ //getWindDay
  if( NumObs > MaxWindObs[StationNumber] - 1 )
    NumObs = MaxWindObs[StationNumber] - 1;

  return wddt[StationNumber][NumObs].dy;
} //getWindDay

//============================================================================
long GetWindHour( long StationNumber, long NumObs )
{ //GetWindHour
  if( NumObs > MaxWindObs[StationNumber] - 1 )
    NumObs = MaxWindObs[StationNumber] - 1;

  return wddt[StationNumber][NumObs].hr;
} //GetWindHour

//============================================================================
double GetWindSpeed( long StationNumber, long NumObs )
{ //GetWindSpeed
  if( NumObs > MaxWindObs[StationNumber] - 1 )
    NumObs = MaxWindObs[StationNumber] - 1;

  return wddt[StationNumber][NumObs].ws;
} //GetWindSpeed

//============================================================================
long GetWindDir( long StationNumber, long NumObs )
{ //GetWindDir
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:GetWindDir:1\n", CallLevel, "" );

  if( NumObs > MaxWindObs[StationNumber] - 1 )
    NumObs = MaxWindObs[StationNumber] - 1;

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:GetWindDir:1\n", CallLevel, "" );
  CallLevel--;

  return wddt[StationNumber][NumObs].wd;
} //GetWindDir

//============================================================================
long GetWindCloud( long StationNumber, long NumObs )
{ //GetWindCloud
  if( NumObs > MaxWindObs[StationNumber] - 1 )
    NumObs = MaxWindObs[StationNumber] - 1;

  return wddt[StationNumber][NumObs].cl;
} //GetWindCloud

//============================================================================
long GetMaxWindObs( long StationNumber )
{ //GetMaxWindObs
  if( StationNumber > 4 ) return -1;

  return MaxWindObs[StationNumber] - 1;
} //GetMaxWindObs

//----------------------------------------------------------------------------
//Weather data structures
//----------------------------------------------------------------------------

//============================================================================
long GetOpenWeatherStation()
{ //GetOpenWeatherStation
  long i;

  for( i = 0; i < 5; i++ )
    if( ! wtrdt[i] ) break;

  return i;
} //GetOpenWeatherStation

//============================================================================
long GetNumStations()
{ //GetNumStations
  if( NumWeatherStations > NumWindStations ) return NumWindStations;

  return NumWeatherStations;
} //GetNumStations

//============================================================================
long AllocWeatherData( long StatNum, long NumObs )
{ //AllocWeatherData
  long StationNumber = StatNum;

  if( wtrdt[StationNumber] ) {
    delete[] wtrdt[StationNumber];
    wtrdt[StationNumber] = 0;
    MaxWeatherObs[StatNum] = 0;
  }
  else {
    StationNumber = GetOpenWeatherStation();
    if( StationNumber < 5 ) NumWeatherStations = StationNumber + 1;
  }
  if( StationNumber < 5 && NumObs > 0 ) {
    //Alloc 20 more than needed.
    size_t nmemb = MaxWeatherObs[StationNumber] = NumObs + 20;
    if( (wtrdt[StationNumber] = new WeatherData[nmemb]) == NULL )
      StationNumber = -1;
  }
  else StationNumber = -1;

  return StationNumber;
} //AllocWeatherData

//============================================================================
void FreeWeatherData( long StationNumber )
{ //FreeWeatherData
  if( wtrdt[StationNumber] ) {
    delete[] wtrdt[StationNumber];
    MaxWeatherObs[StationNumber] = 0;
    NumWeatherStations--;
  }
  wtrdt[StationNumber] = 0;
  FirstMonth[StationNumber].wtr = 0;
  LastMonth[StationNumber].wtr = 0;
  FirstDay[StationNumber].wtr = 0;
  LastDay[StationNumber].wtr = 0;
  FirstHour[StationNumber].wtr = 0;
  LastHour[StationNumber].wtr = 0;
} //FreeWeatherData

//============================================================================
long SetWeatherData( long StationNumber, long NumObs, long month, long day,
                     double rain, long time1, long time2, double temp1,
                     double temp2, long humid1, long humid2, double elevation,
                     long tr1, long tr2 )
{ //SetWeatherData
  if( NumObs < MaxWeatherObs[StationNumber] ) {
    wtrdt[StationNumber][NumObs].mo = month;
    wtrdt[StationNumber][NumObs].dy = day;
    wtrdt[StationNumber][NumObs].rn = rain;
    wtrdt[StationNumber][NumObs].t1 = time1;
    wtrdt[StationNumber][NumObs].t2 = time2;
    wtrdt[StationNumber][NumObs].T1 = temp1;
    wtrdt[StationNumber][NumObs].T2 = temp2;
    wtrdt[StationNumber][NumObs].H1 = humid1;
    wtrdt[StationNumber][NumObs].H2 = humid2;
    wtrdt[StationNumber][NumObs].el = elevation;
    wtrdt[StationNumber][NumObs].tr1 = tr1;
    wtrdt[StationNumber][NumObs].tr2 = tr2;

    if( month == 13 ) {
      FirstMonth[StationNumber].wtr = wtrdt[StationNumber][0].mo;
      LastMonth[StationNumber].wtr = wtrdt[StationNumber][NumObs - 1].mo;
      FirstDay[StationNumber].wtr = wtrdt[StationNumber][0].dy;
      LastDay[StationNumber].wtr = wtrdt[StationNumber][NumObs - 1].dy;
      FirstHour[StationNumber].wtr = wtrdt[StationNumber][0].t1;
      LastHour[StationNumber].wtr = wtrdt[StationNumber][NumObs - 1].t2;
    }

    EnvtChanged[0][StationNumber] = true;   // 1hr fuels
    EnvtChanged[1][StationNumber] = true;   // 10hr fuels
    EnvtChanged[2][StationNumber] = true;   // 100hr fuels
    EnvtChanged[3][StationNumber] = true;   // 1000hr fuels
    return 1;
  }

  return 0;
} //SetWeatherData

//============================================================================
long GetWeatherMonth( long StationNumber, long NumObs )
{ //GetWeatherMonth
  if( NumObs > MaxWeatherObs[StationNumber] - 1 )
    NumObs = MaxWeatherObs[StationNumber] - 1;

  return wtrdt[StationNumber][NumObs].mo;
} //GetWeatherMonth

//============================================================================
long GetWeatherDay( long StationNumber, long NumObs )
{ //GetWeatherDay
  if( NumObs > MaxWeatherObs[StationNumber] - 1 )
    NumObs = MaxWeatherObs[StationNumber] - 1;

  return wtrdt[StationNumber][NumObs].dy;
} //GetWeatherDay

//============================================================================
double GetWeatherRain( long StationNumber, long NumObs )
{ //GetWeatherRain
  if( NumObs > MaxWeatherObs[StationNumber] - 1 )
    NumObs = MaxWeatherObs[StationNumber] - 1;

  return wtrdt[StationNumber][NumObs].rn;
} //GetWeatherRain

//============================================================================
long GetWeatherTime1( long StationNumber, long NumObs )
{ //GetWeatherTime1
  if( NumObs > MaxWeatherObs[StationNumber] - 1 )
    NumObs = MaxWeatherObs[StationNumber] - 1;

  return wtrdt[StationNumber][NumObs].t1;
} //GetWeatherTime1

//============================================================================
long GetWeatherTime2( long StationNumber, long NumObs )
{ //GetWeatherTime2
  if( NumObs > MaxWeatherObs[StationNumber] - 1 )
    NumObs = MaxWeatherObs[StationNumber] - 1;

  return wtrdt[StationNumber][NumObs].t2;
} //GetWeatherTime2

//============================================================================
double GetWeatherTemp1( long StationNumber, long NumObs )
{ //GetWeatherTemp1
  if( NumObs > MaxWeatherObs[StationNumber] - 1 )
    NumObs = MaxWeatherObs[StationNumber] - 1;

  return wtrdt[StationNumber][NumObs].T1;
} //GetWeatherTemp1

//============================================================================
double GetWeatherTemp2( long StationNumber, long NumObs )
{ //GetWeatherTemp2
  if( NumObs > MaxWeatherObs[StationNumber] - 1 )
    NumObs = MaxWeatherObs[StationNumber] - 1;

  return wtrdt[StationNumber][NumObs].T2;
} //GetWeatherTemp2

//============================================================================
long GetWeatherHumid1( long StationNumber, long NumObs )
{ //GetWeatherHumid1
  if( NumObs > MaxWeatherObs[StationNumber] - 1 )
    NumObs = MaxWeatherObs[StationNumber] - 1;

  return wtrdt[StationNumber][NumObs].H1;
} //GetWeatherHumid1

//============================================================================
long GetWeatherHumid2( long StationNumber, long NumObs )
{ //GetWeatherHumid2
  if( NumObs > MaxWeatherObs[StationNumber] - 1 )
    NumObs = MaxWeatherObs[StationNumber] - 1;

  return wtrdt[StationNumber][NumObs].H2;
} //GetWeatherHumid2

//============================================================================
double GetWeatherElev( long StationNumber, long NumObs )
{ //GetWeatherElev
  if( NumObs > MaxWeatherObs[StationNumber] - 1 )
    NumObs = MaxWeatherObs[StationNumber] - 1;

  return wtrdt[StationNumber][NumObs].el;
} //GetWeatherElev

//============================================================================
void GetWeatherRainTimes( long StationNumber, long NumObs, long* tr1,
                          long* tr2 )
{ //GetWeatherRainTimes
  if( NumObs > MaxWeatherObs[StationNumber] - 1 )
    NumObs = MaxWeatherObs[StationNumber] - 1;

  *tr1 = wtrdt[StationNumber][NumObs].tr1;
  *tr2 = wtrdt[StationNumber][NumObs].tr2;
} //GetWeatherRainTimes

//============================================================================
long GetMaxWeatherObs( long StationNumber )
{ //GetMaxWeatherObs
  if( StationNumber > 4 ) return -1;

  return MaxWeatherObs[StationNumber] - 1;
} //GetMaxWeatherObs

//============================================================================
long GetMoistCalcInterval( long FM_SIZECLASS, long CATEGORY )
{ return MoistCalcInterval[FM_SIZECLASS][CATEGORY]; }

//----------------------------------------------------------------------------
//Weather/Wind units
//----------------------------------------------------------------------------

long WeatherUnits[5] = { -1, -1, -1, -1, -1 };
long WindUnits[5] = { -1, -1, -1, -1, -1 };

//============================================================================
void SetWindUnits( long StationNumber, long Units )
{ WindUnits[StationNumber] = Units; }

//============================================================================
void SetWeatherUnits( long StationNumber, long Units )
{ WeatherUnits[StationNumber] = Units; }

//============================================================================
long GetWindUnits( long StationNumber )
{ return WindUnits[StationNumber]; }

//============================================================================
long GetWeatherUnits( long StationNumber )
{ return WeatherUnits[StationNumber]; }

//----------------------------------------------------------------------------
//Weather/Wind station grid functions
//----------------------------------------------------------------------------

//============================================================================
StationGrid::StationGrid()
{ //StationGrid::StationGrid
  Grid = 0;
} //StationGrid::StationGrid

//============================================================================
StationGrid::~StationGrid()
{ //StationGrid::StationGrid
  if( Grid ) delete[] Grid;
} //StationGrid::StationGrid

//============================================================================
void AllocStationGrid(long XDim, long YDim)
{ //AllocStationGrid
  long x, y, Num;

  FreeStationGrid();
  size_t nmemb = 2 * XDim* YDim;
  grid.XDim = XDim;
  grid.YDim = YDim;
  grid.Width = ( (double) (GetHiEast() - GetLoEast()) ) / ( (double) XDim );
  grid.Height =
               ( (double) (GetHiNorth() - GetLoNorth()) ) / ( (double) YDim );
  grid.Grid = new long[nmemb];
  for( y = 0; y < YDim; y++ ) {
    for( x = 0; x < XDim; x++ ) {  //Initialize grid to station 1
      Num = x + y * grid.XDim;
      grid.Grid[Num] = 1;
    }
  }
} //AllocStationGrid

//============================================================================
long FreeStationGrid()
{ //FreeStationGrid
  if( grid.Grid ) {
    delete[] grid.Grid;
    grid.Grid = 0;

    return 1;
  }
  grid.Grid = 0;

  return 0;
} //FreeStationGrid

//============================================================================
long GetStationNumber( double xpt, double ypt )
{ //GetStationNumber
  long XCell = (long)( (xpt - GetLoEast()) / grid.Width );
  long YCell = (long)( (ypt - GetLoNorth()) / grid.Height );
  long CellNum = XCell + YCell* grid.XDim;

  //Check to see if data exist for a given weather station.
  if( grid.Grid[CellNum] > NumWeatherStations ) return 0;
  if( grid.Grid[CellNum] > NumWindStations ) return 0;

  return grid.Grid[CellNum];
} //GetStationNumber

//============================================================================
long SetStationNumber( long XPos, long YPos, long StationNumber )
{ //SetStationNumber
  long CellNumber = XPos + YPos* grid.XDim;

  if( StationNumber <= NumWeatherStations &&
      StationNumber <= NumWindStations && StationNumber > 0 )
    grid.Grid[CellNumber] = StationNumber;
  else return 0;

  return 1;
} //SetStationNumber

//============================================================================
long GridInitialized()
{ //GridInitialized
  if( grid.Grid ) return 1;

  return 0;
} //GridInitialized

//============================================================================
void SetGridEastDimension( long XDim ) { grid.XDim = XDim; }

//============================================================================
void SetGridNorthDimension( long YDim ) { grid.YDim = YDim; }

//============================================================================
long GetGridEastDimension() { return grid.XDim; }

//============================================================================
long GetGridNorthDimension() { return grid.YDim; }

//============================================================================
void SetGridNorthOffset( double offset ) { NorthGridOffset = offset; }

//============================================================================
void SetGridEastOffset( double offset ) { EastGridOffset = offset; }

//----------------------------------------------------------------------------
//Landscape Theme functions
//----------------------------------------------------------------------------

LandscapeTheme* lcptheme = 0;

//============================================================================
LandscapeTheme* GetLandscapeTheme()
{ //LandscapeTheme
  CallLevel++;
  
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:GetLandscapeTheme:1\n", CallLevel, "" );

  CallLevel--;

  return lcptheme;
} //LandscapeTheme

//============================================================================
LandscapeTheme::LandscapeTheme( bool Analyze ) : GridTheme()
{ //LandscapeTheme::LandscapeTheme
  CallLevel++;

  long i;

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:LandscapeTheme::LandscapeTheme:1\n",
            CallLevel, "" );

  memcpy( Name, GetLandFileName(), sizeof(Name) );
  for( i = 0; i < 10; i++ )
    memset( &AllCats[i], 0x0, 100 * sizeof(long) );

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:LandscapeTheme::LandscapeTheme:2\n",
              CallLevel, "" );

  if( Analyze ) AnalyzeStats();
  else ReadStats();

  if( Verbose > CallLevel ) {
    printf( "%*sfsglbvar:LandscapeTheme::LandscapeTheme:3\n", CallLevel, "" );
    printf( "%*s", CallLevel, "" );
    for( int i = 0; i < 6; i++ )
      printf( "[%ld] ", AllCats[3][i] );
    printf( "\n" );
  }

  CopyStats( F_DATA );
  CreateRamp();
  WantNewRamp = false;

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:LandscapeTheme::LandscapeTheme:4\n",
              CallLevel, "" );

  CallLevel--;
} //LandscapeTheme::LandscapeTheme

//============================================================================
void LandscapeTheme::ReadStats()
{ //LandscapeTheme::ReadStats
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:LandscapeTheme:ReadStats:1\n", CallLevel, "" );

  NumAllCats[0] = Header.numelev;
  NumAllCats[1] = Header.numslope;
  NumAllCats[2] = Header.numaspect;
  NumAllCats[3] = Header.numfuel;
  NumAllCats[4] = Header.numcover;
  NumAllCats[5] = Header.numheight;
  NumAllCats[6] = Header.numbase;
  NumAllCats[7] = Header.numdensity;
  NumAllCats[8] = Header.numduff;
  NumAllCats[9] = Header.numwoody;
  memcpy( &AllCats[0], Header.elevs, 100 * sizeof(long) );
  memcpy( &AllCats[1], Header.slopes, 100 * sizeof(long) );
  memcpy( &AllCats[2], Header.aspects, 100 * sizeof(long) );
  memcpy( &AllCats[3], Header.fuels, 100 * sizeof(long) );
  memcpy( &AllCats[4], Header.covers, 100 * sizeof(long) );
  memcpy( &AllCats[5], Header.heights, 100 * sizeof(long) );
  memcpy( &AllCats[6], Header.bases, 100 * sizeof(long) );
  memcpy( &AllCats[7], Header.densities, 100 * sizeof(long) );
  memcpy( &AllCats[8], Header.duffs, 100 * sizeof(long) );
  memcpy( &AllCats[9], Header.woodies, 100 * sizeof(long) );
  maxval[0] = Header.hielev;
  minval[0] = Header.loelev;
  maxval[1] = Header.hislope;
  minval[1] = Header.loslope;
  maxval[2] = Header.hiaspect;
  minval[2] = Header.loaspect;
  maxval[3] = Header.hifuel;
  minval[3] = Header.lofuel;
  maxval[4] = Header.hicover;
  minval[4] = Header.locover;
  maxval[5] = Header.hiheight;
  minval[5] = Header.loheight;
  maxval[6] = Header.hibase;
  minval[6] = Header.lobase;
  maxval[7] = Header.hidensity;
  minval[7] = Header.lodensity;
  maxval[8] = Header.hiduff;
  minval[8] = Header.loduff;
  maxval[9] = Header.hiwoody;
  minval[9] = Header.lowoody;
  Continuous = 0;

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:LandscapeTheme:ReadStats:2\n", CallLevel, "" );
  CallLevel--;
} //LandscapeTheme::ReadStats

//============================================================================
void LandscapeTheme::AnalyzeStats()
{ //LandscapeTheme::AnalyzeStats
  long i, j;

  for( i = 0; i < 10; i++ ) {
    for( j = 0; j < 100; j++ ) AllCats[i][j] = -2;
  }

  FillCats();
  SortCats();
  Header.numelev = NumAllCats[0];
  Header.numslope = NumAllCats[1];
  Header.numaspect = NumAllCats[2];
  Header.numfuel = NumAllCats[3];
  Header.numcover = NumAllCats[4];
  Header.numheight = NumAllCats[5];
  Header.numbase = NumAllCats[6];
  Header.numdensity = NumAllCats[7];
  Header.numduff = NumAllCats[8];
  Header.numwoody = NumAllCats[9];
  memcpy( Header.elevs, &AllCats[0], 100 * sizeof(long) );
  memcpy( Header.slopes, &AllCats[1], 100 * sizeof(long) );
  memcpy( Header.aspects, &AllCats[2], 100 * sizeof(long) );
  memcpy( Header.fuels, &AllCats[3], 100 * sizeof(long) );
  memcpy( Header.covers, &AllCats[4], 100 * sizeof(long) );
  memcpy( Header.heights, &AllCats[5], 100 * sizeof(long) );
  memcpy( Header.bases, &AllCats[6], 100 * sizeof(long) );
  memcpy( Header.densities, &AllCats[7], 100 * sizeof(long) );
  memcpy( Header.duffs, &AllCats[8], 100 * sizeof(long) );
  memcpy( Header.woodies, &AllCats[9], 100 * sizeof(long) );
  Header.hielev = (long)maxval[0];
  Header.loelev = (long)minval[0];
  Header.hislope = (long)maxval[1];
  Header.loslope = (long)minval[1];
  Header.hiaspect = (long)maxval[2];
  Header.loaspect = (long)minval[2];
  Header.hifuel = (long)maxval[3];
  Header.lofuel = (long)minval[3];
  Header.hicover = (long)maxval[4];
  Header.locover = (long)minval[4];
  Header.hiheight = (long)maxval[5];
  Header.loheight = (long)minval[5];
  Header.hibase = (long)maxval[6];
  Header.lobase = (long)minval[6];
  Header.hidensity = (long)maxval[7];
  Header.lodensity = (long)minval[7];
  Header.hiduff = (long)maxval[8];
  Header.loduff = (long)minval[8];
  Header.hiwoody = (long)maxval[9];
  Header.lowoody = (long)minval[9];
  Continuous = 0;
} //LandscapeTheme::AnalyzeStats

//============================================================================
void LandscapeTheme::CopyStats( long layer )
{ //LandscapeTheme::CopyStats
  memcpy( Cats, AllCats[layer], 99 * sizeof(long) );
  NumCats = NumAllCats[layer];
  MaxVal = maxval[layer];
  MinVal = minval[layer];
  if( NumCats > 0 ) CatsOK = true;
  else CatsOK = false;
} //LandscapeTheme::CopyStats

//============================================================================
void LandscapeTheme::FillCats()
{ //LandscapeTheme::FillCats
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsglbvar:LandscapeTheme:FillCats:1\n", CallLevel, "" );

  long   i, j, k, m, pos;
  double x, y, resx, resy;
  celldata cell;
  crowndata cfuel;
  grounddata gfuel;

  resx = GetCellResolutionX();
  resy = GetCellResolutionY();
  for( m = 0; m < 10; m++ ) {
    maxval[m] = -1e100;
    minval[m] = 1e100;
    NumAllCats[m] = 0;
    for( k = 0; k < 100; k++ ) AllCats[m][k] = -1;
  }
  for( i = 0; i < GetNumNorth(); i++ ) {
    y = GetHiNorth() - i * resy - resy / 2.0;
    for( j = 0; j < GetNumEast(); j++ ) {
      x = GetLoEast() + j * resx + resx / 2.0;
      CellData( x, y, cell, cfuel, gfuel, &pos );
      AllCats[0][NumAllCats[0]] = cell.e;
      AllCats[1][NumAllCats[1]] = cell.s;
      AllCats[2][NumAllCats[2]] = cell.a;
      AllCats[3][NumAllCats[3]] = cell.f;
      AllCats[4][NumAllCats[4]] = cell.c;
      if( HaveCrownFuels() ) {
        if( cfuel.h >= 0 ) AllCats[5][NumAllCats[5]] = cfuel.h;
        if( cfuel.b >= 0 ) AllCats[6][NumAllCats[6]] = cfuel.b;
        if( cfuel.p >= 0 ) AllCats[7][NumAllCats[7]] = cfuel.p;
      }
      if( HaveGroundFuels() ) {
        if( gfuel.d >= 0 ) AllCats[8][NumAllCats[8]] = gfuel.d;
        if( gfuel.w >= 0 ) AllCats[9][NumAllCats[9]] = gfuel.w;
      }
      for( m = 0; m < 10; m++ ) {
        if( maxval[m] < AllCats[m][NumAllCats[m]] )
          maxval[m] = AllCats[m][NumAllCats[m]];
        if( AllCats[m][NumAllCats[m]] >= 0 ) {
          if( minval[m] > AllCats[m][NumAllCats[m]] )
            minval[m] = AllCats[m][NumAllCats[m]];
        }
      }
      for( m = 0; m < 10; m++ ) {
        if( NumAllCats[m] > 98 ) continue;
        for( k = 0; k < NumAllCats[m]; k++ ) {
          if( AllCats[m][NumAllCats[m]] == AllCats[m][k] ) break;
        }
        if( k == NumAllCats[m] ) NumAllCats[m]++;
      }
    }
  }
  for( m = 0; m < 10; m++ )
    if( NumAllCats[m] > 98 ) NumAllCats[m] = -1;

  if( Verbose >= CallLevel )
    printf( "%*sfsglbvar:LandscapeTheme:FillCats:2\n", CallLevel, "" );
  CallLevel--;
} //LandscapeTheme::FillCats

//============================================================================
void LandscapeTheme::SortCats()
{ //LandscapeTheme::SortCats
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsglbvar:LandscapeTheme:SortCats:1\n",
            CallLevel, "" );

  long i, j, m;
  long SwapCats[101];

  for( m = 0; m < 10; m++ ) {
    if( NumAllCats[m] < 0 ) continue;
    memcpy( SwapCats, AllCats[m], 100 * sizeof(long) );
    for( i = 0; i < NumAllCats[m] - 1; i++ ) {
      for( j = i + 1; j < NumAllCats[m]; j++ ) {
        if( SwapCats[j] < SwapCats[i] ) {
          SwapCats[100] = SwapCats[i];
          SwapCats[i] = SwapCats[j];
          SwapCats[j] = SwapCats[100];
        }
      }
    }
    AllCats[m][0] = 0;
    for( i = 0; i < NumAllCats[m]; i++ ) AllCats[m][i + 1] = SwapCats[i];
    minval[m] = AllCats[m][1];
    if( minval[m] < 0 ) minval[m] = 0;
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsglbvar:LandscapeTheme:SortCats:2\n",
            CallLevel, "" );
  CallLevel--;
} //LandscapeTheme::SortCats

//----------------------------------------------------------------------------

//============================================================================
RasterTheme::RasterTheme() : GridTheme()
{ //RasterTheme::RasterTheme
  map = 0;
  Priority = 0;
  CatsOK = false;
} //RasterTheme::RasterTheme

//============================================================================
RasterTheme::~RasterTheme()
{ //RasterTheme::~RasterTheme
  if( map ) delete[] map;
  map = 0;
} //RasterTheme::~RasterTheme

//============================================================================
bool RasterTheme::SetTheme( char* FileName )
{ //RasterTheme::SetTheme
  long   i, j;
  char   TestRead[30];
  double value;
  FILE*  Rast;

  if( (Rast = fopen(FileName, "r")) == NULL ) return false;

  if( map ) delete[] map;
  map = 0;
  strcpy( Name, FileName );

  rMaxVal = 1e-300;
  rMinVal = 1e300;
  fscanf( Rast, "%s", TestRead );
  if( ! strcmp(TestRead, "north:") ) {  //GRASS File
    fscanf( Rast, "%lf", &rN );
    fscanf( Rast, "%s", TestRead );
    fscanf( Rast, "%lf", &rS );
    fscanf( Rast, "%s", TestRead );
    fscanf( Rast, "%lf", &rE );
    fscanf( Rast, "%s", TestRead );
    fscanf( Rast, "%lf", &rW );
    fscanf( Rast, "%s", TestRead );
    fscanf( Rast, "%ld", &rRows );
    fscanf( Rast, "%s", TestRead );
    fscanf( Rast, "%ld", &rCols );
    rCellSizeY = (rN - rS) / (double) rRows;
    rCellSizeX = (rE - rW) / (double) rCols;
  }
  else if( ! strcmp(strupr(TestRead), "NCOLS") ) {  //ARC grid file
    fscanf( Rast, "%ld", &rCols );
    fscanf( Rast, "%s", TestRead );
    fscanf( Rast, "%ld", &rRows );
    fscanf( Rast, "%s", TestRead );
    fscanf( Rast, "%lf", &rW );
    fscanf( Rast, "%s", TestRead );
    fscanf( Rast, "%lf", &rS );
    fscanf( Rast, "%s", TestRead );
    fscanf( Rast, "%lf", &rCellSizeX );
    fscanf( Rast, "%s", TestRead );
    fscanf( Rast, "%s", TestRead );
    rN = rS + rCellSizeX * (double) rRows;
    rE = rW + rCellSizeX * (double) rCols;
    rCellSizeY = rCellSizeX;
  }

  if( (map = new double[rRows * rCols]) == NULL ) return false;
  memset( map,0x0, rRows * rCols * sizeof(double) );

  for( i = 0; i < rRows; i++ ) {
    for( j = 0; j < rCols; j++ ) {
      fscanf( Rast, "%lf", &value );
      map[i * rCols + j] = value;
      if( value >= 0.0 ) {
        if( value < rMinVal ) rMinVal = value;
        if( value > rMaxVal ) rMaxVal = value;
      }
    }
  }
  fclose( Rast );

  if( CheckCellResUnits() == 2 ) {
    rCellSizeX *= 1000.0;
    rCellSizeY *= 1000.0;
  }

  rW = ConvertUtmToEastingOffset( rW );
  rE = ConvertUtmToEastingOffset( rE );
  rN = ConvertUtmToNorthingOffset( rN );
  rS = ConvertUtmToNorthingOffset( rS );

  Continuous = 1;
  FillCats();
  SortCats();
  RedVal = 50;
  GreenVal = 0;
  BlueVal = 0;
  VarVal = 0;
  NumColors = 12;
  MaxBrite = 255;
  OnOff = true;
  WantNewRamp = true;
  CreateRamp();

  return true;
} //RasterTheme::SetTheme

//============================================================================
void RasterTheme::FillCats()
{ //RasterTheme::FillCats
  NumCats = 0;
  long   i, j, k, m;
  double Int;

  for( i = 0; i < rRows; i++ ) {
    for( j = 0; j < rCols; j++ ) {
      m = i * rCols + j;
      modf( map[m], &Int );
      if( Int >= 0.0 ) {
        Cats[NumCats] = (long)ceil( map[m] );
        for( k = 0; k < NumCats; k++ ) {
          if( Cats[NumCats] == Cats[k] ) break;
        }
        if( k == NumCats ) NumCats++;
      }
      if( NumCats > 99 ) break;
    }
    if( NumCats > 99 ) {
      NumCats = -1;
      break;
    }
  }
  MaxVal = rMaxVal;  //Base class copies
  MinVal = rMinVal;
} //RasterTheme::FillCats

//============================================================================
void RasterTheme::SortCats()
{ //RasterTheme::SortCats
  if( NumCats == -1 ) return;
  else CatsOK = 1;

  long i, j, SwapCats[101];

  memcpy( SwapCats, Cats, 100 * sizeof(long) );
  for( i = 0; i < NumCats - 1; i++ ) {
    for( j = i + 1; j < NumCats; j++ ) {
      if( SwapCats[j] < SwapCats[i] ) {
        SwapCats[100] = SwapCats[i];
        SwapCats[i] = SwapCats[j];
        SwapCats[j] = SwapCats[100];
      }
    }
  }
  Cats[0] = 0;
  for( i = 0; i < NumCats; i++ ) Cats[i + 1] = SwapCats[i];
} //RasterTheme::SortCats

static long ThemeID = 0;
static long NumThemes = 0;

Themes* FirstTheme = 0;
Themes* CurrentTheme;
Themes* NextTheme;
Themes* LastTheme;

//============================================================================
long GetNumRasterThemes() { return NumThemes; }

//============================================================================
Themes* GetRasterTheme( long Num )
{ //GetRasterTheme
  long i;

  CurrentTheme = FirstTheme;
  for( i = 0; i < NumThemes; i++ ) {
    NextTheme = (Themes *) CurrentTheme->next;
    if( CurrentTheme->ID == Num ) break;
    CurrentTheme = NextTheme;
  }

  return CurrentTheme;
} //GetRasterTheme

//============================================================================
Themes* AllocRasterTheme()
{ //AllocRasterTheme
  if( FirstTheme == 0 ) {
    if( (FirstTheme = new Themes) == NULL ) return NULL;

    CurrentTheme = FirstTheme;
    CurrentTheme->last = 0;
    CurrentTheme->next = 0;
    LastTheme = 0;
  }
  else {
    long i;

    CurrentTheme = FirstTheme;
    for( i = 0; i < NumThemes; i++ ) {
      LastTheme = CurrentTheme;
      NextTheme = (Themes *) CurrentTheme->next;
      CurrentTheme = NextTheme;
    }
    CurrentTheme = new Themes;
    CurrentTheme->next = 0;
    CurrentTheme->last = (Themes *) LastTheme;
    if( LastTheme ) LastTheme->next = (Themes *) CurrentTheme;
  }
  NumThemes++;
  CurrentTheme->Changed = 1;
  CurrentTheme->ID = ThemeID++;
  CurrentTheme->Selected = true;
  CurrentTheme->theme = new RasterTheme();

  return CurrentTheme;
} //AllocRasterTheme

//----------------------------------------------------------------------------
//Landscape header and cell positioning functions
//----------------------------------------------------------------------------

static long NumVals;
static bool CantAllocLCP = false;

//============================================================================
void ReadHeader()
{ //ReadHeader
  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ReadHeader:1\n", CallLevel, "" );

  //If headsize has not been set yet, set it using LCPAnalyzer results.
  LCPAnalyzer LA( LandFName );
  LA.Analyze();
  if( LA.GetNumErrors() > 0 ) {
    printf( "ReadHeader: ## Errors found reading LCP file: ##\n" );
    string s = LA.GetMessages();
    printf( s.c_str() );
  }
  headsize = LA.GetHeaderSize();
  unsigned int LongSize = LA.GetLCPLongSize();
  unsigned int DoubleSize = LA.GetLCPDoubleSize();
  unsigned int ShortSize = LA.GetLCPShortSize();

  fseek( landfile, 0, SEEK_SET );
  memset( &Header, 0, sizeof(Header) );
  /*Using fread() is unsafe here, when the sizes of data types in the file can
    be different than the sizes used in the compiled code.
  */
  //fread( &Header.CrownFuels, LongSize, 1, landfile );
  //fread( &Header.GroundFuels, LongSize, 1, landfile );
  //fread( &Header.latitude,  LongSize, 1, landfile );
  //fread( &Header.loeast,    DoubleSize, 1, landfile );
  //fread( &Header.hieast,    DoubleSize, 1, landfile );
  //fread( &Header.lonorth,   DoubleSize, 1, landfile );
  //fread( &Header.hinorth,   DoubleSize, 1, landfile );
  //fread( &Header.loelev,    LongSize, 1, landfile );
  //fread( &Header.hielev,    LongSize, 1, landfile );
  //fread( &Header.numelev,   LongSize, 1, landfile );
  //fread( &Header.elevs,     LongSize, 100, landfile );
  //fread( &Header.loslope,   LongSize, 1, landfile );
  //fread( &Header.hislope,   LongSize, 1, landfile );
  //fread( &Header.numslope,  LongSize, 1, landfile );
  //fread( &Header.slopes,    LongSize, 100, landfile );
  //fread( &Header.loaspect,  LongSize, 1, landfile );
  //fread( &Header.hiaspect,  LongSize, 1, landfile );
  //fread( &Header.numaspect, LongSize, 1, landfile );
  //fread( &Header.aspects,   LongSize, 100, landfile );
  //fread( &Header.lofuel,    LongSize, 1, landfile );
  //fread( &Header.hifuel,    LongSize, 1, landfile );
  //fread( &Header.numfuel,   LongSize, 1, landfile );
  //fread( &Header.fuels,     LongSize, 100, landfile );
  //fread( &Header.locover,   LongSize, 1, landfile );
  //fread( &Header.hicover,   LongSize, 1, landfile );
  //fread( &Header.numcover,  LongSize, 1, landfile );
  //fread( &Header.covers,    LongSize, 100, landfile );
  //fread( &Header.loheight,  LongSize, 1, landfile );
  //fread( &Header.hiheight,  LongSize, 1, landfile );
  //fread( &Header.numheight, LongSize, 1, landfile );
  //fread( &Header.heights,   LongSize, 100, landfile );
  //fread( &Header.lobase,    LongSize, 1, landfile );
  //fread( &Header.hibase,    LongSize, 1, landfile );
  //fread( &Header.numbase,   LongSize, 1, landfile );
  //fread( &Header.bases,     LongSize, 100, landfile );
  //fread( &Header.lodensity, LongSize, 1, landfile );
  //fread( &Header.hidensity, LongSize, 1, landfile );
  //fread( &Header.numdensity, LongSize, 1, landfile );
  //fread( &Header.densities, LongSize, 100, landfile );
  //fread( &Header.loduff,    LongSize, 1, landfile );
  //fread( &Header.hiduff,    LongSize, 1, landfile );
  //fread( &Header.numduff,   LongSize, 1, landfile );
  //fread( &Header.duffs,     LongSize, 100, landfile );
  //fread( &Header.lowoody,   LongSize, 1, landfile );
  //fread( &Header.hiwoody,   LongSize, 1, landfile );
  //fread( &Header.numwoody,  LongSize, 1, landfile );
  //fread( &Header.woodies,   LongSize, 100, landfile );
  //fread( &Header.numeast,   LongSize, 1, landfile );
  //fread( &Header.numnorth,  LongSize, 1, landfile );
  //fread( &Header.EastUtm,   DoubleSize, 1, landfile );
  //fread( &Header.WestUtm,   DoubleSize, 1, landfile );
  //fread( &Header.NorthUtm,  DoubleSize, 1, landfile );
  //fread( &Header.SouthUtm,  DoubleSize, 1, landfile );
  //fread( &Header.GridUnits, LongSize, 1, landfile );
  //fread( &Header.XResol,    DoubleSize, 1, landfile );
  //fread( &Header.YResol,    DoubleSize, 1, landfile );
  //fread( &Header.EUnits, sizeof(short), 1, landfile );
  //fread( &Header.SUnits, sizeof(short), 1, landfile );
  //fread( &Header.AUnits, sizeof(short), 1, landfile );
  //fread( &Header.FOptions, sizeof(short), 1, landfile );
  //fread( &Header.CUnits, sizeof(short), 1, landfile );
  //fread( &Header.HUnits, sizeof(short), 1, landfile );
  //fread( &Header.BUnits, sizeof(short), 1, landfile );
  //fread( &Header.PUnits, sizeof(short), 1, landfile );
  //fread( &Header.DUnits, sizeof(short), 1, landfile );
  //fread( &Header.WOptions, sizeof(short), 1, landfile );
  //fread( &Header.ElevFile, sizeof(char), 256, landfile );
  //fread( &Header.SlopeFile, sizeof(char), 256, landfile );
  //fread( &Header.AspectFile, sizeof(char), 256, landfile );
  //fread( &Header.FuelFile, sizeof(char), 256, landfile );
  //fread( &Header.CoverFile, sizeof(char), 256, landfile );
  //fread( &Header.HeightFile, sizeof(char), 256, landfile );
  //fread( &Header.BaseFile, sizeof(char), 256, landfile );
  //fread( &Header.DensityFile, sizeof(char), 256, landfile );
  //fread( &Header.DuffFile, sizeof(char), 256, landfile );
  //fread( &Header.WoodyFile, sizeof(char), 256, landfile );
  //fread( &Header.Description, sizeof(char), 512, landfile );
  fseek( landfile,
       LongSize*(36+1000)+DoubleSize*10+ShortSize*10+3072*sizeof(char),
       SEEK_CUR );

  LA.SetFilePos(); //Reset file ptr to start of file
  Header.CrownFuels = LA.ExtractInteger( LongSize );
  Header.GroundFuels = LA.ExtractInteger( LongSize );
  Header.latitude = LA.ExtractInteger( LongSize );
  Header.loeast = LA.ExtractDouble();
  Header.hieast = LA.ExtractDouble();
  Header.lonorth = LA.ExtractDouble();
  Header.hinorth = LA.ExtractDouble();
  Header.loelev = LA.ExtractInteger( LongSize );
  Header.hielev = LA.ExtractInteger( LongSize );
  Header.numelev = LA.ExtractInteger( LongSize );
  LA.ExtractIntegers( Header.elevs, 100, LongSize );
  Header.loslope = LA.ExtractInteger( LongSize );
  Header.hislope = LA.ExtractInteger( LongSize );
  Header.numslope = LA.ExtractInteger( LongSize );
  LA.ExtractIntegers( Header.slopes, 100, LongSize );
  Header.loaspect = LA.ExtractInteger( LongSize );
  Header.hiaspect = LA.ExtractInteger( LongSize );
  Header.numaspect = LA.ExtractInteger( LongSize );
  LA.ExtractIntegers( Header.aspects, 100, LongSize );
  Header.lofuel = LA.ExtractInteger( LongSize );
  Header.hifuel = LA.ExtractInteger( LongSize );
  Header.numfuel = LA.ExtractInteger( LongSize );
  LA.ExtractIntegers( Header.fuels, 100, LongSize );
  Header.locover = LA.ExtractInteger( LongSize );
  Header.hicover = LA.ExtractInteger( LongSize );
  Header.numcover = LA.ExtractInteger( LongSize );
  LA.ExtractIntegers( Header.covers, 100, LongSize );
  Header.loheight = LA.ExtractInteger( LongSize );
  Header.hiheight = LA.ExtractInteger( LongSize );
  Header.numheight = LA.ExtractInteger( LongSize );
  LA.ExtractIntegers( Header.heights, 100, LongSize );
  Header.lobase = LA.ExtractInteger( LongSize );
  Header.hibase = LA.ExtractInteger( LongSize );
  Header.numbase = LA.ExtractInteger( LongSize );
  LA.ExtractIntegers( Header.bases, 100, LongSize );
  Header.lodensity = LA.ExtractInteger( LongSize );
  Header.hidensity = LA.ExtractInteger( LongSize );
  Header.numdensity = LA.ExtractInteger( LongSize );
  LA.ExtractIntegers( Header.densities, 100, LongSize );
  Header.loduff = LA.ExtractInteger( LongSize );
  Header.hiduff = LA.ExtractInteger( LongSize );
  Header.numduff = LA.ExtractInteger( LongSize );
  LA.ExtractIntegers( Header.duffs, 100, LongSize );
  Header.lowoody = LA.ExtractInteger( LongSize );
  Header.hiwoody = LA.ExtractInteger( LongSize );
  Header.numwoody = LA.ExtractInteger( LongSize );
  LA.ExtractIntegers( Header.woodies, 100, LongSize );
  Header.numeast = LA.ExtractInteger( LongSize );
  Header.numnorth = LA.ExtractInteger( LongSize );
  Header.EastUtm = LA.ExtractDouble();
  Header.WestUtm = LA.ExtractDouble();
  Header.NorthUtm = LA.ExtractDouble();
  Header.SouthUtm = LA.ExtractDouble();
  Header.GridUnits = LA.ExtractInteger( LongSize );
  Header.XResol = LA.ExtractDouble();
  Header.YResol = LA.ExtractDouble();
  Header.EUnits = LA.ExtractInteger( ShortSize );
  Header.SUnits = LA.ExtractInteger( ShortSize );
  Header.AUnits = LA.ExtractInteger( ShortSize );
  Header.FOptions = LA.ExtractInteger( ShortSize );
  Header.CUnits = LA.ExtractInteger( ShortSize );
  Header.HUnits = LA.ExtractInteger( ShortSize );
  Header.BUnits = LA.ExtractInteger( ShortSize );
  Header.PUnits = LA.ExtractInteger( ShortSize );
  Header.DUnits = LA.ExtractInteger( ShortSize );
  Header.WOptions = LA.ExtractInteger( ShortSize );
  LA.ExtractChars( Header.ElevFile, 256*sizeof(char) );
  LA.ExtractChars( Header.SlopeFile, 256*sizeof(char) );
  LA.ExtractChars( Header.AspectFile, 256*sizeof(char) );
  LA.ExtractChars( Header.FuelFile, 256*sizeof(char) );
  LA.ExtractChars( Header.CoverFile, 256*sizeof(char) );
  LA.ExtractChars( Header.HeightFile, 256*sizeof(char) );
  LA.ExtractChars( Header.BaseFile, 256*sizeof(char) );
  LA.ExtractChars( Header.DensityFile, 256*sizeof(char) );
  LA.ExtractChars( Header.DuffFile, 256*sizeof(char) );
  LA.ExtractChars( Header.WoodyFile, 256*sizeof(char) );
  LA.ExtractChars( Header.Description, 512*sizeof(char) );

  //Check header.
  if( (Header.CrownFuels != 20 && Header.CrownFuels != 21) ||
      (Header.GroundFuels != 20 && Header.GroundFuels != 21) )
    printf( "## fsglbvar:ReadHeader: Odd value in Landscape file.     ##\n"
            "## Was the Landscape made on a different platform?       ##\n" );
  if( Header.latitude < -90 || Header.latitude > 90 )
    printf( "## fsglbvar:ReadHeader: Bad latitude in Landscape file (%ld). "
            "##\n", Header.latitude );
  if( Header.loelev > Header.hielev || Header.loelev < -1500 ||
      Header.hielev > 30000 ) {
    printf( "## fsglbvar:ReadHeader: Bad value for loelev or hielev ##\n"
            "## in Landscape file (loelev = %ld) (hielev = %ld). ##\n"
            "## This might be due to a byte-boundary problem.        ##\n"
            "## Make sure that the Landscape file was built on the   ##\n"
            "## current platform.                                    ##\n",
            Header.loelev, Header.hielev );
  }

  //Do this in case a version 1.0 file has gotten through.
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ReadHeader:2\n"
            "%*s                    Header.Latitude=%ld\n"
            "%*s                    Header.loelev=%ld Header.hielev=%ld\n"
            "%*s                    Header.numeast=%ld Header.numnorth=%ld\n"
            "%*s                    Header.WestUtm=%lf\n"
            "%*s                    Header.EastUtm=%lf\n"
            "%*s                    Header.SouthUtm=%lf\n"
            "%*s                    Header.NorthUtm=%lf\n",
            CallLevel, "", CallLevel, "", Header.latitude, CallLevel, "",
            Header.loelev, Header.hielev, CallLevel, "", Header.numeast,
            Header.numnorth, CallLevel, "", Header.WestUtm, CallLevel, "",
            Header.EastUtm, CallLevel, "", Header.SouthUtm, CallLevel, "",
            Header.NorthUtm );
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ReadHeader:2a\n Header.numeast=%ld\n",
            CallLevel, "", Header.numeast );
  Header.loeast = ConvertUtmToEastingOffset( Header.WestUtm );
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ReadHeader:2b\n Header.numeast=%ld\n",
            CallLevel, "", Header.numeast );
  Header.hieast = ConvertUtmToEastingOffset( Header.EastUtm );
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ReadHeader:2c\n Header.numeast=%ld\n",
            CallLevel, "", Header.numeast );
  Header.lonorth = ConvertUtmToNorthingOffset( Header.SouthUtm );
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ReadHeader:2d\n Header.numeast=%ld\n",
            CallLevel, "", Header.numeast );
  Header.hinorth = ConvertUtmToNorthingOffset( Header.NorthUtm );
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ReadHeader:2e\n Header.numeast=%ld\n",
            CallLevel, "", Header.numeast );

  if( Verbose > CallLevel ) {
    printf( "%*sfsglbvar:ReadHeader:2f\n", CallLevel, "" );
    printf("%*s                        Header.numnorth=%ld\n",
            CallLevel, "", Header.numnorth );
    printf("%*s                        Header.numeast=%ld\n",
            CallLevel, "", Header.numeast );
    printf("%*s                        NumVals=%ld\n",
            CallLevel, "", NumVals );
  }

  //Do some error-checking. If we see coordinate values of magnitude
  //50,000,000 or greater, we know something's wrong!
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ReadHeader:3 "
            "West: %lf, East: %lf, South: %lf, North: %lf\n",
            CallLevel, "", Header.loeast, Header.hieast, Header.lonorth,
            Header.hinorth );
  if( Header.loeast < -50000000.0 || Header.loeast > 50000000.0 ||
      Header.hieast < -50000000.0 || Header.hieast > 50000000.0 ||
      Header.lonorth < -50000000.0 || Header.lonorth > 50000000.0 ||
      Header.hinorth < -50000000.0 || Header.hinorth > 50000000.0 ||
      (fabs(Header.hieast-Header.loeast) < 0.1) ||
      (fabs(Header.hinorth-Header.lonorth) < 0.1) )
    printf( "## fsglbvar:ReadHeader: Odd coordinate value(s) in ##\n"
            "## Landscape header:                               ##\n"
            "##  West: %15.6lf                          ##\n"
            "##  East: %15.6lf                          ##\n"
            "## South: %15.6lf                          ##\n"
            "## North: %15.6lf                          ##\n",
            Header.loeast, Header.hieast, Header.lonorth, Header.hinorth );

  if( Verbose > CallLevel ) {
    printf( "%*sfsglbvar:ReadHeader:3a\n", CallLevel, "" );
    printf("%*s                        headsize=%ld\n",
            CallLevel, "", headsize );
    printf("%*s                        sizeof(headdata)=%ld\n",
            CallLevel, "", sizeof(headdata) );
  }

  if( Header.FOptions == 1 || Header.FOptions == 3 ) NEED_CUST_MODELS = true;
  else NEED_CUST_MODELS = false;
  if( Header.FOptions == 2 || Header.FOptions == 3 ) NEED_CONV_MODELS = true;
  else NEED_CONV_MODELS = false;

  //Set raster resolution.
  RasterCellResolutionX = ( Header.EastUtm - Header.WestUtm ) /
                          (double) Header.numeast;
  RasterCellResolutionY = ( Header.NorthUtm - Header.SouthUtm ) /
                          (double) Header.numnorth;
  ViewPortNorth = RasterCellResolutionY * (double) Header.numnorth +
                  Header.lonorth;
  ViewPortSouth = Header.lonorth;
  ViewPortEast = RasterCellResolutionX * (double) Header.numeast +
                 Header.loeast;
  ViewPortWest = Header.loeast;

  double rows, cols;
  rows = ( ViewPortNorth - ViewPortSouth ) / Header.YResol;
  NumViewNorth = (long) rows;
  if( modf(rows, &rows) > 0.5 ) NumViewNorth++;
  cols = ( ViewPortEast - ViewPortWest ) / Header.XResol;
  NumViewEast = (long) cols;
  if( modf(cols, &cols) > 0.5 ) NumViewEast++;

  if( HaveCrownFuels() ) {
    if( HaveGroundFuels() ) NumVals = 10;
    else NumVals = 8;
  }
  else {
    if( HaveGroundFuels() ) NumVals = 7;
    else NumVals = 5;
  }
  CantAllocLCP = false;

  if( lcptheme ) {
    delete lcptheme;
    lcptheme = 0;
  }

  if( Verbose > CallLevel ) {
    printf( "%*sfsglbvar:ReadHeader:4\n", CallLevel, "" );
    printf("%*s                        Header.numnorth=%ld\n",
            CallLevel, "", Header.numnorth );
    printf("%*s                        Header.numeast=%ld\n",
            CallLevel, "", Header.numeast );
    printf("%*s                        NumVals=%ld\n",
            CallLevel, "", NumVals );
  }
  
  lcptheme = new LandscapeTheme( false );

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ReadHeader:5 CallLevel=%d\n",
            CallLevel, "", CallLevel );

  GetLandscapeTheme(); //AAA Called just to get status of AllCats


  if( landscape == 0 ) {
    if( CantAllocLCP == false ) {
      long   i;
      double NumAlloc;

      fseek( landfile, headsize, SEEK_SET );
      NumAlloc = (double) ( Header.numnorth * Header.numeast * NumVals *
                            sizeof(short) );
      if( NumAlloc > 2147483647 ) {
        CantAllocLCP = true;
        CallLevel--;
        return;
      }

      try {
        landscape = new short[Header.numnorth * Header.numeast * NumVals];
      }
      catch(...) { landscape = 0; }
      if( landscape != NULL ) {
        if( Verbose > CallLevel ) {
          printf("%*sfsglbvar:ReadHeader:5a sizeof(short)=%ld\n",
                 CallLevel, "", sizeof(short) );
          printf("%*s                        sizeof(char)=%ld\n",
                 CallLevel, "", sizeof(char) );
          printf("%*s                        sizeof(double)=%ld\n",
                 CallLevel, "", sizeof(double) );
          printf("%*s                        sizeof(long)=%ld\n",
                 CallLevel, "", sizeof(long) );
        }

        memset( landscape, 0x0,
                 Header.numnorth * Header.numeast * NumVals * sizeof(short) );
        for( i = 0; i < Header.numnorth; i++ )
          fread( &landscape[i * NumVals * Header.numeast], sizeof(short),
                                         NumVals * Header.numeast, landfile );
        fseek( landfile, headsize, SEEK_SET );
        CantAllocLCP = false;
      }
      else CantAllocLCP = true;
    }
  }

  CallLevel--;
} //ReadHeader

//============================================================================
void GetCellDataFromMemory( long posit, celldata& cell, crowndata& cfuel,
                            grounddata& gfuel )
{ //GetCellDataFromMemory
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:GetCellDataFromMemory:1 posit=%ld NumVals=%ld "
            "landscape[%ld]=\n", CallLevel, "", posit, NumVals,
            posit*NumVals );

  short ldata[10];

  memcpy( ldata, &landscape[posit * NumVals], NumVals * sizeof(short) );

  if( Verbose > CallLevel ) {
    printf( "%*sfsglbvar:GetCellDataFromMemory:2 ", CallLevel, "" );
    for( int i = 0; i < NumVals; i++ )
      printf( "%02x ", ldata[i] );
    printf( "\n" );
  }

  switch( NumVals ) {
    case 5:
      //Only 5 basic themes.
      memcpy( &cell, ldata, NumVals * sizeof(short) );
      break;
    case 7:
      //5 basic and duff and woody.
      memcpy( &cell, ldata, 5 * sizeof(short) );
      memcpy( &gfuel, &ldata[5], 2 * sizeof(short) );
      break;
    case 8:
      //5 basic and crown fuels.
      memcpy( &cell, ldata, 5 * sizeof(short) );
      memcpy( &cfuel, &ldata[5], 3 * sizeof(short) );
      break;
    case 10:
      //5 basic, crown fuels, and duff and woody.
      memcpy( &cell, ldata, 5 * sizeof(short) );
      memcpy( &cfuel, &ldata[5], 3 * sizeof(short) );
      memcpy( &gfuel, &ldata[8], 2 * sizeof(short) );
      break;
  }

  CallLevel--;
} //GetCellDataFromMemory

//============================================================================
celldata CellData( double east, double north, celldata& cell,
                   crowndata& cfuel, grounddata& gfuel, long* posit )
{ //CellData
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:CellData:1  headsize=%ld\n",
            CallLevel, "", headsize );
 
  //If headsize has not been set yet, set it using LCPAnalyzer results.
  if( headsize == 0 ) {
    LCPAnalyzer LA( LandFName );
    LA.Analyze();
    headsize = LA.GetHeaderSize();
  }

  long Position;

  if( landscape == 0 ) {
    if( CantAllocLCP == false ) {
      long   i;
      double AllocSize;

      fseek( landfile, headsize, SEEK_SET );
      AllocSize = Header.numnorth * Header.numeast * NumVals;
      if( AllocSize > 2147483647 ) CantAllocLCP = true;
      else {
        try
        { landscape = new short[Header.numnorth * Header.numeast * NumVals]; }
        catch(...) {
          CantAllocLCP = true;
          landscape = 0;
        }
        if( landscape != NULL ) {
          if( Verbose > CallLevel )
            printf( "%*sfsglbvar:CellData:1a READING landscape from file\n",
                     CallLevel, "" );

          memset( landscape, 0x0,
                 Header.numnorth * Header.numeast * NumVals * sizeof(short) );
          for( i = 0; i < Header.numnorth; i++ )
            fread( &landscape[i * NumVals * Header.numeast], sizeof(short),
                                         NumVals * Header.numeast, landfile );
          fseek( landfile, headsize, SEEK_SET );
          CantAllocLCP = false;
        }
        else CantAllocLCP = true;
      }
    }
  }

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:CellData:2 east=%lf north=%lf\n",
            CallLevel, "", east, north );

  Position = GetCellPosition( east, north );
  if( Position ) *posit = Position;

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:CellData:3\n", CallLevel, "" );

  if( ! CantAllocLCP ) {
    if( Verbose > CallLevel )
      printf( "%*sfsglbvar:CellData:3a cell.f=%ld\n",
               CallLevel, "", (long)cell.f );

    GetCellDataFromMemory( Position, cell, cfuel, gfuel );

    if( Verbose > CallLevel )
      printf( "%*sfsglbvar:CellData:3b cell.f=%ld\n",
               CallLevel, "", (long)cell.f );
    CallLevel--;
    return cell;
  }

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:CellData:4\n", CallLevel, "" );

  if( Header.CrownFuels == 20 ) {
    if( Header.GroundFuels == 20 ) {
      fseek( landfile, (Position - OldFilePosition) * sizeof(celldata),
             SEEK_CUR );
      fread( &cell, sizeof(celldata), 1, landfile );
    }
    else {
      fseek( landfile,
             (Position - OldFilePosition) * (sizeof(celldata) +
             sizeof(grounddata)),
             SEEK_CUR );
      fread( &cell, sizeof(celldata), 1, landfile );
      fread( &gfuel, sizeof(grounddata), 1, landfile );
    }
  }
  else {
    if( Header.GroundFuels == 20 ) { //None
      fseek( landfile,
             (Position - OldFilePosition) * (sizeof(celldata) +
             sizeof(crowndata)),
             SEEK_CUR );
      fread( &cell, sizeof(celldata), 1, landfile );
      fread( &cfuel, sizeof(crowndata), 1, landfile );
    }
    else {
      fseek( landfile,
             (Position - OldFilePosition) * (sizeof(celldata) +
             sizeof(crowndata) + sizeof(grounddata)),
             SEEK_CUR );
      fread( &cell, sizeof(celldata), 1, landfile );
      fread( &cfuel, sizeof(crowndata), 1, landfile );
      fread( &gfuel, sizeof(grounddata), 1, landfile );
    }
    if( cfuel.h > 0 ) {
      CanopyChx.Height = (double) cfuel.h / 10.0;
      if( Header.HUnits == 2 ) CanopyChx.Height /= 3.280839;
    }
    else CanopyChx.Height = CanopyChx.DefaultHeight;
    if( cfuel.b > 0 ) {
      CanopyChx.CrownBase = (double) cfuel.b / 10.0;
      if( Header.BUnits == 2 ) CanopyChx.CrownBase /= 3.280839;
    }
    else CanopyChx.CrownBase = CanopyChx.DefaultBase;
    if( cfuel.p > 0 ) {
      if( Header.PUnits == 1 )
        CanopyChx.BulkDensity = ((double) cfuel.p) / 100.0;
      else if( Header.PUnits == 2 )
        CanopyChx.BulkDensity = ((double) cfuel.p / 1000.0) * 16.01845;
    }
    else CanopyChx.BulkDensity = CanopyChx.DefaultDensity;
  }

  OldFilePosition = Position + 1;

  CallLevel--;

  return cell;
} //CellData

//============================================================================
long GetCellPosition( double east, double north )
{ //GetCellPosition
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:GetCellPosition:1 "
            "east=%lf north=%lf "
            "Header.loeast=%lf Header.lonorth=%lf "
            "XCellRes=%lf YCellRes=%lf\n", CallLevel, "",
            east, north, Header.loeast, Header.lonorth, GetCellResolutionX(),
            GetCellResolutionY() );

  double xpt = ( east - Header.loeast ) / GetCellResolutionX();
  double ypt = ( north - Header.lonorth ) / GetCellResolutionY();

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:GetCellPosition:2 xpt=%lf ypt=%lf\n",
            CallLevel, "", xpt, ypt );

  long easti = ( (long) xpt );
  long northi = ( (long) ypt );
  northi = Header.numnorth - northi - 1;
  if( northi < 0 ) northi = 0;
  long posit = ( northi * Header.numeast + easti );

  CallLevel--;

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:GetCellPosition:3 posit=%ld\n",
            CallLevel, "", posit );
  return posit;
} //GetCellPosition

//============================================================================
long GetVisibleTheme()
{ //GetVisibleTheme
  if( ! HaveCrownFuels() && VisibleThemeNumber > 4 ) VisibleThemeNumber = 3;

  return VisibleThemeNumber;
} //GetVisibleTheme

//--------------------------------------------------------

//============================================================================
long GetNumEast() { return Header.numeast; }

//============================================================================
long GetNumNorth() { return Header.numnorth; }

//============================================================================
double GetWestUtm() { return Header.WestUtm; }

//============================================================================
double GetEastUtm() { return Header.EastUtm; }

//============================================================================
double GetSouthUtm() { return Header.SouthUtm; }

//============================================================================
double GetNorthUtm() { return Header.NorthUtm; }

//============================================================================
double GetLoEast() { return Header.loeast; }

//============================================================================
double GetHiEast() { return Header.hieast; }

//============================================================================
double GetLoNorth() { return Header.lonorth; }

//============================================================================
double GetHiNorth() { return Header.hinorth; }

//============================================================================
double ConvertEastingOffsetToUtm( double input )
{ //ConvertEastingOffsetToUtm
  double MetersToKm = 1.0;
  double ipart;

  if( Header.GridUnits == 2 ) MetersToKm = 0.001;

  modf( Header.WestUtm / 1000.0, &ipart );

  return ( input + ipart * 1000.0 ) * MetersToKm;
} //ConvertEastingOffsetToUtm

//============================================================================
double ConvertNorthingOffsetToUtm( double input )
{ //ConvertNorthingOffsetToUtm
  double MetersToKm = 1.0;
  double ipart;

  if( Header.GridUnits == 2 ) MetersToKm = 0.001;

  modf( Header.SouthUtm / 1000.0, &ipart );

  return ( input + ipart * 1000.0 ) * MetersToKm;
} //ConvertNorthingOffsetToUtm

//============================================================================
double ConvertUtmToEastingOffset( double input )
{ //ConvertUtmToEastingOffset
  double KmToMeters = 1.0;
  double ipart;

  if( Header.GridUnits == 2 ) KmToMeters = 1000.0;

  modf( Header.WestUtm / 1000.0, &ipart );

  return input * KmToMeters - ipart * 1000.0;
} //ConvertUtmToEastingOffset

//============================================================================
double ConvertUtmToNorthingOffset( double input )
{ //ConvertUtmToNorthingOffset
  double KmToMeters = 1.0;
  double ipart;

  if( Header.GridUnits == 2 ) KmToMeters = 1000.0;

  modf( Header.SouthUtm / 1000.0, &ipart );

  return input * KmToMeters - ipart * 1000.0;
} //ConvertUtmToNorthingOffset

//============================================================================
long GetLoElev() { return Header.loelev; }

//============================================================================
long GetHiElev() { return Header.hielev; }

//============================================================================
long GetConditMonth() { return ConditMonth; }

//============================================================================
long GetConditDay() { return ConditDay; }

//============================================================================
long GetConditMinDeficit()
{ //GetConditMinDeficit
  if( CondPeriod == false ) return 0;

  //00:01 on this day.
  long cond_min = ( GetJulianDays(ConditMonth) + ConditDay ) * 1440;
  long start_min = ( GetJulianDays(StartMonth) + StartDay ) * 1440 +
                   ( StartHour / 100 * 60 ) + StartMin;
  if( start_min < cond_min ) start_min += 365 * 1440;

  return start_min - cond_min;
} //GetConditMinDeficit

//============================================================================
long GetLatitude() { return Header.latitude; }

//============================================================================
long GetStartMonth() { return StartMonth; }

//============================================================================
long GetStartDay() { return StartDay; }

//============================================================================
long GetStartHour() { return StartHour; }

//============================================================================
long GetStartMin() { return StartMin; }

//============================================================================
long GetStartDate() { return startdate; }

//============================================================================
long GetMaxMonth()
{ //GetMaxMonth
  long maxmonth = LastMonth[0].all;
  long numstations = GetNumStations();

  for( long i = 0; i < numstations; i++ ) {
    if( LastMonth[i].all < maxmonth ) maxmonth = LastMonth[i].all;
  }

  return maxmonth;
} //GetMaxMonth

//============================================================================
long GetMaxDay()
{ //GetMaxDay
  long i;
  long maxday = LastDay[0].all;
  long numstations = GetNumStations();
  long maxmonth = GetMaxMonth();

  for( i = 0; i < numstations; i++ ) {
    if( LastMonth[i].all == maxmonth && maxday > LastDay[i].all )
      maxday = LastDay[i].all;
  }

  return maxday;
} //GetMaxDay

//============================================================================
long GetMinMonth()
{ //GetMinMonth
  long i;
  long minmonth = FirstMonth[0].all;
  long NStat = GetNumStations();

  for( i = 0; i < NStat; i++ ) {
    if( FirstMonth[i].all > minmonth ) minmonth = FirstMonth[i].all;
  }

  return minmonth;
} //GetMinMonth

//============================================================================
long GetMinDay()
{ //GetMinDay
  long i;
  long minday = FirstDay[0].all;
  long NStat = GetNumStations();
  long minmonth = GetMinMonth();

  for( i = 0; i < NStat; i++ ) {
    if( FirstMonth[i].all == minmonth ) {
      if( FirstDay[i].all > minday ) minday = FirstDay[i].all;
    }
  }

  return minday;
} //GetMinDay

//============================================================================
long IgnitionResetAtRestart( long YesNo )
{ //IgnitionResetAtRestart
  if( YesNo >= 0 ) IgnitionReset = YesNo;

  return IgnitionReset;
} //IgnitionResetAtRestart

//============================================================================
long RotateSensitiveIgnitions( long YesNo )
{ //RotateSensitiveIgnitions
  if( YesNo >= 0 ) RotateIgnitions = YesNo;

  return RotateIgnitions;
} //RotateSensitiveIgnitions

//============================================================================
long ShowFiresAsGrown( long YesNo )
{ //ShowFiresAsGrown
  if( YesNo >= 0 ) ShowVectsAsGrown = YesNo;

  return ShowVectsAsGrown;
} //ShowFiresAsGrown

//============================================================================
long DurationResetAtRestart( long YesNo )
{ //DurationResetAtRestart
  if( YesNo >= 0 ) DurationReset = YesNo;

  return DurationReset;
} //DurationResetAtRestart

//============================================================================
bool PreserveInactiveEnclaves( long YesNo )
{ //PreserveInactiveEnclaves
  if( YesNo >= 0 ) InactiveEnclaves = YesNo ? true : false;

  return InactiveEnclaves;
} //PreserveInactiveEnclaves

//============================================================================
void SetStartDate( long input ) { startdate = input; }

//============================================================================
double GetSimulationDuration() { return SimulationDuration; }

//============================================================================
void SetSimulationDuration( double simdur ) { SimulationDuration = simdur; }

//============================================================================
bool UseConditioningPeriod( long YesNo )
{ //UseConditioningPeriod
  if( YesNo > -1 ) CondPeriod = YesNo ? true : false;

  return CondPeriod;
} //UseConditioningPeriod

//============================================================================
bool EnvironmentChanged( long YesNo, long StationNumber, long FuelSize )
{ //EnvironmentChanged
  if( FuelSize > 3 ) FuelSize = 3;
  if( YesNo > -1 )
    EnvtChanged[FuelSize][StationNumber] = YesNo ? true : false;

  return EnvtChanged[FuelSize][StationNumber];
} //EnvironmentChanged

//============================================================================
bool OpenLandFile()
{ //OpenLandFile
  if( landfile ) fclose( landfile );
  if( landscape ) {
    delete[] landscape;
    landscape = 0;
    CantAllocLCP = false;
  }
  if( (landfile = fopen(LandFName, "rb")) == NULL ) {
    landfile = 0;

    return false;
  }

  return true;
} //OpenLandFile

//============================================================================
void CloseLandFile()
{ //CloseLandFile
  if( landscape ) {
    delete[] landscape;
    landscape = 0;
  }
  CantAllocLCP = false;
  if( landfile ) fclose(landfile);
  if( lcptheme ) {
    delete lcptheme;
    lcptheme = 0;
  }
  memset( LandFName, 0x0, sizeof LandFName );
  landfile = 0;
  CanopyChx.Height = CanopyChx.DefaultHeight;
  CanopyChx.CrownBase = CanopyChx.DefaultBase;
  CanopyChx.BulkDensity = CanopyChx.DefaultDensity;
} //CloseLandFile

//============================================================================
bool TestForLCPVersion1()
{ //TestForLCPVersion1
  long TestCrownFuelValue;

  rewind( landfile );
  fread( &TestCrownFuelValue, sizeof(long), 1, landfile );

  //v1 LCP file had crown fuel indicator as 0 or 1.
  //v2 LCP file has crown fuel indicator as 10 or 11.
  if( TestCrownFuelValue < 10 ) return true;

  return false;
} //TestForLCPVersion1

//============================================================================
bool TestForLCPVersion2()
{ //TestForLCPVersion2
  long TestCrownFuelValue;

  rewind( landfile );
  fread( &TestCrownFuelValue, sizeof(long), 1, landfile );

  // v1 LCP file had crown fuel indicator as 0 or 1.
  // v2 LCP file has crown fuel indicator as 10 or 11.
  if( TestCrownFuelValue > 9 && TestCrownFuelValue < 20 ) return true;

  return false;
} //TestForLCPVersion2

//============================================================================
bool ConvertLCPFileToVersion2()
{ //ConvertLCPFileToVersion2
  size_t FileNameLength;
  long   i, j;
  char   OldLandFName[256];
  FILE*  oldlandfile;
  celldata cell;
  crowndata cfuel;

  memset( OldLandFName, 0x0, sizeof(OldLandFName) );
  FileNameLength = strlen( LandFName );
  strncpy( OldLandFName, LandFName, FileNameLength - 4 );
  strcat( OldLandFName, ".LCP1" );
  fclose( landfile );
  if( rename(LandFName, OldLandFName)!=-1 ) {
    //-------Change Header Info  ---------------------------
    oldlandfile = fopen( OldLandFName, "rb" );
    fread( &OldHeader, sizeof(oldheaddata), 1, oldlandfile );
    Header2.CrownFuels = OldHeader.CrownFuels + 10;
    Header2.latitude = OldHeader.latitude;
    Header2.loelev = OldHeader.loelev;
    Header2.hielev = OldHeader.hielev;
    Header2.numeast = OldHeader.numeast;
    Header2.numnorth = OldHeader.numnorth;
    Header2.EastUtm = OldHeader.EastUtm;
    Header2.WestUtm = OldHeader.WestUtm;
    Header2.NorthUtm = OldHeader.NorthUtm;
    Header2.SouthUtm = OldHeader.SouthUtm;
    Header2.loeast = ConvertUtmToEastingOffset( Header.WestUtm );
    Header2.hieast = ConvertUtmToEastingOffset( Header.EastUtm );
    Header2.lonorth = ConvertUtmToNorthingOffset( Header.SouthUtm );
    Header2.hinorth = ConvertUtmToNorthingOffset( Header.NorthUtm );
    Header2.GridUnits = OldHeader.GridUnits;
    Header2.XResol = OldHeader.XResol;
    Header2.YResol = OldHeader.YResol;
    Header2.EUnits = OldHeader.EUnits;
    Header2.SUnits = OldHeader.SUnits;
    Header2.AUnits = OldHeader.AUnits;
    Header2.FOptions = OldHeader.FOptions;
    Header2.CUnits = OldHeader.CUnits;
    Header2.HUnits = OldHeader.HUnits;
    Header2.BUnits = OldHeader.BUnits;
    Header2.PUnits = OldHeader.PUnits;

    if( landfile ) fclose( landfile );
    landfile = fopen( LandFName, "wb" );
    if( landfile == NULL ) {
      SetFileMode( LandFName, FILE_MODE_READ | FILE_MODE_WRITE );
      landfile = fopen( LandFName, "wb" );
    }
    fwrite( &Header2, sizeof(headdata2), 1, landfile );

    for( i = 0; i < Header2.numnorth; i++ ) {
      for( j = 0; j < Header2.numeast; j++ ) {
        fseek( oldlandfile, 2 * sizeof(long), SEEK_CUR );
        fread( &cell, sizeof(celldata), 1, oldlandfile );
        fwrite( &cell, sizeof(celldata), 1, landfile );
        if( Header.CrownFuels == 11 ) {
          fread( &cfuel, sizeof(crowndata), 1, oldlandfile );
          cfuel.h = (short) ( cfuel.h * 10 );  //Increase precision to tenths
          cfuel.b = (short) ( cfuel.b * 10 );
          if( Header.PUnits == 2 )
            cfuel.p = (short) ( cfuel.p * 10 );
          fwrite( &cfuel, sizeof(crowndata), 1, landfile );
        }
      }
    }
    fclose( oldlandfile );
    fclose( landfile );
    if( (landfile = fopen(LandFName, "rb")) == NULL ) {
      landfile = 0;

      return false;
    }

    return true;
  }
  else return false;
} //ConvertLCPFileToVersion2

//============================================================================
bool ConvertLCPFile2ToVersion4()
{ //ConvertLCPFile2ToVersion4
  size_t FileNameLength;
  long   i, j;
  char   OldLandFName[256];
  char   BkupLandFName[256] = "";
  FILE*  oldlandfile;
  celldata cell;
  crowndata cfuel;

  strcpy( BkupLandFName, LandFName );
  memset( OldLandFName, 0x0, sizeof(OldLandFName) );
  FileNameLength = strlen( LandFName );
  strncpy( OldLandFName, LandFName, FileNameLength - 4 );
  strcat( OldLandFName, ".LCP3" );
  fclose( landfile );
  if( rename(LandFName, OldLandFName)!=-1 ) {
    //-------Change Header Info  ---------------------------
    oldlandfile = fopen( OldLandFName, "rb" );
    fread( &Header2, sizeof(headdata2), 1, oldlandfile );
    Header.CrownFuels = Header2.CrownFuels + 10;
    Header.latitude = Header2.latitude;
    Header.loelev = Header2.loelev;
    Header.hielev = Header2.hielev;
    Header.numeast = Header2.numeast;
    Header.numnorth = Header2.numnorth;
    Header.EastUtm = Header2.EastUtm;
    Header.WestUtm = Header2.WestUtm;
    Header.NorthUtm = Header2.NorthUtm;
    Header.SouthUtm = Header2.SouthUtm;
    Header.loeast = ConvertUtmToEastingOffset( Header.WestUtm );
    Header.hieast = ConvertUtmToEastingOffset( Header.EastUtm );
    Header.lonorth = ConvertUtmToNorthingOffset( Header.SouthUtm );
    Header.hinorth = ConvertUtmToNorthingOffset( Header.NorthUtm );
    Header.GridUnits = Header2.GridUnits;
    Header.XResol = Header2.XResol;
    Header.YResol = Header2.YResol;
    Header.EUnits = Header2.EUnits;
    Header.SUnits = Header2.SUnits;
    Header.AUnits = Header2.AUnits;
    Header.FOptions = Header2.FOptions;
    Header.CUnits = Header2.CUnits;
    Header.HUnits = Header2.HUnits;
    Header.BUnits = Header2.BUnits;
    Header.PUnits = Header2.PUnits;
    //Stuff not found in Version 2.0.
    Header.DUnits = 0;
    Header.WOptions = 0;
    Header.GroundFuels = 20;  //None
    memset( Header.ElevFile, 0x0, sizeof(Header.ElevFile) );
    memset( Header.SlopeFile, 0x0, sizeof(Header.ElevFile) );
    memset( Header.AspectFile, 0x0, sizeof(Header.ElevFile) );
    memset( Header.FuelFile, 0x0, sizeof(Header.ElevFile) );
    memset( Header.CoverFile, 0x0, sizeof(Header.ElevFile) );
    memset( Header.HeightFile, 0x0, sizeof(Header.ElevFile) );
    memset( Header.BaseFile, 0x0, sizeof(Header.ElevFile) );
    memset( Header.DensityFile, 0x0, sizeof(Header.ElevFile) );
    memset( Header.DuffFile, 0x0, sizeof(Header.ElevFile) );
    memset( Header.WoodyFile, 0x0, sizeof(Header.ElevFile) );
    memset( Header.Description, 0x0, sizeof(Header.Description) );

    landfile = fopen( LandFName, "wb" );
    if( landfile == NULL ) {
      SetFileMode( LandFName, FILE_MODE_READ | FILE_MODE_WRITE );
      landfile = fopen( LandFName, "wb" );
    }
    fwrite( &Header, sizeof(headdata), 1, landfile );

    for( i = 0; i < Header.numnorth; i++ ) {
      for( j = 0; j < Header.numeast; j++ ) {
        fread( &cell, sizeof(celldata), 1, oldlandfile );
        fwrite( &cell, sizeof(celldata), 1, landfile );
        if( Header.CrownFuels == 21 ) {
          fread( &cfuel, sizeof(crowndata), 1, oldlandfile );
          fwrite( &cfuel, sizeof(crowndata), 1, landfile );
        }
      }
    }
    fclose( oldlandfile );
    CloseLandFile();

    strcpy( LandFName, BkupLandFName );
    if( ! OpenLandFile() ) return false;

    ReadHeader();
    lcptheme->AnalyzeStats();
    Header.numelev = lcptheme->NumAllCats[0];
    Header.numslope = lcptheme->NumAllCats[1];
    Header.numaspect = lcptheme->NumAllCats[2];
    Header.numfuel = lcptheme->NumAllCats[3];
    Header.numcover = lcptheme->NumAllCats[4];
    Header.numheight = lcptheme->NumAllCats[5];
    Header.numbase = lcptheme->NumAllCats[6];
    Header.numdensity = lcptheme->NumAllCats[7];
    Header.numduff = 0;
    Header.numwoody = 0;
    memcpy( Header.elevs, lcptheme->AllCats[0], 100 * sizeof(long) );
    memcpy( Header.slopes, lcptheme->AllCats[1], 100 * sizeof(long) );
    memcpy( Header.aspects, lcptheme->AllCats[2], 100 * sizeof(long) );
    memcpy( Header.fuels, lcptheme->AllCats[3], 100 * sizeof(long) );
    memcpy( Header.covers, lcptheme->AllCats[4], 100 * sizeof(long) );
    memcpy( Header.heights, lcptheme->AllCats[5], 100 * sizeof(long) );
    memcpy( Header.bases, lcptheme->AllCats[6], 100 * sizeof(long) );
    memcpy( Header.densities, lcptheme->AllCats[7], 100 * sizeof(long) );
    memset( Header.duffs,0x0, 100 * sizeof(long) );
    memset( Header.woodies,0x0, 100 * sizeof(long) );
    Header.loslope = (long)lcptheme->minval[1];
    Header.hislope = (long)lcptheme->maxval[1];
    Header.loaspect = (long)lcptheme->minval[2];
    Header.hiaspect = (long)lcptheme->maxval[2];
    Header.lofuel = (long)lcptheme->minval[3];
    Header.hifuel = (long)lcptheme->maxval[3];
    Header.locover = (long)lcptheme->minval[4];
    Header.hicover = (long)lcptheme->maxval[4];
    Header.loheight = (long)lcptheme->minval[5];
    Header.hiheight = (long)lcptheme->maxval[5];
    Header.lobase = (long)lcptheme->minval[6];
    Header.hibase = (long)lcptheme->maxval[6];
    Header.lodensity = (long)lcptheme->minval[7];
    Header.hidensity = (long)lcptheme->maxval[7];
    Header.loduff = 0;
    Header.hiduff = 0;
    Header.lowoody = 0;
    Header.hiwoody = 0;
    CloseLandFile();

    strcpy( LandFName, BkupLandFName );
    landfile = fopen( LandFName, "rb+" );
    fwrite( &Header, sizeof(headdata), 1, landfile );
    fclose( landfile );
    OpenLandFile();

    return true;
  }
  else return false;
} //ConvertLCPFile2ToVersion4

//============================================================================
char* GetLandFileName() { return LandFName; }

//============================================================================
void SetLandFileName( const char* FileName )
{ //SetLandFileName
  memset( LandFName, 0x0, sizeof(LandFName) );
  sprintf( LandFName, "%s", FileName );
} //SetLandFileName

//----------------------------------------------------------------------------
//Access to Fire Display Functions.
//----------------------------------------------------------------------------

/*============================================================================
  OutputFireParam
  Select fire chx for display.
*/
long OutputFireParam( long Param )
{ //OutputFireParam
  if( Param >= 0 ) FireOutputParam = Param;

  return FireOutputParam;
} //OutputFireParam

//----------------------------------------------------------------------------
//Atmosphere Grid Global Functions.
//----------------------------------------------------------------------------

//============================================================================
bool SetAtmosphereGrid( long NumGrids )
{ //SetAtmosphereGrid
  if( NumGrids == 0 ) {
    if( AtmGrid ) delete AtmGrid;
    AtmGrid = 0;
    NorthGridOffset = 0.0;
    EastGridOffset = 0.0;

    return false;
  }
  if( AtmGrid == 0 ) AtmGrid = new AtmosphereGrid( NumGrids );
  else {
    delete AtmGrid;
    AtmGrid = new AtmosphereGrid( NumGrids );
  }

  return true;
} //SetAtmosphereGrid

//============================================================================
AtmosphereGrid* GetAtmosphereGrid() { return AtmGrid; }

//============================================================================
long AtmosphereGridExists()
{ //AtmosphereGridExists
  if( AtmGrid ) {
    if( AtmGrid->AtmGridWTR ) return 2;
    else if( AtmGrid->AtmGridWND ) return 1;
  }

  return 0;
} //AtmosphereGridExists

//----------------------------------------------------------------------------
//BurnPeriod Data and Functions.
//----------------------------------------------------------------------------

static double DownTime = 0.0;
static long NumAbsoluteData = 0;
static long NumRelativeData = 0;
static long LastAccess = -1;
static AbsoluteBurnPeriod* abp = 0;
static RelativeBurnPeriod* rbp = 0;

//============================================================================
bool AllocBurnPeriod( long Num )
{ //AllocBurnPeriod
  FreeBurnPeriod();

  long i,FirstDay, NumDays, LastDay;
  long mo, dy, JDay;

  FirstDay = GetJulianDays( GetMinMonth() );
  FirstDay += GetMinDay();
  LastDay = GetJulianDays( GetMaxMonth() );
  LastDay += GetMaxDay();
  if( FirstDay < LastDay ) NumDays = LastDay - FirstDay;
  else NumDays = 365 - FirstDay + LastDay;
  if( Num < NumDays ) Num = NumDays;

  abp = new AbsoluteBurnPeriod[Num];
  if( abp == NULL ) return false;
  memset( abp, 0x0, Num * sizeof(AbsoluteBurnPeriod) );
  NumAbsoluteData = Num;

  mo = GetMinMonth();
  dy = GetMinDay();
  for( i = 0; i < NumDays; i++ ) {
    abp[i].Month = mo;
    abp[i].Day = dy;
    abp[i].Start = 0;
    abp[i].End = 2400;
    dy++;
    JDay = dy + GetJulianDays( mo );
    if( mo < 12 ) {
      if( JDay > GetJulianDays(mo + 1) ) {
        dy = 1;
        mo++;
      }
    }
    else {
      if( JDay > 365 ) {
        dy = 1;
        mo = 1;
      }
    }
  }

  return true;
} //AllocBurnPeriod

//============================================================================
void FreeBurnPeriod()
{ //FreeBurnPeriod
  NumRelativeData = 0;
  NumAbsoluteData = 0;
  if( abp ) delete[] abp;
  abp = 0;
  if( rbp ) delete[] rbp;
  rbp = 0;
  DownTime = 0.0;
  LastAccess = -1;
} //FreeBurnPeriod

//============================================================================
void SetBurnPeriod( long num, long mo, long dy, long start, long end )
{ //SetBurnPeriod
  if( abp ) {
    for( i = 0; i < NumAbsoluteData; i++ ) {
      num = i;
      if( abp[i].Month == mo && abp[i].Day == dy ) break;
    }
    abp[num].Month = mo;
    abp[num].Day = dy;
    abp[num].Start = start;
    abp[num].End = end;
  }
} //SetBurnPeriod

//============================================================================
bool InquireInBurnPeriod( double SimTime )
{ //InquireInBurnPeriod
  if( ! rbp ) return true;

  for( i = LastAccess + 1; i < NumRelativeData; i++ ) {
    if( SimTime >= rbp[i].Start ) {
      if( SimTime <= rbp[i].End ) {
        LastAccess = i - 1;

        return true;
      }
    }
    else break;
  }

  return false;
} //InquireInBurnPeriod

//============================================================================
long GetJulianDays( long Month )
{ //GetJulianDays
  long days;

  switch( Month ) {
    case 1: days = 0;  break;  //Cumulative days to begin of month
    case 2: days = 31;  break; //Except ignores leapyear, but who cares anyway
    case 3: days = 59;  break;
    case 4: days = 90;  break;
    case 5: days = 120;  break;
    case 6: days = 151;  break;
    case 7: days = 181;  break;
    case 8: days = 212;  break;
    case 9: days = 243;  break;
    case 10: days = 273;  break;
    case 11: days = 304;  break;
    case 12: days = 334;  break;
    default: days = 0;  break;
  }

  return days;
} //GetJulianDays

/*============================================================================
  ConvertActualTimeToSimtime
  Converts a time expressed in "component form" (ie: month, day, hour,
  minutes) to the same time expressed in minutes from the beginning of the
  simulation.

  This algorithm assumes that the arguments have the following units:
    mo  "month of year" number (1-12)
    dy  "day of month" number (1-31)
    hr  "hour of day times 100" number (0000 - 2400)
    min "minute of hour" number (0-59)

  This algorithm works by converting the simulation start time to "minutes
  since the start of the year" for a reference. Then the provided time is also
  converted to "minutes since the start of the year." The "simulation time" is
  then computed by a simple subtraction of the start time from the provided
  time.

  Because there is no provision for specifying the year of any of the times,
  certain assumptions must be made. For instance, if the "month" of the
  provided time is less than the month of the simulation start, the algorithm
  assumes that the provided time refers to a time in the next year.
*/
double ConvertActualTimeToSimtime( long mo, long dy, long hr, long mn,
                                   bool FromCondit )
{ //ConvertActualTimeToSimtime
  double SimTime, RefStart;

  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ConvertActualTimeToSimtime:1 %ld %ld %ld %ld %d\n",
            CallLevel, "", mo, dy, hr, mn, FromCondit );

  if( ! FromCondit ) {
    if( Verbose > CallLevel )
      printf( "%*sfsglbvar:ConvertActualTimeToSimtime:1a "
              "startmonth=%ld startday=%ld starthour=%ld startmin=%ld\n",
              CallLevel, "", StartMonth, StartDay, StartHour, StartMin );

    if( GetStartDate() == 0 ) {
      SimTime = -1.0;

      CallLevel--;

      return SimTime;
    }
    RefStart = ( GetJulianDays(StartMonth) + StartDay ) * 1440.0 +
               StartHour / 100.0 * 60.0 + StartMin;
  }
  else {
    if( ConditMonth == 0 && ConditDay == 0 ) {
      SimTime = -1.0;

      CallLevel--;

      return SimTime;
    }
    RefStart = ( GetJulianDays(ConditMonth) + ConditDay ) * 1440.0;
  }

  if( ! FromCondit ) {
    if( mo >= StartMonth )
      SimTime = ( GetJulianDays(mo) + dy ) * 1440.0 + hr / 100.0 * 60.0 + mn;
    else
      SimTime = ( GetJulianDays(mo) + dy + 365.0 ) * 1440.0 +
                hr / 100.0 * 60.0 + mn;
  }
  else {
    if( mo >= ConditMonth )
      SimTime = (GetJulianDays(mo) + dy) * 1440.0 + hr / 100.0 * 60.0 + mn;
    else
      SimTime = ( GetJulianDays(mo) + dy + 365.0 ) * 1440.0 +
                hr / 100.0 * 60.0 + mn;
  }
  SimTime -= RefStart;

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ConvertActualTimeToSimtime:2 SimTime=%lf\n",
            CallLevel, "", SimTime );
  CallLevel--;

  return SimTime;
} //ConvertActualTimeToSimtime

//============================================================================
void ConvertSimtimeToActualTime( double SimTime, long* mo, long* dy, long* hr,
                                 long* mn, bool FromCondit )
{ //ConvertSimtimeToActualTime
  long   months, days, hours, mins, ndays;
  double fday, fhour;

  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ConvertSimtimeToActualTime:1 SimTime=%lf\n",
            CallLevel, "", SimTime );

  fday = ( SimTime / 1440.0 );  //Days from minutes
  days = (long) fday;
  fhour = ( (fday - days) * 24.0 );  //Hours from days
  hours = (long) fhour;
  mins = (long)( (fhour - hours) * 60 );  //Minutes from hours

  if( ! FromCondit ) {
    hours += GetStartHour() / 100;
    mins += GetStartMin();
  }
  if( mins > 60 ) {
    mins -= 60;
    hours++;
  }
  if( hours >= 24 ) {
    hours -= 24;
    days++;
  }
  hours *= 100;

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ConvertSimtimeToActualTime:2 days=%ld\n",
            CallLevel, "", days );

  if( ! FromCondit ) {
    if( GetStartDate() + days > 365 ) {
      days = GetStartDate() + days - 365;
      for( i = 1; i < 12; i++ ) {
        if( days < GetJulianDays(i) ) break;
      }
      days -= GetJulianDays( i - 1 );
      months = i - 1;
    }
    else {
      for( i = 1; i < 12; i++ ) {
        if( GetStartMonth() + i > 12 )
          ndays = 365 + GetJulianDays( i );
        else ndays = GetJulianDays( GetStartMonth() + i );
        if( days + GetStartDate() <= ndays ) break;
      }
      if( GetStartMonth() + i - 1 > 12 )
        ndays = 365 + GetJulianDays( i - 1 );
      else ndays = GetJulianDays( GetStartMonth() + i - 1 );
      days -= ( ndays - GetStartDate() );
      months = GetStartMonth() + ( i - 1 );
      if( months > 12 ) months -= 12;
    }
  }
  else {
    if( Verbose > CallLevel )
      printf( "%*sfsglbvar:ConvertSimtimeToActualTime:2a days=%ld\n",
              CallLevel, "", days );

    long ConDate = GetJulianDays( ConditMonth ) + ConditDay;

    if( ConDate + days > 365 ) {
      if( Verbose > CallLevel )
        printf( "%*sfsglbvar:ConvertSimtimeToActualTime:2a1 days=%ld\n",
                CallLevel, "", days );
      days = ConDate + days - 365;
      for( i = 1; i < 12; i++ ) {
        if( days < GetJulianDays(i) ) break;
      }
      days -= GetJulianDays( i - 1 );
      months = i - 1;
    }
    else {
      if( Verbose > CallLevel )
        printf( "%*sfsglbvar:ConvertSimtimeToActualTime:2a2 days=%ld\n",
                CallLevel, "", days );

      for( i = 1; i < 12; i++ ) {
        if( ConditMonth + i > 12 )
          ndays = 365 + GetJulianDays( i );
        else ndays = GetJulianDays( ConditMonth + i );
        if( days + ConDate < ndays ) break;
      }
      if( Verbose > CallLevel )
        printf( "%*sfsglbvar:ConvertSimtimeToActualTime:2a3 days=%ld\n",
                CallLevel, "", days );

      if( ConditMonth + i - 1 > 12 )
        ndays = 365 + GetJulianDays( i - 1 );
      else ndays = GetJulianDays( ConditMonth + i - 1 );

      if( Verbose > CallLevel )
        printf( "%*sfsglbvar:ConvertSimtimeToActualTime:2a4 days=%ld "
                "ndays=%ld ConDate=%ld\n",
                CallLevel, "", days, ndays, ConDate );

      days -= ( ndays - ConDate );
      months = ConditMonth + ( i - 1 );
      if( months > 12 ) months -= 12;
    }

    if( Verbose > CallLevel )
      printf( "%*sfsglbvar:ConvertSimtimeToActualTime:2b days=%ld\n",
              CallLevel, "", days );
  }

  *mo = months;
  *dy = days;
  *hr = hours;
  *mn = mins;

  if( Verbose > CallLevel )
    printf( "%*sfsglbvar:ConvertSimtimeToActualTime:3 days=%ld hours=%ld\n",
            CallLevel, "", days, hours );
  CallLevel--;
} //ConvertSimtimeToActualTime

//============================================================================
void ConvertAbsoluteToRelativeBurnPeriod()
{ //ConvertAbsoluteToRelativeBurnPeriod
  long   i, j, days, Day, Month, NumAlloc;
  double Add, Begin, End, Start, hrs, mins;
  RelativeBurnPeriod* trbp;

  if( abp == 0 ) return;
  if( rbp ) delete[] rbp;

  Start = GetJulianDays( StartMonth );
  Start += StartDay - 1;
  End = GetJulianDays( EndMonth );
  End += (long) EndDay;
  if( End < Start ) NumRelativeData = (long)( End + 365 - Start );
  else NumRelativeData = (long)(End - Start);

  NumAlloc = NumAbsoluteData;
  if( NumRelativeData > NumAbsoluteData ) NumAlloc = NumRelativeData;

  rbp = new RelativeBurnPeriod[NumAlloc];
  memset( rbp, 0x0, NumAlloc * sizeof(RelativeBurnPeriod) );
  trbp = new RelativeBurnPeriod[NumAlloc];
  memset( trbp, 0x0, NumAlloc * sizeof(RelativeBurnPeriod) );

  if( NumRelativeData > NumAbsoluteData ) {
    AbsoluteBurnPeriod* tabp = new AbsoluteBurnPeriod[NumRelativeData];
    memcpy( tabp, abp, NumAbsoluteData * sizeof(AbsoluteBurnPeriod) );
    delete[] abp;
    abp = new AbsoluteBurnPeriod[NumRelativeData];
    memcpy( abp, tabp, NumAbsoluteData * sizeof(AbsoluteBurnPeriod) );
    delete[] tabp;

    Month = abp[NumAbsoluteData - 1].Month;
    Day = abp[NumAbsoluteData - 1].Day;
    for( i = NumAbsoluteData; i < NumRelativeData; i++ ) {
      Day++;
      if( Month < 12 ) {
        if( Day > GetJulianDays(Month + 1) - GetJulianDays(Month) ) {
          Day = 1;
          Month++;
        }
      }
      else {
        if( Day > 31 ) {
          Day = 1;
          Month = 1;
        }
      }
      abp[i].Month = Month;
      abp[i].Day = Day;
      abp[i].Start = 0;
      abp[i].End = 2400;
    }
    NumAbsoluteData = NumRelativeData;
  }

  rbp[0].Start = 0.0;
  rbp[0].End = 1440 - StartHour * 0.6;
  for( i = 1; i < NumRelativeData; i++ ) {  //Initialize all data to minutes
    rbp[i].Start = rbp[i - 1].End;
    rbp[i].End = 1440 * ( i + 1 ) - StartHour * 0.6;
  }

  days = GetJulianDays( abp[0].Month );
  days += abp[0].Day - 1;
  End = Start = days * 1440;
  mins = modf( abp[0].Start / 100.0, &hrs );
  Start += ( hrs * 60.0 + mins );
  mins = modf( abp[0].End / 100.0, &hrs );
  End += ( hrs * 60.0 + mins );
  End -= Start;

  trbp[0].Start = 0;
  trbp[0].End = End;

  for( i = 1; i < NumAbsoluteData; i++ ) {
    days = GetJulianDays( abp[i].Month );
    if( abp[i].Month < abp[0].Month ) days += 365;
    days += abp[i].Day - 1;
    End = Begin = days * 1440;
    mins = modf( abp[i].Start / 100.0, &hrs );
    Begin += ( hrs * 60.0 + mins );
    mins = modf( abp[i].End / 100.0, &hrs );
    End += ( hrs * 60.0 + mins );
    Begin -= Start;
    End -= Start;

    trbp[i].Start = Begin;
    trbp[i].End = End;
  }

  days = GetJulianDays( StartMonth );
  days += StartDay - 1;
  if( StartMonth < abp[0].Month ) days += 365;
  Begin = days * 1440;
  mins = modf( StartHour / 100.0, &hrs );
  Begin += ( hrs * 60.0 + mins );  //Beginning of simulation time

  if( trbp[NumAbsoluteData - 1].End + Start < Begin )  //If after new year
    Add = Begin - (Start + (365 * 1440));  //Start of data af
  else Add = Start - Begin;  //Start of data earlier than begin of simulation

  for( i = 0; i < NumAbsoluteData; i++ ) {
    trbp[i].Start += Add;
    trbp[i].End += Add;
  }

  long first_day, start_day, end_day;

  first_day = GetJulianDays( GetMinMonth() );
  first_day += GetMinDay();
  start_day = GetJulianDays( GetStartMonth() );
  start_day += GetStartDay();
  end_day = GetJulianDays( EndMonth );
  end_day += EndDay;
  if( start_day < first_day ) start_day += 365;
  if( end_day < first_day ) end_day += 365;

  long num_days = end_day - start_day;

  for( i = 0; i <= num_days; i++ ) {
    j = i + start_day - first_day;
    if( i < NumAlloc && j < NumAlloc ) rbp[i] = trbp[j];
  }

  LastAccess = -1;

  delete[] trbp;
} //ConvertAbsoluteToRelativeBurnPeriod

//============================================================================
void AddDownTime( double time )
{ //AddDownTime
  if( time < 0 ) DownTime = 0.0;
  else DownTime += time;
} //AddDownTime

//============================================================================
double GetDownTime() { return DownTime; }

//----------------------------------------------------------------------------
//Fuel Moisture Data and Functions.
//----------------------------------------------------------------------------

static InitialFuelMoisture fm[257];

//============================================================================
bool SetInitialFuelMoistures( long Model, long t1, long t10, long t100,
                              long tlh, long tlw )
{ //SetInitialFuelMoistures
  if( Model > 256 || Model < 1 ) return false;

  fm[Model].TL1 = t1;
  fm[Model].TL10 = t10;
  fm[Model].TL100 = t100;
  fm[Model].TLLH = tlh;
  fm[Model].TLLW = tlw;

  if( t1 > 1 && t10 > 1 ) fm[Model].FuelMoistureIsHere = true;
  else fm[Model].FuelMoistureIsHere = false;

  long i, j;
  for( i = 0; i < 3; i++ ) {  //Only up to 100 hr fuels [3]
    for( j = 0; j < 5; j++ ) EnvtChanged[i][j] = true;
  }

  return fm[Model].FuelMoistureIsHere;
} //SetInitialFuelMoistures

//============================================================================
bool GetInitialFuelMoistures( long Model, long* t1, long* t10, long* t100,
                              long* tlh, long* tlw )
{ //GetInitialFuelMoistures
  if( Model > 256 || Model < 1 ) return false;

  if( fm[Model].FuelMoistureIsHere ) {
    *t1 = fm[Model].TL1;
    *t10 = fm[Model].TL10;
    *t100 = fm[Model].TL100;
    *tlh = fm[Model].TLLH;
    *tlw = fm[Model].TLLW;

    return true;
  }

  return false;
} //GetInitialFuelMoistures

//============================================================================
long GetInitialFuelMoisture( long Model, long FuelClass )
{ //GetInitialFuelMoisture
  if( Model > 256 || Model < 1 ) return 2;

  long mx;

  switch( FuelClass ) {
    case 0: mx = fm[Model].TL1;  break;
    case 1: mx = fm[Model].TL10;  break;
    case 2: mx = fm[Model].TL100;  break;
    case 3: mx = fm[Model].TLLH;  break;
    case 4: mx = fm[Model].TLLW;  break;
  }

  return mx;
} //GetInitialFuelMoisture

//============================================================================
bool InitialFuelMoistureIsHere( long Model )
{ //InitialFuelMoistureIsHere
  if( Model > 256 || Model < 1 ) return false;

  return fm[Model].FuelMoistureIsHere;
} //InitialFuelMoistureIsHere

//----------------------------------------------------------------------------
//Coarse Woody Data and Functions.
//----------------------------------------------------------------------------

CoarseWoody coarsewoody[MAXNUM_COARSEWOODY_MODELS];
static double WeightLossErrorTol = 1.0;	// Mg/ha
CoarseWoody NFFLWoody[256];
CoarseWoody tempwoody;
static long CombineOption = CWD_COMBINE_ABSENT;

//============================================================================
CoarseWoody::CoarseWoody()
{ //CoarseWoody::CoarseWoody
  wd = 0;
  NumClasses = 0;
  Units = -1;
  TotalWeight = 0.0;
  Depth = 0.0;
} //CoarseWoody::CoarseWoody

//============================================================================
bool AllocCoarseWoody( long ModelNumber, long NumClasses )
{ //AllocCoarseWoody
  if( coarsewoody[ModelNumber - 1].wd ) FreeCoarseWoody( ModelNumber );
  if( (coarsewoody[ModelNumber - 1].wd = new WoodyData[NumClasses]) == NULL )
    return false;
  memset( coarsewoody[ModelNumber - 1].wd, 0x0,
          NumClasses * sizeof(WoodyData) );
  coarsewoody[ModelNumber - 1].NumClasses = NumClasses;
  coarsewoody[ModelNumber - 1].TotalWeight = 0.0;
  memset( coarsewoody[ModelNumber - 1].Description, 0x0, 64 * sizeof(char) );

  return true;
} //AllocCoarseWoody

//============================================================================
void FreeCoarseWoody( long ModelNumber )
{ //FreeCoarseWoody
  if( coarsewoody[ModelNumber - 1].wd )
    delete[] coarsewoody[ModelNumber - 1].wd;
  coarsewoody[ModelNumber - 1].NumClasses = 0;
  coarsewoody[ModelNumber - 1].wd = 0;
  coarsewoody[ModelNumber - 1].Units = -1;
  coarsewoody[ModelNumber - 1].TotalWeight = 0.0;
  coarsewoody[ModelNumber - 1].Depth = 0.0;
} //FreeCoarseWoody

//============================================================================
void FreeAllCoarseWoody()
{ //FreeAllCoarseWoody
  long i;

  for( i = 1; i < MAXNUM_COARSEWOODY_MODELS + 1; i++ ) FreeCoarseWoody( i );
  for( i = 0; i < 256; i++ ) {
    if( NFFLWoody[i].wd ) delete[] NFFLWoody[i].wd;
    NFFLWoody[i].NumClasses = 0;
    NFFLWoody[i].wd = 0;
  }
  if( tempwoody.wd )
    delete[] tempwoody.wd;
} //FreeAllCoarseWoody

//============================================================================
bool SetWoodyData( long ModelNumber, long ClassNumber, WoodyData* wd,
                   long units )
{ //SetWoodyData
  if( ModelNumber > MAXNUM_COARSEWOODY_MODELS - 1 ) return false;

  if( coarsewoody[ModelNumber - 1].NumClasses <= ClassNumber ) return false;

  double Total = 0.0;

  memcpy( &coarsewoody[ModelNumber - 1].wd[ClassNumber], wd,
          sizeof(WoodyData) );
  coarsewoody[ModelNumber - 1].Units = units;
  Total = coarsewoody[ModelNumber - 1].TotalWeight + wd[0].Load;
  coarsewoody[ModelNumber - 1].TotalWeight = Total;
  SetNFFLWoody();
  EnvtChanged[3][0] = true;
  EnvtChanged[3][1] = true;
  EnvtChanged[3][2] = true;
  EnvtChanged[3][3] = true;
  EnvtChanged[3][4] = true;

  return true;
} //SetWoodyData

//============================================================================
bool SetWoodyDataDepth( long ModelNumber, double depth,
                        const char* Description )
{ //SetWoodyDataDepth
  if( ModelNumber > MAXNUM_COARSEWOODY_MODELS - 1 ) return false;

  coarsewoody[ModelNumber - 1].Depth = depth;
  memcpy( coarsewoody[ModelNumber - 1].Description, Description,
          64 * sizeof(char) );

  return true;
} //SetWoodyDataDepth

//============================================================================
bool SetNFFLWoody()
{ //SetNFFLWoody
  long i, k;

  if( NFFLWoody[0].wd ) return true;

  for( i = 0; i < 256; i++ ) {
    if( NFFLWoody[i].wd ) delete[] NFFLWoody[i].wd;
    NFFLWoody[i].NumClasses = 0;
    NFFLWoody[i].wd = 0;
  }

  for( i = 0; i < 256; i++ ) {
    if( newfuels[i + 1].number > 0 ) {
      NFFLWoody[i].NumClasses = 0;
      if( newfuels[i + 1].h1 > 0.0 ) NFFLWoody[i].NumClasses++;
      if( newfuels[i + 1].h10 > 0.0 ) NFFLWoody[i].NumClasses++;
      if( newfuels[i + 1].h100 > 0.0 ) NFFLWoody[i].NumClasses++;
      if( newfuels[i + 1].lh > 0.0 ) NFFLWoody[i].NumClasses++;
      if( newfuels[i + 1].lw > 0.0 ) NFFLWoody[i].NumClasses++;
      if( (NFFLWoody[i].wd = new WoodyData[NFFLWoody[i].NumClasses]) == NULL )
        return false;
      memset( NFFLWoody[i].wd, 0x0,
              NFFLWoody[i].NumClasses * sizeof(WoodyData) );
      k = 0;
      if( newfuels[i + 1].h1 > 0.0 ) {
        NFFLWoody[i].wd[k].Load = newfuels[i + 1].h1;
        NFFLWoody[i].wd[k++].Load = newfuels[i + 1].sav1;
      }
      if( newfuels[i + 1].h10 > 0.0 ) {
        NFFLWoody[i].wd[k].Load = newfuels[i + 1].h10;
        NFFLWoody[i].wd[k++].Load = 109.0;
      }
      if( newfuels[i + 1].h100 > 0.0 ) {
        NFFLWoody[i].wd[k].Load = newfuels[i + 1].h100;
        NFFLWoody[i].wd[k++].Load = 30.0;
      }
      if( newfuels[i + 1].lh > 0.0 ) {
        NFFLWoody[i].wd[k].Load = newfuels[i + 1].lh;
        NFFLWoody[i].wd[k++].Load = newfuels[i + 1].savlh;
      }
      if( newfuels[i + 1].lw > 0.0 ) {
        NFFLWoody[i].wd[k].Load = newfuels[i + 1].lw;
        NFFLWoody[i].wd[k++].Load = newfuels[i + 1].savlw;
      }
    }
    else {
      NFFLWoody[i].NumClasses = 0;
      NFFLWoody[i].Depth = 0.0;
      NFFLWoody[i].TotalWeight = 0.0;
      NFFLWoody[i].Units = 1;
      NFFLWoody[i].wd = 0;
    }
  }

  return true;
} //SetNFFLWoody

//============================================================================
double Get1000HrMoisture( long ModelNumber, bool Average )
{ //Get1000HrMoisture
  long   i, Num;
  double AvgMx, sav1000;

  Num=0;
  AvgMx=0.0;
  sav1000=4.0/0.0106;  //4 inch diameter
  for( i = 0; i < coarsewoody[ModelNumber-1].NumClasses; i++ ) {
    if( coarsewoody[ModelNumber-1].wd[i].SurfaceAreaToVolume > sav1000 )
      continue;
    AvgMx += coarsewoody[ModelNumber-1].wd[i].FuelMoisture;
    Num++;
    if( ! Average ) break;         //Pick first one
  }
  if( Num > 0 ) AvgMx /= (double) Num;
  else AvgMx = 0.0;

  return AvgMx;
} //Get1000HrMoisture

//============================================================================
long GetWoodyDataUnits( long ModelNumber, char* Description )
{ //GetWoodyDataUnits
  memcpy( Description, coarsewoody[ModelNumber - 1].Description,
          64 * sizeof(char) );

  return coarsewoody[ModelNumber - 1].Units;
} //GetWoodyDataUnits

//============================================================================
void GetWoodyData( long WoodyModelNumber, long SurfModelNumber,
                   long* NumClasses, WoodyData* woody, double* depth,
                   double* load )
{ //GetWoodyData
  bool Combine = false;

  *NumClasses = 0;
  *load = 0.0;

  switch( WoodyCombineOptions(GETVAL) ) {
    case CWD_COMBINE_NEVER: Combine = false;  break;
    case CWD_COMBINE_ALWAYS: Combine = true;  break;
    case CWD_COMBINE_ABSENT:
      if( WoodyModelNumber < 1 ) Combine = true;
      else if( coarsewoody[WoodyModelNumber - 1].wd == 0 ) Combine = true;
      else Combine = false;
      break;
  }

  if( Combine ) {
    //Gather surface fuel model data.
    if( SurfModelNumber > 0 && SurfModelNumber < 14 ) {  //Alloc the max
      *NumClasses = NFFLWoody[SurfModelNumber - 1].NumClasses;
      *depth = NFFLWoody[SurfModelNumber - 1].Depth;
      *load = NFFLWoody[SurfModelNumber - 1].TotalWeight;
      memcpy( woody, NFFLWoody[SurfModelNumber - 1].wd,
              NFFLWoody[SurfModelNumber - 1].NumClasses * sizeof(WoodyData) );
    }
    else if( SurfModelNumber > 0 ) {
      //Initing vars.
      double  s1=0, hd=0, hl=0;
      long    i = 0, j;
      NewFuel nf;
      memset( &nf, 0x0, sizeof(NewFuel) );

      GetNewFuel( SurfModelNumber, &nf );
      *depth = nf.depth;
      *load = 0.0;
      if( nf.h1 > 0.0 ) {
        woody[i].Load = nf.h1;  //*0.224169061;
        woody[i].SurfaceAreaToVolume = s1;  //*3.280839895;
        woody[i++].HeatContent = hd;  //*2.32599;
      }
      if( nf.h10 > 0.0 ) {
        woody[i].Load = nf.h10;
        woody[i].SurfaceAreaToVolume = 109.0;  //*3.280839895;
        woody[i++].HeatContent = hd;  //*2.32599;
      }
      if( nf.h100 > 0.0 ) {
        woody[i].Load = nf.h100;
        woody[i].SurfaceAreaToVolume = 30;  //*3.280839895;
        woody[i++].HeatContent = hd;  //*2.32599;
      }
      if( nf.lw > 0.0 ) {
        woody[i].Load = nf.lw;
        woody[i].SurfaceAreaToVolume = nf.savlw;  //*3.280839895;
        woody[i++].HeatContent = hl;  //*2.32599;
      }
      if( nf.lh > 0.0 ) {
        woody[i].Load = nf.lh;
        woody[i].SurfaceAreaToVolume = nf.savlh;  //*3.280839895;
        woody[i++].HeatContent = hl;  //*2.32599;
      }
      for( j = 0; j < i; j++ ) {
        woody[j].Density = 513.0;
        *load += woody[j].Load;
      }
      *NumClasses = tempwoody.NumClasses = i;
    }
  }
  //Patch into coarsewoody model data if present.
  if( WoodyModelNumber > 0 ) {
    if( coarsewoody[WoodyModelNumber - 1].wd ) {
      memcpy( &woody[*NumClasses], coarsewoody[WoodyModelNumber - 1].wd,
              coarsewoody[WoodyModelNumber - 1].NumClasses *
              sizeof(WoodyData) );
      *NumClasses += coarsewoody[WoodyModelNumber - 1].NumClasses;
      *depth = coarsewoody[WoodyModelNumber - 1].Depth;
      *load += coarsewoody[WoodyModelNumber - 1].TotalWeight;
    }
  }
} //GetWoodyData

//============================================================================
void GetCurrentFuelMoistures( long fuelmod, long woodymod, double* mxin,
                              double* mxout, long NumMx )
{ //GetCurrentFuelMoistures
  bool Combine;
  long i, NumClasses = 0;

  memset( mxout,0x0, 20 * sizeof(double) );

  switch( WoodyCombineOptions(GETVAL) ) {
    case CWD_COMBINE_NEVER: Combine = false;  break;
    case CWD_COMBINE_ALWAYS: Combine = true;  break;
    case CWD_COMBINE_ABSENT:
      if( woodymod < 1 ) Combine = true;
      else if( coarsewoody[woodymod - 1].wd == 0 ) Combine = true;
      else Combine = false;
      break;
  }

  if( Combine ) {
    if( fuelmod < 14 ) {
      switch( fuelmod ) {
        case 0: break;
        case 1:
          memcpy( mxout, mxin, 3 * sizeof(double) );
          NumClasses = 3;
          break;
        case 2:
          memcpy( mxout, mxin, 3 * sizeof(double) );
          mxout[3] = mxin[5];
          NumClasses = 4;
          break;
        case 3:
          memcpy( mxout, mxin, 3 * sizeof(double) );
          NumClasses = 3;
          break;
        case 4:
          memcpy( mxout, mxin, 3 * sizeof(double) );
          mxout[3] = mxin[5];
          NumClasses = 4;
          break;
        case 5:
          memcpy( mxout, mxin, 3 * sizeof(double) );
          mxout[3] = mxin[5];
          NumClasses = 4;
          break;
        case 6:
          memcpy( mxout, mxin, 3 * sizeof(double) );
          NumClasses = 3;
          break;
        case 7:
          memcpy( mxout, mxin, 4 * sizeof(double) );
          NumClasses = 4;
          break;
        case 8:
          memcpy( mxout, mxin, 3 * sizeof(double) );
          NumClasses = 3;
          break;
        case 9:
          memcpy( mxout, mxin, 3 * sizeof(double) );
          NumClasses = 3;
          break;
        case 10:
          memcpy( mxout, mxin, 3 * sizeof(double) );
          mxout[3] = mxin[5];
          NumClasses = 4;
          break;
        case 11:
          memcpy( mxout, mxin, 3 * sizeof(double) );
          NumClasses = 3;
          break;
        case 12:
          memcpy( mxout, mxin, 3 * sizeof(double) );
          NumClasses = 3;
          break;
        case 13:
          memcpy( mxout, mxin, 3 * sizeof(double) );
          NumClasses = 3;
          break;
      }
    }
    else {
      mxout[NumClasses++] = mxin[0];
      mxout[NumClasses++] = mxin[1];
      mxout[NumClasses++] = mxin[2];
      if( newfuels[fuelmod].lh > 0.0 ) mxout[NumClasses++] = mxin[6];
      if( newfuels[fuelmod].lw > 0.0 ) mxout[NumClasses++] = mxin[5];
    }
  }
  if( woodymod > 0 ) {
    for( i = 0; i < coarsewoody[woodymod - 1].NumClasses; i++ )
      mxout[NumClasses + i] = coarsewoody[woodymod - 1].wd[i].FuelMoisture;
    memcpy( &mxout[NumClasses], mxin, 3 * sizeof(double) );
    mxout[3] = mxin[5];
  }
} //GetCurrentFuelMoistures

//============================================================================
long WoodyCombineOptions( long Option )
{ //WoodyCombineOptions
  if( Option > 0 ) CombineOption = Option;

  return CombineOption;
} //WoodyCombineOptions

//============================================================================
double WeightLossErrorTolerance( double value )
{ //WeightLossErrorTolerance
  if( value > 0.0 ) WeightLossErrorTol = value;

  return WeightLossErrorTol;
} //WeightLossErrorTolerance

//----------------------------------------------------------------

static long MaxThreads = 1;

//============================================================================
long GetMaxThreads() { return MaxThreads; }

//============================================================================
void ResetNewFuels()
{ //ResetNewFuels
  memset( newfuels, 0x0, 256 * sizeof(NewFuel) );

  InitializeNewFuel();

  for( long i = 0; i < 257; i++ ) {
    if( newfuels[i].number )
      newfuels[i].number *= -1;  //Indicate permanence
  }

  HAVE_CUST_MODELS = false;
} //ResetNewFuels

//============================================================================
bool SetNewFuel( NewFuel* newfuel )
{ //SetNewFuel
  long FuelNum;

  if( newfuel == 0 ) return false;

  FuelNum = newfuel->number;
  if( FuelNum > 256 || FuelNum < 0 ) return false;

  newfuel->number = 0;
  if( newfuel->h1 > 0.0 ) newfuel->number = FuelNum;
  else if( newfuel->h10 > 0.0 ) newfuel->number = FuelNum;
  else if( newfuel->h100 > 0.0 ) newfuel->number = FuelNum;
  else if( newfuel->lh > 0.0 ) newfuel->number = FuelNum;
  else if( newfuel->lw > 0.0 ) newfuel->number = FuelNum;

  memcpy( &newfuels[FuelNum], newfuel, sizeof(NewFuel) );

  return true;
} //SetNewFuel

//============================================================================
bool GetNewFuel(long number, NewFuel* newfuel)
{ //GetNewFuel
  if( number < 0 ) return false;

  if( newfuels[number].number == 0 ) return false;

  if( newfuel != 0 ) {
    memcpy( newfuel, &newfuels[number], sizeof(NewFuel) );
    //Return absolute value of number.
    newfuel->number = labs( newfuel->number );
  }

  return true;
} //GetNewFuel

//============================================================================
bool IsNewFuelReserved( long number )
{ //IsNewFuelReserved
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsglbvar:IsNewFuelReserved:1 number=%ld\n",
            CallLevel, "", number );

  if( number < 0 ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsglbvar:IsNewFuelReserved:1a\n", CallLevel, "" );
    CallLevel--;

    return false;
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsglbvar:IsNewFuelReserved:2 newfuels[%ld].number=%ld\n",
            CallLevel, "",number, newfuels[number].number );

  if( newfuels[number].number < 0 ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsglbvar:IsNewFuelReserved:2a newfuels[%ld].number=%ld\n",
              CallLevel, "",number, newfuels[number].number );
    CallLevel--;

    return true;
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsglbvar:IsNewFuelReserved:3\n", CallLevel, "" );
  CallLevel--;

  return false;
} //IsNewFuelReserved

//============================================================================
double GetFuelDepth( int Number )
{ //GetFuelDepth
  if( Number < 0 ) return 0.0;

  if( newfuels[Number].number == 0 ) return 0.0;

  return newfuels[Number].depth;
} //GetFuelDepth

//============================================================================
void InitializeNewFuel()
{ //InitializeNewFuel
  newfuels[1].number = 1;
  strcpy( newfuels[1].code, "FM1" );
  newfuels[1].h1 = 0.740000;
  newfuels[1].h10 = 0.000000;
  newfuels[1].h100 = 0.000000;
  newfuels[1].lh = 0.000000;
  newfuels[1].lw = 0.000000;
  newfuels[1].dynamic = 0;
  newfuels[1].sav1 = 3500;
  newfuels[1].savlh = 1800;
  newfuels[1].savlw = 1500;
  newfuels[1].depth = 1.000000;
  newfuels[1].xmext = 0.120000;
  newfuels[1].heatd = 8000.000000;
  newfuels[1].heatl = 8000.000000;
  strcpy( newfuels[1].desc, "Short Grass" );

  newfuels[2].number = 2;
  strcpy( newfuels[2].code, "FM2" );
  newfuels[2].h1 = 2.000000;
  newfuels[2].h10 = 1.000000;
  newfuels[2].h100 = 0.500000;
  newfuels[2].lh = 0.000000;
  newfuels[2].lw = 0.500000;
  newfuels[2].dynamic = 0;
  newfuels[2].sav1 = 3000;
  newfuels[2].savlh = 1800;
  newfuels[2].savlw = 1500;
  newfuels[2].depth = 1.000000;
  newfuels[2].xmext = 0.150000;
  newfuels[2].heatd = 8000.000000;
  newfuels[2].heatl = 8000.000000;
  strcpy( newfuels[2].desc, "Timber Grass/Understory" );

  newfuels[3].number = 3;
  strcpy( newfuels[3].code, "FM3" );
  newfuels[3].h1 = 3.010000;
  newfuels[3].h10 = 0.000000;
  newfuels[3].h100 = 0.000000;
  newfuels[3].lh = 0.000000;
  newfuels[3].lw = 0.000000;
  newfuels[3].dynamic = 0;
  newfuels[3].sav1 = 1500;
  newfuels[3].savlh = 1800;
  newfuels[3].savlw = 1500;
  newfuels[3].depth = 2.500000;
  newfuels[3].xmext = 0.250000;
  newfuels[3].heatd = 8000.000000;
  newfuels[3].heatl = 8000.000000;
  strcpy( newfuels[3].desc, "Tall Grass" );

  newfuels[4].number = 4;
  strcpy( newfuels[4].code, "FM4" );
  newfuels[4].h1 = 5.010000;
  newfuels[4].h10 = 4.010000;
  newfuels[4].h100 = 2.000000;
  newfuels[4].lh = 0.000000;
  newfuels[4].lw = 5.010000;
  newfuels[4].dynamic = 0;
  newfuels[4].sav1 = 2000;
  newfuels[4].savlh = 1800;
  newfuels[4].savlw = 1500;
  newfuels[4].depth = 6.000000;
  newfuels[4].xmext = 0.200000;
  newfuels[4].heatd = 8000.000000;
  newfuels[4].heatl = 8000.000000;
  strcpy( newfuels[4].desc, "Chaparral" );

  newfuels[5].number = 5;
  strcpy( newfuels[5].code, "FM5" );
  newfuels[5].h1 = 1.000000;
  newfuels[5].h10 = 0.500000;
  newfuels[5].h100 = 0.000000;
  newfuels[5].lh = 0.000000;
  newfuels[5].lw = 2.000000;
  newfuels[5].dynamic = 0;
  newfuels[5].sav1 = 2000;
  newfuels[5].savlh = 1800;
  newfuels[5].savlw = 1500;
  newfuels[5].depth = 2.000000;
  newfuels[5].xmext = 0.200000;
  newfuels[5].heatd = 8000.000000;
  newfuels[5].heatl = 8000.000000;
  strcpy( newfuels[5].desc, "Short Brush" );

  newfuels[6].number = 6;
  strcpy( newfuels[6].code, "FM6" );
  newfuels[6].h1 = 1.500000;
  newfuels[6].h10 = 2.500000;
  newfuels[6].h100 = 2.000000;
  newfuels[6].lh = 0.000000;
  newfuels[6].lw = 0.000000;
  newfuels[6].dynamic = 0;
  newfuels[6].sav1 = 1750;
  newfuels[6].savlh = 1800;
  newfuels[6].savlw = 1500;
  newfuels[6].depth = 2.500000;
  newfuels[6].xmext = 0.250000;
  newfuels[6].heatd = 8000.000000;
  newfuels[6].heatl = 8000.000000;
  strcpy( newfuels[6].desc, "Dormant Brush" );

  newfuels[7].number = 7;
  strcpy( newfuels[7].code, "FM7" );
  newfuels[7].h1 = 1.130000;
  newfuels[7].h10 = 1.870000;
  newfuels[7].h100 = 1.500000;
  newfuels[7].lh = 0.000000;
  newfuels[7].lw = 0.370000;
  newfuels[7].dynamic = 0;
  newfuels[7].sav1 = 1550;
  newfuels[7].savlh = 1800;
  newfuels[7].savlw = 1500;
  newfuels[7].depth = 2.500000;
  newfuels[7].xmext = 0.400000;
  newfuels[7].heatd = 8000.000000;
  newfuels[7].heatl = 8000.000000;
  strcpy( newfuels[7].desc, "Southern Rough" );

  newfuels[8].number = 8;
  strcpy( newfuels[8].code, "FM8" );
  newfuels[8].h1 = 1.500000;
  newfuels[8].h10 = 1.000000;
  newfuels[8].h100 = 2.500000;
  newfuels[8].lh = 0.000000;
  newfuels[8].lw = 0.000000;
  newfuels[8].dynamic = 0;
  newfuels[8].sav1 = 2000;
  newfuels[8].savlh = 1800;
  newfuels[8].savlw = 1500;
  newfuels[8].depth = 0.200000;
  newfuels[8].xmext = 0.300000;
  newfuels[8].heatd = 8000.000000;
  newfuels[8].heatl = 8000.000000;
  strcpy( newfuels[8].desc, "Closed Timber Litter" );

  newfuels[9].number = 9;
  strcpy( newfuels[9].code, "FM9" );
  newfuels[9].h1 = 2.920000;
  newfuels[9].h10 = 0.410000;
  newfuels[9].h100 = 0.150000;
  newfuels[9].lh = 0.000000;
  newfuels[9].lw = 0.000000;
  newfuels[9].dynamic = 0;
  newfuels[9].sav1 = 2500;
  newfuels[9].savlh = 1800;
  newfuels[9].savlw = 1500;
  newfuels[9].depth = 0.200000;
  newfuels[9].xmext = 0.250000;
  newfuels[9].heatd = 8000.000000;
  newfuels[9].heatl = 8000.000000;
  strcpy( newfuels[9].desc, "Hardwood Litter" );

  newfuels[10].number = 10;
  strcpy( newfuels[10].code, "FM10" );
  newfuels[10].h1 = 3.010000;
  newfuels[10].h10 = 2.000000;
  newfuels[10].h100 = 5.010000;
  newfuels[10].lh = 0.000000;
  newfuels[10].lw = 2.000000;
  newfuels[10].dynamic = 0;
  newfuels[10].sav1 = 2000;
  newfuels[10].savlh = 1800;
  newfuels[10].savlw = 1500;
  newfuels[10].depth = 1.000000;
  newfuels[10].xmext = 0.250000;
  newfuels[10].heatd = 8000.000000;
  newfuels[10].heatl = 8000.000000;
  strcpy( newfuels[10].desc, "Timber Litter/Understory" );

  newfuels[11].number = 11;
  strcpy( newfuels[11].code, "FM11" );
  newfuels[11].h1 = 1.500000;
  newfuels[11].h10 = 4.510000;
  newfuels[11].h100 = 5.510000;
  newfuels[11].lh = 0.000000;
  newfuels[11].lw = 0.000000;
  newfuels[11].dynamic = 0;
  newfuels[11].sav1 = 1500;
  newfuels[11].savlh = 1800;
  newfuels[11].savlw = 1500;
  newfuels[11].depth = 1.000000;
  newfuels[11].xmext = 0.150000;
  newfuels[11].heatd = 8000.000000;
  newfuels[11].heatl = 8000.000000;
  strcpy( newfuels[11].desc, "Light Slash" );

  newfuels[12].number = 12;
  strcpy( newfuels[12].code, "FM12" );
  newfuels[12].h1 = 4.010000;
  newfuels[12].h10 = 14.030000;
  newfuels[12].h100 = 16.530000;
  newfuels[12].lh = 0.000000;
  newfuels[12].lw = 0.000000;
  newfuels[12].dynamic = 0;
  newfuels[12].sav1 = 1500;
  newfuels[12].savlh = 1800;
  newfuels[12].savlw = 1500;
  newfuels[12].depth = 2.300000;
  newfuels[12].xmext = 0.200000;
  newfuels[12].heatd = 8000.000000;
  newfuels[12].heatl = 8000.000000;
  strcpy( newfuels[12].desc, "Medium Slash" );

  newfuels[13].number = 13;
  strcpy( newfuels[13].code, "FM13" );
  newfuels[13].h1 = 7.010000;
  newfuels[13].h10 = 23.040000;
  newfuels[13].h100 = 28.050000;
  newfuels[13].lh = 0.000000;
  newfuels[13].lw = 0.000000;
  newfuels[13].dynamic = 0;
  newfuels[13].sav1 = 1500;
  newfuels[13].savlh = 1800;
  newfuels[13].savlw = 1500;
  newfuels[13].depth = 3.000000;
  newfuels[13].xmext = 0.250000;
  newfuels[13].heatd = 8000.000000;
  newfuels[13].heatl = 8000.000000;
  strcpy( newfuels[13].desc, "Heavy Slash" );

  newfuels[99].number = 99;
  strcpy( newfuels[99].code, "NB1" );
  strcpy( newfuels[99].desc, "Barren" );

  newfuels[98].number = 98;
  strcpy( newfuels[98].code, "NB2" );
  strcpy( newfuels[98].desc, "Water" );

  newfuels[97].number = 97;
  strcpy( newfuels[97].code, "NB3" );
  strcpy( newfuels[97].desc, "Snow or Ice" );

  newfuels[96].number = 96;
  strcpy( newfuels[96].code, "NB4" );
  strcpy( newfuels[96].desc, "Agricultural or Cropland" );

  newfuels[95].number = 95;
  strcpy( newfuels[95].code, "NB5" );
  strcpy( newfuels[95].desc, "Urban or Developed" );

  newfuels[101].number = 101;
  strcpy( newfuels[101].code, "GR1" );
  newfuels[101].h1 = 0.100000;
  newfuels[101].h10 = 0.000000;
  newfuels[101].h100 = 0.000000;
  newfuels[101].lh = 0.300000;
  newfuels[101].lw = 0.000000;
  newfuels[101].dynamic = 1;
  newfuels[101].sav1 = 2200;
  newfuels[101].savlh = 2000;
  newfuels[101].savlw = 1500;
  newfuels[101].depth = 0.400000;
  newfuels[101].xmext = 0.150000;
  newfuels[101].heatd = 8000.000000;
  newfuels[101].heatl = 8000.000000;
  strcpy( newfuels[101].desc, "Short, sparse, dry climate grass" );

  newfuels[102].number = 102;
  strcpy( newfuels[102].code, "GR2" );
  newfuels[102].h1 = 0.100000;
  newfuels[102].h10 = 0.000000;
  newfuels[102].h100 = 0.000000;
  newfuels[102].lh = 1.000000;
  newfuels[102].lw = 0.000000;
  newfuels[102].dynamic = 1;
  newfuels[102].sav1 = 2000;
  newfuels[102].savlh = 1800;
  newfuels[102].savlw = 1500;
  newfuels[102].depth = 1.000000;
  newfuels[102].xmext = 0.150000;
  newfuels[102].heatd = 8000.000000;
  newfuels[102].heatl = 8000.000000;
  strcpy( newfuels[102].desc, "Low load, dry climate grass" );

  newfuels[103].number = 103;
  strcpy( newfuels[103].code, "GR3" );
  newfuels[103].h1 = 0.100000;
  newfuels[103].h10 = 0.400000;
  newfuels[103].h100 = 0.000000;
  newfuels[103].lh = 1.500000;
  newfuels[103].lw = 0.000000;
  newfuels[103].dynamic = 1;
  newfuels[103].sav1 = 1500;
  newfuels[103].savlh = 1300;
  newfuels[103].savlw = 1500;
  newfuels[103].depth = 2.000000;
  newfuels[103].xmext = 0.300000;
  newfuels[103].heatd = 8000.000000;
  newfuels[103].heatl = 8000.000000;
  strcpy( newfuels[103].desc, "Low load, very coarse, humid climate grass" );

  newfuels[104].number = 104;
  strcpy( newfuels[104].code, "GR4" );
  newfuels[104].h1 = 0.250000;
  newfuels[104].h10 = 0.000000;
  newfuels[104].h100 = 0.000000;
  newfuels[104].lh = 1.900000;
  newfuels[104].lw = 0.000000;
  newfuels[104].dynamic = 1;
  newfuels[104].sav1 = 2000;
  newfuels[104].savlh = 1800;
  newfuels[104].savlw = 1500;
  newfuels[104].depth = 2.000000;
  newfuels[104].xmext = 0.150000;
  newfuels[104].heatd = 8000.000000;
  newfuels[104].heatl = 8000.000000;
  strcpy( newfuels[104].desc, "Moderate load, dry climate grass" );

  newfuels[105].number = 105;
  strcpy( newfuels[105].code, "GR5" );
  newfuels[105].h1 = 0.400000;
  newfuels[105].h10 = 0.000000;
  newfuels[105].h100 = 0.000000;
  newfuels[105].lh = 2.500000;
  newfuels[105].lw = 0.000000;
  newfuels[105].dynamic = 1;
  newfuels[105].sav1 = 1800;
  newfuels[105].savlh = 1600;
  newfuels[105].savlw = 1500;
  newfuels[105].depth = 1.500000;
  newfuels[105].xmext = 0.400000;
  newfuels[105].heatd = 8000.000000;
  newfuels[105].heatl = 8000.000000;
  strcpy( newfuels[105].desc, "Low load, humid climate grass" );

  newfuels[106].number = 106;
  strcpy( newfuels[106].code, "GR6" );
  newfuels[106].h1 = 0.100000;
  newfuels[106].h10 = 0.000000;
  newfuels[106].h100 = 0.000000;
  newfuels[106].lh = 3.400000;
  newfuels[106].lw = 0.000000;
  newfuels[106].dynamic = 1;
  newfuels[106].sav1 = 2200;
  newfuels[106].savlh = 2000;
  newfuels[106].savlw = 1500;
  newfuels[106].depth = 1.500000;
  newfuels[106].xmext = 0.400000;
  newfuels[106].heatd = 9000.000000;
  newfuels[106].heatl = 9000.000000;
  strcpy( newfuels[106].desc, "Moderate load, humid climate grass" );

  newfuels[107].number = 107;
  strcpy( newfuels[107].code, "GR7" );
  newfuels[107].h1 = 1.000000;
  newfuels[107].h10 = 0.000000;
  newfuels[107].h100 = 0.000000;
  newfuels[107].lh = 5.400000;
  newfuels[107].lw = 0.000000;
  newfuels[107].dynamic = 1;
  newfuels[107].sav1 = 2000;
  newfuels[107].savlh = 1800;
  newfuels[107].savlw = 1500;
  newfuels[107].depth = 3.000000;
  newfuels[107].xmext = 0.150000;
  newfuels[107].heatd = 8000.000000;
  newfuels[107].heatl = 8000.000000;
  strcpy( newfuels[107].desc, "High load, dry climate grass" );

  newfuels[108].number = 108;
  strcpy( newfuels[108].code, "GR8" );
  newfuels[108].h1 = 0.500000;
  newfuels[108].h10 = 1.000000;
  newfuels[108].h100 = 0.000000;
  newfuels[108].lh = 7.300000;
  newfuels[108].lw = 0.000000;
  newfuels[108].dynamic = 1;
  newfuels[108].sav1 = 1500;
  newfuels[108].savlh = 1300;
  newfuels[108].savlw = 1500;
  newfuels[108].depth = 4.000000;
  newfuels[108].xmext = 0.300000;
  newfuels[108].heatd = 8000.000000;
  newfuels[108].heatl = 8000.000000;
  strcpy( newfuels[108].desc, "High load, very coarse, humid climate grass" );

  newfuels[109].number = 109;
  strcpy( newfuels[109].code, "GR9" );
  newfuels[109].h1 = 1.000000;
  newfuels[109].h10 = 1.000000;
  newfuels[109].h100 = 0.000000;
  newfuels[109].lh = 9.000000;
  newfuels[109].lw = 0.000000;
  newfuels[109].dynamic = 1;
  newfuels[109].sav1 = 1800;
  newfuels[109].savlh = 1600;
  newfuels[109].savlw = 1500;
  newfuels[109].depth = 5.000000;
  newfuels[109].xmext = 0.400000;
  newfuels[109].heatd = 8000.000000;
  newfuels[109].heatl = 8000.000000;
  strcpy( newfuels[109].desc, "Very high load, humid climate grass" );

  newfuels[121].number = 121;
  strcpy( newfuels[121].code, "GS1" );
  newfuels[121].h1 = 0.200000;
  newfuels[121].h10 = 0.000000;
  newfuels[121].h100 = 0.000000;
  newfuels[121].lh = 0.500000;
  newfuels[121].lw = 0.650000;
  newfuels[121].dynamic = 1;
  newfuels[121].sav1 = 2000;
  newfuels[121].savlh = 1800;
  newfuels[121].savlw = 1800;
  newfuels[121].depth = 0.900000;
  newfuels[121].xmext = 0.150000;
  newfuels[121].heatd = 8000.000000;
  newfuels[121].heatl = 8000.000000;
  strcpy( newfuels[121].desc, "Low load, dry climate grass-shrub" );

  newfuels[122].number = 122;
  strcpy( newfuels[122].code, "GS2" );
  newfuels[122].h1 = 0.500000;
  newfuels[122].h10 = 0.500000;
  newfuels[122].h100 = 0.000000;
  newfuels[122].lh = 0.600000;
  newfuels[122].lw = 1.000000;
  newfuels[122].dynamic = 1;
  newfuels[122].sav1 = 2000;
  newfuels[122].savlh = 1800;
  newfuels[122].savlw = 1800;
  newfuels[122].depth = 1.500000;
  newfuels[122].xmext = 0.150000;
  newfuels[122].heatd = 8000.000000;
  newfuels[122].heatl = 8000.000000;
  strcpy( newfuels[122].desc, "Moderate load, dry climate grass-shrub" );

  newfuels[123].number = 123;
  strcpy( newfuels[123].code, "GS3" );
  newfuels[123].h1 = 0.300000;
  newfuels[123].h10 = 0.250000;
  newfuels[123].h100 = 0.000000;
  newfuels[123].lh = 1.450000;
  newfuels[123].lw = 1.250000;
  newfuels[123].dynamic = 1;
  newfuels[123].sav1 = 1800;
  newfuels[123].savlh = 1600;
  newfuels[123].savlw = 1600;
  newfuels[123].depth = 1.800000;
  newfuels[123].xmext = 0.400000;
  newfuels[123].heatd = 8000.000000;
  newfuels[123].heatl = 8000.000000;
  strcpy( newfuels[123].desc, "Moderate load, humid climate grass-shrub" );

  newfuels[124].number = 124;
  strcpy( newfuels[124].code, "GS4" );
  newfuels[124].h1 = 1.900000;
  newfuels[124].h10 = 0.300000;
  newfuels[124].h100 = 0.100000;
  newfuels[124].lh = 3.400000;
  newfuels[124].lw = 7.100000;
  newfuels[124].dynamic = 1;
  newfuels[124].sav1 = 1800;
  newfuels[124].savlh = 1600;
  newfuels[124].savlw = 1600;
  newfuels[124].depth = 2.100000;
  newfuels[124].xmext = 0.400000;
  newfuels[124].heatd = 8000.000000;
  newfuels[124].heatl = 8000.000000;
  strcpy( newfuels[124].desc, "High load, humid climate grass-shrub" );

  newfuels[141].number = 141;
  strcpy( newfuels[141].code, "SH1" );
  newfuels[141].h1 = 0.250000;
  newfuels[141].h10 = 0.250000;
  newfuels[141].h100 = 0.000000;
  newfuels[141].lh = 0.150000;
  newfuels[141].lw = 1.300000;
  newfuels[141].dynamic = 1;
  newfuels[141].sav1 = 2000;
  newfuels[141].savlh = 1800;
  newfuels[141].savlw = 1600;
  newfuels[141].depth = 1.000000;
  newfuels[141].xmext = 0.150000;
  newfuels[141].heatd = 8000.000000;
  newfuels[141].heatl = 8000.000000;
  strcpy( newfuels[141].desc, "Low load, dry climate shrub" );

  newfuels[142].number = 142;
  strcpy( newfuels[142].code, "SH2" );
  newfuels[142].h1 = 1.350000;
  newfuels[142].h10 = 2.400000;
  newfuels[142].h100 = 0.750000;
  newfuels[142].lh = 0.000000;
  newfuels[142].lw = 3.850000;
  newfuels[142].dynamic = 0;
  newfuels[142].sav1 = 2000;
  newfuels[142].savlh = 1800;
  newfuels[142].savlw = 1600;
  newfuels[142].depth = 1.000000;
  newfuels[142].xmext = 0.150000;
  newfuels[142].heatd = 8000.000000;
  newfuels[142].heatl = 8000.000000;
  strcpy( newfuels[142].desc, "Moderate load, dry climate shrub" );

  newfuels[143].number = 143;
  strcpy( newfuels[143].code, "SH3" );
  newfuels[143].h1 = 0.450000;
  newfuels[143].h10 = 3.000000;
  newfuels[143].h100 = 0.000000;
  newfuels[143].lh = 0.000000;
  newfuels[143].lw = 6.200000;
  newfuels[143].dynamic = 0;
  newfuels[143].sav1 = 1600;
  newfuels[143].savlh = 1800;
  newfuels[143].savlw = 1400;
  newfuels[143].depth = 2.400000;
  newfuels[143].xmext = 0.400000;
  newfuels[143].heatd = 8000.000000;
  newfuels[143].heatl = 8000.000000;
  strcpy( newfuels[143].desc, "Moderate load, humid climate shrub" );

  newfuels[144].number = 144;
  strcpy( newfuels[144].code, "SH4" );
  newfuels[144].h1 = 0.850000;
  newfuels[144].h10 = 1.150000;
  newfuels[144].h100 = 0.200000;
  newfuels[144].lh = 0.000000;
  newfuels[144].lw = 2.550000;
  newfuels[144].dynamic = 0;
  newfuels[144].sav1 = 2000;
  newfuels[144].savlh = 1800;
  newfuels[144].savlw = 1600;
  newfuels[144].depth = 3.000000;
  newfuels[144].xmext = 0.300000;
  newfuels[144].heatd = 8000.000000;
  newfuels[144].heatl = 8000.000000;
  strcpy( newfuels[144].desc, "Low load, humid climate timber-shrub" );

  newfuels[145].number = 145;
  strcpy( newfuels[145].code, "SH5" );
  newfuels[145].h1 = 3.600000;
  newfuels[145].h10 = 2.100000;
  newfuels[145].h100 = 0.000000;
  newfuels[145].lh = 0.000000;
  newfuels[145].lw = 2.900000;
  newfuels[145].dynamic = 0;
  newfuels[145].sav1 = 750;
  newfuels[145].savlh = 1800;
  newfuels[145].savlw = 1600;
  newfuels[145].depth = 6.000000;
  newfuels[145].xmext = 0.150000;
  newfuels[145].heatd = 8000.000000;
  newfuels[145].heatl = 8000.000000;
  strcpy( newfuels[145].desc, "High load, dry climate shrub" );

  newfuels[146].number = 146;
  strcpy( newfuels[146].code, "SH6" );
  newfuels[146].h1 = 2.900000;
  newfuels[146].h10 = 1.450000;
  newfuels[146].h100 = 0.000000;
  newfuels[146].lh = 0.000000;
  newfuels[146].lw = 1.400000;
  newfuels[146].dynamic = 0;
  newfuels[146].sav1 = 750;
  newfuels[146].savlh = 1800;
  newfuels[146].savlw = 1600;
  newfuels[146].depth = 2.000000;
  newfuels[146].xmext = 0.300000;
  newfuels[146].heatd = 8000.000000;
  newfuels[146].heatl = 8000.000000;
  strcpy( newfuels[146].desc, "Low load, humid climate shrub" );

  newfuels[147].number = 147;
  strcpy( newfuels[147].code, "SH7" );
  newfuels[147].h1 = 3.500000;
  newfuels[147].h10 = 5.300000;
  newfuels[147].h100 = 2.200000;
  newfuels[147].lh = 0.000000;
  newfuels[147].lw = 3.400000;
  newfuels[147].dynamic = 0;
  newfuels[147].sav1 = 750;
  newfuels[147].savlh = 1800;
  newfuels[147].savlw = 1600;
  newfuels[147].depth = 6.000000;
  newfuels[147].xmext = 0.150000;
  newfuels[147].heatd = 8000.000000;
  newfuels[147].heatl = 8000.000000;
  strcpy( newfuels[147].desc, "Very high load, dry climate shrub" );

  newfuels[148].number = 148;
  strcpy( newfuels[148].code, "SH8" );
  newfuels[148].h1 = 2.050000;
  newfuels[148].h10 = 3.400000;
  newfuels[148].h100 = 0.850000;
  newfuels[148].lh = 0.000000;
  newfuels[148].lw = 4.350000;
  newfuels[148].dynamic = 0;
  newfuels[148].sav1 = 750;
  newfuels[148].savlh = 1800;
  newfuels[148].savlw = 1600;
  newfuels[148].depth = 3.000000;
  newfuels[148].xmext = 0.400000;
  newfuels[148].heatd = 8000.000000;
  newfuels[148].heatl = 8000.000000;
  strcpy( newfuels[148].desc, "High load, humid climate shrub" );

  newfuels[149].number = 149;
  strcpy( newfuels[149].code, "SH9" );
  newfuels[149].h1 = 4.500000;
  newfuels[149].h10 = 2.450000;
  newfuels[149].h100 = 0.000000;
  newfuels[149].lh = 1.550000;
  newfuels[149].lw = 7.000000;
  newfuels[149].dynamic = 1;
  newfuels[149].sav1 = 750;
  newfuels[149].savlh = 1800;
  newfuels[149].savlw = 1500;
  newfuels[149].depth = 4.400000;
  newfuels[149].xmext = 0.400000;
  newfuels[149].heatd = 8000.000000;
  newfuels[149].heatl = 8000.000000;
  strcpy( newfuels[149].desc, "Very high load, humid climate shrub" );

  newfuels[161].number = 161;
  strcpy( newfuels[161].code, "TU1" );
  newfuels[161].h1 = 0.200000;
  newfuels[161].h10 = 0.900000;
  newfuels[161].h100 = 1.500000;
  newfuels[161].lh = 0.200000;
  newfuels[161].lw = 0.900000;
  newfuels[161].dynamic = 1;
  newfuels[161].sav1 = 2000;
  newfuels[161].savlh = 1800;
  newfuels[161].savlw = 1600;
  newfuels[161].depth = 0.600000;
  newfuels[161].xmext = 0.200000;
  newfuels[161].heatd = 8000.000000;
  newfuels[161].heatl = 8000.000000;
  strcpy( newfuels[161].desc, "Light load, dry climate timber-grass-shrub" );

  newfuels[162].number = 162;
  strcpy( newfuels[162].code, "TU2" );
  newfuels[162].h1 = 0.950000;
  newfuels[162].h10 = 1.800000;
  newfuels[162].h100 = 1.250000;
  newfuels[162].lh = 0.000000;
  newfuels[162].lw = 0.200000;
  newfuels[162].dynamic = 0;
  newfuels[162].sav1 = 2000;
  newfuels[162].savlh = 1800;
  newfuels[162].savlw = 1600;
  newfuels[162].depth = 1.000000;
  newfuels[162].xmext = 0.300000;
  newfuels[162].heatd = 8000.000000;
  newfuels[162].heatl = 8000.000000;
  strcpy( newfuels[162].desc, "Moderate load, humid climate timber-shrub" );

  newfuels[163].number = 163;
  strcpy( newfuels[163].code, "TU3" );
  newfuels[163].h1 = 1.100000;
  newfuels[163].h10 = 0.150000;
  newfuels[163].h100 = 0.250000;
  newfuels[163].lh = 0.650000;
  newfuels[163].lw = 1.100000;
  newfuels[163].dynamic = 1;
  newfuels[163].sav1 = 1800;
  newfuels[163].savlh = 1600;
  newfuels[163].savlw = 1400;
  newfuels[163].depth = 1.300000;
  newfuels[163].xmext = 0.300000;
  newfuels[163].heatd = 8000.000000;
  newfuels[163].heatl = 8000.000000;
  strcpy( newfuels[163].desc,
   "Moderate load, humid climate timber-grass-shrub" );

  newfuels[164].number = 164;
  strcpy( newfuels[164].code, "TU4" );
  newfuels[164].h1 = 4.500000;
  newfuels[164].h10 = 0.000000;
  newfuels[164].h100 = 0.000000;
  newfuels[164].lh = 0.000000;
  newfuels[164].lw = 2.000000;
  newfuels[164].dynamic = 0;
  newfuels[164].sav1 = 2300;
  newfuels[164].savlh = 1800;
  newfuels[164].savlw = 2000;
  newfuels[164].depth = 0.500000;
  newfuels[164].xmext = 0.120000;
  newfuels[164].heatd = 8000.000000;
  newfuels[164].heatl = 8000.000000;
  strcpy( newfuels[164].desc, "Dwarf conifer with understory" );

  newfuels[165].number = 165;
  strcpy( newfuels[165].code, "TU5" );
  newfuels[165].h1 = 4.000000;
  newfuels[165].h10 = 4.000000;
  newfuels[165].h100 = 3.000000;
  newfuels[165].lh = 0.000000;
  newfuels[165].lw = 3.000000;
  newfuels[165].dynamic = 0;
  newfuels[165].sav1 = 1500;
  newfuels[165].savlh = 1800;
  newfuels[165].savlw = 750;
  newfuels[165].depth = 1.000000;
  newfuels[165].xmext = 0.250000;
  newfuels[165].heatd = 8000.000000;
  newfuels[165].heatl = 8000.000000;
  strcpy( newfuels[165].desc, "Very high load, dry climate timber-shrub" );

  newfuels[181].number = 181;
  strcpy( newfuels[181].code, "TL1" );
  newfuels[181].h1 = 1.000000;
  newfuels[181].h10 = 2.200000;
  newfuels[181].h100 = 3.600000;
  newfuels[181].lh = 0.000000;
  newfuels[181].lw = 0.000000;
  newfuels[181].dynamic = 0;
  newfuels[181].sav1 = 2000;
  newfuels[181].savlh = 1800;
  newfuels[181].savlw = 1600;
  newfuels[181].depth = 0.200000;
  newfuels[181].xmext = 0.300000;
  newfuels[181].heatd = 8000.000000;
  newfuels[181].heatl = 8000.000000;
  strcpy( newfuels[181].desc, "Low load, compact conifer litter" );

  newfuels[182].number = 182;
  strcpy( newfuels[182].code, "TL2" );
  newfuels[182].h1 = 1.400000;
  newfuels[182].h10 = 2.300000;
  newfuels[182].h100 = 2.200000;
  newfuels[182].lh = 0.000000;
  newfuels[182].lw = 0.000000;
  newfuels[182].dynamic = 0;
  newfuels[182].sav1 = 2000;
  newfuels[182].savlh = 1800;
  newfuels[182].savlw = 1600;
  newfuels[182].depth = 0.200000;
  newfuels[182].xmext = 0.250000;
  newfuels[182].heatd = 8000.000000;
  newfuels[182].heatl = 8000.000000;
  strcpy( newfuels[182].desc, "Low load broadleaf litter" );

  newfuels[183].number = 183;
  strcpy( newfuels[183].code, "TL3" );
  newfuels[183].h1 = 0.500000;
  newfuels[183].h10 = 2.200000;
  newfuels[183].h100 = 2.800000;
  newfuels[183].lh = 0.000000;
  newfuels[183].lw = 0.000000;
  newfuels[183].dynamic = 0;
  newfuels[183].sav1 = 2000;
  newfuels[183].savlh = 1800;
  newfuels[183].savlw = 1600;
  newfuels[183].depth = 0.300000;
  newfuels[183].xmext = 0.200000;
  newfuels[183].heatd = 8000.000000;
  newfuels[183].heatl = 8000.000000;
  strcpy( newfuels[183].desc, "Moderate load confier litter" );

  newfuels[184].number = 184;
  strcpy( newfuels[184].code, "TL4" );
  newfuels[184].h1 = 0.500000;
  newfuels[184].h10 = 1.500000;
  newfuels[184].h100 = 4.200000;
  newfuels[184].lh = 0.000000;
  newfuels[184].lw = 0.000000;
  newfuels[184].dynamic = 0;
  newfuels[184].sav1 = 2000;
  newfuels[184].savlh = 1800;
  newfuels[184].savlw = 1600;
  newfuels[184].depth = 0.400000;
  newfuels[184].xmext = 0.250000;
  newfuels[184].heatd = 8000.000000;
  newfuels[184].heatl = 8000.000000;
  strcpy( newfuels[184].desc, "Small downed logs" );

  newfuels[185].number = 185;
  strcpy( newfuels[185].code, "TL5" );
  newfuels[185].h1 = 1.150000;
  newfuels[185].h10 = 2.500000;
  newfuels[185].h100 = 4.400000;
  newfuels[185].lh = 0.000000;
  newfuels[185].lw = 0.000000;
  newfuels[185].dynamic = 0;
  newfuels[185].sav1 = 2000;
  newfuels[185].savlh = 1800;
  newfuels[185].savlw = 1600;
  newfuels[185].depth = 0.600000;
  newfuels[185].xmext = 0.250000;
  newfuels[185].heatd = 8000.000000;
  newfuels[185].heatl = 8000.000000;
  strcpy( newfuels[185].desc, "High load conifer litter" );

  newfuels[186].number = 186;
  strcpy( newfuels[186].code, "TL6" );
  newfuels[186].h1 = 2.400000;
  newfuels[186].h10 = 1.200000;
  newfuels[186].h100 = 1.200000;
  newfuels[186].lh = 0.000000;
  newfuels[186].lw = 0.000000;
  newfuels[186].dynamic = 0;
  newfuels[186].sav1 = 2000;
  newfuels[186].savlh = 1800;
  newfuels[186].savlw = 1600;
  newfuels[186].depth = 0.300000;
  newfuels[186].xmext = 0.250000;
  newfuels[186].heatd = 8000.000000;
  newfuels[186].heatl = 8000.000000;
  strcpy( newfuels[186].desc, "High load broadleaf litter" );

  newfuels[187].number = 187;
  strcpy( newfuels[187].code, "TL7" );
  newfuels[187].h1 = 0.300000;
  newfuels[187].h10 = 1.400000;
  newfuels[187].h100 = 8.100000;
  newfuels[187].lh = 0.000000;
  newfuels[187].lw = 0.000000;
  newfuels[187].dynamic = 0;
  newfuels[187].sav1 = 2000;
  newfuels[187].savlh = 1800;
  newfuels[187].savlw = 1600;
  newfuels[187].depth = 0.400000;
  newfuels[187].xmext = 0.250000;
  newfuels[187].heatd = 8000.000000;
  newfuels[187].heatl = 8000.000000;
  strcpy( newfuels[187].desc, "Large downed logs" );

  newfuels[188].number = 188;
  strcpy( newfuels[188].code, "TL8" );
  newfuels[188].h1 = 5.800000;
  newfuels[188].h10 = 1.400000;
  newfuels[188].h100 = 1.100000;
  newfuels[188].lh = 0.000000;
  newfuels[188].lw = 0.000000;
  newfuels[188].dynamic = 0;
  newfuels[188].sav1 = 1800;
  newfuels[188].savlh = 1800;
  newfuels[188].savlw = 1600;
  newfuels[188].depth = 0.300000;
  newfuels[188].xmext = 0.350000;
  newfuels[188].heatd = 8000.000000;
  newfuels[188].heatl = 8000.000000;
  strcpy( newfuels[188].desc, "Long-needle litter" );

  newfuels[189].number = 189;
  strcpy( newfuels[189].code, "TL9" );
  newfuels[189].h1 = 6.650000;
  newfuels[189].h10 = 3.300000;
  newfuels[189].h100 = 4.150000;
  newfuels[189].lh = 0.000000;
  newfuels[189].lw = 0.000000;
  newfuels[189].dynamic = 0;
  newfuels[189].sav1 = 1800;
  newfuels[189].savlh = 1800;
  newfuels[189].savlw = 1600;
  newfuels[189].depth = 0.600000;
  newfuels[189].xmext = 0.350000;
  newfuels[189].heatd = 8000.000000;
  newfuels[189].heatl = 8000.000000;
  strcpy( newfuels[189].desc, "Very high load broadleaf litter" );

  newfuels[201].number = 201;
  strcpy( newfuels[201].code, "SB1" );
  newfuels[201].h1 = 1.500000;
  newfuels[201].h10 = 3.000000;
  newfuels[201].h100 = 11.000000;
  newfuels[201].lh = 0.000000;
  newfuels[201].lw = 0.000000;
  newfuels[201].dynamic = 0;
  newfuels[201].sav1 = 2000;
  newfuels[201].savlh = 1800;
  newfuels[201].savlw = 1600;
  newfuels[201].depth = 1.000000;
  newfuels[201].xmext = 0.250000;
  newfuels[201].heatd = 8000.000000;
  newfuels[201].heatl = 8000.000000;
  strcpy( newfuels[201].desc, "Low load activity fuel" );

  newfuels[202].number = 202;
  strcpy( newfuels[202].code, "SB2" );
  newfuels[202].h1 = 4.500000;
  newfuels[202].h10 = 4.250000;
  newfuels[202].h100 = 4.000000;
  newfuels[202].lh = 0.000000;
  newfuels[202].lw = 0.000000;
  newfuels[202].dynamic = 0;
  newfuels[202].sav1 = 2000;
  newfuels[202].savlh = 1800;
  newfuels[202].savlw = 1600;
  newfuels[202].depth = 1.000000;
  newfuels[202].xmext = 0.250000;
  newfuels[202].heatd = 8000.000000;
  newfuels[202].heatl = 8000.000000;
  strcpy( newfuels[202].desc, "Moderate load activity or low load blowdown" );

  newfuels[203].number = 203;
  strcpy( newfuels[203].code, "SB3" );
  newfuels[203].h1 = 5.500000;
  newfuels[203].h10 = 2.750000;
  newfuels[203].h100 = 3.000000;
  newfuels[203].lh = 0.000000;
  newfuels[203].lw = 0.000000;
  newfuels[203].dynamic = 0;
  newfuels[203].sav1 = 2000;
  newfuels[203].savlh = 1800;
  newfuels[203].savlw = 1600;
  newfuels[203].depth = 1.200000;
  newfuels[203].xmext = 0.250000;
  newfuels[203].heatd = 8000.000000;
  newfuels[203].heatl = 8000.000000;
  strcpy( newfuels[203].desc,
          "High load activity fuel or moderate load blowdown" );

  newfuels[204].number = 204;
  strcpy( newfuels[204].code, "SB4" );
  newfuels[204].h1 = 5.250000;
  newfuels[204].h10 = 3.500000;
  newfuels[204].h100 = 5.250000;
  newfuels[204].lh = 0.000000;
  newfuels[204].lw = 0.000000;
  newfuels[204].dynamic = 0;
  newfuels[204].sav1 = 2000;
  newfuels[204].savlh = 1800;
  newfuels[204].savlw = 1600;
  newfuels[204].depth = 2.700000;
  newfuels[204].xmext = 0.250000;
  newfuels[204].heatd = 8000.000000;
  newfuels[204].heatl = 8000.000000;
  strcpy( newfuels[204].desc, "High load blowdown" );
} //InitializeNewFuel

//============================================================================
void trim( char* totrim )
{ //trim
  for( int i = strlen(totrim) - 1; i > 0; i-- ) {
    switch( totrim[i] ) {
      case '\n':
      case '\r':
      case '\t':
      case ' ':
        totrim[i] = '\0';
        break;
      default: i = -1;
    }
  }
  bool found = false;
  while( ! found ) {
    int j = 0;
    switch( totrim[0] ) {
      case '\n':
      case '\t':
      case ' ':
        for( j = 0; j < (int)strlen(totrim) - 1; j++ )
          totrim[j] = totrim[j + 1];
        totrim[j] = '\0';
        break;
      default:
        found = true;
        break;
    }
  }
} //trim
