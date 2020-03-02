/*============================================================================
  fsxw.cpp
  See LICENSE.TXT file for license information.
*/
#include<memory.h>
#include<math.h>
#include<stdio.h>
#include<string.h>
#include"globals.h"
#include"fsxw.h"

#define SIZE_FIREPERIM_BLOCK 10000

double RosRed[257];  //Rate of spread reduction factors
double *perimeter2 = 0;  //Swap array
long numfires = 0;  //Number of fires
static double PerimeterResolution = 0.0;  //Maximum tangential perimeter res
static double DistanceResolution = 0.0;  //Minimum radial spread distance res
static double DynamicDistanceResolution = 0.0;  //Minimum run-time radial res
static double actual = 0.0;  //Actual time step
static double visible = 0.0;  //Visible time step
static double TemporaryTimestep = 0.0; //Temp timestep for sim-level proc ctrl
static double secondaryvisible = -1.0;  //Seconary visible timestep
static double EventMinTimeStep = -1.0;  //Event driven minimum timestep
static bool checkexpansions = false;
static bool checkpostfrontal = false;
static long DistanceMethod = 1;  //Method for distance checking
static bool AccelerationState = true;  //Flag for using acceleration constants

static long NumPerimAlloc = 0;  //Current # of perimeter arrays alloc w/ new
static long* inout = 0;    //Fire doesn't exist (0), burning out(2), in(2)
static long* numpts = 0;  //Number of points in each fire
static double** perimeter1 = 0;  //Pointers to arrays with perimeter points

static size_t nmemb;
static double PercentageOfEmberIgnitions = .10;  // % embers that start fires
static double SpotIgnitionDelay = 0.0;  //Delay (min) for ignit of spot fires
static bool CrowningOK = true;    //Enable crowning
static bool SpottingOK = true;  //Enable spotting (just ember gen & flight)
static bool SpotGrowthOK = false;    //Enable spot fire growth
static bool ConstBack = false; //Use const backing spread rate (no wind/slope)
static long CrownFireCalculation=0;  //0=Finney(1998), 1=Scott&Reinhardt(2001)

static long newfires = 0;  //Number of new fires
static long numspots = 0;  //Number of spot fires
static long skipfire = 0;  //Number of extinguished fires
static long p2numalloc = 0;  //Allocated space in perimeter2 array
static long* GroundElev = 0;  //Stores elevation of points
static long numelev = 0;

static long NumStopLocations = 0;
static double StopLocation[MAXNUM_STOPLOCATIONS*2];
static bool StopEnabled[MAXNUM_STOPLOCATIONS];

//============================================================================
bool AccelerationON() { return AccelerationState; }

//============================================================================
void SetAccelerationON( bool State ) { AccelerationState = State; }

//============================================================================
double GetRosRed( int fuel )
{ //GetRosRed
  if( RosRed[fuel] > 0.0 ) return RosRed[fuel];
  else return 1.0;
} //GetRosRed

//============================================================================
void SetRosRed( int fuel, double r ) { RosRed[fuel] = fabs(r); }

//============================================================================
long GetInout( long FireNumber ) { return inout[FireNumber]; }

//============================================================================
void SetInout( long FireNumber, int Inout ) { inout[FireNumber] = Inout; }

//============================================================================
long GetNumPoints( long FireNumber ) { return numpts[FireNumber]; }

//============================================================================
void SetNumPoints( long FireNumber, long NumPoints )
{ //SetNumPoints
  CallLevel++;

  if( Verbose >= CallLevel )
    printf( "%*sfsxw:SetNumPoints:1 %ld %ld\n",
            CallLevel, "", FireNumber, NumPoints );

  numpts[FireNumber] = NumPoints;

  CallLevel--;
} //SetNumPoints

//============================================================================
double PercentIgnition( double percent )
{ //PercentIgnition
  if( percent >= 0.01 && percent <= 100.0 )
    PercentageOfEmberIgnitions = percent;
  else if( percent >= 0.0 && percent < 0.01 )
    PercentageOfEmberIgnitions = 0.01;
  else if( percent > 100.0 )
    PercentageOfEmberIgnitions = 100.0;

  return PercentageOfEmberIgnitions;
} //PercentIgnition

