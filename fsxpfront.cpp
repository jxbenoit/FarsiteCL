/*fsxpfront.cpp

  Post frontal combustion calculations
  See LICENSE.TXT file for license information.
*/
#include<math.h>
#include<string.h>
#include"fsxpfront.h"
#include"fsglbvar.h"
#include"burnupw.h"

const double PI = acos(-1.0);
//extern const double PI;

//----------------------------------------------------------------------------
//  Global data for firering structures
//----------------------------------------------------------------------------
static long PRECISION = 12;		// number of sampling points
static long BURNUP_TIMESTEP = 15;	// seconds

static long NumRingStructs = 0;
static long NumRings = 0;
static RingStruct* FirstRing = 0;
static RingStruct* NextRing = 0;
static RingStruct* CurRing = 0;

//----------------------------------------------------------------------------
//  Global functions for allocating/deallocating/accessing firering structures
//----------------------------------------------------------------------------

FireRing* AllocFireRing( long NumPoints, double start, double end )
{ //AllocFireRing
  long curplace, i, ThisRing;
  double Ring;

  modf( ((double) NumRings / (double) RINGS_PER_STRUCT), &Ring );
  ThisRing = (long) Ring;
  curplace = NumRings - ThisRing * RINGS_PER_STRUCT;

  if( FirstRing == 0 ) {
    if( (FirstRing = new RingStruct) == NULL ) return 0;
      CurRing = FirstRing;
      CurRing->NumFireRings = 0;
      CurRing->StructNum = 0;
      NumRingStructs++;
      for( i = 0; i < RINGS_PER_STRUCT; i++ ) {
        CurRing->firering[i].perimpoints = 0;
        CurRing->firering[i].mergepoints = 0;
        CurRing->firering[i].NumPoints = 0;
      }
    }
    else if( ThisRing >= NumRingStructs ) {
      if( CurRing->StructNum != NumRingStructs - 1 )
        GetLastRingStruct();
      //NextRing = (RingStruct *) CurRing->next = new RingStruct;
      RingStruct* t_next;
      t_next = new RingStruct;
      CurRing->next=t_next;
      //NextRing=CurRing->next;
      NextRing = t_next;
      CurRing = NextRing;
      CurRing->NumFireRings = 0;
      CurRing->StructNum = NumRingStructs;
      NumRingStructs++;
      for( i = 0; i < RINGS_PER_STRUCT; i++ ) {
        CurRing->firering[i].perimpoints = 0;
        CurRing->firering[i].mergepoints = 0;
        CurRing->firering[i].NumPoints = 0;
      }
    }
    else if( ThisRing != CurRing->StructNum ) GetRing( NumRings );

    CurRing->firering[curplace].perimpoints = new PerimPoints[NumPoints];
    for( i = 0; i < NumPoints; i++ )
      memset( &CurRing->firering[curplace].perimpoints[i],0x0,
              sizeof(PerimPoints) );
    CurRing->firering[curplace].NumPoints = new long[2];
    CurRing->firering[curplace].NumPoints[0] = NumPoints;
    CurRing->firering[curplace].NumFires = 1;
    CurRing->firering[curplace].StartTime = start;
    CurRing->firering[curplace].ElapsedTime = end - start;
    CurRing->firering[curplace].mergepoints = 0;
    CurRing->firering[curplace].NumMergePoints[0] = 0;
    CurRing->firering[curplace].NumMergePoints[1] = 0;

    CurRing->NumFireRings++;
    NumRings++;

    return &CurRing->firering[curplace];
} //AllocFireRing


void GetLastRingStruct()
{ //GetLastRingStruct
  long i;

  CurRing = FirstRing;
  for( i = 0; i < NumRingStructs - 1; i++ ) {
    NextRing = (RingStruct *) CurRing->next;
    CurRing = NextRing;
  }
} //GetLastRingStruct


void FreeAllFireRings()
{ //FreeAllFireRings
  long i, j;

  CurRing = FirstRing;
  for( i = 0; i < NumRingStructs; i++ ) {
    if( CurRing != NULL ) {
      for( j = 0; j < RINGS_PER_STRUCT; j++ ) {
        if( CurRing->firering[j].perimpoints ) {
          delete[] CurRing->firering[j].perimpoints;
          if( CurRing->firering[j].NumPoints )
            delete[] CurRing->firering[j].NumPoints;
          if( CurRing->firering[j].mergepoints )
            delete[] CurRing->firering[j].mergepoints;
          CurRing->firering[j].perimpoints = 0;
          CurRing->firering[j].NumPoints = 0;
          CurRing->firering[j].mergepoints = 0;
        }
      }
    }

    NextRing = (RingStruct *) CurRing->next;
    delete CurRing;
    CurRing = NextRing;
  }

  NumRings = 0;
  NumRingStructs = 0;
  FirstRing = 0;
  NextRing = 0;
  CurRing = 0;
} //FreeAllFireRings


void FreeFireRing( long RingNum )
{ //FreeFireRing
  long i, curplace, ThisRing;
  double Ring;

  modf( ((double) RingNum / (double) RINGS_PER_STRUCT), &Ring );
  ThisRing = (long) Ring;
  curplace = RingNum - ThisRing * RINGS_PER_STRUCT;

  if( CurRing->StructNum != ThisRing ) {
    CurRing = FirstRing;
    for( i = 0; i < ThisRing; i++ ) {
      NextRing = (RingStruct *) CurRing->next;
      CurRing = NextRing;
    }
  }

  if( CurRing->firering[curplace].perimpoints ) {
    try {
      delete[] CurRing->firering[curplace].perimpoints;
      if( CurRing->firering[curplace].NumPoints )
        delete[] CurRing->firering[curplace].NumPoints;
      CurRing->firering[curplace].perimpoints = 0;
      CurRing->firering[curplace].NumPoints = 0;
      if( CurRing->firering[curplace].mergepoints )
        delete[] CurRing->firering[curplace].mergepoints;
      CurRing->firering[curplace].mergepoints = 0;
      CurRing->firering[curplace].OriginalFireNumber =
                      -( CurRing->firering[curplace].OriginalFireNumber + 1 );
    }
    catch (...) { }
  }
} //FreeFireRing


FireRing* GetRing( long RingNum )
{ //GetRing
  long i, curplace, ThisRing;
  double Ring;

  if (RingNum < 0) //original JAS!
  //if (RingNum <= 0)   //Modified JAS!
    return (FireRing *) NULL;

  modf( ((double) RingNum / (double) RINGS_PER_STRUCT), &Ring );
  ThisRing = (long) Ring;
  curplace = RingNum - ThisRing * RINGS_PER_STRUCT;//-1;
  CurRing->StructNum = 0;  //added JAS!
	
  if( CurRing->StructNum != ThisRing ) {
    CurRing = FirstRing;
    for( i = 0; i < ThisRing; i++ ) {
      NextRing = (RingStruct *) CurRing->next;
      CurRing = NextRing;
    }
  }
 

  if( CurRing == NULL ) return NULL;

  return &CurRing->firering[curplace];
} //GetRing


FireRing* GetSpecificRing( long FireNumber, double StartTime )
{ //GetSpecifiedRing
  long i, j;

  CurRing = FirstRing;
  j = 0;
  for( i = 0; i < NumRings; i++ ) {
    if( fabs(CurRing->firering[j].StartTime - StartTime) < 1e-9 ) {
      if( CurRing->firering[j].OriginalFireNumber == FireNumber ) break;
    }
    else continue;

    j++;
    if( j == RINGS_PER_STRUCT ) {
      NextRing = (RingStruct *) CurRing->next;
      CurRing = NextRing;
      j = 0;
    }
  }

  return &CurRing->firering[j];
} //GetSpecifiedRing

//============================================================================
void CondenseRings( long RingNum )
{ //CondenseRings
  //Searches starting with RingNum for FireRings with no points. Then it
  //shifts the pointers down to the earliest vacant slot. This function is
  //called after all mergers are completed.

  if( RingNum < 0 ) return;
  else if( RingNum == GetNumRings() - 1 ) return;

  FireRing* ring1, * ring2;
  long i, j;
  long NewRingNum;//=GetNumRings();
  long NewStructNum;

  for( i = RingNum; i < GetNumRings(); i++ ) {
    ring1 = GetRing( i );
    if( ! ring1 ) continue;
    if( ring1->mergepoints ) {
      delete[] ring1->mergepoints;
      ring1->mergepoints = 0;
      ring1->NumMergePoints[0] = 0;
    }
    if( ring1->perimpoints == NULL ) {
      FreeFireRing( i );
      for( j = i + 1; j < GetNumRings(); j++ ) {
        ring2 = GetRing( j );
        if( ! ring2 ) continue;
        if( ring2->perimpoints ) {
          ring1->perimpoints = ring2->perimpoints;
          ring1->NumPoints = ring2->NumPoints;
          ring2->perimpoints = 0;
          ring2->NumPoints = 0;
          ring1->NumFires = ring2->NumFires;
          ring1->OriginalFireNumber = ring2->OriginalFireNumber;
          ring1->StartTime = ring2->StartTime;
          ring2->OriginalFireNumber = (long)0;
          ring2->StartTime = 0.0;
          ring1->ElapsedTime = ring2->ElapsedTime;
          ring2->NumFires = 0;
          ring2->OriginalFireNumber = -1;

          break;
        }
      }
    }
  }

  //Free up CurRing if no points left!!!!!
  RingStruct* LastRing;
  CurRing = LastRing = FirstRing;
  NewStructNum = 0;
  for( i = 0; i < NumRingStructs; i++ ) {
    if( CurRing != NULL ) {
      NewRingNum = 0;
      for( j = 0; j < RINGS_PER_STRUCT; j++ ) {
        if( CurRing->firering[j].perimpoints != NULL ) NewRingNum++;
      }
      CurRing->NumFireRings = NewRingNum;
      if( NewRingNum > 0 ) NewStructNum++;
      else {
        NextRing = (RingStruct *) CurRing->next;
        LastRing->next = NextRing;
        if( CurRing == LastRing ) { //Will only occur if no rings now
          delete FirstRing;
          FirstRing = LastRing = 0;
        }
        else delete CurRing;

        CurRing = LastRing;
      }
    }

    if( CurRing == NULL ) break;

    LastRing = CurRing;
    NextRing = (RingStruct *) CurRing->next;
    CurRing = NextRing;
  }
  CurRing = FirstRing;
  NewRingNum = 0;
  for( i = 0; i < NewStructNum; i++ ) {
    NewRingNum += CurRing->NumFireRings;
    NextRing = (RingStruct *) CurRing->next;
    CurRing = NextRing;
  }
  SetNumRings( NewRingNum );//(NewStructNum-1)*RINGS_PER_STRUCT+NewRingNum);
  NumRingStructs = NewStructNum;
  CurRing = FirstRing;
} //CondenseRings

//============================================================================
void SetNewFireNumber( long OldNum, long NewNum, long RefNum )
{ //SetNewFireNumber
  bool found = false;
  long i;
  FireRing* ring;

  for( i = RefNum; i < GetNumRings(); i++ ) {
    ring = GetRing( i );
    if( OldNum == ring->OriginalFireNumber ) {
      ring->OriginalFireNumber = NewNum;
      found = true;
      break;
    }
  }
  if( ! found ) found = true; // for debugging
} //SetNewFireNumber

//============================================================================
void SetNumRings( long NewNumRings ) { NumRings = NewNumRings; }

//============================================================================
long GetNumRings() { return NumRings; }

//============================================================================
long GetNumRingStructs() { return NumRingStructs; }

//============================================================================
bool AddToCurrentFireRing( FireRing* firering, long PointNum,
                           long SurfFuelType, long WoodyModel,
                           double DuffLoad, double* mx,
                           double CrownLoadingBurned )
{ //AddToCurrentFireRing
  if( ! firering ) return false;

  long i, NumFire;
  double x, y, r, f, c;

  NumFire = firering->OriginalFireNumber;
  f = GetPerimeter2Value( PointNum, FLIVAL );
  if( f < 0.0 ) {
    x = GetPerimeter2Value( PointNum, XCOORD );
    y = GetPerimeter2Value( PointNum, YCOORD );
    r = fabs( GetPerimeter2Value(PointNum, ROSVAL) );
    f = fabs( f );
    c = GetPerimeter2Value( PointNum, RCXVAL );
  }
  else {
    x = GetPerimeter1Value( NumFire, PointNum, XCOORD );
    y = GetPerimeter1Value( NumFire, PointNum, YCOORD );
    r = fabs( GetPerimeter1Value(NumFire, PointNum, ROSVAL) );
    f = fabs( GetPerimeter1Value(NumFire, PointNum, FLIVAL) );
    c = GetPerimeter1Value( NumFire, PointNum, RCXVAL );
  }
  firering->perimpoints[PointNum].x2 = x;
  firering->perimpoints[PointNum].y2 = y;
  if( c < 1e-2 ) c = 0.0;
  firering->perimpoints[PointNum].hist.ReactionIntensity[1] = (float) c;
  if( fabs(r) > 0.0 && c > 0.0 )
    firering->perimpoints[PointNum].hist.FlameResidenceTime[1] =
                                    (float) ((f * 60.0) / (r * c)); // seconds
  else firering->perimpoints[PointNum].hist.FlameResidenceTime[1] = 0.0;

  firering->perimpoints[PointNum].hist.WoodyFuelType =
                                                   (unsigned char) WoodyModel;
  firering->perimpoints[PointNum].hist.SurfaceFuelModel =
                                                 (unsigned char) SurfFuelType;
  if( DuffLoad < 0.1 ) DuffLoad = 0.1;
  firering->perimpoints[PointNum].hist.DuffLoad = (short) (DuffLoad * 10.0);
  //mx=GetAllCurrentMoistures(&NumMx);

  if( mx ) {
    for( i = 0; i < MAXNO; i++ )
      firering->perimpoints[PointNum].hist.Moistures[i] =
                                              (unsigned char) (mx[i] * 100.0);
  }
  
  //for(i=NumMx-1; i<20; i++)
  //  firering->perimpoints[PointNum].hist.Moistures[i]=0;
  //Initialize last values of integration.
  firering->perimpoints[PointNum].hist.LastWtRemoved = 0.0;
  firering->perimpoints[PointNum].hist.WeightPolyNum = 0;
  firering->perimpoints[PointNum].hist.FlamePolyNum = 0;
  firering->perimpoints[PointNum].hist.CrownLoadingBurned =
                                                   (float) CrownLoadingBurned;

  return true;
} //AddToCurrentFireRing


//============================================================================
RingBurn::RingBurn()
{ //RingBurn::RingBurn
  burnup = 0;
  hThread = 0;
  burnup = 0;
  ring = 0;
  ThreadStarted = false;
} //RingBurn::RingBurn

//============================================================================
RingBurn::~RingBurn()
{ //RingBurn::~RingBurn
  if( burnup ) delete burnup;
} //RingBurn::~RingBurn

//============================================================================
void RingBurn::SetRange( FireRing* rring, long firenum, long begin, long end )
{ //RingBurn::SetRange
  ring = rring;
  FireNum = firenum;
  Begin = begin;
  End = end;
  CurOldRing = FirstRing;
} //RingBurn::SetRange

long RingBurn::StartBurnThread( long ID )
{ //RingBurn::StartBurnThread
  burnup = new BurnUp();  //AAA In FromG5 ver

  RunBurnThread( this );

  return true;
} //RingBurn::StartBurnThread

unsigned RingBurn::RunBurnThread( void* ringburn )
{ //RingBurn::RunBurnThread
  static_cast <RingBurn*>(ringburn)->BurnThread();

  return 1;
} //RingBurn::RunBurnThread

//============================================================================
void RingBurn::BurnThread()
{ //RingBurn::BurnThread
  long   k, m, num, ma, lastpt, nextpt;
  double depth, total, DuffMx;
  bool   SimCont;
  double coefx, coefy;
  WoodyData wd[20];

  ThreadStarted = true;
  do {
    if( End < 0 ) break;
    for( k = Begin; k < End; k++ ) {
      GetWoodyData( ring->perimpoints[k].hist.WoodyFuelType,
                    ring->perimpoints[k].hist.SurfaceFuelModel, &num, wd,
                    &depth, &total );
      for( m = 0; m < num; m++ )
        wd[m].FuelMoisture =
                    ((double) ring->perimpoints[k].hist.Moistures[m]) / 100.0;
        // this is from Harrington
        DuffMx = -0.347 + 6.42 * wd[2].FuelMoisture;
        if( DuffMx < 0.10 ) DuffMx = 0.10;
        burnup->SetFuelInfo( num, &(wd[0].SurfaceAreaToVolume) );
        burnup->SetFireDat( 5000,
                            ring->perimpoints[k].hist.ReactionIntensity[1],
                            ring->perimpoints[k].hist.FlameResidenceTime[1],
                            0.0, depth, 27.0, 1.8, 0.4,
                            (double) BURNUP_TIMESTEP,
                          (double) ring->perimpoints[k].hist.DuffLoad / 100.0,
                            DuffMx);
                            //ring->perimpoints[k].hist.Moistures[num-1]);
                            // use last value of fms for duff
        lastpt = k - 1;
        if( FireNum == 0 ) {
          if( lastpt < 0 ) lastpt = ring->NumPoints[FireNum] - 1;
        }
        else if( lastpt < ring->NumPoints[FireNum - 1] )
          lastpt = ring->NumPoints[FireNum] - 1;

        nextpt = k + 1;
        if( nextpt > ring->NumPoints[FireNum] - 1 ) {
          if( FireNum == 0 ) nextpt = 0;
          else nextpt = ring->NumPoints[FireNum - 1];
        }

        CalculateArea( ring, k, lastpt, nextpt );

        if( ring->perimpoints[k].Area == 0.0 ) continue;
        //Check to see if useable one exists already.
        else if( CombustionHistoryExists(k) ) { 
          ring->perimpoints[k].hist.TotalWeight =
                  total + (double) ring->perimpoints[k].hist.DuffLoad / 100.0;
          //ring->perimpoints[k].Area=-1.0;
          continue;
        }
        if( burnup->CheckData() ) {
          if( burnup->StartLoop() ) {
            do {
              SimCont = burnup->BurnLoop();
            } while( SimCont == true );

            ma = burnup->GetSamplePoints( 0, PRECISION );
            ring->perimpoints[k].hist.WeightPolyNum = (unsigned char) ma;
            for( m = 0; m < ma; m++ ) {
              burnup->GetSample( m, &coefx, &coefy );
              ring->perimpoints[k].hist.WeightCoefX[m] = (float) coefx;
              ring->perimpoints[k].hist.WeightCoefY[m] = (float) coefy;
            }
            ring->perimpoints[k].hist.TotalTime = (float) coefx;
            ma = burnup->GetSamplePoints( 1, PRECISION );
            ring->perimpoints[k].hist.FlamePolyNum = (unsigned char) ma;
            for( m = 0; m < ma; m++ ) {
              burnup->GetSample( m, &coefx, &coefy );
              ring->perimpoints[k].hist.FlameCoefX[m] = (float) coefx;
              ring->perimpoints[k].hist.FlameCoefY[m] = (float) coefy;
            }
            ring->perimpoints[k].hist.FlameTime = (float) coefx;
          }
        }
        if( ring->perimpoints[k].hist.WeightPolyNum == 0 ||
            ring->perimpoints[k].hist.TotalTime < 0.1) { // seconds
          //ring->perimpoints[k].Area=0.0;
          ring->perimpoints[k].hist.TotalTime = (float) 0.1;
          //Just set below to amount consumed in flaming front.
          ring->perimpoints[k].hist.TotalWeight =
                    ring->perimpoints[k].hist.ReactionIntensity[1] *
                    ring->perimpoints[k].hist.FlameResidenceTime[1] / 18600.0;
          //ring->perimpoints[k].hist.TotalWeight=
          //total+(double) ring->perimpoints[k].hist.DuffLoad/10.0;
          ring->perimpoints[k].hist.WeightPolyNum = 2;
          ring->perimpoints[k].hist.FlamePolyNum = 2;
          ring->perimpoints[k].hist.FlameCoefX[1] = (float) 0.1;
          ring->perimpoints[k].hist.FlameCoefY[0] = 1.0;
          ring->perimpoints[k].hist.FlameCoefY[1] = 1.0;
          ring->perimpoints[k].hist.WeightCoefX[1] = (float) 0.1;
          ring->perimpoints[k].hist.WeightCoefY[0] = 1.0;
          ring->perimpoints[k].hist.WeightCoefY[1] = 0.0;
          ring->perimpoints[k].hist.FlameTime =
                            ring->perimpoints[k].hist.TotalTime = (float) 0.1;
        }
        else ring->perimpoints[k].hist.TotalWeight =
                  total + (double) ring->perimpoints[k].hist.DuffLoad / 100.0;

        //Initialize last values of integration.
        ring->perimpoints[k].hist.LastWtRemoved = 0.0;
      }

      //Eliminate break if multithreading is restored
      break;

      if( Begin < 0 || End < 0 ) break;
  } while( End > -1 );
} //RingBurn::BurnThread

