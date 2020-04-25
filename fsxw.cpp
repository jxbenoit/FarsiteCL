/*============================================================================
  fsxw.cpp
  See LICENSE.TXT file for license information.
*/
#include<memory.h>
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<iostream>
#include"globals.h"
#include"fsxw.h"
#include"Perimeter.h"

#define SIZE_FIREPERIM_BLOCK 10000

static Perimeter **Perimeters; //Main perimeter storage
static long NumPerimeters = 0;  //Current # of perimeters
Perimeter *Perimeter2 = NULL;  //Perimeter2 is swap space for growth calcs
double RosRed[257];  //Rate of spread reduction factors
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

static long* inout = 0;    //Fire doesn't exist (0), burning out(2), in(2)
static long* numpts = 0;  //Number of points in each fire

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
long GetNumPoints( long FireNumber ) {
  CallLevel++;

  if( Verbose >= CallLevel )
    printf( "%*sfsxw:GetNumPoints:1 numpts[%ld]=%ld\n",
            CallLevel, "", FireNumber, numpts[FireNumber] );
  CallLevel--;

  return numpts[FireNumber];
}

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
long GetNewFires() {
std::cerr<<"AAA fsxw:GetNewFires:1\n"; //AAA
  return newfires;
}

//============================================================================
void SetNewFires( long input )
{ //SetNewFires
std::cerr<<"AAA fsxw:SetNewFires:1\n"; //AAA
  newfires = input;
  numspots = input;
} //SetNewFires