//============================================================================
double IgnitionDelay( double delay )
{ //IgnitionDelay
  if( delay >= 0.0 ) SpotIgnitionDelay = delay;

  return SpotIgnitionDelay;
} //IgnitionDelay

//============================================================================
bool EnableCrowning( long Crowning )
{ //EnableCrowning
  if( Crowning >= 0 ) CrowningOK = Crowning ? true : false;

  return CrowningOK;
} //EnableCrowning

//============================================================================
long GetCrownFireCalculation() { return CrownFireCalculation; }

//============================================================================
long SetCrownFireCalculation( long Type )
{ //SetCrownFireCalculation
  CrownFireCalculation=Type;

  return CrownFireCalculation;
} //SetCrownFireCalculation

//============================================================================
bool EnableSpotting( long Spotting )
{ //EnableSpotting
  if( Spotting >= 0 ) SpottingOK = Spotting ? true : false;

  return SpottingOK;
} //EnableSpotting

//============================================================================
bool EnableSpotFireGrowth( long Growth )
{ //EnableSpotFireGrowth
  if( Growth >= 0 ) SpotGrowthOK = Growth ? true : false;

  return SpotGrowthOK;
} //EnableSpotFireGrowth

//============================================================================
bool ConstantBackingSpreadRate( long Back )
{ //ConstantBackingSpreadRate
  if( Back >= 0 ) ConstBack = Back ? true : false;

  return ConstBack;
} //ConstantBackingSpreadRate

//============================================================================
long GetNumFires() { return numfires; }

//============================================================================
void SetNumFires( long input ) { numfires = input; }

//============================================================================
void IncNumFires( long MoreFires ) { numfires += MoreFires; }

//============================================================================
long GetNewFires() { return newfires; }

//============================================================================
void SetNewFires( long input )
{ //SetNewFires
  newfires = input;
  numspots = input;
} //SetNewFires

//============================================================================
void IncNewFires( long increment )
{ //IncNewFires
  newfires += increment;
  numspots = newfires;
} //IncNewFires

//============================================================================
void GetNumSpots( long* num, bool inc )
{ //GetNumSpots
  *num = numspots;
  if( inc ) {
    numspots++;
    newfires = numspots;
  }
} //GetNumSpots

//============================================================================
void SetNumSpots( long input ) { numspots = input; }

//============================================================================
long GetSkipFires() { return skipfire; }

//============================================================================
void SetSkipFires( long newvalue ) { skipfire = newvalue; }

//============================================================================
void IncSkipFires( long increment ) { skipfire += increment; }

//============================================================================
double GetPerimRes() { return PerimeterResolution; }

//============================================================================
void SetPerimRes( double input ) { PerimeterResolution = input; }

//============================================================================
double GetDynamicDistRes() { return DynamicDistanceResolution; }

//============================================================================
void SetDynamicDistRes( double input ) { DynamicDistanceResolution = input; }

//============================================================================
double GetDistRes() { return DistanceResolution; }

/*============================================================================
  SetDistRes
  Sets both default and dynamic min dist.
*/
void SetDistRes( double input )
{ //SetDistRes
  DistanceResolution = DynamicDistanceResolution = input;
} //SetDistRes

//============================================================================
double GetTemporaryTimeStep() { return TemporaryTimestep; }

//============================================================================
void SetTemporaryTimeStep( double value ) { TemporaryTimestep = value; }

//============================================================================
double GetActualTimeStep() { return actual; }

//============================================================================
void SetActualTimeStep( double input ) { actual = input; }

//============================================================================
double GetVisibleTimeStep() { return visible; }

//============================================================================
void SetVisibleTimeStep( double input )
{ //SetVisibleTimeStep
  long nuvis = (long)( input / actual );
  if( nuvis < 1 ) nuvis = 1;
  visible = nuvis * actual;
} //SetVisibleTimeStep

//============================================================================
void SetSecondaryVisibleTimeStep( double input ) { secondaryvisible = input; }

//============================================================================
double GetSecondaryVisibleTimeStep() { return secondaryvisible; }

/*============================================================================
  EventMinimumTimeStep
  Returns EventMinTimeStep if "time" is negative.
*/
double EventMinimumTimeStep( double time )
{ //EventMinimumTimeStep
  if( time >= 0 ) EventMinTimeStep = time;

  return EventMinTimeStep;
} //EventMinimumTimeStep