//============================================================================
bool RingBurn::CombustionHistoryExists( long CurIndex )
{ //RingBurn::CombustionHistoryExists
  bool   Exists = false;
  long   i, j, k, m, n, nrings, nfires;
  double heat, xheat, heatrat;
  FireRing* xring;
  BurnHistory* hist, * xhist;

  hist = &ring->perimpoints[CurIndex].hist;
  heat = hist->ReactionIntensity[1] * hist->FlameResidenceTime[1];
  if( CurIndex > Begin ) {
    for( i = Begin; i < CurIndex; i++ ) {
      if( ring->perimpoints[i].Area <= 0.0 ) continue;
      xhist = &ring->perimpoints[i].hist;
      if( xhist->WeightPolyNum == 0 ) return false;
      if( hist->SurfaceFuelModel != xhist->SurfaceFuelModel ) continue;
      if( hist->WoodyFuelType != xhist->WoodyFuelType ) continue;
      if( hist->DuffLoad != xhist->DuffLoad ) continue;
      xheat = xhist->ReactionIntensity[1] * xhist->FlameResidenceTime[1];
      if( heat > xheat ) heatrat = heat / xheat;
      else heatrat = xheat / heat;
      if( heatrat > 1.5 ) continue;

      //Below: Ignore small stuff, will appear in reaction intensity.
      for( m = 2; m < 20; m++ ) {
        // 5 percent tolerance
        if( abs(hist->Moistures[m] - xhist->Moistures[m]) > 5 ) break;
      }
      if( m == 20 ) {
        Exists = true;
        break;
      }
    }
  }

  if( ! Exists && ring->StartTime > 0.0 ) {
    nrings = GetNumRings();
    nfires = GetNumFires();
    //Below: For the last one and before.
    for( i = nrings - nfires - 1; i >= nrings - 3 * nfires - 1; i-- ) {
      if( i < 0 ) continue;
      xring = GetOldRing( i );  // local copy
      if( xring->perimpoints == NULL ) continue;
      for( j = 0; j < xring->NumFires; j++ ) {
        if( j == 0 ) n = 0;
        else n = xring->NumPoints[j - 1];
        for( k = n; k < xring->NumPoints[j]; k++ ) {
          xhist = &xring->perimpoints[k].hist;
          if( xhist->WeightPolyNum == 0 ) continue;
          if( hist->SurfaceFuelModel != xhist->SurfaceFuelModel ) continue;
          if( hist->WoodyFuelType != xhist->WoodyFuelType ) continue;
          if( hist->DuffLoad != xhist->DuffLoad ) continue;
          xheat = xhist->ReactionIntensity[1] * xhist->FlameResidenceTime[1];
          if( heat > xheat ) heatrat = heat / xheat;
          else heatrat = xheat / heat;
          if( heatrat > 1.5 ) continue;
          //Below: Ignore small stuff, will appear in reaction intensity.
          for( m = 2; m < 20; m++ ) {
            // 5 percent tolerance
            if( abs(hist->Moistures[m] - xhist->Moistures[m]) > 5 ) break;
          }
          if( m == 20 ) {
            Exists = true;
            break;
          }
        }
        if( Exists ) break;
      }
      if( Exists ) break;
    }
  }

  if( Exists ) {
    memcpy( hist->FlameCoefX, xhist->FlameCoefX, 18 * sizeof(float) );
    memcpy( hist->FlameCoefY, xhist->FlameCoefY, 18 * sizeof(float) );
    memcpy( hist->WeightCoefX, xhist->WeightCoefX, 18 * sizeof(float) );
    memcpy( hist->WeightCoefY, xhist->WeightCoefY, 18 * sizeof(float) );
    hist->FlamePolyNum = xhist->FlamePolyNum;
    hist->WeightPolyNum = xhist->WeightPolyNum;
    hist->FlameTime = xhist->FlameTime;
    hist->TotalTime = xhist->TotalTime;
    hist->LastWtRemoved = 0.0;
  }

  return Exists;
} //RingBurn::CombustionHistoryExists

//============================================================================
FireRing* RingBurn::GetOldRing( long RingNum )
{ //RingBurn::GetOldRing
  long   i, curplace, ThisRing;
  double Ring;

  if( RingNum < 0 ) return (FireRing *) NULL;

  modf( ((double) RingNum / (double) RINGS_PER_STRUCT), &Ring );
  ThisRing = (long) Ring;
  curplace = RingNum - ThisRing * RINGS_PER_STRUCT;

  if( CurOldRing->StructNum != ThisRing ) {
    CurOldRing = FirstRing;
    for( i = 0; i < ThisRing; i++ ) {
      NextOldRing = (RingStruct *) CurOldRing->next;
      CurOldRing = NextOldRing;
    }
  }
  if( CurOldRing == NULL ) return NULL;

  return &CurOldRing->firering[curplace];
} //RingBurn::GetOldRing

//############################################################################
//	BurnupFireRings class
//############################################################################

//============================================================================
BurnupFireRings::BurnupFireRings()
{ //BurnupFireRings::BurnupFireRings
  ringburn = 0;
  NumRingBurn = 0;
} //BurnupFireRings::BurnupFireRings

//============================================================================
void BurnupFireRings::BurnFireRings( long StartRingNum, long EndRingNum )
{ //BurnupFireRings::BurnupFireRings
  long   i, j, m, start;
  long   begin, end, range, threadct;
  double fract, ipart, interval;

  FireRing* ring;

  AllocRingBurn();  //Try to alloc ring burn
  for( i = StartRingNum; i < EndRingNum; i++ ) {  // can parallelize here
    ring = GetRing( i );
    if( ! ring ) continue;
    start = 0;
    if( ! ring->NumPoints ) continue;
    for( j = 0; j < ring->NumFires; j++ ) {
      interval = ( (double) (ring->NumPoints[j] - start) ) /
                 (double) NumRingBurn;
      fract = modf( interval, &ipart );
      range = (long) interval;
      if( fract > 0.0 ) range++;
      threadct = 0;
      for( m = 0; m < NumRingBurn; m++ ) {
        begin = m * range + start;
        end = (m + 1) * range + start;
        if( begin >= ring->NumPoints[j] ) continue;
        if( end > ring->NumPoints[j] ) end = ring->NumPoints[j];
        ringburn[m].SetRange( ring, j, begin, end );
        threadct++;
      }
      for( m = 0; m < threadct; m++ ) ringburn[m].StartBurnThread( m );

      start = ring->NumPoints[j];
    }
  }
} //BurnupFireRings::BurnupFireRings

//============================================================================
BurnupFireRings::~BurnupFireRings()
{ //BurnupFireRings::~BurnupFireRings
  CloseBurnupFireRings();
} //BurnupFireRings::~BurnupFireRings

//============================================================================
void BurnupFireRings::CloseBurnupFireRings()
{ //BurnupFireRings::CloseBurnupFireRings
  FreeRingBurn();
} //BurnupFireRings::CloseBurnupFireRings

//============================================================================
bool BurnupFireRings::AllocRingBurn()
{ //BurnupFireRings::AllocRingBurn
  if( NumRingBurn == GetMaxThreads() ) return true;

  CloseBurnupFireRings();
  ringburn = new RingBurn[GetMaxThreads()];

  if( ringburn ) {
    NumRingBurn = GetMaxThreads();

    return true;
  }

  return false;
} //BurnupFireRings::AllocRingBurn

//============================================================================
void BurnupFireRings::FreeRingBurn()
{ //BurnupFireRings::FreeRingBurn
  if( ringburn ) delete[] ringburn;
  NumRingBurn = 0;
  ringburn = 0;
} //BurnupFireRings::FreeRingBurn

//============================================================================
void BurnupFireRings::ResetAllThreads()
{ //BurnupFireRings::ResetAllThreads
  CloseBurnupFireRings();
} //BurnupFireRings::ResetAllThreads

//############################################################################
//Post frontal combustion class member functions
//############################################################################

//============================================================================
PostFrontal::PostFrontal()
{ //PostFrontal::PostFrontal
  MergeReferenceRingNum = -1;
  NumPFI = 0;
  pfi = 0;
  WeightLossErrorTolerance( 1.0 );
} //PostFrontal::PostFrontal

//============================================================================
PostFrontal::~PostFrontal()
{ //PostFrontal::~PostFrontal
  ResetAllThreads();
} //PostFrontal::~PostFrontal

//============================================================================
void PostFrontal::ResetAllThreads()
{ //PostFrontal::ResetAllThreads
  bup.ResetAllThreads();
  CloseAllThreads();
} //PostFrontal::ResetAllThreads

//============================================================================
long PostFrontal::BurnupPrecision( long LoHi )
{ //PostFrontal::BurnupPrecision
  long Prec;

  switch( LoHi ) {
    case 0:
      PRECISION = 12;
      BURNUP_TIMESTEP = 15;
      Prec = 0;
      WeightLossErrorTolerance( 1.0 );
      break;
    case 1:
      PRECISION = 18;
      BURNUP_TIMESTEP = 10;
      Prec = 1;
      WeightLossErrorTolerance( 0.25 );
      break;
    default:
      if( PRECISION == 12 ) Prec = 0;
      else Prec = 1;
      break;
  }

  return Prec;
} //PostFrontal::BurnupPrecision

//============================================================================
long PostFrontal::AccessReferenceRingNum( long Merge, long Number )
{ //PostFrontal::AccessReferenceRingNum
  if( Number < 0 ) {
    if( Merge ) return MergeReferenceRingNum;
    else return ReferenceFireRingNum;
  }
  else {
    if( Merge ) MergeReferenceRingNum = Number;
    else ReferenceFireRingNum = Number;
  }

  return 0;
} //PostFrontal::AccessReferenceRingNum

//============================================================================
FireRing* PostFrontal::SetupFireRing( long NumFire, double Start, double End )
{ //PostFrontal::SetupFireRing
  long   i, NumPoints;
  double x, y, r, f, c;
  FireRing* firering;

  if( NumRings == 0 ) MergeReferenceRingNum = -1; //Only at very beginning

  NumPoints = GetNumPoints( NumFire );
  if( (firering = AllocFireRing(NumPoints, Start, End)) == NULL )
    return NULL;
  firering->StartTime = Start;
  firering->ElapsedTime = End - Start;
  firering->OriginalFireNumber = NumFire;

  for( i = 0; i < NumPoints; i++ ) {
    GetPerimeter2( i, &x, &y, &r, &f, &c );
    firering->perimpoints[i].x1 = x;
    firering->perimpoints[i].y1 = y;
    firering->perimpoints[i].hist.ReactionIntensity[0] = (float) c;
    if( fabs(r) > 0.0 && c > 0.0 )
      firering->perimpoints[i].hist.FlameResidenceTime[0] =
                              fabs( (float) ((f * 60.0) / (r * c)) );   // sec
    else firering->perimpoints[i].hist.FlameResidenceTime[0] = 0.0;
    firering->perimpoints[i].Status = 0;
    firering->perimpoints[i].SizeMult = 1.0;
    firering->perimpoints[i].hist.LastIntegTime = -1.0;
  }
  ReferenceFireRingNum = NumRings - 1;    // store for CorrectFireRing

  if( MergeReferenceRingNum < 0 ) MergeReferenceRingNum = NumRings - 1;

  return firering;
} //PostFrontal::SetupFireRing

//============================================================================
void PostFrontal::ComputePostFrontal( double CurrentTime, double* s,
                                      double* f )
{ //PostFrontal::ComputePostFrontal
  long   i, j, k, nrings, start;
  double RingFlameWt, RingSmolderWt;
  double fract, interval, ipart;
  long   range, begin, end, threadct;//, prevct;
  FireRing* ring;

  AllocPFI();
  nrings = GetNumRings();
  SimLevel_FlameWeightConsumed = SimLevel_SmolderWeightConsumed = 0.0;
  for( i = 0; i < nrings; i++ ) {
    ring = GetRing( i );
    if( ring->perimpoints == NULL ) continue;

    start = 0;
    RingFlameWt = RingSmolderWt = 0.0;
    for( j = 0; j < ring->NumFires; j++ ) {
      interval = ( (double) (ring->NumPoints[j] - start) /
                  (double) GetMaxThreads() );
    fract = modf( interval, &ipart );
    range = (long) interval;
    if( fract > 0.0 ) range++;
    threadct = 0;
    for( k = 0; k < NumPFI; k++) { //Number of postfrontal integration objects
      begin = k * range + start;
      end = begin + range;
      if( begin >= ring->NumPoints[j] ) continue;
      if( end > ring->NumPoints[j] ) end = ring->NumPoints[j];
      pfi[k].SetRange( ring, j, CurrentTime, begin, end );
      threadct++;
    }

    //Below: Only start as many as have data, not NumPFI.
    for( k = 0; k < threadct; k++ ) pfi[k].StartIntegThread( k );

    for( k = 0; k < threadct; k++) {  //Add up from all segments of ring
      RingSmolderWt += pfi[k].SmolderWeightConsumed;
      RingFlameWt += pfi[k].FlameWeightConsumed;
    }
    start = ring->NumPoints[j];
  }
  if( RingSmolderWt > 0.0 )
    SimLevel_SmolderWeightConsumed += RingSmolderWt; //Add up from entire ring
    SimLevel_FlameWeightConsumed += RingFlameWt;

    if( RingSmolderWt + RingFlameWt == 0.0 ) {
      if( ring->OriginalFireNumber < 0 ) FreeFireRing( i );
      else {
        ring->OriginalFireNumber += 1;
        ring->OriginalFireNumber *= -1;
      }
    }
  }
  *s += SimLevel_SmolderWeightConsumed;
  *f += SimLevel_FlameWeightConsumed;
  CondenseRings( 0 );
} //PostFrontal::ComputePostFrontal

//============================================================================
void PostFrontal::UpdateAttackPoints( FireRing* firering, long firenum )
{ //PostFrontal::UpdateAttackPoints
  //Fix up firering because of new points added by Direct Attack.

  bool PtFound;
  long i, j, m, lastpt = 0;
  long NewPoints;
  PerimPoints* tempperim;

  m = 0;
  NewPoints = GetNumPoints( firenum );
  tempperim = new PerimPoints[NewPoints];
  for( i = 0; i < NewPoints; i++ ) {
    x1 = GetPerimeter2Value( i, XCOORD );
    y1 = GetPerimeter2Value( i, YCOORD );
    PtFound = false;
    for( j = 0; j < firering->NumPoints[0]; j++ ) {
      if( firering->perimpoints[j].Status < 0 ) continue;
      if( pow2(firering->perimpoints[j].x1 - x1) +
          pow2(firering->perimpoints[j].y1 - y1) < 1e-10 ) {
        memcpy( &tempperim[m], &firering->perimpoints[j],
                sizeof(PerimPoints) );
        tempperim[m].x2 = GetPerimeter1Value( firenum, i, XCOORD );
        tempperim[m++].y2 = GetPerimeter1Value( firenum, i, YCOORD );
        lastpt = j;
        firering->perimpoints[j].Status = -1;
        PtFound = true;
        break;
      }
    }
    if( PtFound == false ) {  // if point not found
      tempperim[m].x1 = x1;
      tempperim[m].y1 = y1;
      tempperim[m].x2 = GetPerimeter1Value( firenum, i, XCOORD );
      tempperim[m].y2 = GetPerimeter1Value( firenum, i, YCOORD );
      tempperim[m].Status = 1;
      tempperim[m].hist = firering->perimpoints[lastpt].hist;
      tempperim[m].l1 = tempperim[m].l2 = tempperim[m].h =
                        tempperim[m].Area = 0.0;
      tempperim[m++].SizeMult = 1.0;
    }
  }
  delete[] firering->perimpoints;
  firering->perimpoints = new PerimPoints[NewPoints];
  memcpy( firering->perimpoints, tempperim, NewPoints * sizeof(PerimPoints) );
  delete[] tempperim;
  firering->NumPoints[0] = NewPoints;
} //PostFrontal::UpdateAttackPoints