//============================================================================
void IncNewFires( long increment )
{ //IncNewFires
std::cerr<<"AAA fsxw:IncNewFires:1\n"; //AAA
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

//============================================================================
void GetPerimeter2( long coord, double* xpt, double* ypt, double* ros,
                    double* fli, double* rct )
{ //GetPerimeter2
  if( coord < Perimeter2->GetMaxSize() ) {
    PerimeterPoint *pt = Perimeter2->GetPoint( coord );
    *xpt = pt->Get( PerimeterPoint::X_VAL );
    *ypt = pt->Get( PerimeterPoint::Y_VAL );
    *ros = pt->Get( PerimeterPoint::ROS_VAL );
    *fli = pt->Get( PerimeterPoint::FLI_VAL );
    *rct = pt->Get( PerimeterPoint::RCX_VAL );
  }
} //GetPerimeter2

//============================================================================
double GetPerimeter2Value( long coord, long value )
{ //GetPerimeter2Value
  if( coord < 0 || value < 0 ) return (double) ( Perimeter2->GetMaxSize() );
  else if( Perimeter2 != NULL && coord < Perimeter2->GetMaxSize() ) {
    double val_type;
    switch ( value ) {
      case XCOORD: val_type = PerimeterPoint::X_VAL;  break;
      case YCOORD: val_type = PerimeterPoint::Y_VAL;  break;
      case ROSVAL: val_type = PerimeterPoint::ROS_VAL;  break;
      case FLIVAL: val_type = PerimeterPoint::FLI_VAL;  break;
      case RCXVAL: val_type = PerimeterPoint::RCX_VAL;  break;
    }
    return Perimeter2->GetValue( coord, val_type );
  }

  return 0.0;
} //GetPerimeter2Value

//============================================================================
void SetPerimeter2( long coord, double xpt, double ypt, double ros,
                    double fli, double rct )
{ //SetPerimeter2
  if( coord < Perimeter2->GetMaxSize() ) {
    Perimeter2->SetPointLoc( coord, xpt, ypt );
    Perimeter2->SetPointCharacteristics( coord, ros, fli );
    Perimeter2->SetPointReact( coord, rct );
  }
} //SetPerimeter2

//============================================================================
void SetPerimeter2( long coord, PerimeterPoint &Pt )
{ //SetPerimeter2
  if( coord < Perimeter2->GetMaxSize() ) {
    Perimeter2->SetPointLoc( coord,
                             Pt.Get(PerimeterPoint::X_VAL),
                             Pt.Get(PerimeterPoint::Y_VAL) );
    Perimeter2->SetPointCharacteristics( coord,
                             Pt.Get(PerimeterPoint::ROS_VAL),
                             Pt.Get(PerimeterPoint::FLI_VAL) );
    Perimeter2->SetPointReact( coord, Pt.Get(PerimeterPoint::RCX_VAL) );
  }
} //SetPerimeter2

//============================================================================
void AllocPerimeter2( long NumPoints )
{ //AllocPerimeter2
  if( NumPoints ) {
    if( ! Perimeter2 || NumPoints >= Perimeter2->GetMaxSize() ) {
      FreePerimeter2();

      Perimeter2 = new Perimeter( NumPoints );
    }
  }
} //AllocPerimeter2

//============================================================================
void FreePerimeter2()
{ //FreePerimeter2
  if( Perimeter2 != NULL ) {
    delete Perimeter2;
    Perimeter2 = NULL;
  }
} //FreePerimeter2

//============================================================================
void FreeAllFirePerims()
{ //FreeAllFirePerims
std::cerr << "AAA fsxw:FreeAllFirePerims:1\n"; //AAA
  if( Perimeters )  delete [] Perimeters;
  NumPerimeters = 0;

  if( numpts ) delete [] numpts;
  if( inout ) delete [] inout;
  numpts = 0;
  inout = 0;
std::cerr << "AAA fsxw:FreeAllFirePerims:2\n"; //AAA
} //FreeAllFirePerims

//============================================================================
bool AllocFirePerims( long Num )
{ //AllocFirePerims
std::cerr << "AAA fsxw:AllocFirePerims:1\n"; //AAA
  FreeAllFirePerims();
  Perimeters = new Perimeter * [Num];
std::cerr << "AAA fsxw:AllocFirePerims:2\n"; //AAA
  if( Perimeters == NULL ) return false;
  for( int i = 0; i < Num; i++ )
    Perimeters[i] = NULL;
  NumPerimeters = Num;

  numpts = new long[Num];
std::cerr << "AAA fsxw:AllocFirePerims:3\n"; //AAA
  if( numpts == NULL ) return false;
  inout = new long[Num];
std::cerr << "AAA fsxw:AllocFirePerims:4\n"; //AAA
  if( inout == NULL ) return false;
  memset( numpts, 0x0, Num * sizeof(long) );
  memset( inout, 0x0, Num * sizeof(long) );
std::cerr << "AAA fsxw:AllocFirePerims:5\n"; //AAA

  return true;
} //AllocFirePerims

//============================================================================
bool ReAllocFirePerims()
{ //ReAllocFirePerims
std::cerr << "AAA fsxw:ReAllocFirePerims:1\n"; //AAA
  long     i, OldNumAlloc;
  long*    newinout, * newnumpts;
  double*  temp1;
  double** newperim1;
  Perimeter **pPerimeters;  //ptr to original Perimeters array

  pPerimeters = Perimeters;
  newinout = inout;
  newnumpts = numpts;

  Perimeters = 0;
  inout = 0;
  numpts = 0;
  OldNumAlloc = NumPerimeters;

std::cerr << "AAA fsxw:ReAllocFirePerims:2\n"; //AAA
  if( ! AllocFirePerims(NumPerimeters + SIZE_FIREPERIM_BLOCK) ) return false;

  if( newinout ) {
    memcpy( inout, newinout, OldNumAlloc * sizeof(long) );
    delete [] newinout;
  }
  if( newnumpts ) {
    memcpy( numpts, newnumpts, OldNumAlloc * sizeof(long) );
    delete [] newnumpts;
  }

  if( pPerimeters ) {
    for( i = 0; i < OldNumAlloc; i++ ) {
      Perimeters[i] = pPerimeters[i];
    }
  }

std::cerr << "AAA fsxw:ReAllocFirePerims:3\n"; //AAA
  return true;
} //ReAllocFirePerims

//============================================================================
void AllocPerimeter1( long FireIndex, long NumPoints )
{ //AllocPerimeter1
std::cerr << "AAA fsxw:AllocPerimeter1:1\n"; //AAA
  if( FireIndex >= 0 && NumPoints > 0 ) {
    if( FireIndex >= NumPerimeters ) {
      //Perimeters array not large enough - reallocate with FireIndex+1 elems.
      //if( ! AllocFirePerims(FireIndex+1) ) return;  //Return if unsuccessful
std::cerr << "AAA fsxw:AllocPerimeter1:2\n"; //AAA
      if( ReAllocFirePerims() == false )
        return;
    }

    if( Perimeters[FireIndex] != NULL ) {
      delete Perimeters[FireIndex];
      Perimeters[FireIndex] = NULL;
    }
    Perimeters[FireIndex] = new Perimeter( NumPoints );
  } 
std::cerr << "AAA fsxw:AllocPerimeter1:3\n"; //AAA
} //AllocPerimeter1

//============================================================================
double GetPerimeter1Value( long NumFire, long NumPoint, int coord )
{ //GetPerimeter1Value
  if( Perimeters[NumFire] ) {
    int val_type;
    switch ( coord ) {
      case XCOORD: val_type = PerimeterPoint::X_VAL;  break;
      case YCOORD: val_type = PerimeterPoint::Y_VAL;  break;
      case ROSVAL: val_type = PerimeterPoint::ROS_VAL;  break;
      case FLIVAL: val_type = PerimeterPoint::FLI_VAL;  break;
      case RCXVAL: val_type = PerimeterPoint::RCX_VAL;  break;
    }
    return Perimeters[NumFire]->GetValue( NumPoint, val_type );
  }

  return 0.0;
} //GetPerimeter1Value

//============================================================================
void GetPerimeter1Point( long NumFire, long NumPoint, PerimeterPoint *Pt )
{ //GetPerimeter1Point
  //Make a COPY of the point, NOT a pointer to it.
  PerimeterPoint p0( Perimeters[NumFire]->GetPoint(NumPoint) );
  Pt->SetLoc( p0.GetX(), p0.GetY() );
  Pt->SetCharacteristics( p0.Get(PerimeterPoint::ROS_VAL),
                          p0.Get(PerimeterPoint::FLI_VAL) );
  Pt->SetReact( p0.Get(PerimeterPoint::RCX_VAL) );
} //GetPerimeter1Point
    
//============================================================================
void DeletePoint( long NumFire, long NumPoint )
{ //DeletePoint
  Perimeters[NumFire]->DeletePoint( NumPoint );
} //DeletePoint

//============================================================================
void SetPerimeter1( long NumFire, long NumPoint, double xpt, double ypt )
{ //SetPerimeter1
  Perimeters[NumFire]->SetPointLoc( NumPoint, xpt, ypt );
} //SetPerimeter1

//============================================================================
void SetFireChx( long NumFire, long NumPoint, double ros, double fli )
{ //SetFireChx
  Perimeters[NumFire]->SetPointCharacteristics( NumPoint, ros, fli );
} //SetFireChx

//============================================================================
void SetReact( long NumFire, long NumPoint, double react )
{ //SetReact
  Perimeters[NumFire]->SetPointReact( NumPoint, react );
} //SetReact

//============================================================================
long SwapFirePerims( long NumFire1, long NumFire2 )
{ //SwapFirePerims
std::cerr << "AAA fsxw:SwapFirePerims:1\n"; //AAA
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsxw:SwapFirePerims:1\n",
            CallLevel, "" );

  long    TempInout, TempNum;

  if( NumFire1 >= 0 && NumFire2 >= 0 ) {    //Two fires in perim1
    Perimeter *ptmp = Perimeters[NumFire2];
    Perimeters[NumFire2] = Perimeters[NumFire1];
    Perimeters[NumFire1] = ptmp;

    TempInout = inout[NumFire2];
    inout[NumFire2] = inout[NumFire1];
    inout[NumFire1] = TempInout;
    TempNum = numpts[NumFire2];
    numpts[NumFire2] = numpts[NumFire1];
    numpts[NumFire1] = TempNum;

    if( Verbose >= CallLevel )
      printf( "%*sfsxw:SwapFirePerims:1a\n",
              CallLevel, "" );
    CallLevel--;
std::cerr << "AAA fsxw:SwapFirePerims:1a\n"; //AAA

    return 1;
  }
  else if( NumFire1 < 0 && NumFire2 >= 0 ) {
    AllocPerimeter2( numpts[NumFire2] );
    Perimeter2->Set( Perimeters[NumFire2] );
  }
  else if( NumFire1 >= 0 && NumFire2 < 0 ) {
    if( Perimeters[NumFire1] ) {
      Perimeters[NumFire1]->Set( Perimeter2 );
      return 1; //I.e. returning 1 is 'good'
    }
  }
std::cerr << "AAA fsxw:SwapFirePerims:2\n"; //AAA
  if( Verbose >= CallLevel )
    printf( "%*sfsxw:SwapFirePerims:2\n",
            CallLevel, "" );
  CallLevel--;

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
    delete [] GroundElev;
    GroundElev = new long[numelev * 2];
    memcpy( GroundElev, tempelev, numelev * sizeof(long) );
    numelev *= 2;
    delete [] tempelev;
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
  if( GroundElev ) delete [] GroundElev;
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