//============================================================================
bool CheckExpansions( long YesNo )
{ //CheckExpansions
  if( YesNo > -1 ) checkexpansions = YesNo ? true : false;

  return checkexpansions;
} //CheckExpansions

//============================================================================
bool CheckPostFrontal( long YesNo )
{ //CheckPostFrontal
  if( YesNo > -1 ) checkpostfrontal = YesNo ? true : false;

  return checkpostfrontal;
} //CheckPostFrontal

//============================================================================
long DistanceCheckMethod( long Method )
{ //DistanceCheckMethod
  if( Method >= 0 ) DistanceMethod = Method;

  return DistanceMethod;
} //DistanceCheckMethod

//----------------------------------------------------------------------------
//Fire perimeter2, swap space for fire growth calculations.
//----------------------------------------------------------------------------

//============================================================================
void GetPerimeter2( long coord, double* xpt, double* ypt, double* ros,
                    double* fli, double* rct )
{ //GetPerimeter2
  if( coord < p2numalloc ) {
    coord *= NUMDATA;
    *xpt = perimeter2[coord];
    *ypt = perimeter2[++coord];
    *ros = perimeter2[++coord];
    *fli = perimeter2[++coord];
    *rct = perimeter2[++coord];
  }
} //GetPerimeter2

//============================================================================
double GetPerimeter2Value( long coord, long value )
{ //GetPerimeter2Value
  if( coord < 0 || value < 0 ) return (double) p2numalloc;
  else if( perimeter2 && coord < p2numalloc )
    return perimeter2[coord * NUMDATA + value];

  return 0.0;
} //GetPerimeter2Value

//============================================================================
void SetPerimeter2( long coord, double xpt, double ypt, double ros,
                    double fli, double rct )
{ //SetPerimeter2
  if( coord < p2numalloc ) {
    coord *= NUMDATA;
    perimeter2[coord] = xpt;
    perimeter2[++coord] = ypt;
    perimeter2[++coord] = ros;
    perimeter2[++coord] = fli;
    perimeter2[++coord] = rct;
  }
} //SetPerimeter2

//============================================================================
void SetPerimeter2( long coord, PerimeterPoint &Pt )
{ //SetPerimeter2
  if( coord < p2numalloc ) Pt.Get( &perimeter2[coord*NUMDATA] );
} //SetPerimeter2

//============================================================================
double* AllocPerimeter2( long NumPoints )
{ //AllocPerimeter2
  if( NumPoints ) {
    if( NumPoints >= p2numalloc ) {
      FreePerimeter2();
      nmemb = NumPoints * NUMDATA;
      perimeter2 = new double[nmemb];
      if( perimeter2 != NULL ) p2numalloc = NumPoints;
    }
  }
  else return NULL;

  return perimeter2;
} //AllocPerimeter2

//============================================================================
void FreePerimeter2()
{ //FreePerimeter2
  if( perimeter2 ) delete[] perimeter2;
  perimeter2 = 0;
  p2numalloc = 0;
} //FreePerimeter2

//----------------------------------------------------------------------------
//Must Call at begining of the program
//----------------------------------------------------------------------------

//============================================================================
void FreeAllFirePerims()
{ //FreeAllFirePerims
  if( perimeter1 ) delete[] perimeter1;
  if( numpts ) delete[] numpts;
  if( inout ) delete[] inout;
  perimeter1 = 0;
  numpts = 0;
  inout = 0;
  NumPerimAlloc = 0;
} //FreeAllFirePerims

//============================================================================
long GetNumPerimAlloc() { return NumPerimAlloc; }

//============================================================================
bool AllocFirePerims( long num )
{ //AllocFirePerims
  FreeAllFirePerims();
  perimeter1 = new double * [num];
  if( perimeter1 == NULL ) return false;
  numpts = new long[num];
  if( numpts == NULL ) return false;
  inout = new long[num];
  if( inout == NULL ) return false;
  NumPerimAlloc = num;
  memset( perimeter1, 0x0, num * sizeof(double *) );
  memset( numpts, 0x0, num * sizeof(long) );
  memset( inout, 0x0, num * sizeof(long) );

  return true;
} //AllocFirePerims