//============================================================================
void PostFrontal::UpdateFireRing( FireRing* firering, long firenum,
                                  long NewPoints )
{ //PostFrontal::UpdateFireRing
  //Fix up firering because of new points added in Intersect::OrganizeCrosses
  //and then reorders points in the firering based on current ordering that
  //may have changed when crosses were identified.

  bool PtFound;
  long i, j, m, n, p, LastPt = -1;
  PerimPoints* tempperim;

  m = 0;
  tempperim = new PerimPoints[NewPoints];
  for( i = 0; i < NewPoints; i++ ) {
    x2 = GetPerimeter1Value( firenum, i, XCOORD );
    y2 = GetPerimeter1Value( firenum, i, YCOORD );
    PtFound = false;
    for( j = 0; j < firering->NumPoints[0]; j++ ) {
      if( firering->perimpoints[j].Status < 0 ) continue;
      if( pow2(firering->perimpoints[j].x2 - x2) +
          pow2(firering->perimpoints[j].y2 - y2) < 1e-10 ) {
        memcpy( &tempperim[m++], &firering->perimpoints[j],
                sizeof(PerimPoints) );
        firering->perimpoints[j].Status = -1;
        LastPt = j;
        PtFound = true;
        break;
      }
    }
    if( PtFound == false ) {  // if point not found
      n = LastPt;
      if( n < 0 ) n += firering->NumPoints[0];
      p = LastPt + 1;
      if( p > firering->NumPoints[0] - 1 ) p -= firering->NumPoints[0];
      xm1 = firering->perimpoints[n].x1;
      ym1 = firering->perimpoints[n].y1;
      xm2 = firering->perimpoints[n].x2;
      ym2 = firering->perimpoints[n].y2;
      xn1 = firering->perimpoints[p].x1;
      yn1 = firering->perimpoints[p].y1;
      xn2 = firering->perimpoints[p].x2;
      yn2 = firering->perimpoints[p].y2;
      tempperim[m].x2 = x2;
      tempperim[m].y2 = y2;
      if( fabs(xm2 - xn2) > 1e-9 )
        tempperim[m].x1 = xm1 - (xm1 - xn1) * (xm2 - x2) / (xm2 - xn2);
      else tempperim[m].x1 = xm1;
      if( fabs(ym2 - yn2) > 1e-9 )
        tempperim[m].y1 = ym1 - (ym1 - yn1) * (ym2 - y2) / (ym2 - yn2);
      else tempperim[m].y1 = ym1;
      tempperim[m].Status = 1;
      tempperim[m].hist = firering->perimpoints[n].hist;
      tempperim[m].Area = -1;
      tempperim[m].l1 = tempperim[m].l2 = tempperim[m].h = 0.0;
      tempperim[m++].SizeMult = 1.0;
    }
  }

  delete[] firering->perimpoints;
  firering->perimpoints = new PerimPoints[NewPoints];
  memcpy( firering->perimpoints, tempperim, NewPoints * sizeof(PerimPoints) );
  delete[] tempperim;
  firering->NumPoints[0] = NewPoints;
} //PostFrontal::UpdateFireRing

//============================================================================
void PostFrontal::UpdatePointOrder( FireRing* firering, long firenum )
{ //PostFrontal::UpdatePointOrder
  //Reorders points in the firering based on current ordering that may have
  //changed when crosses were identified.

  long i, j, k;
  PerimPoints* tempperim;

  x2 = GetPerimeter1Value( firenum, 0, XCOORD );
  y2 = GetPerimeter1Value( firenum, 0, YCOORD );
  for( i = 0; i < firering->NumPoints[0]; i++ ) {
    if( pow2(firering->perimpoints[i].x2 - x2) +
        pow2(firering->perimpoints[i].y2 - y2) < 1e-8 )
      break;
  }
  if( i == 0 ) return;

  tempperim = new PerimPoints[firering->NumPoints[0]];
  memcpy( tempperim, firering->perimpoints,
          firering->NumPoints[0] * sizeof(PerimPoints) );
  for( j = 0; j < firering->NumPoints[0]; j++ ) {
    k = j + i;
    if( k > firering->NumPoints[0] - 1 ) k -= firering->NumPoints[0];
    memcpy( &firering->perimpoints[j], &tempperim[k],
            sizeof(PerimPoints) );
  }
  delete[] tempperim;
} //PostFrontal::UpdatePointOrder

//============================================================================
void PostFrontal::UpdateMergeOrder( FireRing* firering, long firenum )
{ //PostFrontal::UpdateMergeOrder
  //Reorders points in the firering based on current ordering that may have
  //changed when crosses were identified.

  bool PtFound;
  long i, j, k, m, p, q, r, LastID = 0, NewTotal;
  long Start = 0;
  MergePoints* tempmerge = 0;
  PerimPoints* tempperim = 0;

  k = 0;
  tempmerge = new MergePoints[GetNumPoints(firenum)];
  //Update mergepoints array with exact points in perim2 array for outer fire.
  for( i = 0; i < GetNumPoints(firenum); i++ ) {
    x2 = GetPerimVal( 1, firenum, i, XCOORD );
    y2 = GetPerimVal( 1, firenum, i, YCOORD );
    PtFound = false;
    for( j = Start; j < firering->NumMergePoints[0] + Start; j++ ) {
      p = j;
      if( p > firering->NumMergePoints[0] - 1 )
        p -= firering->NumMergePoints[0];
      x1 = firering->mergepoints[p].x;
      y1 = firering->mergepoints[p].y;
      if( pow2(x2 - x1) + pow2(y2 - y1) < 1e-10 ) {
        tempmerge[k].x = x2;
        tempmerge[k].y = y2;
        tempmerge[k].Status = 1;
        tempmerge[k].FireID = 0;
        tempmerge[k++].VertexID = LastID = firering->mergepoints[p].VertexID;
        PtFound = true;
        Start = p;
        break;
      }
    }
    if( PtFound == false ) {
      tempmerge[k].x = x2;
      tempmerge[k].y = y2;
      tempmerge[k].Status = 1;
      tempmerge[k].FireID = 0;
      tempmerge[k++].VertexID = -( LastID + 1 );
      LastID++;
    }
  }
  
  //Below: Fire acquired more pts since becoming ring.
  if( k > firering->NumMergePoints[0] ) {
    delete[] firering->mergepoints;
    firering->mergepoints = new MergePoints[GetNumPoints(firenum)];

    NewTotal = 10 * (k - firering->NumMergePoints[0]) +
               firering->NumPoints[firering->NumFires - 1] + 1;
    tempperim = new PerimPoints[firering->NumPoints[firering->NumFires - 1]];
    memcpy( tempperim, firering->perimpoints,
            firering->NumPoints[firering->NumFires - 1] *
            sizeof(PerimPoints) );
    delete[] firering->perimpoints;
    firering->perimpoints = new PerimPoints[NewTotal];

    memcpy( firering->mergepoints, tempmerge, k * sizeof(MergePoints) );
    firering->NumMergePoints[0] = k;
    firering->NumMergePoints[1] = 0;

    for( i = 0; i < firering->NumMergePoints[0]; i++ ) {
      firering->mergepoints[i].VertexID = -1;
      tempmerge->VertexID = -1;
    }
    r = 0;
    for( i = 0; i < firering->NumMergePoints[0]; i++ ) {
      x1 = firering->mergepoints[i].x;
      y1 = firering->mergepoints[i].y;
      PtFound = false;
      for( j = 0; j < firering->NumFires; j++ ) {
        if( j == 0 ) k = 0;
        else k = firering->NumPoints[j - 1];
        for( m = k; m < firering->NumPoints[j]; m++ ) {
          x2 = tempperim[m].x2;
          y2 = tempperim[m].y2;
          if( pow2(x1 - x2) + pow2(y1 - y2) > 1e-10 ) continue;
          firering->mergepoints[i].VertexID = m;
          tempmerge->VertexID = m;
          {
            PtFound = true;
            break;
          }
        }
        if( PtFound ) break;
      }
    }
    memcpy( firering->perimpoints, tempperim,
            firering->NumPoints[firering->NumFires - 1] *
            sizeof(PerimPoints) );
    for( i = 0; i < firering->NumMergePoints[0]; i++ ) {
      if( firering->mergepoints[i].VertexID >= 0 ) continue;
        //Find the previous point with correspoinding perimpoint.
        p = i;
        do {
          p--;
          if( p < 0 ) p = firering->NumMergePoints[0] - 1;
        } while( firering->mergepoints[p].VertexID < 0 );
        p = firering->mergepoints[p].VertexID;

        //Find the right fire among those in perimpoints.
        for( j = 0; j < firering->NumFires; j++ ) {
          if( p < firering->NumPoints[j] ) break;    // j=firenum
        }

        //Find next point with corresponding perimpoint.
        q = p + 1;
        if( q >= firering->NumPoints[j] ) {
        if( j == 0 ) q = 0;
        else q = firering->NumPoints[j - 1];
      }

      x2 = firering->mergepoints[i].x;
      y2 = firering->mergepoints[i].y;

      xm2 = firering->perimpoints[p].x2;
      ym2 = firering->perimpoints[p].y2;
      xn2 = firering->perimpoints[q].x2;
      yn2 = firering->perimpoints[q].y2;

      xm1 = firering->perimpoints[p].x1;
      ym1 = firering->perimpoints[p].y1;
      xn1 = firering->perimpoints[q].x1;
      yn1 = firering->perimpoints[q].y1;

      if( p > q )
        memmove( &firering->perimpoints[q + 1], &firering->perimpoints[q],
                 (firering->NumPoints[firering->NumFires - 1] - q) *
                 sizeof(PerimPoints) );
      else memmove( &firering->perimpoints[p + 2],
                    &firering->perimpoints[p + 1],
                    (firering->NumPoints[firering->NumFires - 1] - (p + 1)) *
                    sizeof(PerimPoints) );
      for( r = 0; r < firering->NumMergePoints[0]; r++ ) {
        if( firering->mergepoints[r].VertexID > p )
          firering->mergepoints[r].VertexID += 1;
      }
      m = p + 1;

      if( fabs(xm2 - xn2) > 1e-9 )
        x1 = xm1 - (xm1 - xn1) * (xm2 - x2) / (xm2 - xn2);
      else x1 = xm1;
      if( fabs(ym2 - yn2) > 1e-9 )
        y1 = ym1 - (ym1 - yn1) * (ym2 - y2) / (ym2 - yn2);
      else y1 = ym1;
      firering->perimpoints[m].x1 = x1;
      firering->perimpoints[m].y1 = y1;
      firering->perimpoints[m].x2 = x2;
      firering->perimpoints[m].y2 = y2;
      firering->perimpoints[m].Status = 1;
      firering->perimpoints[m].SizeMult = 1.0;
      firering->perimpoints[m].hist = firering->perimpoints[p].hist;
      firering->perimpoints[m].Area = -1.0;
      firering->mergepoints[i].VertexID = m;
      for( r = j; r < firering->NumFires; r++ )
        firering->NumPoints[r] += 1;
    }
    if( tempperim ) delete[] tempperim;
  }
  else {
    memcpy( firering->mergepoints, tempmerge, k * sizeof(MergePoints) );
    firering->NumMergePoints[1] = 0;
  }
  if( tempmerge ) delete[] tempmerge;
} //PostFrontal::UpdateMergeOrder

//============================================================================
void PostFrontal::CorrectFireRing( long NumIsects, long* isects,
                                   double* ipoint, long* fires,
                                   long NumPerims, long NewPoints )
{ //PostFrontal::CorrectFireRing
  //Corrects the firering on a single fire for overlaps. Does this by first
  //splicing the intersection points at the edges of the overlap into the
  //firering array. Then it flags each itersection point with a 2. Next it IDs
  //all other points that are currently on the outer edge of the fire with a
  //1 by comparing the outermost edge of the firering with the current
  //corrected perimeters for each fire (that includes the main fire front and
  //all inward burning fires that were created in the crossover correction
  //(prior to this step). Then it reorders the perimeter if the first point on
  //the fire is inside an overlapping portion. Finally it calls the
  //FillOuterRing function to correctly pair-up the intersections that define
  //the overlaps, which calls the InterpolateOverlaps function, and so on....

  bool  Found, WriteIt;
  long  i, j, k, m, n, NumInsert, TotalPts, SumStatus;
  long  NPOld, NPNew, NPOut, CurStat;
  long  Start, FirstCross;
  double xpt, ypt, x1a, x1b, x2a, x2b, y1a, y1b, y2a, y2b;
  double xold, yold, xnew, ynew, dist;

  FireRing* firering, ** newring = 0;
  PerimPoints* tempperim;

  if( ReferenceFireRingNum == -1 ) return;

  firering = GetRing( ReferenceFireRingNum );

  UpdateFireRing( firering, fires[0], NewPoints );

  for( j = 0; j < firering->NumPoints[0]; j++ ) {
    //New intersection point added in UpdateMergeOrder.
    if( firering->perimpoints[j].Status > 0 ) {
      firering->perimpoints[j].Status = 0;
    }
  }

  tempperim = new PerimPoints[(firering->NumPoints[0] + NumIsects * 2)];
  Start = -1;
  TotalPts = 0;
  NumInsert = 0;

  //Splice intersection points into perimeter array.
  for( i = 0; i < NumIsects; i++ ) {
    FirstCross = isects[i * 2];
    Start++;
    for( j = Start; j <= FirstCross; j++ ) {
      memcpy( &tempperim[j + NumInsert], &firering->perimpoints[j],
              sizeof(PerimPoints) );
      TotalPts++;
    }
    Start = FirstCross;
    x1a = tempperim[FirstCross + NumInsert].x1;
    x2a = tempperim[FirstCross + NumInsert].x2;
    y1a = tempperim[FirstCross + NumInsert].y1;
    y2a = tempperim[FirstCross + NumInsert].y2;
    k = FirstCross + 1;
    if( k > firering->NumPoints[0] - 1 ) k -= firering->NumPoints[0];
    x1b = firering->perimpoints[k].x1;
    x2b = firering->perimpoints[k].x2;
    y1b = firering->perimpoints[k].y1;
    y2b = firering->perimpoints[k].y2;

    if( pow2(ipoint[i * 5] - x2a) + pow2(ipoint[i * 5 + 1] - y2a) < 1e-10 ) {
      ipoint[i * 5] = x2a - 0.01 * (x2a - x2b);
      ipoint[i * 5 + 1] = y2a - 0.01 * (y2a - y2b);
    }
    else if( pow2(ipoint[i * 5] - x2b) +
             pow2(ipoint[i * 5 + 1] - y2b) < 1e-10 ) {
      ipoint[i * 5] = x2b - 0.01 * (x2b - x2a);
      ipoint[i * 5 + 1] = y2b - 0.01 * (y2b - y2a);
    }

    if( fabs(x2a - x2b) > 1e-10 )
      xpt = x1a - (x1a - x1b) * (x2a - ipoint[i * 5]) / (x2a - x2b);
    else xpt = x1a;
    if( fabs(y2a - y2b) > 1e-10 )
      ypt = y1a - (y1a - y1b) * (y2a - ipoint[i * 5 + 1]) / (y2a - y2b);
    else ypt = y1a;

    NumInsert++;
    tempperim[FirstCross + NumInsert].x1 = xpt;
    tempperim[FirstCross + NumInsert].y1 = ypt;
    tempperim[FirstCross + NumInsert].x2 = ipoint[i * 5];
    tempperim[FirstCross + NumInsert].y2 = ipoint[i * 5 + 1];
    tempperim[FirstCross + NumInsert].hist = firering->perimpoints[k].hist;
    tempperim[FirstCross + NumInsert].SizeMult = 1.0;
    tempperim[FirstCross + NumInsert].Status = 2;
    tempperim[FirstCross + NumInsert].Area = -1.0;
    TotalPts++;
  }
  for( j = Start + 1; j < firering->NumPoints[0]; j++ ) {
    memcpy( &tempperim[j + NumInsert], &firering->perimpoints[j],
            sizeof(PerimPoints) );
    TotalPts++;
  }
  delete[] firering->perimpoints;
  firering->perimpoints = new PerimPoints[TotalPts];
  memcpy( firering->perimpoints, tempperim, TotalPts * sizeof(PerimPoints) );
  firering->NumPoints[0] = TotalPts;

  //Find out which perim points are still on the new perimeter, flag with 1.
  NPNew = GetNumPoints( fires[0] );
  NPOld = NPOut = firering->NumPoints[0] = TotalPts;
  for( k = 0; k < NPNew; k++ ) {
    xnew = GetPerimVal( 2, fires[0], k, XCOORD );
    ynew = GetPerimVal( 2, fires[0], k, YCOORD );
    for( j = 0; j < NPOld; j++ ) {
      CurStat = firering->perimpoints[j].Status;
      if( CurStat != 1 ) { // 1 means match was found
        xold = firering->perimpoints[j].x2;
        yold = firering->perimpoints[j].y2;
        dist = pow2( xold - xnew ) + pow2( yold - ynew );
        if( dist < 1e-12 && CurStat != 2 ) {
          firering->perimpoints[j].Status = 1;
          NPOut--;

          break;
        }
      }
    }
  }

  //Allocate New Rings and move inward burning perim segments to newrings.
  if( NumPerims > 1 ) {
    newring = new FireRing * [NumPerims];
    newring[0] = firering;
    for( k = 1; k < NumPerims; k++ )
      newring[k] = SpawnFireRing( fires[k], NPOut * 2, firering->StartTime,
                                  firering->ElapsedTime );
    for( k = 1; k < NumPerims; k++ ) {
      Found = false;
      m = 0;
      for( n = k - 1; n > -1; n-- ) {
        if( Found ) break;
        j = 0;
        xnew = GetPerimVal( 1, fires[k], j, XCOORD );
        ynew = GetPerimVal( 1, fires[k], j, YCOORD );
        for( i = 0; i < newring[n]->NumPoints[0]; i++) {
          xold = newring[n]->perimpoints[i].x2;
          yold = newring[n]->perimpoints[i].y2;
          dist = pow2( xold - xnew ) + pow2( yold - ynew );
          //If same pt and 1st already found...
          if( dist < 1e-12 && Found == true ) break;
          //Else if same pt and 1st not found...
          else if( dist < 1e-12 && Found == false ) WriteIt = true;
          //Else if not same pt and 1st found...
          else if( dist >= 1e-12 && Found == true ) WriteIt = true;
          //Else if already on outside.
          else if( newring[n]->perimpoints[i].Status == 1 ) WriteIt = false;
          else WriteIt = false;

          if( WriteIt ) {
            memcpy( &newring[k]->perimpoints[m], &newring[n]->perimpoints[i],
                    sizeof(PerimPoints) );
            if( Found ) {
              newring[n]->perimpoints[i].Status = -1;
              newring[n]->perimpoints[i].SizeMult = 0.0;
              newring[n]->perimpoints[i].Area = -1.0;
            }
            else newring[k]->perimpoints[m].Status = 1;
            newring[k]->perimpoints[m].SizeMult = 0.0; //Make all of them zero
            m++;
            Found = true;
          }
        }
      }
      newring[k]->NumPoints[0] = m;
    }
  }

  //Flag.
  for( i = 1; i < NumPerims; i++ ) {
    NPNew = GetNumPoints( fires[i] );
    for( j = 0; j < NPNew; j++ ) {
      xnew = GetPerimVal( 1, fires[i], j, XCOORD );
      ynew = GetPerimVal( 1, fires[i], j, YCOORD );
      for( k = 0; k < newring[i]->NumPoints[0]; k++ ) {
        if( newring[i]->perimpoints[k].Status == 1 ) continue;
        xold = newring[i]->perimpoints[k].x2;
        yold = newring[i]->perimpoints[k].y2;
        dist = pow2( xold - xnew ) + pow2( yold - ynew );
        if( dist < 1e-12 ) {
          if( newring[i]->perimpoints[k].Status != 2 )
            newring[i]->perimpoints[k].Status = 1;
          newring[i]->perimpoints[k].SizeMult = 1.0;
          break;
        }
      }
    }
  }

  for( k = 1; k < NumPerims; k++ ) {
    RemoveRingEnclaves( newring[k] );
    FillOuterRing( newring[k] );
    FillMergeArray( newring[k] );
  }

  //Reorder perimeter if first outside points have been eliminated
  //(status flag==0).

  SumStatus = 0;
  for( i = 0; i < TotalPts; i++ ) {
    if( firering->perimpoints[i].Status < 2 )
      SumStatus += abs( firering->perimpoints[i].Status );
    else {
      if( SumStatus < i ) {  //There was a zero status perim point
        for( j = 0; j < TotalPts; j++ )
          tempperim[j].Status = firering->perimpoints[j].Status;
        m = 0;    //New-order point counter
        for( j = TotalPts - 1; j > (-1); j-- ) {
          if( firering->perimpoints[j].Status == 2 ) {
            for( k = j; k < TotalPts; k++ )
              memcpy( &firering->perimpoints[m++], &tempperim[k],
                      sizeof(PerimPoints) );
            for( k = 0; k < j; k++ )
              memcpy( &firering->perimpoints[m++], &tempperim[k],
                      sizeof(PerimPoints) );
            break;
          }
        }

        break;    //Reordering done, get out
      }
      else break;
    }
  }

  if( tempperim ) delete[] tempperim;

  if( NumPerims > 1 ) RemoveRingEnclaves( firering );
  FillOuterRing( firering );

  FillMergeArray( firering );
  if( newring ) delete[] newring;
} //PostFrontal::CorrectFireRing