//============================================================================
bool ReAllocFirePerims()
{ //ReAllocFirePerims
  long     i, OldNumAlloc;
  long*    newinout, * newnumpts;
  double*  temp1;
  double** newperim1;

  newperim1 = perimeter1;
  newinout = inout;
  newnumpts = numpts;

  perimeter1 = 0;
  inout = 0;
  numpts = 0;
  OldNumAlloc = NumPerimAlloc;

  if( ! AllocFirePerims(NumPerimAlloc + SIZE_FIREPERIM_BLOCK) ) return false;

  if( newinout ) {
    memcpy( inout, newinout, OldNumAlloc * sizeof(long) );
    delete[] newinout;
  }
  if( newnumpts ) {
    memcpy( numpts, newnumpts, OldNumAlloc * sizeof(long) );
    delete[] newnumpts;
  }

  if( newperim1 ) {
    for( i = 0; i < OldNumAlloc; i++ ) {
      temp1 = perimeter1[i];
      perimeter1[i] = newperim1[i];
      if( numpts[i] > 0 ) delete[] temp1;
    }
    delete[] newperim1;
  }

  return true;
} //ReAllocFirePerims

//----------------------------------------------------------------------------
//Fire Perimeter1, main perimeter storage and retrieval functions.
//----------------------------------------------------------------------------

//============================================================================
double* AllocPerimeter1( long NumFire, long NumPoints )
{ //AllocPerimeter1
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsxw:AllocPerimeter1:1\n", CallLevel, "" );

  if( NumPoints ) {
    if( NumFire >= NumPerimAlloc ) {
      if( ReAllocFirePerims() == false ) {
        if( Verbose > CallLevel )
          printf( "%*sfsxw:AllocPerimeter1:1a\n", CallLevel, "" );
        CallLevel--;
        return NULL;
      }
    }
    nmemb = (NumPoints) * NUMDATA; //Add 1 to make room for bounding rectangle
    if( perimeter1[NumFire] > 0 ) FreePerimeter1( NumFire );
    perimeter1[NumFire] = new double[nmemb];

    if( perimeter1[NumFire] == NULL ) {
      NumFire = -1;    //Debugging
      perimeter1[NumFire] = 0;

        if( Verbose > CallLevel )
          printf( "%*sfsxw:AllocPerimeter1:1b\n", CallLevel, "" );
      CallLevel--;
      return NULL;
    }
  }

  if( Verbose > CallLevel )
    printf( "%*sfsxw:AllocPerimeter1:2\n", CallLevel, "" );
  CallLevel--;

  return perimeter1[NumFire];
} //AllocPerimeter1

//============================================================================
void FreePerimeter1( long NumFire )
{ //FreePerimeter1
  if( perimeter1[NumFire] ) {
    delete[] perimeter1[NumFire];
    perimeter1[NumFire] = 0;
  }
} //FreePerimeter1

//============================================================================
double GetPerimeter1Value( long NumFire, long NumPoint, int coord )
{ //GetPerimeter1Value
  if( perimeter1[NumFire] )
    return perimeter1[NumFire][NumPoint * NUMDATA + coord];

  return 0.0;
} //GetPerimeter1Value

//============================================================================
void GetPerimeter1Point( long NumFire, long NumPoint, PerimeterPoint *Pt )
{ //GetPerimeter1Point
  long index = NumPoint * NUMDATA;
  Pt->x = perimeter1[NumFire][index];
  Pt->y = perimeter1[NumFire][++index];
  Pt->ROS = perimeter1[NumFire][++index];
  Pt->FLI = perimeter1[NumFire][++index];
  Pt->RCX = perimeter1[NumFire][++index];
} //GetPerimeter1Point
    
//============================================================================
double* GetPerimeter1Address( long NumFire, long NumPoint )
{ return &perimeter1[NumFire][NumPoint * NUMDATA]; }

//============================================================================
void SetPerimeter1( long NumFire, long NumPoint, double xpt, double ypt )
{ //SetPerimeter1
  NumPoint *= NUMDATA;
  perimeter1[NumFire][NumPoint] = xpt;
  perimeter1[NumFire][NumPoint + 1] = ypt;
} //SetPerimeter1