//============================================================================
void PostFrontal::RemoveDuplicatePoints( FireRing* firering )
{ //PostFrontal::RemoveDuplicatePoints
  long   i, j, NumPts;
  double xpt, ypt, xn, yn, dist;

  NumPts = firering->NumPoints[0];
  xpt = firering->perimpoints[0].x2;
  ypt = firering->perimpoints[0].y2;
  for( i = 1; i <= NumPts; i++ ) {
    j = i;
    if( j == NumPts ) j = 0;
    xn = firering->perimpoints[j].x2;
    yn = firering->perimpoints[j].y2;
    dist = pow2( xpt - xn ) + pow2( ypt - yn );
    if( dist < 1e-12 ) {
      memmove( &firering->perimpoints[j], &firering->perimpoints[j + 1],
               (NumPts - j - 1) * sizeof(PerimPoints) );
      if( i < NumPts - 1 ) NumPts--;
    }
    xpt = xn;
    ypt = yn;
  }
  firering->NumPoints[0] = NumPts;
} //PostFrontal::RemoveDuplicatePoints

//============================================================================
void PostFrontal::FillMergeArray( FireRing* firering )
{ //PostFrontal::FillMergeArray
  long i, j, k, NumPoints, StartPt;

  NumPoints = 0;
  for( i = 0; i < firering->NumPoints[firering->NumFires - 1]; i++ ) {
    if( firering->perimpoints[i].SizeMult == 1.0 ) NumPoints++;
  }
  if( firering->mergepoints ) delete[] firering->mergepoints;
  firering->mergepoints = new MergePoints[NumPoints];
  firering->NumMergePoints[0] = NumPoints;
  firering->NumMergePoints[1] = 0;
  k = 0;
  for( i = 0; i < firering->NumFires; i++ ) {
    StartPt = 0;
    if( i > 0 ) StartPt = firering->NumPoints[i - 1];
    for( j = StartPt; j < firering->NumPoints[i]; j++ ) {
      if( firering->perimpoints[j].SizeMult == 1.0 ) {
        firering->perimpoints[j].Status = 1;
        firering->mergepoints[k].Status = 0;
        firering->mergepoints[k].FireID = i;
        firering->mergepoints[k].VertexID = j;
        firering->mergepoints[k].x = firering->perimpoints[j].x2;
        firering->mergepoints[k++].y = firering->perimpoints[j].y2;
      }
    }
  }
} //PostFrontal::FillMergeArray

//============================================================================
void PostFrontal::RemoveRingEnclaves( FireRing* ring )
{ //PostFrontal::RemoveRingEnclaves
  long i, j, k;
  long IslandPts = 0, NewPts = 0;
  PerimPoints* tempperim;

  tempperim = new PerimPoints[ring->NumPoints[ring->NumFires - 1]];

  for( i = 0; i < ring->NumFires; i++ ) {
    k = 0;
    if( i > 0 ) {
      k = ring->NumPoints[i - 1];
      ring->NumPoints[i - 1] -= IslandPts;
    }
    for( j = k; j < ring->NumPoints[i]; j++ ) {
      if( ring->perimpoints[j].Status < 0 ) IslandPts++;
      else {
        memcpy( &tempperim[NewPts], &ring->perimpoints[j],
                sizeof(PerimPoints) );
        NewPts++;
      }
    }
  }

  ring->NumPoints[ring->NumFires - 1] = NewPts;
  delete[] ring->perimpoints;
  ring->perimpoints = new PerimPoints[NewPts];
  memcpy( ring->perimpoints, tempperim, NewPts * sizeof(PerimPoints) );
  delete[] tempperim;
} //PostFrontal::RemoveRingEnclaves

//============================================================================
void PostFrontal::FillOuterRing( FireRing* firering )
{ //PostFrontal::FillOuterRing
  //Goes through list of intersections and pairs-up those that define an
  //overlapping area, either a single loop or a double loop (the only two
  //kinds). Once ID'd, the intersections are copied to the tempperim array and
  //processed through the InterpolateOverlaps function that determines the
  //fractional area occupied by each quadrilateral relative to the actual area
  //of the overlapped area this will be used to reduce the post-frontal
  //combustion later on.

  bool   Exit;
  long   i, j, k;
  long   Start1, End1, Start2, Num1, Num2;
  long   CurStat, StoreCount;
  double x1a, x2a, x2b, y1a, y2a, y2b;
  double dist;

  PerimPoints* tempperim;

  Exit = false;
  for( i = 0; i < firering->NumPoints[0]; i++ ) {
    do {   //Loop until beginning of overlapped section
      CurStat = firering->perimpoints[i++].Status;
      if( i > firering->NumPoints[0] - 1 ) {
        Exit = true;
        break;
      }
    } while( CurStat < 2 );

    if( Exit ) break;

    Start1 = i - 1;
    Num1 = 1;  //Needs to be 2 because j==i
    for( j = i; j < firering->NumPoints[0]; j++ ) {
      CurStat = firering->perimpoints[j].Status;
      Num1++;
      if( CurStat == 2 ) break;
    }
    if( j >= firering->NumPoints[0] )
      j = firering->NumPoints[0] - 1; //Debugging
    End1 = i = j;
    x2a = firering->perimpoints[Start1].x2;
    y2a = firering->perimpoints[Start1].y2;
    x2b = firering->perimpoints[End1].x2;
    y2b = firering->perimpoints[End1].y2;
    dist = pow2( x2a - x2b ) + pow2( y2a - y2b );
    if( dist < 1e-8 ) {    //If single intersect point
      tempperim = new PerimPoints[Num1];
      for( k = 0; k < Num1; k++ ) {
        j = Start1 + k;
        memcpy( &tempperim[k], &firering->perimpoints[j],
                sizeof(PerimPoints) );
      }
      InterpolateOverlaps( tempperim, Num1 );
      if( Verts ) delete[] Verts;
      for( k = 0; k < Num1; k++ ) {
        j = Start1 + k;
        firering->perimpoints[j].x2 = tempperim[k].x2;
        firering->perimpoints[j].y2 = tempperim[k].y2;
        firering->perimpoints[j].SizeMult = tempperim[k].SizeMult;
        firering->perimpoints[j].Status = tempperim[k].Status;
        firering->perimpoints[j].Area = tempperim[k].Area;
        firering->perimpoints[j].hist = tempperim[k].hist;
      }
    }
    else {    //If double intersect point
      Exit = false;
      do {    //Find Start2=End1
        do {
          ++j;
          if( j > firering->NumPoints[0] - 1 ) {
            Exit = true;
            break;
          }
          CurStat = firering->perimpoints[j].Status;
        } while( CurStat < 2 );

        if( Exit ) break;

        x1a = firering->perimpoints[j].x2;
        y1a = firering->perimpoints[j].y2;
        dist = pow2( x1a - x2b ) + pow2( y1a - y2b );
      } while( dist > 1e-9 );

      if( Exit ) break;

      Start2 = j;
      Num2 = 1;
      do {
        do {
          ++j;
          if( j > firering->NumPoints[0] - 1 ) {
            Exit = true;
            break;
          }
          CurStat = firering->perimpoints[j].Status;
          Num2++;
        } while( CurStat < 2 );

        if( Exit ) break;

        x1a = firering->perimpoints[j].x2;
        y1a = firering->perimpoints[j].y2;
        dist = pow2( x1a - x2a ) + pow2( y1a - y2a );
      } while( dist > 1e-9 );

      if( Exit ) break;

      tempperim = new PerimPoints[Num1 + Num2];
      for( StoreCount = 0; StoreCount < Num1; StoreCount++ ) {
        k = StoreCount + Start1;
        memcpy( &tempperim[StoreCount], &firering->perimpoints[k],
                sizeof(PerimPoints) );
      }
      for( StoreCount = 0; StoreCount < Num2; StoreCount++ ) {
        k = StoreCount + Start2;
        memcpy( &tempperim[Num1 + StoreCount], &firering->perimpoints[k],
                sizeof(PerimPoints) );
      }
      InterpolateOverlaps( tempperim, Num1 + Num2 );
      if( Verts ) delete[] Verts;
      for( StoreCount = 0; StoreCount < Num1; StoreCount++ ) {
        k = Start1 + StoreCount;
        if( tempperim[StoreCount].SizeMult < 1e-6 )
          tempperim[StoreCount].SizeMult = 1e-6;
        else if( tempperim[StoreCount].SizeMult > 1.0 )
          tempperim[StoreCount].SizeMult = 1.0;
        firering->perimpoints[k].SizeMult = tempperim[StoreCount].SizeMult;
        firering->perimpoints[k].Status = tempperim[StoreCount].Status;
        firering->perimpoints[k].Area = -1.0;
      }
      for( StoreCount = 0; StoreCount < Num2; StoreCount++ ) {
        k = Start2 + StoreCount;
        if( tempperim[Num1 + StoreCount].SizeMult < 1e-6 )
          tempperim[Num1 + StoreCount].SizeMult = 1e-6;
        else if( tempperim[Num1 + StoreCount].SizeMult > 1.0 )
          tempperim[Num1 + StoreCount].SizeMult = 1.0;
        firering->perimpoints[k].SizeMult =
                                        tempperim[Num1 + StoreCount].SizeMult;
        firering->perimpoints[k].Status = tempperim[Num1 + StoreCount].Status;
        firering->perimpoints[k].Area = -1.0;
      }
    }

    if( tempperim ) delete[] tempperim;
  }
} //PostFrontal::FillOuterRing

//============================================================================
void PostFrontal::InterpolateOverlaps( PerimPoints* verts, long numverts )
{ //PostFrontal::InterpolateOverlaps
  //Uses a non-spatial technique for aportioning area to each set of vertices
  //by the fraction of the original area required to achieve the correct area
  //but only by quadrilateral sections that are not spatially accurate.

  long   i, j;
  double p, q, nx1, ny1;
  double Dir1, Dir2, xdiff, ydiff, sindiff;
  double SubArea, AFract, SuperArea, TargetArea, TargetPerim;
  double a, b, c, d, part1, part2;

  //Clean up segment crosses by inserting intersection point when cross is
  //found.
  SuperArea = 0.0;
  for( i = 1; i < numverts; i++ ) {
    j = i - 1;
    if( verts[j].Status == 2 && verts[i].Status == 2 ) continue;
    if( pow2(verts[j].x1 - verts[i].x1) +
        pow2(verts[j].y1 - verts[i].y1) > 1e-9 ) {
      if( Cross(verts[j].x1, verts[j].y1, verts[j].x2, verts[j].y2,
          true, verts[i].x1, verts[i].y1, verts[i].x2, verts[i].y2,
          true, &nx1, &ny1) ) {
        verts[j].x2 = nx1;    //Replace points with intersection
        verts[j].y2 = ny1;
        verts[i].x2 = nx1;
        verts[i].y2 = ny1;
      }
    }
    a = pow2( verts[j].x1 - verts[j].x2 ) + pow2( verts[j].y1 - verts[j].y2 );
    b = pow2( verts[j].x1 - verts[i].x1 ) + pow2( verts[j].y1 - verts[i].y1 );
    c = pow2( verts[i].x1 - verts[i].x2 ) + pow2( verts[i].y1 - verts[i].y2 );
    d = pow2( verts[i].x2 - verts[j].x2 ) + pow2( verts[i].y2 - verts[j].y2 );
    p = pow2( verts[j].x1 - verts[i].x2 ) + pow2( verts[j].y1 - verts[i].y2 );
    q = pow2( verts[i].x1 - verts[j].x2 ) + pow2( verts[i].y1 - verts[j].y2 );
    part1 = 4.0 * p * q;
    part2 = pow2( b + d - a - c );
    if( part2 < part1 ) SuperArea += 0.25 * sqrt(part1 - part2);
  }
  Verts = new double[numverts * 2];
  for( i = 0; i < numverts; i++ ) {
    Verts[i * 2] = verts[i].x1;
    Verts[i * 2 + 1] = verts[i].y1;
  }

  AreaPerim( numverts, Verts, &TargetArea, &TargetPerim );
  TargetArea = fabs( TargetArea );

  //Compute proportioned area for each point.
  x1 = verts[0].x1;
  y1 = verts[0].y1;
  xn1 = verts[0].x2;
  yn1 = verts[0].y2;
  for( i = 1; i < numverts; i++ ) {
    x2 = verts[i].x1;
    y2 = verts[i].y1;
    xn2 = verts[i].x2;
    yn2 = verts[i].y2;
    xdiff = x1 - xn2;
    ydiff = y1 - yn2;
    p = sqrt( pow2(xdiff) + pow2(ydiff) );
    if( p > 1e-9 ) Dir1 = atan2( ydiff, xdiff );
    else Dir1 = 0.0;
    xdiff = x2 - xn1;
    ydiff = y2 - yn1;
    q = sqrt( pow2(xdiff) + pow2(ydiff) );
    if( q > 1e-9 ) Dir2 = atan2( ydiff, xdiff );
    else Dir2 = 0.0;
    sindiff = sin( Dir1 ) * cos( Dir2 ) - cos( Dir1 ) * sin( Dir2 );
    SubArea = fabs( 0.5 * p * q * sindiff );
    if( SubArea < 1e-9 ) continue;
    if( SuperArea < 1e-9 ) SuperArea = SubArea + TargetArea;
    if( SuperArea > 1e-9 )
      AFract = SubArea / SuperArea * TargetArea / SuperArea;
    else AFract = 1.0;
    if( AFract < 0.0 ) AFract = 0.0;
    else if( AFract > 1.0 ) AFract = 1.0;
    verts[i - 1].SizeMult = AFract;
    verts[i - 1].Status = 1;
    if( verts[i].Status == 2 ) { //If current point is on the edge of overlap
      verts[i].Status = 1;
      i++;
      if( i > numverts - 1 ) continue;
      x1 = verts[i].x1;
      y1 = verts[i].y1;
      xn1 = verts[i].x2;
      yn1 = verts[i].y2;
    }
    else {
      x1 = x2;
      y1 = y2;
      xn1 = xn2;
      yn1 = yn2;
    }
  }
} //PostFrontal::InterpolateOverlaps

//============================================================================
bool PostFrontal::Cross( double xpt1, double ypt1, double xpt2, double ypt2,
                         bool OnSpan1, double xpt1n, double ypt1n,
                         double xpt2n, double ypt2n, bool OnSpan2,
                         double* newx, double* newy )
{ //PostFrontal::Cross
  //Computes crosses between two spans defined by their endpoint returns true
  //if the intersection occurrs within the segment and updates the 
  //intersection point newx,newy regardless.

  double xdiff1, ydiff1, xdiff2, ydiff2, ycept1, ycept2;
  double slope1, slope2, ycommon, xcommon;
  bool   Intersection = false;

  xdiff1 = xpt2 - xpt1;
  ydiff1 = ypt2 - ypt1;
  if( fabs(xdiff1) < 1e-9 ) xdiff1 = 0.0;
  if( xdiff1 != 0.0 ) {
    slope1 = ydiff1 / xdiff1;
    ycept1 = ypt1 - ( slope1 * xpt1 );
  }
  else {
    slope1 = 1.0;      //SLOPE NON-ZERO
    ycept1 = xpt1;
  }
  xdiff2 = xpt2n - xpt1n;
  ydiff2 = ypt2n - ypt1n;
  if( fabs(xdiff2) < 1e-9 ) xdiff2 = 0.0;
  if( xdiff2 != 0.0 ) {
    slope2 = ydiff2 / xdiff2;
    ycept2 = ypt1n - ( slope2 * xpt1n );
  }
  else {
    slope2 = 1.0;      //SLOPE NON-ZERO
    ycept2 = xpt1n;
  }
  if( fabs(slope1 - slope2) < 1e-9 ) {
    if( fabs(ycept1 - ycept2) < 1e-9 ) {
      if( xdiff1 == 0.0 && xdiff2 == 0.0 ) {
        if( OnSpan1 && OnSpan2 ) {
          if( ((ypt1 <= ypt1n && ypt1 > ypt2n) ||
               (ypt1 >= ypt1n && ypt1 < ypt2n)) &&
              ((ypt1n <= ypt1 && ypt1n > ypt2) ||
               (ypt1n >= ypt1 && ypt1n < ypt2)) ) {
            Intersection = true;
            *newx = xpt1;
            *newy = ypt1;
          }
        }
        else if( OnSpan1 ) {
          if( (ypt1 <= ypt1n && ypt1 > ypt2n) ||
              (ypt1 >= ypt1n && ypt1 < ypt2n) ) {
            Intersection = true;
            *newx = xpt1;
            *newy = ypt1;
          }
        }
        else if( OnSpan2 ) {
          if( (ypt1n <= ypt1 && ypt1n > ypt2) ||
              (ypt1n >= ypt1 && ypt1n < ypt2) ) {
            Intersection = true;
            *newx = xpt1n;
            *newy = ypt1n;
          }
        }
      }
      else {
        if( OnSpan1 && OnSpan2 ) {
          if( ((xpt1 <= xpt1n && xpt1 > xpt2n) ||
               (xpt1 >= xpt1n && xpt1 < xpt2n)) &&
              ((xpt1n <= xpt1 && xpt1n > xpt2) ||
               (xpt1n >= xpt1 && xpt1n < xpt2)) ) {
            Intersection = true;
            *newx = xpt1;
            *newy = ypt1;
          }
        }
        else if( OnSpan1 ) {
          if( (xpt1 <= xpt1n && xpt1 > xpt2n) ||
              (xpt1 >= xpt1n && xpt1 < xpt2n) ) {
            Intersection = true;
            *newx = xpt1;
            *newy = ypt1;
          }
        }
        else if( OnSpan2 ) {
          if( (xpt1n <= xpt1 && xpt1n > xpt2) ||
              (xpt1n >= xpt1 && xpt1n < xpt2) ) {
            Intersection = true;
            *newx = xpt1n;
            *newy = ypt1n;
          }
        }
      }
    }
  }
  else {
    if( xdiff1 != 0.0 && xdiff2 != 0.0 ) {
      xcommon = (ycept2 - ycept1) / (slope1 - slope2);
      *newx = xcommon;
      ycommon = ycept1 + slope1 * xcommon;
      *newy = ycommon;
      if( OnSpan1 && OnSpan2 ) {
        if( (*newx >= xpt1 && *newx <= xpt2) ||
            (*newx <= xpt1 && *newx >= xpt2) ) {
          if( (*newx >= xpt1n && *newx <= xpt2n) ||
              (*newx <= xpt1n && *newx >= xpt2n) )
            Intersection = true;
        }
      }
      else if( OnSpan1 ) {
        if( (*newx >= xpt1 && *newx <= xpt2) ||
            (*newx <= xpt1 && *newx >= xpt2) )
          Intersection = true;
      }
      else if( OnSpan2 ) {
        if( (*newx >= xpt1n && *newx <= xpt2n) ||
            (*newx <= xpt1n && *newx >= xpt2n) )
          Intersection = true;
      }
    }
    else {
      if( xdiff1 == 0.0 && xdiff2 != 0.0 ) {
        ycommon = slope2 * xpt1 + ycept2;
        *newx = xpt1;
        *newy = ycommon;
        if( OnSpan1 && OnSpan2 ) {
          if( (*newy >= ypt1 && *newy <= ypt2) ||
              (*newy <= ypt1 && *newy >= ypt2) ) {
            if( (*newx >= xpt1n && *newx <= xpt2n) ||
                (*newx <= xpt1n && *newx >= xpt2n) )
              Intersection = true;
          }
        }
        else if( OnSpan1 ) {
          if( (*newy >= ypt1 && *newy <= ypt2) ||
              (*newy <= ypt1 && *newy >= ypt2) )
            Intersection = true;
        }
        else if( OnSpan2 ) {
          if( (*newx >= xpt1n && *newx <= xpt2n) ||
              (*newx <= xpt1n && *newx >= xpt2n) )
            Intersection = true;
        }
      }
      else {
        if( xdiff1 != 0.0 && xdiff2 == 0.0 ) {
          ycommon = slope1 * xpt1n + ycept1;
          *newx = xpt1n;
          *newy = ycommon;
          if( OnSpan1 && OnSpan2 ) {
            if( (*newy >= ypt1n && *newy <= ypt2n) ||
                (*newy <= ypt1n && *newy >= ypt2n) ) {
              if( (*newx >= xpt1 && *newx <= xpt2) ||
                  (*newx <= xpt1 && *newx >= xpt2) )
                Intersection = true;
            }
          }
          else if( OnSpan1 ) {
            if( (*newx >= xpt1 && *newx <= xpt2) ||
                (*newx <= xpt1 && *newx >= xpt2) )
              Intersection = true;
          }
          else if( OnSpan2 ) {
            if( (*newy >= ypt1n && *newy <= ypt2n) ||
                (*newy <= ypt1n && *newy >= ypt2n) )
              Intersection = true;
          }
        }
      }
    }
  }

  return Intersection;
} //PostFrontal::Cross

//============================================================================
void PostFrontal::AreaPerim( long NumPoints, double* verts, double* area,
                             double* perimeter )
{ //PostFrontal::AreaPerim
  //Calculates area and perimeter as a planimeter (with triangles)

  long   i, j;
  double xpt1, ypt1, xpt2, ypt2, aangle, zangle;
  double newarea, xdiff, ydiff, DiffAngle, hdist;

  *area = 0.0;
  *perimeter = 0.0;
  if( NumPoints > 0 ) {
    StartX = verts[0];     //Must use old perim array
    StartY = verts[1];     //New array not merged or clipped yet
    j = 0;
    while( j < NumPoints ) {
      j++;
      xpt1 = verts[j * 2];
      ypt1 = verts[j * 2 + 1];
      zangle = Direction(xpt1, ypt1);      //Reference angle
      if( zangle != 999.9 ) break;  //Make sure that startx,starty!=x[0]y[0]
    }
    xdiff = xpt1 - StartX;
    ydiff = ypt1 - StartY;
    hdist = sqrt(pow2(xdiff) + pow2(ydiff));
    *perimeter = hdist;
    j++;
    for( i = j; i < NumPoints; i++ ) {
      xpt2 = verts[i * 2];
      ypt2 = verts[i * 2 + 1];
      xdiff = xpt2 - xpt1;
      ydiff = ypt2 - ypt1;
      hdist = sqrt( pow2(xdiff) + pow2(ydiff) );
      *perimeter += hdist;
      newarea = .5 * ( StartX * ypt1 - xpt1 * StartY + xpt1 * ypt2 -
                       xpt2 * ypt1 + xpt2 * StartY - StartX * ypt2 );
      newarea = fabs( newarea );
      aangle = Direction( xpt2, ypt2 );
      if( aangle != 999.9 ) {
        DiffAngle = aangle - zangle;
        if( DiffAngle > PI ) DiffAngle = -( 2.0 * PI - DiffAngle );
        else if( DiffAngle < -PI ) DiffAngle = ( 2.0 * PI + DiffAngle );

        if( DiffAngle > 0.0 ) *area -= newarea;
        else if( DiffAngle < 0.0 ) *area += newarea;
        zangle = aangle;
      }
      xpt1 = xpt2;
      ypt1 = ypt2;
    }
  }
} //PostFrontal::AreaPerim

//============================================================================
double PostFrontal::Direction( double xpt, double ypt )
{ //PostFrontal::Direction
  //Calculates sweep direction for angle determination.

  double zangle = 999.9, xdiff, ydiff;

  xdiff = xpt - StartX;
  ydiff = ypt - StartY;
  if( fabs(xdiff) < 1e-9 ) xdiff = 0.0;
  if( fabs(ydiff) < 1e-9 ) ydiff = 0.0;

  if( xdiff != 0.0 ) {
    zangle = atan( ydiff / xdiff );
    if( xdiff > 0.0 ) zangle = ( PI / 2.0 ) - zangle;
    else zangle = ( 3.0 * PI / 2.0 ) - zangle;
  }
  else {
    if( ydiff >= 0.0 ) zangle = 0;
    else if( ydiff < 0.0 ) zangle = PI;
  }

  return zangle;
} //PostFrontal::Direction

//============================================================================
long PostFrontal::Overlap( long NumPoints, double* verts )
{ //PostFrontal::Overlap
  //Determines if point is inside or outside a fire polygon defined by verts.

  long   i, j, k, inside = 0;
  double Aangle = 0.0, Bangle;
  double CuumAngle = 0.0, DiffAngle;
  double xpt, ypt;

  j = 0;
  while( j < NumPoints ) {   //Make sure that startx,starty!=x[0]y[0]
    xpt = verts[j * 2];
    ypt = verts[j * 2 + 1];
    Aangle = Direction( xpt, ypt );
    j++;
    if( Aangle != 999.9 ) break;
  }

  for( i = j; i <= NumPoints; i++ ) {
    if( i == NumPoints ) k = j - 1;
    else k = j;
    xpt = verts[k * 2];
    ypt = verts[k * 2 + 1];
    Bangle = Direction( xpt, ypt );
    if( Bangle != 999.9 ) {
      DiffAngle = Bangle - Aangle;
      if( DiffAngle > PI ) DiffAngle = -( 2.0 * PI - DiffAngle );
      else if( DiffAngle < -PI ) DiffAngle = ( 2.0 * PI + DiffAngle );
      CuumAngle -= DiffAngle;
      Aangle = Bangle;
    }
  }

  //If absolute value of CuumAngle is>PI then point is inside polygon.
  if( fabs(CuumAngle) > PI ) inside = 1;

  return inside;
} //PostFrontal::Overlap

//============================================================================
FireRing* PostFrontal::SpawnFireRing( long NumFire, long NumAlloc,
                                      double Start, double Elapsed )
{ //PostFrontal::SpawnFireRing
  //Loads first information into new fire ring structure.
  long   i, NumPoints;
  double x, y;
  FireRing* firering;

  NumPoints = GetNumPoints( NumFire );
  if( (firering = AllocFireRing(NumAlloc, Start, Elapsed + Start)) == NULL )
    return NULL;

  firering->OriginalFireNumber = NumFire;
  firering->NumPoints[0] = NumPoints;

  for( i = 0; i < NumPoints; i++ ) {
    x = GetPerimeter1Value( NumFire, i, XCOORD );
    y = GetPerimeter1Value( NumFire, i, YCOORD );
    firering->perimpoints[i].x2 = x;
    firering->perimpoints[i].y2 = y;
    firering->perimpoints[i].x1 = x;
    firering->perimpoints[i].y1 = y;
    firering->perimpoints[i].SizeMult = 1.0;
    firering->perimpoints[i].Area = -1.0;
    firering->perimpoints[i].Status = 0;
    firering->perimpoints[i].l1 = firering->perimpoints[i].l2 =
                                  firering->perimpoints[i].h = 0.0;
  }

  ReferenceFireRingNum = NumRings;    //Store for CorrectFireRing

  return firering;
} //PostFrontal::SpawnFireRing

//============================================================================
void PostFrontal::MergeFireRings( long* Fires, long NumPerims, long* isects,
                                  double* ipoints, long NumIsects,
                                  long CurNumPts )
{ //PostFrontal::MergeFireRings
  //Corrects a fire ring after merging of two fires by comparing the
  //post-merger outer perimeter with the pre-merger outer perimeters of the
  //two fires, Fire1 and Fire2. Once the same points have been found, and
  //intersection points have been added to the pre-merger perimeters, both
  //pre-merger perimeters are copied into the structure for Fire1. Fire2 is
  //eliminated. Fire1 is then processed through the interpolation routine to
  //apportion the overlapping area among the points that define its edge.
  //If a vertex is already in an overlap and again falls within one of these
  //new overlaps, it's area influence factor (SizeMult) is reduced to zero
  //(a point can only be in 1 overlap at a time).

  bool   Reverse = false;
  long   i, j, k, m, n, p, s, SumStatus, FireType, NumPoints;
  long   Start, End, First, StartPt, NumFire, FirstCross, TotalPts,
         TotalFires;
  double xold, yold, xpt, ypt;
  FireRing* testring, * ring[2] = {0, 0}, * newring;
  PerimPoints* tempperim;
  MergePoints* tempmerge;
  long*  tempnum, TempMergeAlloc;

  //Find the correct fires.
  for( i = MergeReferenceRingNum; i < GetNumRings(); i++ ) {
    testring = GetRing( i );
    if( ! ring[0] ) {
      if( Fires[0] == testring->OriginalFireNumber ) {
        ring[0] = testring;
        continue;
      }
    }   							  	
    if( ! ring[1] ) {
      if( Fires[1] == testring->OriginalFireNumber )
        ring[1] = testring;
    }
    if( ring[0] && ring[1] ) break;
    else if( i == GetNumRings() - 1 ) {
      ring[0] = 0;
      ring[1] = 0;
    }
  }

  for( i = 0; i < 2; i++ ) {
    if( ring[i]->NumMergePoints[0] == 0 ) {
      //Fix up firering because of new points added in
      //Intersect::OrganizeCrosses.
      //If point number is correct, then only check order of firering.
      if( GetNumPoints(ring[i]->OriginalFireNumber) != ring[i]->NumPoints[0])
        UpdateFireRing( ring[i], ring[i]->OriginalFireNumber,
                        GetNumPoints(ring[i]->OriginalFireNumber) );
      else UpdatePointOrder( ring[i], ring[i]->OriginalFireNumber );
    }
    FillMergeArray( ring[i] );
  }

  //Reorder ring[0] && ring[1] to match current order of Fire1 and Fire2.
  UpdateMergeOrder( ring[0], Fires[0] );
  UpdateMergeOrder( ring[1], Fires[1] );

  //Allocate temporary data structures.
  TempMergeAlloc = ( ring[0]->NumMergePoints[0] + ring[1]->NumMergePoints[0] +
                     NumIsects * 2 );
  tempmerge = new MergePoints[TempMergeAlloc];
  memset( tempmerge,0x0, (ring[0]->NumMergePoints[0] +
          ring[1]->NumMergePoints[0] +NumIsects * 2) * sizeof(MergePoints) );
  TotalPts = 0;
  for( i = 0; i < 2; i++ ) {
    for( j = 0; j < ring[i]->NumFires; j++ )
      TotalPts += ring[i]->NumPoints[j];
  }
  tempperim = new PerimPoints[TotalPts + NumIsects * 2];

  memset( tempperim,0x0, (TotalPts + NumIsects * 2) * sizeof(PerimPoints) );

  //Insert intersection points into mergepoint array.
  TotalPts = 0;
  double temp[5];
  long   q1, q2;

  for( i = 0; i < 2; i++ ) {    //For each ring
    switch( i ) {
      case 0:
        NumFire = Fires[0];
        break;
      case 1:
        NumFire = Fires[1];
        if( isects[1] > isects[((NumIsects - 1) * 2) + 1] ) {  //Reverse 'em
          Reverse = true;
          for( j = 0; j < NumIsects / 2; j++ ) {
            m = isects[j * 2 + 1];
            isects[j * 2 + 1] = isects[(NumIsects - j) * 2 - 1];
            isects[(NumIsects - j) * 2 - 1] = m;
            q1 = j * 5;
            q2 = (NumIsects - j) * 5 - 5;
            memcpy( temp, &ipoints[q1], 5 * sizeof(double) );
            memcpy( &ipoints[q1], &ipoints[q2], 5 * sizeof(double) );
            memcpy( &ipoints[q2], temp, 5 * sizeof(double) );
          }
        }
        break;
    }

    Start = 0;
    for( j = 0; j < NumIsects; j++ ) {
      FirstCross = isects[j * 2 + i];
      for( k = Start; k <= FirstCross; k++ )
        memcpy( &tempmerge[TotalPts++], &ring[i]->mergepoints[k],
                sizeof(MergePoints) );
      if( FirstCross >= Start )
        Start = FirstCross + 1;
      tempmerge[TotalPts].Status = 2;
      tempmerge[TotalPts].x = ipoints[j * 5];
      tempmerge[TotalPts].y = ipoints[j * 5 + 1];
      tempmerge[TotalPts].FireID = -1;    //Flag as unknown for now
      tempmerge[TotalPts].VertexID = -(NumFire + 1);  //Flag as neg # of fire
      TotalPts++;
    }
    for( j = Start; j < ring[i]->NumMergePoints[0]; j++ )
      memcpy( &tempmerge[TotalPts++], &ring[i]->mergepoints[j],
              sizeof(MergePoints) );
  }

  delete[] ring[0]->mergepoints;
  ring[0]->mergepoints = new MergePoints[TotalPts];
  memcpy( ring[0]->mergepoints, tempmerge, TotalPts * sizeof(MergePoints) );
  ring[0]->NumMergePoints[0] += NumIsects;
  ring[0]->NumMergePoints[1] = ring[0]->NumMergePoints[0] +
                               ring[1]->NumMergePoints[0] + NumIsects;
  delete[] ring[1]->mergepoints;
  ring[1]->mergepoints = 0;
  ring[1]->NumMergePoints[0] = 0;

  if( Reverse ) {    //Need to restore original order
    for( j = 0; j < NumIsects / 2; j++ ) {
      m = isects[j * 2 + 1];
      isects[j * 2 + 1] = isects[(NumIsects - j) * 2 - 1];
      isects[(NumIsects - j) * 2 - 1] = m;
      q1 = j * 5;
      q2 = (NumIsects - j) * 5 - 5;
      memcpy( temp, &ipoints[q1], 5 * sizeof(double) );
      memcpy( &ipoints[q1], &ipoints[q2], 5 * sizeof(double) );
      memcpy( &ipoints[q2], temp, 5 * sizeof(double) );
    }
  }

  //Now insert intersection points into ring[0] and ring[1] arrays and store
  //together in tempperim.
  TotalFires = ring[0]->NumFires + ring[1]->NumFires;
  tempnum = new long[TotalFires];
  memset( tempnum,0x0, TotalFires * sizeof(long) );
  TotalFires = 0;
  for( i = 0; i < 2; i++ ) {    //Initialize numpoints
    for( j = 0; j < ring[i]->NumFires; j++ ) {
      tempnum[TotalFires] = ring[i]->NumPoints[j];
      if( i > 0 )
        tempnum[TotalFires] += ring[0]->NumPoints[ring[0]->NumFires - 1];
      TotalFires++;
    }
  }

  TotalPts = 0;
  for( i = 0; i < 2; i++ ) {
    memcpy( &tempperim[TotalPts], &ring[i]->perimpoints[0],
            ring[i]->NumPoints[ring[i]->NumFires - 1] * sizeof(PerimPoints) );
    TotalPts += ring[i]->NumPoints[ring[i]->NumFires - 1];
  }

  for( i = 0; i < 2; i++ ) {    //For each merged ring
    if( i == 0 ) {
      j = 0;
      Start = 0;
    }
    else {
      j = ring[0]->NumMergePoints[0];
      Start = tempnum[ring[0]->NumFires - 1];
    }
    for( k = j; k < ring[0]->NumMergePoints[i]; k++ ) {
      if( ring[0]->mergepoints[k].Status == 2 ) {  //If new intersection point
        x2 = ring[0]->mergepoints[k].x;
        y2 = ring[0]->mergepoints[k].y;
        s = k - 1;
        if( i == 0 ) {
          if( s < 0 )
            s = ring[0]->NumMergePoints[0] - 1;
        }
        else if( s < ring[0]->NumMergePoints[0] )
          s = ring[0]->NumMergePoints[1] - 1;

        xpt = ring[0]->mergepoints[s].x;
        ypt = ring[0]->mergepoints[s].y;
        p = 0;
        for( m = Start; m < TotalPts; m++ ) {
          xm2 = tempperim[m].x2;
          ym2 = tempperim[m].y2;
          if( m >= tempnum[p] ) p++;
          if( (pow2(xpt - xm2) + pow2(ypt - ym2)) < 1e-9 ) { //Point same
            if( m < TotalPts - 1 )
              memmove( &tempperim[m + 2], &tempperim[m + 1],
                       (TotalPts - m - 1) * sizeof(PerimPoints) );
            TotalPts++;
            for( n = 0; n < TotalFires; n++ ) {
              if( m < tempnum[n] )
                tempnum[n]++;  //Increase number of points in fire
            }
            tempperim[m + 1].x2 = x2;
            tempperim[m + 1].y2 = y2;
            n = m + 2;
            if( n > tempnum[p] - 1 ) {
              if( p == 0 ) n = 0;
              else n = tempnum[p - 1];
            }
            xn2 = tempperim[n].x2;
            yn2 = tempperim[n].y2;
            xm1 = tempperim[m].x1;
            ym1 = tempperim[m].y1;
            xn1 = tempperim[n].x1;
            yn1 = tempperim[n].y1;

            if( fabs(xn2 - xm2) > 1e-9 )
              x1 = xn1 - (xn1 - xm1) * (xn2 - x2) / (xn2 - xm2);
            else x1 = xn1;
            if( fabs(yn2 - ym2) > 1e-9 )
              y1 = yn1 - (yn1 - ym1) * (yn2 - y2) / (yn2 - ym2);
            else y1 = yn1;
            tempperim[m + 1].x1 = x1;
            tempperim[m + 1].y1 = y1;
            tempperim[m + 1].SizeMult = 1.0;
            tempperim[m + 1].Status = 2;
            tempperim[m + 1].Area = 0.0;
            tempperim[m + 1].l1 = tempperim[m + 1].l2 =
                                  tempperim[m + 1].h = -1;
            tempperim[m + 1].hist = tempperim[n].hist;
            break;
          }
        }
      }
    }
  }

  delete[] ring[0]->perimpoints;
  ring[0]->perimpoints = new PerimPoints[TotalPts];
  memcpy( ring[0]->perimpoints, tempperim, TotalPts * sizeof(PerimPoints) );

  //Aggregate descriptive data for new ring[0], incl. numfires, numpts etc.

  ring[0]->NumFires += ring[1]->NumFires;
  delete[] ring[0]->NumPoints;
  ring[0]->NumPoints = new long[ring[0]->NumFires];
  for( i = 0; i < ring[0]->NumFires; i++ )
    ring[0]->NumPoints[i] = tempnum[i];
  delete[] tempnum;
  delete[] tempperim;
  delete[] ring[1]->perimpoints;
  ring[1]->perimpoints = 0;
  delete[] ring[1]->NumPoints;
  ring[1]->NumPoints = 0;

  //Update mergfire array with correct vertexnumbers and fire numbers;

  for( i = 0; i < 2; i++ ) {    //For each set of fires
    switch( i ) {
      case 0:
        Start = 0;
        End = ring[0]->NumFires - ring[1]->NumFires;
        StartPt = 0;
        break;
      case 1:
        Start = End;
        End = ring[0]->NumFires;
        StartPt = ring[0]->NumMergePoints[0];
        break;
    }

    //For each mergepoint.
    for( j = StartPt; j < ring[0]->NumMergePoints[i]; j++ ) {
      x2 = ring[0]->mergepoints[j].x;
      y2 = ring[0]->mergepoints[j].y;
      for( k = Start; k < End; k++ ) {    //For each perimfire
        if( k == 0 ) First = 0;
        else First = ring[0]->NumPoints[k - 1];
        for( m = First; m < ring[0]->NumPoints[k]; m++ ) {
          xn2 = ring[0]->perimpoints[m].x2;
          yn2 = ring[0]->perimpoints[m].y2;
          if( pow2(x2 - xn2) + pow2(y2 - yn2) < 1e-10 ) { //Test for same pt
            ring[0]->mergepoints[j].Status -= (short) 4;    //Flag it
            ring[0]->mergepoints[j].FireID = k;
            ring[0]->mergepoints[j].VertexID = m;
            k = End;
            break;
          }
        }
      }
    }
  }

  for( i = 0; i < ring[0]->NumMergePoints[1]; i++ ) {
    if( ring[0]->mergepoints[i].Status < 0 )
      ring[0]->mergepoints[i].Status += (short) 4;    //Unflag it
    else ring[0]->mergepoints[i].Status = 0;        //Only for debugging
  }

  //Relabel status of mergepoints by comparing with current perimeter2 array.
  for( m = 1; m < NumPerims; m++ ) {
    NumFire = Fires[m];
    if( m < 2 ) {
      FireType = 2;
      NumPoints = CurNumPts;
      newring = 0;
    }
    else {
      FireType = 1;
      NumPoints = GetNumPoints( NumFire );
      newring = SpawnFireRing( NumFire, NumPoints, ring[0]->StartTime,
                               ring[0]->ElapsedTime );
    }
    Start = 0;
    for( i = 0; i < NumPoints; i++ ) {
      xpt = GetPerimVal( FireType, NumFire, i, XCOORD );
      ypt = GetPerimVal( FireType, NumFire, i, YCOORD );
      for( j = abs(Start); j < 2; j++ ) {
        StartPt = 0;
        if( j > 0 ) StartPt = ring[0]->NumMergePoints[0];
        for( k = StartPt; k < ring[0]->NumMergePoints[j]; k++ ) {
          xold = ring[0]->mergepoints[k].x;
          yold = ring[0]->mergepoints[k].y;
          if( pow2(xpt - xold) + pow2(ypt - yold) < 1e-10 ) {
            if( m == 0 && ring[0]->mergepoints[k].Status == 2 ) {
              Start = abs(Start) - 1;    //Alternate fires for searching
              j = 2;
              break;
            }
            if( newring ) {
              n = ring[0]->mergepoints[k].VertexID;
              newring->perimpoints[i].x1 = ring[0]->perimpoints[n].x1;
              newring->perimpoints[i].y1 = ring[0]->perimpoints[n].y1;
              newring->perimpoints[i].hist = ring[0]->perimpoints[n].hist;
              if( ring[0]->mergepoints[k].Status != 2 ) {
                ring[0]->perimpoints[n].Status = -1;
                ring[0]->perimpoints[n].Area = -1;
                ring[0]->perimpoints[n].SizeMult = 1.0;
                ring[0]->mergepoints[k].Status = -1;
              }
            }
            else ring[0]->mergepoints[k].Status = 1;

            j = 2;  //Force exit from loop
            break;
          }
        }
      }
    }

    if( newring ) FillMergeArray( newring );
  }

  //Re-order arrays if 1st point is inside.
  SumStatus = 0;
  for( i = 0; i < ring[0]->NumMergePoints[0]; i++ ) {
    if( ring[0]->mergepoints[i].Status < 2 )
      SumStatus += abs( ring[0]->mergepoints[i].Status );
    else {
      if( SumStatus < i ) {    //There was a zero status perim point
        for( j = 0; j < ring[0]->NumMergePoints[0]; j++ ) {
          tempmerge[j].Status = ring[0]->mergepoints[j].Status;
          tempmerge[j].FireID = ring[0]->mergepoints[j].FireID;
          tempmerge[j].VertexID = ring[0]->mergepoints[j].VertexID;
        }
        m = 0;    //New-order point counter
        for( j = ring[0]->NumMergePoints[0] - 1; j > (-1); j-- ) {
          if( ring[0]->mergepoints[j].Status == 2 ) {
            for( k = j; k < ring[0]->NumMergePoints[0]; k++ )
              memcpy( &ring[0]->mergepoints[m++], &tempmerge[k],
                      sizeof(MergePoints) );
            for( k = 0; k < j; k++ )
              memcpy( &ring[0]->mergepoints[m++],
                      &tempmerge[k], sizeof(MergePoints) );
            break;
          }
        }
        break;    //Reordering done, get out
      }
      else break;
    }
  }

  PartitionMergeRing( ring[0] );
  if( NumPerims > 2 ) RemoveRingEnclaves( ring[0] );

  //Update mergepoints array with exact points in perim2 array for outer fire.
  if( CurNumPts > TempMergeAlloc ) {
    delete[] tempmerge;
    tempmerge = new MergePoints[CurNumPts];
  }
  if( CurNumPts > ring[0]->NumMergePoints[1] ) {
    delete[] ring[0]->mergepoints;
    ring[0]->mergepoints = new MergePoints[CurNumPts];
  }
  for( i = 0; i < CurNumPts; i++ ) {
    xpt = GetPerimVal( 2, 0, i, XCOORD );
    ypt = GetPerimVal( 2, 0, i, YCOORD );
    for( j = 0; j < ring[0]->NumMergePoints[1]; j++ ) {
      xold = ring[0]->mergepoints[j].x;
      yold = ring[0]->mergepoints[j].y;
      if( pow2(xpt - xold) + pow2(ypt - yold) < 1e-10 ) {
        tempmerge[i].x = xpt;
        tempmerge[i].y = ypt;
        tempmerge[i].Status = 0;
        tempmerge[i].FireID = 0;
        tempmerge[i].VertexID = ring[0]->mergepoints[i].VertexID;
        break;
      }
    }
  }

  memcpy( ring[0]->mergepoints, tempmerge, CurNumPts * sizeof(MergePoints) );
  delete[] tempmerge;
  ring[1]->OriginalFireNumber = -1;
  ring[0]->NumMergePoints[0] = CurNumPts;
  ring[0]->NumMergePoints[1] = 0;
} //PostFrontal::MergeFireRings