//============================================================================
void SetFireChx( long NumFire, long NumPoint, double ros, double fli )
{ //SetFireChx
  NumPoint *= NUMDATA;
  perimeter1[NumFire][NumPoint + 2] = ros;
  perimeter1[NumFire][NumPoint + 3] = fli;
} //SetFireChx

//============================================================================
void SetReact( long NumFire, long NumPoint, double react )
{ perimeter1[NumFire][NumPoint * NUMDATA + 4] = react; }

//============================================================================
long SwapFirePerims( long NumFire1, long NumFire2 )
{ //SwapFirePerims
  double* TempFire;
  long    TempInout, TempNum;

  if( NumFire1 >= 0 && NumFire2 >= 0 ) {    //Two fires in perim1
    TempFire = perimeter1[NumFire2];
    perimeter1[NumFire2] = perimeter1[NumFire1];
    perimeter1[NumFire1] = TempFire;
    TempInout = inout[NumFire2];
    inout[NumFire2] = inout[NumFire1];
    inout[NumFire1] = TempInout;
    TempNum = numpts[NumFire2];
    numpts[NumFire2] = numpts[NumFire1];
    numpts[NumFire1] = TempNum;

    return 1;
  }
  else if( NumFire1 < 0 && NumFire2 >= 0 ) {
    AllocPerimeter2( numpts[NumFire2] );
    if( memmove(perimeter2, perimeter1[NumFire2],
        numpts[NumFire2] * NUMDATA * sizeof(double)) )
      return 1;
    else return 0;
  }
  else if( NumFire1 >= 0 && NumFire2 < 0 ) {
    if( perimeter1[NumFire1] ) {
      if( memmove(perimeter1[NumFire1], perimeter2,
          (NumFire2 * -1) * NUMDATA * sizeof(double)) )
        return 1;
    }
    return 0;
  }

  return 0;
} //SwapFirePerims

//============================================================================
void AllocElev( long CurrentFire )
{ //AllocElev
  nmemb = numpts[CurrentFire];
  if( nmemb >= (unsigned long) numelev ) {
    FreeElev();
    GroundElev = new long[nmemb];
    numelev = nmemb;
  }
} //AllocElev

//============================================================================
void SetElev( long Num, long elev )
{ //SetElev
  if( Num >= numelev ) {
    long* tempelev;

    tempelev = new long[numelev];
    memcpy( tempelev, GroundElev, numelev * sizeof(long) );
    delete[] GroundElev;
    GroundElev = new long[numelev * 2];
    memcpy( GroundElev, tempelev, numelev * sizeof(long) );
    numelev *= 2;
    delete[] tempelev;
  }
  GroundElev[Num] = elev;
} //SetElev

//============================================================================
long GetElev( long Num )
{ //GetElev
  if( ! GroundElev ) return (long) NULL;

  return GroundElev[Num];
} //GetElev

//============================================================================
void FreeElev()
{ //FreeElev
  if( GroundElev ) delete[] GroundElev;
  GroundElev = 0;
  numelev = 0;
} //FreeElev

//----------------------------------------------------------------------------
//Stop Location Functions
//----------------------------------------------------------------------------

//============================================================================
long SetStopLocation( double xcoord, double ycoord )
{ //SetStopLocation
  if( NumStopLocations < MAXNUM_STOPLOCATIONS ) {
    StopLocation[NumStopLocations * 2] = xcoord;
    StopLocation[NumStopLocations * 2 + 1] = ycoord;
    StopEnabled[NumStopLocations] = true;
  }
  else return 0;

  return ++NumStopLocations;
} //SetStopLocation

//============================================================================
bool GetStopLocation( long StopNum, double* xcoord, double* ycoord )
{ //GetStopLocation
  if( StopNum < MAXNUM_STOPLOCATIONS ) {
    *xcoord = StopLocation[StopNum * 2];
    *ycoord = StopLocation[StopNum * 2 + 1];
  }
  if( StopEnabled[StopNum] ) return true;

  return false;
} //GetStopLocation

//============================================================================
bool EnableStopLocation( long StopNum, long Action )
{ //EnableStopLocation
  if( Action >= 0 && StopNum < NumStopLocations )
    StopEnabled[StopNum] = Action ? true : false;

  return (bool) StopEnabled[StopNum];
} //EnableStopLocation

//============================================================================
long GetNumStopLocations() { return NumStopLocations; }