//============================================================================
void PostFrontal::PartitionMergeRing( FireRing* firering )
{ //PostFrontal::PartitionMergeRing
  //Goes through list of intersections and pairs-up those that define an
  //overlapping area, either a single loop or a double loop (the only two
  //kinds). Once IDd, the intersections are copied to the tempperim array and
  //processed through the InterpolateOverlaps function that determines the
  //fractional area occupied by each quadrilateral relative to the actual area
  //of the overlapped area this will be used to reduce the post-frontal
  //combustion later on.

  bool   Exit, TestIt;
  long   i, j, k, m[2] = {0, 0}, n, o, p, q, r;
  long   Start[2];
  long   End[2] = { 0, 0 };
  long   Num[2] = { 0, 0 };
  long   CurStat, StoreCount;
  double xa[2], ya[2], xb[2], yb[2];

  PerimPoints* tempperim;

  Exit = false;
  Start[0] = 0;
  Start[1] = firering->NumMergePoints[0];
  do {
    for( k = 0; k < 2; k++ ) {
      for( i = Start[k]; i < firering->NumMergePoints[k]; i++ ) {
        do {    //Loop until beginning of overlapped section
          CurStat = firering->mergepoints[i++].Status;
          m[k]++;
          if( i > firering->NumMergePoints[k] - 1 ) {
            Exit = true;
            break;
          }
          if( CurStat == 2 && k == 1 ) {
            x1 = firering->mergepoints[i - 1].x;
            y1 = firering->mergepoints[i - 1].y;
            if( pow2(x1 - xb[0]) + pow2(y1 - yb[0]) < 1e-10 ) break;
            else CurStat = 1;
          }
        } while( CurStat < 2 );

        if( Exit ) break;

        firering->mergepoints[i - 1].Status -= (short) 4;
        Start[k] = i - 1;
        Num[k] = 1;    //Needs to be 2 because j==i

        if( k == 1 ) i = firering->NumMergePoints[0];
        for( j = i; j < firering->NumMergePoints[k]; j++ ) {
          CurStat = firering->mergepoints[j].Status;
          m[k]++;
          Num[k]++;
          if( CurStat == 2 ) {
            if( k == 0 ) break;
            else {
              x1 = firering->mergepoints[j].x;
              y1 = firering->mergepoints[j].y;
              if( pow2(x1 - xa[0]) + pow2(y1 - ya[0]) < 1e-10 ) break;
              else CurStat = 1;
            }
          }
        }
        if( j >= firering->NumMergePoints[k] )
          End[k] = firering->NumMergePoints[k] - 1;  //Debugging
        End[k] = j;
        firering->mergepoints[j].Status -= (short) 4;
        xa[k] = firering->mergepoints[Start[k]].x;
        ya[k] = firering->mergepoints[Start[k]].y;
        xb[k] = firering->mergepoints[End[k]].x;
        yb[k] = firering->mergepoints[End[k]].y;
        break;
      }
    }

    if( Exit ) break;

    i = End[1] + 1;
    if( i > firering->NumMergePoints[1] - 1 )
      i = firering->NumMergePoints[0];
    if( firering->mergepoints[i].Status == 0 ) {
      Num[1] = Start[1] - End[1] + 1;
      k = Start[1];
      Start[1] = End[1];
      End[1] = k;
    }
    else if( Start[1] > End[1] ) {
      Num[1] = firering->NumMergePoints[1] - Start[1] +
               ( End[1] - firering->NumMergePoints[0] ) + 1;
      k = Start[1];
      Start[1] = End[1];
      End[1] = k;
    }
    else Num[1] = End[1] - Start[1] + 1;

    tempperim = new PerimPoints[Num[0] + Num[1]];
    for( StoreCount = 0; StoreCount < Num[0]; StoreCount++ ) {
      k = StoreCount + Start[0];    //Must be start
      if( k > firering->NumMergePoints[0] - 1 )
        k -= firering->NumMergePoints[0];
      n = firering->mergepoints[k].VertexID;
      memcpy( &tempperim[StoreCount], &firering->perimpoints[n],
              sizeof(PerimPoints) );
    }
    for( StoreCount = 0; StoreCount < Num[1]; StoreCount++ ) {
      k = StoreCount + Start[1];
      if( k > firering->NumMergePoints[1] - 1 ) {
        k -= firering->NumMergePoints[1];
        k += firering->NumMergePoints[0];
      }
      n = firering->mergepoints[k].VertexID;
      memcpy( &tempperim[Num[0] + StoreCount], &firering->perimpoints[n],
              sizeof(PerimPoints) );
    }
    InterpolateOverlaps( tempperim, Num[0] + Num[1] );
    for( StoreCount = 0; StoreCount < Num[0]; StoreCount++ ) {
      k = StoreCount + Start[0];    //Must be start
      if( k > firering->NumMergePoints[0] - 1 )
        k -= firering->NumMergePoints[0];
      n = firering->mergepoints[k].VertexID;
      if( tempperim[StoreCount].SizeMult < 1e-6 )
        tempperim[StoreCount].SizeMult = 1e-6;
      else if( tempperim[StoreCount].SizeMult > 1.0 )
        tempperim[StoreCount].SizeMult = 1.0;
        firering->perimpoints[n].SizeMult = tempperim[StoreCount].SizeMult;
        firering->perimpoints[n].Status = tempperim[StoreCount].Status;
    }
    for( StoreCount = 0; StoreCount < Num[1]; StoreCount++ ) {
      k = StoreCount + Start[1];
      if( k > firering->NumMergePoints[1] - 1 ) {
        k -= firering->NumMergePoints[1];
        k += firering->NumMergePoints[0];
      }
      n = firering->mergepoints[k].VertexID;
      if( tempperim[Num[0] + StoreCount].SizeMult < 1e-6 )
        tempperim[Num[0] + StoreCount].SizeMult = 1e-6;
      else if( tempperim[Num[0] + StoreCount].SizeMult > 1.0 )
        tempperim[Num[0] + StoreCount].SizeMult = 1.0;
      firering->perimpoints[n].SizeMult =
                                      tempperim[Num[0] + StoreCount].SizeMult;
      firering->perimpoints[n].Status =
                                      tempperim[Num[0] + StoreCount].Status;
    }

    //Find other points inside of overlap and set to zero influence.
    for( o = 0; o < firering->NumPoints[firering->NumFires - 1]; o++ ) {
      TestIt = true;
      if( firering->perimpoints[o].SizeMult < 1.0 ) {
        for( p = 0; p < 2; p++ ) {
          for( q = 0; q < Num[p]; q++ ) {
            r = q + Start[p];
            if( r > firering->NumMergePoints[1] - 1 ) break;
            n = firering->mergepoints[r].VertexID;
            if( o == n ) {
              TestIt = false;
              break;
            }
          }
          if( ! TestIt ) break;
        }
        if( TestIt ) {
          StartX = firering->perimpoints[o].x2;
          StartY = firering->perimpoints[o].y2;
          if( Overlap(Num[0] + Num[1] + 4, Verts) )
            firering->perimpoints[o].SizeMult = 0.0;
        }
      }
    }
    Start[0] = End[0];
    if( tempperim ) delete[] tempperim;
    if( Verts ) delete[] Verts;
  } while( m[0] < firering->NumMergePoints[0] );
} //PostFrontal::PartitionMergeRing

//============================================================================
double PostFrontal::GetPerimVal( long PerimLocation, long NumFire,
                                 long PointNum, long TYPE )
{ //PostFrontal::GetPerimVal
  //Wraps the global get-functions for fire perimeters.
  double value;

  switch( PerimLocation ) {
    case 1:    //New inward burning fire, so look in perimeter1
      value = GetPerimeter1Value( NumFire, PointNum, TYPE );
      break;
    case 2:    //External fire perimeter, so look on perimeter2
      value = GetPerimeter2Value( PointNum, TYPE );
      break;
    }

    return value;
} //PostFrontal::GetPerimVal

//============================================================================
double PostFrontal::ComputeSmoke( double CurrentTime, long Species )
{ //PostFrontal::ComputeSmoke
  return 0.0;
} //PostFrontal::ComputeSmoke

//============================================================================
double PostFrontal::ComputeHeat( double CurrentTime )
{ //PostFrontal::ComputeHeat
  return 0.0;
} //PostFrontal::ComputeHeat

//============================================================================
void PostFrontal::GetRingPoint( FireRing* ring, long PointNum )
{ //PostFrontal::GetRingPoint
  x1 = ring->perimpoints[PointNum].x1;
  y1 = ring->perimpoints[PointNum].y1;
  x2 = ring->perimpoints[PointNum].x2;
  y2 = ring->perimpoints[PointNum].y2;
} //PostFrontal::GetRingPoint

//============================================================================
void PostFrontal::CloseAllThreads()
{ //PostFrontal::CloseAllThreads
  FreePFI();
} //PostFrontal::CloseAllThreads

//============================================================================
bool PostFrontal::AllocPFI()
{ //PostFrontal::AllocPFI
  if( NumPFI == GetMaxThreads() ) return true;

  CloseAllThreads();
  pfi = new PFIntegration[GetMaxThreads()];

  if( pfi ) {
    NumPFI = GetMaxThreads();
    return true;
  }

  return false;
} //PostFrontal::AllocPFI

//============================================================================
void PostFrontal::FreePFI()
{ //PostFrontal::FreePFI
  if( pfi ) delete[] pfi;
  pfi = 0;
  NumPFI = 0;
} //PostFrontal::FreePFI

//----------------------------------------------------------------------------
// Integration functions, members of PFINtegration
//----------------------------------------------------------------------------

//============================================================================
PFIntegration::PFIntegration()
{ //PFIntegration::PFIntegration
  hThread = 0;
  ThreadStarted = false;
  Begin = -1;
  End = -1;
  ring = 0;
} //PFIntegration::PFIntegration

//============================================================================
PFIntegration::~PFIntegration() {}

//============================================================================
void PFIntegration::SetRange( FireRing* rring, long firenum,
                              double currenttime, long begin, long end )
{ //PFIntegration::SetRange
  Begin = begin;
  End = end;
  ring = rring;
  CurrentTime = currenttime;
  FireNum = firenum;
} //PFIntegration::SetRange

//============================================================================
long PFIntegration::StartIntegThread( long ID )
{ //PFIntegration::StartIntegThread
  RunIntegThread( this );
  return hThread;
} //PFIntegration::StartIntegThread

//============================================================================
unsigned PFIntegration::RunIntegThread( void* pfi )
{ //PFIntegration::RunIntegThread
  static_cast <PFIntegration*>(pfi)->IntegThread();

  return 1;
} //PFIntegration::RunIntegThread

//============================================================================
void PFIntegration::IntegThread()
{ //PFIntegration::IntegThread
  ThreadStarted = true;
  do {
    if( End < 0 ) break;
    FlameWeightConsumed = SmolderWeightConsumed = 0.0;
    Integrate();

    break;
  } while (End > -1);
} //PFIntegration::IntegThread

//============================================================================
void RingBurn::CalculateArea(FireRing* ring, long pt, long ptl, long ptn)
{ //PFIntegration::IntegThread
  double Area, l1, l2, h, x, y, FeetToMeters;

  l1 = l2 = 0.0;
  x1 = ring->perimpoints[pt].x1;
  y1 = ring->perimpoints[pt].y1;
  x2 = ring->perimpoints[pt].x2;
  y2 = ring->perimpoints[pt].y2;
  GetWholePolygon( ring, ptl, false );
  GetSubPolygon( 1.0, false );
  x = pow2( b2 + d2 - a2 - c2 );
  y = 4.0 * q2 * p2;
  if( y - x > 1e-8 ) Area = 0.25 * sqrt( y - x );
  else Area = 0.0;
  if( a2 > 1e-8 ) l1 = sqrt( a2 );
  if( c2 > 1e-8 ) l2 = sqrt( c2 );
  GetWholePolygon( ring, ptn, false );
  GetSubPolygon( 1.0, false );
  x = pow2( b2 + d2 - a2 - c2 );
  y = 4.0 * q2 * p2;
  if( y - x > 1e-8 ) Area += 0.25 * sqrt( y - x );
  if( a2 > 1e-8 ) l1 += sqrt( a2 );
  if( c2 > 1e-8 ) l2 += sqrt( c2 );
  if( l1 + l2 > 0.0 ) h = (2.0 * Area) / (l1 + l2);
  else h = 0.0;

  FeetToMeters = pow2( MetricResolutionConvert() );

  ring->perimpoints[pt].Area = Area * FeetToMeters;
  ring->perimpoints[pt].l1 = l1 * MetricResolutionConvert();
  ring->perimpoints[pt].l2 = l2 * MetricResolutionConvert();
  ring->perimpoints[pt].h = h * MetricResolutionConvert();
} //PFIntegration::IntegThread

//============================================================================
void RingBurn::GetWholePolygon( FireRing* ring, long PointNum, bool Reverse )
{ //RingBurn::GetWholePolygon
  xn1 = ring->perimpoints[PointNum].x1;
  yn1 = ring->perimpoints[PointNum].y1;
  xn2 = ring->perimpoints[PointNum].x2;
  yn2 = ring->perimpoints[PointNum].y2;
  xm1 = xn1 - (xn1 - x1) / 2.0;    //Mid points on span
  ym1 = yn1 - (yn1 - y1) / 2.0;
  xm2 = xn2 - (xn2 - x2) / 2.0;
  ym2 = yn2 - (yn2 - y2) / 2.0;

  if( ! Reverse ) {
    xip = x2;
    yip = y2;
    xmip = xm2;
    ymip = ym2;
  }
  else {
    xip = x1;
    yip = y1;
    xmip = xm1;
    ymip = ym1;
  }
} //RingBurn::GetWholePolygon

//============================================================================
void RingBurn::GetSubPolygon( double end, bool Reverse )
{ //RingBurn::GetSubPolygon
  if( ! Reverse ) {
    xi = x2 - (x2 - x1) * end;
    yi = y2 - (y2 - y1) * end;
    xmi = xm2 - (xm2 - xm1) * end;
    ymi = ym2 - (ym2 - ym1) * end;
  }
  else {
    xi = x1 - (x1 - x2) * end;
    yi = y1 - (y1 - y2) * end;
    xmi = xm1 - (xm1 - xm2) * end;
    ymi = ym1 - (ym1 - ym2) * end;
  }

  a2 = pow2( xi - xmi ) + pow2( yi - ymi );
  b2 = pow2( xmi - xmip ) + pow2( ymi - ymip );
  c2 = pow2( xmip - xip ) + pow2( ymip - yip );
  d2 = pow2( xip - xi ) + pow2( yip - yi );
  p2 = pow2( xi - xmip ) + pow2( yi - ymip );
  q2 = pow2( xmi - xip ) + pow2( ymi - yip );

  xip = xi;    //For next time around
  yip = yi;
  xmip = xmi;
  ymip = ymi;
} //RingBurn::GetSubPolygon

//============================================================================
double PFIntegration::Interpolate( FireRing* ring, long j, double CurrentTime,
                                   long type )
{ //PFIntegration::Interpolate
  long     m;
  float    result;
  float    TimeNow;
  float*   xx, * yy;
  unsigned long ma, pos;

  TimeNow = (float) fabs( CurrentTime );
  if( type == 1 ) {
    xx = ring->perimpoints[j].hist.FlameCoefX;
    yy = ring->perimpoints[j].hist.FlameCoefY;
    ma = ring->perimpoints[j].hist.FlamePolyNum;
    if( TimeNow <= xx[0] ) {
      if( xx[0] < 1e-9 ) return 1.0;

      result = 1.0 - ((1.0 - yy[1]) * TimeNow / xx[1]);   //AAA FromG5 ver

      //AAAresult = 1.0 - ((1.0 - yy[0]) * TimeNow / xx[0]);  JAS! ver

      return result;
    }
    else if( TimeNow > xx[ma - 1] ) {
      if( xx[ma - 1] < ring->perimpoints[j].hist.WeightCoefX[ring->
                                      perimpoints[j].hist.WeightPolyNum - 1] )
        return 0.0;
      else return yy[ma - 1];
    }
  }
  else {
    xx = ring->perimpoints[j].hist.WeightCoefX;
    yy = ring->perimpoints[j].hist.WeightCoefY;
    ma = ring->perimpoints[j].hist.WeightPolyNum;
    if( TimeNow < xx[0] ) {
      if( xx[0]<1e-9 ) return 1.0;
      result = 1.0 - ( (1.0-yy[1])*TimeNow/xx[1] );
      return result;
    }
    else if( TimeNow > xx[ma - 1] ) return (double) yy[ma - 1];
  }

  locate( xx - 1, ma, TimeNow, &pos );

  //Linear Interpolation.
  m = ( pos - 1 );
  if( m < 0 ) m = 0;
  result = yy[m] -
            ( yy[m] - yy[m + 1]) * ((xx[m] - TimeNow) / (xx[m] - xx[m + 1]) );

  if( result > 1.0000000000 ) result = 1.0000000000;

  return (double) result;
} //PFIntegration::Interpolate

//============================================================================
void PFIntegration::locate( float xx[], unsigned long n, float x,
                            unsigned long* j )
{ //PFIntegration::locate
  unsigned long ju, jm, jl;
  long     ascnd;

  jl = 0;
  ju = n + 1;
  ascnd = (xx[n] > xx[1]);
  while( (ju - jl) > 1 ) {
    jm = (ju + jl) >> 1;
    if( (x > xx[jm]) == ascnd ) jl = jm;
    else ju = jm;
  }

  *j = jl;
} //PFIntegration::locate

//============================================================================
double PFIntegration::CalculateTemporalIntegrationSteps( FireRing* ring,
                                                         long j,
                                                         double* CurrentTime,
                                                         long* NumCalcs,
                                                         double* BaseFract )
{ //PFIntegration::CalculateTemporalIntegrationSteps
  double TimeInc = 0.0;
  double Tolerance = WeightLossErrorTolerance( GETVAL );
  double ct, et, st, s_tt, e_tt;
  double t1 = 0.0, t2;
  double wt1, wt2, wtest, wtmid;

  ct = *CurrentTime;
  st = ring->StartTime;
  et = st + ring->ElapsedTime;
  s_tt = st + ring->perimpoints[j].hist.TotalTime;
  e_tt = et + ring->perimpoints[j].hist.TotalTime;

  if( ct - et > 1e-9 ) {    //Later than endtime of quadrangle
    if( ct <= s_tt ) {
      //Ct occurs before burnout finished at startpoint of quadrangle.
      *BaseFract = 1.0;
      t1 = ct - et;     //Most recent time
      t2 = ring->perimpoints[j].hist.LastIntegTime;  //ct-   oldest time
      if( t2 < 0.0 ) t2 = 0.0;
    }
    else if( ct <= e_tt ) {
      //ct occurs after burnout finished at start but before it finished at
      //endpoint of quadrangle.
      t1 = ct - et;
      t2 = ring->perimpoints[j].hist.LastIntegTime;
      if( t2 < 0.0 ) t2 = 0.0;
      *BaseFract = (e_tt - ct) / (e_tt - s_tt);
    }
    else {
      //ct occurs after burnout finished after start and endpts of qudrangle
      if( ring->perimpoints[j].hist.LastIntegTime >=
                                       ring->perimpoints[j].hist.TotalTime ) {
        *NumCalcs = 1;
        *BaseFract = 0.0;
        *NumCalcs = 0;

        return 0.0;
      }
      else {
        t1 = e_tt - et;
        t2 = ring->perimpoints[j].hist.LastIntegTime;
        if( t2 < 0.0 ) t2 = 0.0;
        *BaseFract = (t1 - t2) / (ct - t2);
      }
    }

    //Weight error-based determination of timestep.
    wt1 = Interpolate(ring, j, t1, 0);
    wt2 = Interpolate(ring, j, t2, 0);

    wtest = (wt1 + wt2) / 2.0;
    wtmid = Interpolate( ring, j, (t1 + t2) / 2.0, 0 );
    *NumCalcs = (long) ( (fabs(wtest - wtmid) *
                         ring->perimpoints[j].hist.TotalWeight ) /
                Tolerance) + 1;   //2 kg difference maximum

    if( *NumCalcs > 200 ) *NumCalcs = 200;
    if( t1 < (ring->perimpoints[j].hist.FlameTime) ) {
      if( *NumCalcs < 10 ) *NumCalcs = 10;
    }

    TimeInc = ( t1 - t2 ) / ( (double) * NumCalcs );

    if( t1 - t2 > 20.0 * ring->ElapsedTime ) *BaseFract = 1.0;
    else if( t1 - t2 < 0.05 * ring->ElapsedTime ) *BaseFract = -1.0;
  }
  else {    //If(ct==et)   ct is exactly at the end point of quadrangle
    t1 = 0.0;        //Recent
    t2 = ct - st;    //Oldest
    *BaseFract = 1.0;

    //Weight error-based determination of timestep.
    wt1 = Interpolate( ring, j, t1, 0 );
    wt2 = Interpolate( ring, j, t2, 0 );
    wtest = (wt1 + wt2) / 2.0;
    wtmid = Interpolate( ring, j, (t1 + t2) / 2.0, 0 );
    *NumCalcs = (long) ( (fabs(wtest - wtmid) *
                       ring->perimpoints[j].hist.TotalWeight) /
                       Tolerance ) + 1;    //2 kg difference maximum
    if( *NumCalcs > 200 ) *NumCalcs = 200;
    else if( *NumCalcs < 5 ) *NumCalcs = 5;

    TimeInc = (t2 - t1) / ((double) * NumCalcs);
  }
  if( t1 < 1e-3 ) t1 = 0.0;
  *CurrentTime = t1;    //Produces relative time since end of polygon

  return TimeInc;
} //PFIntegration::CalculateTemporalIntegrationSteps

//============================================================================
void PFIntegration::CalculateSpatialIntegrationSteps(
          FireRing* ring, long j, double CurrentRelativeTime, long* NumCalcs )
{ //PFIntegration::CalculateSpatialIntegrationSteps
  //All times in minutes.

  double wt1, wt2, wtest, wtmid;
  double Tolerance = WeightLossErrorTolerance(GETVAL);
  double TimeDiff, TimeNow;

  TimeDiff = ring->ElapsedTime * ring->perimpoints[j].SizeMult;
  TimeNow = CurrentRelativeTime;
  //Calculate most recent value for weight history.
  wt1 = Interpolate( ring, j, TimeNow, 0 ); 
  TimeNow = CurrentRelativeTime + TimeDiff;
  wt2 = Interpolate( ring, j, TimeNow, 0 );
  wtest = (wt1 + wt2) / 2.0;
  TimeNow = CurrentRelativeTime + TimeDiff / 2.0;
  wtmid = Interpolate( ring, j, TimeNow, 0 );

  *NumCalcs = (long) ( (fabs(wtest - wtmid) *
                       ring->perimpoints[j].hist.TotalWeight) /
                       Tolerance ) + 1;  //2 kg difference maximum
  if( *NumCalcs > 200 ) *NumCalcs = 200;
  if( *NumCalcs < 5 ) *NumCalcs = 5;  //AAA FromG5 ver
} //PFIntegration::CalculateSpatialIntegrationSteps

//============================================================================
bool PFIntegration::Integrate()
{ //PFIntegration::Integrate
  //Integrates the burnup function over time and space. It starts by
  //calculating the timesteps required for agiven level of precision for the
  //time-integration (e.g. differencing the burnup function called at two
  //successive times. Then for each time-time step, it determines the
  //spatial-timestep required to integrate over the area represented by the
  //point's trajectory (which is really just a time-gradient for dt. The
  //integration proceeds by computing the surface of the burnup function at
  //increments of time (for the spatial integration) at intervals defined by
  //the time integration.
  //
  //CurrentTime in minutes of elapsed simulation time
  // FlameWt is Mg
  // SmolderWt is Mg

  long   j, k;
  long   lastpt, nextpt;
  long   NumSpatCalcs = 1, NumTimeCalcs = 1;
  double CurTempTime, TempTimeInc;
  double AllPts_WeightConsumption, AllPts_FlameConsumption;
  double TotalWeight, FlameWeight, TimeFract, RelativeTime, NetWeight;

  AllPts_WeightConsumption = 0.0;
  AllPts_FlameConsumption = 0.0;

  lastpt = Begin - 1;
  if( FireNum == 0 ) {
    if( Begin == 0 ) lastpt = ring->NumPoints[FireNum] - 1;
  }
  else if( lastpt < ring->NumPoints[FireNum - 1] )
    lastpt = ring->NumPoints[FireNum] - 1;

  for( j = Begin; j < End; j++ ) {
    //Find number of calculations for integration between StartTime
    //and StartTime+Elapsed Time for each point trajectory
    if( ring->perimpoints[j].hist.LastIntegTime >=
        ring->perimpoints[j].hist.TotalTime ) {
      //If a given point already burned out....
      lastpt = j;
      ring->perimpoints[j].hist.CurWtRemoved = 0;
      continue;
    }
    if( ring->perimpoints[j].SizeMult == 0.0 ) {
      //If the point has been merged out....
      lastpt = j;
      ring->perimpoints[j].hist.CurWtRemoved = 0;
      continue;
    }
    if( ring->perimpoints[j].hist.WeightPolyNum == 0 ) {
      //No post frontal calculations here....
      lastpt = j;
      ring->perimpoints[j].hist.CurWtRemoved = 0;
      continue;
    }

    nextpt = j + 1;
    if( nextpt > ring->NumPoints[FireNum] - 1 ) {
      if( FireNum == 0 ) nextpt = 0;
      else nextpt = ring->NumPoints[FireNum - 1];
    }
    if( ring->perimpoints[j].Area == 0.0 ) continue;

    TimeFract = 1.0;  //Fraction of polygon that is still actively burning
    RelativeTime = CurrentTime;
    TempTimeInc = CalculateTemporalIntegrationSteps( ring, j,
                                                     &RelativeTime,
                                                     &NumTimeCalcs,
                                                     &TimeFract );
    if( TempTimeInc == 0.0 ) continue;
    if( RelativeTime == 0.0 ) {
      ring->perimpoints[j].hist.CurWtRemoved = 0.0;
      ring->perimpoints[j].hist.FlameWtRemoved = 0.0;
      for( k = 0; k < NumTimeCalcs; k++ ) {
        FirstIntegration( ring, j, TempTimeInc, k + 1, &TotalWeight,
                          &FlameWeight );
        if( TotalWeight > 0.0 ) {
          if( ring->perimpoints[j].SizeMult < 1.0 ) {
            TotalWeight *= ring->perimpoints[j].SizeMult;
            FlameWeight *= ring->perimpoints[j].SizeMult;
          }
          NetWeight = ( (float) TotalWeight ) -
                      ring->perimpoints[j].hist.LastWtRemoved;

          if( NetWeight < 0.0 ) NetWeight = 0.0;
          //Sum up total weights for all quadrangles.
          AllPts_WeightConsumption += NetWeight;
          ring->perimpoints[j].hist.CurWtRemoved += NetWeight;
          AllPts_FlameConsumption += FlameWeight;
          ring->perimpoints[j].hist.FlameWtRemoved += FlameWeight;
          if( NetWeight > 0.0 )    //Subtract weight burned by point
            ring->perimpoints[j].hist.LastWtRemoved = TotalWeight;
        }
      }
      AllPts_WeightConsumption +=
                                ring->perimpoints[j].hist.CrownLoadingBurned /
                                1000.0 * ring->perimpoints[j].Area;
      AllPts_FlameConsumption +=
                                ring->perimpoints[j].hist.CrownLoadingBurned /
                                1000.0 * ring->perimpoints[j].Area;
      ring->perimpoints[j].hist.CurWtRemoved +=
                                ring->perimpoints[j].hist.CrownLoadingBurned /
                                1000.0 * ring->perimpoints[j].Area;
      ring->perimpoints[j].hist.FlameWtRemoved +=
                                ring->perimpoints[j].hist.CrownLoadingBurned /
                                1000.0 * ring->perimpoints[j].Area;
    }
    else {
      CurTempTime = RelativeTime -
                                ( (double) (NumTimeCalcs - 1) * TempTimeInc );
      ring->perimpoints[j].hist.CurWtRemoved = 0.0;
      ring->perimpoints[j].hist.FlameWtRemoved = 0.0;
      for( k = 0; k < NumTimeCalcs; k++ ) {
        CalculateSpatialIntegrationSteps( ring, j, CurTempTime,
                                          &NumSpatCalcs );
        SecondaryIntegration( ring, j, CurTempTime, TempTimeInc,
                                   NumSpatCalcs, &TotalWeight, &FlameWeight );

        if( TotalWeight > 0.0 ) {
          if( ring->perimpoints[j].SizeMult < 1.0 ) {
            //Account for diminished size of polygon after mergers.
            TotalWeight *= ring->perimpoints[j].SizeMult;
            FlameWeight *= ring->perimpoints[j].SizeMult;
          }

          //Sum up total weights for all quadrangles.
          AllPts_WeightConsumption += TotalWeight;
          ring->perimpoints[j].hist.CurWtRemoved += TotalWeight;
          ring->perimpoints[j].hist.FlameWtRemoved += FlameWeight;
          AllPts_FlameConsumption += FlameWeight;
          //Subtract weight burned by point.
          ring->perimpoints[j].hist.LastWtRemoved = TotalWeight;
        }

        //Increment means going back in burnup-time history.
        CurTempTime += TempTimeInc;
      }
    }
    lastpt = j;
    ring->perimpoints[j].hist.LastIntegTime = CurrentTime - ring->StartTime -
                                              ring->ElapsedTime;
  }

  SmolderWeightConsumed = AllPts_WeightConsumption - AllPts_FlameConsumption;
  FlameWeightConsumed = AllPts_FlameConsumption;

  return true;
} //PFIntegration::Integrate

//============================================================================
void PFIntegration::FirstIntegration( FireRing* ring, long j, double TimeInc,
                                      long NumCalcs, double* Weight,
                                      double* Flame )
{ //PFIntegration::FirstIntegration
  //Calculates the Weight loss (total and flaming) by integrating the
  //Weight-Remaining curve AND Flame Fraction curve - forward in time over the
  //area and radial distance of the fire quadrangle over the given timestep.
  //This routine begins at the time the fire just enters the quadrangle and
  //continues until the combustion wave just reaches the end of the
  //quadrangle. Here the time and space intervals for the integration are the
  //same.
  //
  //  *ring    = the fire ring
  //  j        = the point on the fire ring
  //  TimeInc  = the time difference over which the curves are to be
  //             integrated
  //  NumCalcs = the number of spatial/temporal divisions of the curve to use
  //             in the integration
  //
  //  returns:   *Weight (total weight) and *Flame (flame weight).

  long   i;
  double Fract, Fract1, OldFract, AreaFract, CuumArea;
  double wt1, wt2, wtmid;
  double ft1_a, ft1_b, ft2_a, ft2_b, ftmid;
  double TempWeight, FlameWeight, TotalWeight;
  double StartTime, CurrentTime;
  double Area, l1, l1c, l2, h, dldh;

  *Weight = *Flame = 0.0;
  CuumArea = TotalWeight = FlameWeight = 0.0;
  CurrentTime = StartTime = (double) NumCalcs * TimeInc;
  wt1 = Interpolate( ring, j, CurrentTime, 0 );
  ft1_a = Interpolate( ring, j, CurrentTime, 1 );

  l1 = l1c = ring->perimpoints[j].l1;
  l2 = ring->perimpoints[j].l2;
  h = ring->perimpoints[j].h;
  dldh = (l2 - l1) / h;

  //If the end of the curve has past the StartTime, increase number of calcs
  //to maintain definition along the active part of the front.
  if( ring->perimpoints[j].hist.TotalTime < StartTime ) {
    NumCalcs += (long) ( (StartTime - ring->perimpoints[j].hist.TotalTime) /
                         TimeInc + 1.0 );
    TimeInc = StartTime / (double) NumCalcs;
  }

  OldFract = Fract = 0.0;
  for( i = 0; i < NumCalcs; i++ ) {
    CurrentTime -= (double) TimeInc;    //Most recent
    wt2 = Interpolate( ring, j, CurrentTime, 0 );
    ft2_a = Interpolate( ring, j, CurrentTime, 1 );
    if( (CurrentTime + TimeInc) > ring->perimpoints[j].hist.TotalTime ) {
      Fract1 = ( (double) (i + 1) * TimeInc ) / (double) ring->ElapsedTime;
      AreaFract = ( ring->perimpoints[j].hist.TotalTime - CurrentTime ) /
                  TimeInc;
      if( AreaFract > 1e-9 ) {
        Fract += (Fract1 - Fract) * AreaFract;
        l2 = l1 + Fract * h * dldh;
        Area = 0.5 * (l1c + l2) * (Fract - OldFract) * h;
        OldFract = Fract;
      }
      else Area = 0.0;
      wtmid = (wt1 + wt2) / 2.0;
      TempWeight = (1.0 - wtmid) * Area *
                   ring->perimpoints[j].hist.TotalWeight;

      l2 = l1 + Fract1 * h * dldh;
      Area = 0.5 * (l1c + l2) * (Fract1 - OldFract) * h;
      if( Area < 1e-9 ) Area = 0.0;
      TempWeight += Area * ring->perimpoints[j].hist.TotalWeight;

      OldFract = Fract1;
    }
    else {
      Fract = ( (double) (i + 1) * TimeInc ) / (double) ring->ElapsedTime;
      l2 = l1 + Fract * h * dldh;
      Area = 0.5 * (l1c + l2) * (Fract - OldFract) * h;
      wtmid = (wt1 + wt2) / 2.0;
      TempWeight = ( 1.0 - wtmid ) * Area *
                   ring->perimpoints[j].hist.TotalWeight;
      AreaFract = 1.0;
      OldFract = Fract;
    }

    TotalWeight += TempWeight;
    ft2_b = Interpolate( ring, j, CurrentTime + TimeInc, 1 );
    if( ft2_b > 0.0 || ft2_a > 0.0 ) {
      ft1_b = Interpolate( ring, j, CurrentTime, 1 );
      ftmid = ( ft1_a + ft1_b + ft2_a + ft2_b ) / 4.0;
      FlameWeight += ftmid * (wt2 - wt1) / 2.0 * Area *
                     ring->perimpoints[j].hist.TotalWeight;
    }
    wt1 = wt2;
    ft1_a = ft2_a;
    l1c = l2;
    CuumArea += Area;
  }
  *Weight = TotalWeight;
  *Flame = FlameWeight;    //Actual fraction of net weight
} //PFIntegration::FirstIntegration

//============================================================================
void PFIntegration::SecondaryIntegration( FireRing* ring, long j,
                                          double CurTempTime,
                                          double TempTimeInc,
                                          long NumSpatCalcs, double* Weight,
                                          double* Flame )
{ //PFIntegration::SecondaryIntegration
  //Calculates the Weight loss (total and flaming) by integrating the
  //Weight-Remaining curve AND Flame Fraction curve - backward in time over
  //the area and radial distance of the fire quadrangle over the given
  //timestep. The first time this is called for a given quadrangle, the fire
  //front has just reached the leading edge of the quadrangle. The time- and
  //space-steps used for integration may be different.
  //
  //  *ring        = the fire ring
  //  j            = the point on the fire ring
  //  CurTempTime  = the current temporal time at the leading edge of the
  //                 quadrangle
  //  TempTimeInc  = the time difference over which the curves are to be
  //                 integrated
  //  NumSpatCalcs = the number of spatial divisions of the curve to use in
  //                 the integration
  //
  //  returns:  *Weight (total weight) and *Flame (flame weight).

  long   n, p, q, NumFlameSpatCalcs, NumFlameTempCalcs;
  double Fract1, Fract2;
  double ft_temp1, ft_temp2, ft_spat1, ft_spat2, ftmid;
  double FlameWeight, TotalWeight;
  double SpatTimeInc, NextSpatTime;
  double FlameTempTimeInc, FlameSpatTimeInc;
  double FlameStartTime1, FlameStartTime2, FlameSpatTime1, FlameSpatTime2;
  double WeightTime1, WeightTime2;
  double Time1, Time2;

  SpatTimeInc = ring->ElapsedTime / (double) NumSpatCalcs;
  NextSpatTime = CurTempTime;    //Start at most recent time period

  *Weight = *Flame = 0.0;
  TotalWeight = FlameWeight = 0.0;
  Fract1 = 0.0;

  ft_temp1 = Interpolate( ring, j, NextSpatTime - TempTimeInc, 1 );
  if( ft_temp1 > 0.0 ) {    //Flaming still going on
    NumFlameSpatCalcs = (long) ( ring->ElapsedTime /
             (ring->perimpoints[j].hist.FlameTime / (double) PRECISION) ) + 1;
    NumFlameTempCalcs = (long) ( TempTimeInc /
             (ring->perimpoints[j].hist.FlameTime / (double) PRECISION) ) + 1;
    if( NumFlameSpatCalcs > 20 ) NumFlameSpatCalcs = 20;
    if( NumFlameTempCalcs > 20 ) NumFlameTempCalcs = 20;
    FlameSpatTimeInc = ring->ElapsedTime / (double) NumFlameSpatCalcs;
    FlameTempTimeInc = TempTimeInc / (double) NumFlameTempCalcs;

    //Start at the earliest time.
    FlameStartTime1 = FlameStartTime2 = NextSpatTime - TempTimeInc;
    if( FlameStartTime1 < 1e-9 )
      FlameStartTime1 = FlameStartTime2 = 0.0;

    for( p = 0; p < NumFlameTempCalcs; p++ ) {
      ft_temp1 = Interpolate( ring, j, FlameStartTime1, 1 );    //More recent
      FlameStartTime2 += FlameTempTimeInc;
      ft_temp2 = Interpolate( ring, j, FlameStartTime2, 1 );    //Less recent

      FlameSpatTime1 = FlameStartTime1;
      FlameSpatTime2 = FlameStartTime2;
      Time1 = FlameStartTime1;
      Time2 = FlameStartTime2;
      Fract1 = 0.0;
      for( q = 0; q < NumFlameSpatCalcs; q++ ) {
        FlameSpatTime1 += FlameSpatTimeInc;
        FlameSpatTime2 += FlameSpatTimeInc;
        ft_spat1 = Interpolate( ring, j, FlameSpatTime1, 1 );
        ft_spat2 = Interpolate( ring, j, FlameSpatTime2, 1 );
        //Flaming fraction is an instantaneous observation, so average over
        //the time span.
        ftmid = ( ft_temp1 + ft_temp2 + ft_spat1 + ft_spat2 ) / 4.0;

        Fract2 = ( (double) (q + 1) / (double) NumFlameSpatCalcs );

        WeightTime1 = CalcWeightLoss( j, Fract1, Fract2, Time1,
                                      FlameSpatTime1 );
        WeightTime2 = CalcWeightLoss( j, Fract1, Fract2, Time2,
                                      FlameSpatTime2 );

        FlameWeight += ftmid * ( WeightTime2 - WeightTime1 );
        TotalWeight += ( WeightTime2 - WeightTime1 );

        Time1 = FlameSpatTime1;
        Time2 = FlameSpatTime2;
        ft_temp1 = ft_spat1;
        ft_temp2 = ft_spat2;
        Fract1 = Fract2;
      }
      FlameStartTime1 = FlameStartTime2;
    }
  }
  else {
    for( n = 1; n <= NumSpatCalcs; n++ ) {
      //Start at present and integrate backward in time.
      NextSpatTime += SpatTimeInc;  
      Fract2 = (double) n / (double) NumSpatCalcs;
      Time1 = NextSpatTime - SpatTimeInc;
      Time2 = NextSpatTime;
      WeightTime2 = CalcWeightLoss( j, Fract1, Fract2, Time1, Time2 );
      Time1 -= TempTimeInc;
      Time2 -= TempTimeInc;
      WeightTime1 = CalcWeightLoss( j, Fract1, Fract2, Time1, Time2 );
      TotalWeight += ( WeightTime2 - WeightTime1 );    //Temporary for testing
      Fract1 = Fract2;
    }
  }
  *Weight = TotalWeight;
  *Flame = FlameWeight;    //Actual fraction of net weight
} //PFIntegration::SecondaryIntegration

//============================================================================
double PFIntegration::CalcWeightLoss( long j, double Fract1, double Fract2,
                                      double Time1, double Time2 )
{ //PFIntegration::CalcWeightLoss
  //Calculates the total weight lossed between two time periods
  //  j= the point number in the ring
  //  n= the iteration count for the spatial calculations
  //  Time1 = the most recent time, beginning of integration
  //  Time2 = the oldest time, end of integration

  double wt1, wt2, wtmid, TotalWeightLoss;
  double SpatTimeInc;
  double AreaFract, Fract;
  double Area, l1, l2c, l2, h, dldh;

  l1 = ring->perimpoints[j].l1;
  l2 = l2c = ring->perimpoints[j].l2;
  h = ring->perimpoints[j].h;
  dldh = ( l1 - l2 ) / h;
  l1 = l2 + Fract2 * h * dldh;
  l2c = l2 + Fract1 * h * dldh;
  Area = 0.5 * ( l1 + l2c ) * ( Fract2 - Fract1 ) * h;

  if( Time1 > ring->perimpoints[j].hist.TotalTime ) {
    TotalWeightLoss = Area * ( 1.0 - ring->perimpoints[j].hist.
                  WeightCoefY[ring->perimpoints[j].hist.WeightPolyNum - 1] ) *
                      ring->perimpoints[j].hist.TotalWeight;

    return TotalWeightLoss;
  }

  SpatTimeInc = Time2 - Time1;

  wt1 = Interpolate( ring, j, Time1, 0 );
  if( Time2 > ring->perimpoints[j].hist.TotalTime ) { //OK, in seconds
    AreaFract = ( SpatTimeInc -
                  (Time2 - ring->perimpoints[j].hist.TotalTime) ) /
                SpatTimeInc;
    wt2 = ring->perimpoints[j].hist.
          WeightCoefY[ring->perimpoints[j].hist.WeightPolyNum - 1];
    if( AreaFract > 1e-9 ) {
      //Weight fraction is a cumulative observation of mass remaining.
      wtmid = ( wt2 + wt1 ) / 2.0;
      Fract = Fract1 + (Fract2 - Fract1) * AreaFract;
      l1 = l2 + Fract * h * dldh;
      Area = 0.5 * (l1 + l2c) * (Fract - Fract1) * h;
    }
    else {
      Area = 0.0;
      wtmid = 1.0;
      Fract = Fract1;
    }
    TotalWeightLoss = ( 1.0 - wtmid ) * Area *
                      ring->perimpoints[j].hist.TotalWeight;

    l1 = l2 + Fract2 * h * dldh;
    Area = 0.5 * (l1 + l2c) * (Fract2 - Fract) * h;
    TotalWeightLoss += ( 1.0 - wt2 ) * Area *
                       ring->perimpoints[j].hist.TotalWeight;
  }
  else {
    wt2 = Interpolate( ring, j, Time2, 0 );
    //Weight fraction is a cumulative observation of mass remaining.
    wtmid = ( wt2 + wt1 ) / 2.0;
    //Weight remaining.
    TotalWeightLoss = ( 1.0 - wtmid ) * Area *
                      ring->perimpoints[j].hist.TotalWeight;
  }

  return TotalWeightLoss;
} //PFIntegration::CalcWeightLoss

//----------------------------------------------------------------------------
//PostFrontal Data Storage stuff
//----------------------------------------------------------------------------

//============================================================================
PFrontData::PFrontData()
{ //PFrontData::PFrontData
  //Initialize Emissions factors, g/kg.
  pm25f = 67.4 - 0.95 * 66.8;
  pm25s = 67.4 - 0.75 * 66.8;
  pm10f = 1.18 * pm25f;
  pm10s = 1.18 * pm25s;
  ch4f = 42.7 - 0.95 * 43.2;
  ch4s = 42.7 - 0.75 * 43.2;
  coF = 961 - 0.95 * 984.0;
  coS = 961 - 0.75 * 984.0;
  co2f = 0.95 * 1833.0;
  co2s = 0.75 * 1833.0;

  numarrays = 0;
  number = 0;
  pf1 = 0;
  pf2 = 0;
  MaxTime = MaxSmolder = MaxFlaming = MaxTotal = -1.0;
  FractFlameAtMax = 0.0;
} //PFrontData::PFrontData

//============================================================================
PFrontData::~PFrontData()
{ //PFrontData::~PFrontData
  if( pf1 ) delete[] pf1;
} //PFrontData::~PFrontData

//============================================================================
double PFrontData::GetMax( long Species, long Phase )
{ //PFrontData::GetMax
  double MaxVal;

  switch( Phase ) {
    case PF_FLAMING: MaxVal = MaxFlaming * GetFlameMult(Species); break;
    case PF_SMOLDERING: MaxVal = MaxSmolder * GetSmolderMult(Species); break;
    case PF_TOTAL:
      MaxVal = MaxTotal * ( 1.0 - FractFlameAtMax ) *
               GetSmolderMult( Species ) + MaxTotal * FractFlameAtMax *
               GetFlameMult( Species );
      break;
    default: MaxVal = 0.0; break;
  }

  return MaxVal;
} //PFrontData::GetMax

//============================================================================
void PFrontData::SetData( double Time, double Flame, double Smolder )
{ //PFrontData::SetData
  if( number >= numarrays * 50 ) {
    PFrontStruct* temp;

    pf2 = new PFrontStruct[(numarrays + 1) * 50];
    if( pf1 != NULL )
      memcpy( pf2, pf1, numarrays * 50 * sizeof(PFrontStruct) );
    temp = pf1;
    pf1 = pf2;
    pf2 = temp;
    if( pf2 != NULL ) delete[] pf2;
    pf2 = 0;
    numarrays++;
  }
  pf1[number].Time = Time;
  pf1[number].Flaming = Flame;
  pf1[number].Smoldering = Smolder;

  if( MaxTime < Time ) MaxTime = Time;
  if( MaxSmolder < Smolder ) MaxSmolder = Smolder;
  if( MaxFlaming < Flame ) MaxFlaming = Flame;
  if( MaxTotal < Flame + Smolder ) {
    MaxTotal = Flame + Smolder;
    if( MaxTotal > 0.0 ) FractFlameAtMax = Flame / MaxTotal;
    else FractFlameAtMax = 1.0;
  }

  number++;
} //PFrontData::SetData

//============================================================================
void PFrontData::ResetData()
{ //PFrontData::ResetData
  if( pf1 ) delete[] pf1;
  pf1 = 0;
  number = 0;
  numarrays = 0;
  MaxTime = MaxSmolder = MaxFlaming = MaxTotal = -1.0;
  FractFlameAtMax = 0.0;
} //PFrontData::ResetData

//============================================================================
double PFrontData::GetFlameMult( long Species )
{ //PFrontData::GetFlameMult
  //Returns Mg or GJ.
  double mult;

  switch( Species ) {
    case PF_FUELWEIGHT:
      mult = 0.001;
      break;
    case PF_TOTALHEAT:
      mult = 0.0186;  //GJ
      break;
    case PF_PM25:
      mult = pm25f / 1e6;
      break;
    case PF_PM10:
      mult = pm10f / 1e6;
      break;
    case PF_CH4:
      mult = ch4f / 1e6;
      break;
    case PF_CO:
      mult = coF / 1e6;
      break;
    case PF_CO2:
      mult = co2f / 1e6;
      break;
    default:
      mult = 1.0 / 1e6;
  }

  return mult;
} //PFrontData::GetFlameMult

//============================================================================
double PFrontData::GetSmolderMult( long Species )
{ //PFrontData::GetSmolderMult
  //Returns Mg or GJ.
  double mult;

  switch( Species ) {
    case PF_FUELWEIGHT:
      mult = 0.001;
      break;
    case PF_TOTALHEAT:
      mult = 0.0186;  //GJ
      break;
    case PF_PM25:
      mult = pm25s / 1e6;
      break;
    case PF_PM10:
      mult = pm10s / 1e6;
      break;
    case PF_CH4:
      mult = ch4s / 1e6;
      break;
    case PF_CO:
      mult = coS / 1e6;
      break;
    case PF_CO2:
      mult = co2s / 1e6;
      break;
    default:
      mult = 1.0 / 1e6;
  }

  return mult;
} //PFrontData::GetSmolderMult

//============================================================================
double PFrontData::GetPostFrontalProducts( long Species, long Phase,
                                           long Num )
{ //PFrontData::GetPostFrontalProducts
  if( Num >= number ) return -1.0;
  if( pf1 == NULL ) return 0.0;

  double val = 0.0;

  if( Phase == PF_FLAMING || Phase == PF_TOTAL )
    val += pf1[Num].Flaming * GetFlameMult( Species );
  if( Phase == PF_SMOLDERING || Phase == PF_TOTAL )
    val += pf1[Num].Smoldering * GetSmolderMult( Species );

  return val;
} //PFrontData::GetPostFrontalProducts

//============================================================================
double PFrontData::GetTime( long Num )
{ //PFrontData::GetTime
  if( Num >= number ) return -1.0;

  return pf1[Num].Time;
} //PFrontData::GetTime
