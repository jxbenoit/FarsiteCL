/*fsxwfms2.cpp
  FARSITE 4.0
  10/20/2000
  Mark A. Finney (USDA For. Serv., Fire Sciences Laboratory)

  Contains functions for calculating dead fuel moisture maps for an entire
  landscape file (declarations in fsx4.h).
  See LICENSE.TXT file for license information.
*/
#include<math.h>
#include<string.h>
#include"newfms.h"
#include"fsglbvar.h"

#include"globals.h"
#include"portablestrings.h"

//extern const double PI;
static long WindLoc[5]={0, 0, 0, 0, 0};;

//static HANDLE *hMoistureEvent=0;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//   Global Access Functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

static long FM_TOLERANCE = 1;

long GetFmTolerance()
{
	return FM_TOLERANCE;
}


void SetFmTolerance(long Tol)
{
	FM_TOLERANCE = Tol;
}


long Chrono(double SIMTIME, long* hour, double* hours, bool RelCondit)
	// Determines actual date and time from SIMTIME variable
{
	long date, month, day, min;
	double mins, days;

	*hours = SIMTIME / 60.0;
	*hour = (long) * hours; 					  // truncates time to nearest hour
	min = (long) ((*hours - *hour) * 60);
	mins = (*hours - *hour) * 60.0;
	if (RelCondit && UseConditioningPeriod(GETVAL))
		*hours = ((double) * hour) * 100.0;   			  // elapsed time in hours
	else
		*hours = ((double) * hour) * 100.0 +
			(double) GetStartHour() +
			GetStartMin();	// elapsed time in hours
	days = *hours / 2400.0;
	day = (long) days;
	if (RelCondit && UseConditioningPeriod(GETVAL))
	{
		date = day + GetConditDay();
		month = GetJulianDays(GetConditMonth());
	}
	else
	{
		date = day + GetStartDay();
		month = GetJulianDays(GetStartMonth());
	}
	date = date + month;
	if (date > 365)
		date -= 365;
	days = day;
	*hour = (long) (*hours - days * 2400) + min;		 // integer hour
	*hours = (*hours - days * 2400.0) + mins;   		 // double precision hours

	return date;
}


DeadMoistureDescription::DeadMoistureDescription()
{
	long i, j;

	fms = 0;
	for (i = 0; i < MAX_NUM_STATIONS; i++)
	{
		for (j = 0; j < NUM_FUEL_SIZES; j++)
			ResetDescription(i, j);
	}
}


void DeadMoistureDescription::ResetDescription(long Station, long FuelSize)
{
	long i;

	i = FuelSize;
	NumStations = 0;
	memset(FuelKey[Station][i], 0x0, MAXNUM_FUEL_MODELS * sizeof(long));
	NumAlloc[Station][i] = 0;
	NumHist[Station][i] = 0;
	NumFuels[Station][i] = 0;
	EndTime[Station][i] = 0.0;
	NumElevs[Station][i] = NumSlopes[Station][i] = NumAspects[Station][i] = NumCovers[Station][i] = 0;
	if (fms)
		delete[] fms;
	fms = 0;
}


DeadMoistureDescription::~DeadMoistureDescription()
{
	long i, j;

	for (i = 0; i < MAX_NUM_STATIONS; i++)
	{
		for (j = 0; j < NUM_FUEL_SIZES; j++)
			ResetDescription(i, j);
	}
}


//----------------------------------------------------------------------------
//FELocalSite Functions
//----------------------------------------------------------------------------

FELocalSite::FELocalSite()
{
	StationNumber = -1;
  memset(WindLoc, 0x0, 5*sizeof(long)); //WindLoc=0;
	//	SetCanopyChx(15.0, 4.0, 0.2, 30.0, 100, 2, 1); // for each thread
}


//============================================================================
long FELocalSite::GetLandscapeData( double xpt, double ypt )
{ //FELocalSite::GetLandscapeData
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:GetLandscapeData:1\n", CallLevel, "" );

  if( xpt < GetLoEast() || xpt > GetHiEast() ||
      ypt < GetLoNorth() || ypt>GetHiNorth() ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:GetLandscapeData:1a\n",
              CallLevel, "" );
    CallLevel--;

    return -1;
  }

  long posit;

  CellData( xpt, ypt, cell, crown, ground, &posit );
  ElevConvert( cell.e );  //Need to convert units and codes
  SlopeConvert( cell.s );
  AspectConvert( cell.a );
  FuelConvert( cell.f );
  CoverConvert( cell.c );
  HeightConvert( crown.h );
  BaseConvert( crown.b );
  DensityConvert( crown.p );
  DuffConvert( ground.d );
  WoodyConvert( ground.w );
  StationNumber = GetStationNumber( xpt, ypt ) - 1;
  XLocation = xpt;
  YLocation = ypt;

  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:GetLandscapeData:2\n", CallLevel, "" );
  CallLevel--;

  return posit;
} //FELocalSite::GetLandscapeData

//============================================================================
long FELocalSite::GetLandscapeData( double xpt, double ypt,
                                    LandscapeStruct& ls )
{ //FELocalSite::GetLandscapeData
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:GetLandscapeData:1 ld.fuel=%d\n",
            CallLevel, "", ld.fuel );

  if( xpt < GetLoEast() || xpt > GetHiEast() ||
      ypt < GetLoNorth() || ypt > GetHiNorth() ) {

    if( Verbose >= CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:GetLandscapeData:1a\n",
              CallLevel, "" );
    CallLevel--;

    return 0;
  }

  long posit;
  celldata cell;
  crowndata crown;
  grounddata ground;

  CellData( xpt, ypt, cell, crown, ground, &posit );

  if( posit <= 1 ) printf("<%ld>",posit); //AAA

  ElevConvert( cell.e );    //Need to convert units and codes
  SlopeConvert( cell.s );
  AspectConvert( cell.a );

  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:GetLandscapeData:2 ld.fuel=%d\n",
            CallLevel, "", ld.fuel );
  FuelConvert( cell.f );
  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:GetLandscapeData:3 ld.fuel=%d\n",
            CallLevel, "", ld.fuel );
  CoverConvert( cell.c );
  HeightConvert( crown.h );
  BaseConvert( crown.b );
  DensityConvert( crown.p );
  DuffConvert( ground.d );
  WoodyConvert( ground.w );
  StationNumber = GetStationNumber( xpt, ypt ) - 1;
  XLocation = xpt;
  YLocation = ypt;
  ls = ld;

  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:GetLandscapeData:4 ld.fuel=%d\n",
            CallLevel, "", ld.fuel );
  CallLevel--;

  return posit;
} //FELocalSite::GetLandscapeData

void FELocalSite::GetEnvironmentData(EnvironmentData* mw)
{
	(*mw).ones = onehour;
	(*mw).tens = tenhour;
	(*mw).hundreds = hundhour;
	(*mw).liveh = (double) GetInitialFuelMoisture(ld.fuel, 3) / 100.0;
	(*mw).livew = (double) GetInitialFuelMoisture(ld.fuel, 4) / 100.0;
	(*mw).windspd = mwindspd;
	(*mw).tws = twindspd;
	if (wwinddir >= 0.0)
		(*mw).winddir = wwinddir / 180.0 * PI;
	else
		(*mw).winddir = wwinddir;
	(*mw).thousands = thousands;
	(*mw).tenthous = tenthous;
	(*mw).temp = airtemp;
	(*mw).humid = relhumid;
	(*mw).solrad = solrad;
}


//============================================================================
void FELocalSite::GetFireEnvironment( FireEnvironment2* env, double SimTime,
                                      bool All )
{ //FELocalSite::GetFireEnvironment
  long date, cloud, hour;
  double hours; //shade,
  double Radiate = 0, EquilMx;
  CallLevel++;

  tenhour = hundhour = onehour = thousands = solrad = -1.0;
  if( ld.elev == -9999 ) {
    CallLevel--;
    return;
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:GetFireEnvironment:1\n", CallLevel, "" );

  date = Chrono( SimTime, &hour, &hours, false );  // get hours days etc.
  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:GetFireEnvironment:2\n", CallLevel, "" );

  if( AtmosphereGridExists() ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:GetFireEnvironment:2a\n",
              CallLevel, "" );

    AtmWindAdjustments( date, hours, &cloud );

    if( Verbose >= CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:GetFireEnvironment:2b\n",
              CallLevel, "" );
  }
  else {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:GetFireEnvironment:2c\n",
              CallLevel, "" );

    windadj( date, hours, &cloud );

    if( Verbose >= CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:GetFireEnvironment:2d\n",
              CallLevel, "" );
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:GetFireEnvironment:3\n", CallLevel, "" );

  if( ld.fuel < 1 ) tenhour = hundhour = onehour = thousands = 0.0;
  else {
    if( All ) Radiate = 1;
    tenhour = env->GetMx( SimTime, ld.fuel, ld.elev, ld.slope, ld.aspectf,
                          ld.cover, &EquilMx, &Radiate, SIZECLASS_10HR ); // Radiate is not returned if set to 0

    if( Verbose >= CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:GetFireEnvironment:3b\n",
              CallLevel, "" );

    if( All ) // env->elref always in feet.
      env->fmsthread[0].SiteSpecific( (long)(env->elevref - ld.elev * 3.2808),
                                      env->tempref, &airtemp, env->humref, &relhumid );

    if( Verbose >= CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:GetFireEnvironment:3c\n",
              CallLevel, "" );

    onehour = ( 4.0 * EquilMx + tenhour ) / 5.0;
    solrad = Radiate;
    Radiate = 0;
    hundhour = env->GetMx( SimTime, ld.fuel, ld.elev, ld.slope, ld.aspectf,
                           ld.cover, &EquilMx, &Radiate, SIZECLASS_100HR ); // Radiate is not returned if set to 0

    thousands = env->GetMx( SimTime, ld.woody, ld.elev, ld.slope,
                            ld.aspectf, ld.cover, &EquilMx, &Radiate,
                            SIZECLASS_1000HR ); // Radiate is not returned if set to 0
    tenthous = 0.30;
  }
  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:GetFireEnvironment:4\n", CallLevel, "" );

  CallLevel--;
} //FELocalSite::GetFireEnvironment

/*============================================================================
  FELocalSite::windadj
  Finds most current windspd and calculates midflame windspds from
  20-ft ws (mph).
*/
void FELocalSite::windadj( long date, double hours, long* cloud )
{ //FELocalSite::windadj
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:windadj:1\n", CallLevel, "" );

  int count, month, xmonth, xday, day, hhour = 0, xdate;
  double MinTimeStep;

  count = WindLoc[StationNumber] - 1;
  do {
    count++;
    month = GetWindMonth( StationNumber, count );
    day = GetWindDay( StationNumber, count );
    if( month < 13 ) xmonth = GetJulianDays( month );
    else {
      day = 0;
      xmonth = date;
    }
    xdate = xmonth + day;
  } while( xdate < date );

  if( month != 13 ) {   //If hit end of windspeed data
    xday = day;
    hhour = GetWindHour( StationNumber, count );
    while( hours >= (double) hhour ) {
      count++;
      day = GetWindDay( StationNumber, count );
      xmonth = GetWindMonth( StationNumber, count );
      hhour = GetWindHour( StationNumber, count );
      if( day > xday || xmonth > month ) break;
    }
  }

  if( hours <= hhour ) {
    MinTimeStep = (double) hhour - hours;
    EventMinimumTimeStep( MinTimeStep );     //Load into global variable
  }
  count--;
  twindspd = GetWindSpeed( StationNumber, count );
  wwinddir = GetWindDir( StationNumber, count );
  *cloud = GetWindCloud( StationNumber, count );
  WindLoc[StationNumber] = count;
  windreduct();  // GET MIDFLAME WINDSPEED

  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:windadj:1\n", CallLevel, "" );
  CallLevel--;
} //FELocalSite::windadj

//============================================================================
void FELocalSite::AtmWindAdjustments( long curdate, double hours,

                                      long* cloud )
{ //FELocalSite::AtmWindAdjustments

  CallLevel++;
  long count = -1, ddate, month, day, hhour, xmonth, xday;
  short twspd, wndir, cloud_s;
  double MinTimeStep;


  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:1\n", CallLevel, "" );
  do {
		count++;
		month = GetAtmosphereGrid()->GetAtmMonth(count);
		if (month == -1)					// hit end of ATMDATA
			break;
		day = GetAtmosphereGrid()->GetAtmDay(count);
		ddate = day + GetJulianDays(month);
  } while( ddate != curdate );
  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:2\n", CallLevel, "" );



  if( month != -1 ) {						// if hit end of ATMDATA data
    if( Verbose > CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:2a\n",
              CallLevel, "" );



    hhour = GetAtmosphereGrid()->GetAtmHour(count);
		xday = day;
    if( Verbose > CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:2b\n",
              CallLevel, "" );

    while( hours >= hhour ) {
			count++;
      if( Verbose > CallLevel )
        printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:2b1\n",
                CallLevel, "" );

			xmonth = GetAtmosphereGrid()->GetAtmMonth(count);
      if( Verbose > CallLevel )
        printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:2b2\n",
                CallLevel, "" );

			xday = GetAtmosphereGrid()->GetAtmDay(count);
      if( Verbose > CallLevel )
        printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:2b3\n",
                CallLevel, "" );

			hhour = GetAtmosphereGrid()->GetAtmHour(count);
      if( Verbose > CallLevel )
        printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:2b4 "
                "hours=%lf hhour=%ld xday=%ld day=%ld xmonth=%ld month=%ld\n",
                CallLevel, "", hours, hhour, xday, day, xmonth, month );

      if( xday > day || xmonth > month ||
          (month == 12 && day == 31 && xmonth == 1 && xday == 1) ) break;
		}
    if( Verbose > CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:2c\n",
              CallLevel, "" );

		count--;
		GetAtmosphereGrid()->GetAtmosphereValue(ATMWSPD, XLocation, YLocation,
								count, &twspd);
    if( Verbose > CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:2d\n",
              CallLevel, "" );

		GetAtmosphereGrid()->GetAtmosphereValue(ATMWDIR, XLocation, YLocation,
								count, &wndir);
    if( Verbose > CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:2e\n",
              CallLevel, "" );

		GetAtmosphereGrid()->GetAtmosphereValue(ATMCLOUD, XLocation,
								YLocation, count, &cloud_s);
    if( Verbose > CallLevel )
      printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:2f\n",
              CallLevel, "" );

		*cloud = cloud_s;
		twindspd = (double) twspd;
		wwinddir = (double) wndir;
		windreduct();
	}
  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:3\n", CallLevel, "" );

  if( hours <= hhour ) {
		MinTimeStep = (double) hhour - hours;
		EventMinimumTimeStep(MinTimeStep);  	   // load into global variable
	}

  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FELocalSite:AtmWindAdjustments:4\n", CallLevel, "" );

  CallLevel--;
} //FELocalSite::AtmWindAdjustments


void FELocalSite::windreduct()
{
	// FUNCTION TO REDUCE WINDSPEED (MI/HR) TO MIDFLAME OR VEGETATION HEIGHT

	double ffactor, htfuel, htflame, m1, m2;
	double canopyht = ld.height*3.2808;//GetDefaultCrownHeight()*3.28;     // convert to feet

	if (ld.cover <= 5 || canopyht == 0)	// ld.cover==0
	{
		if (ld.fuel > 0)
		{
			switch (ld.fuel)
			{
			case 1:
				htfuel = 1.0; break;
			case 2:
				htfuel = 1.0; break;
			case 3:
				htfuel = 2.5; break;
			case 4:
				htfuel = 6.0; break;
			case 5:
				htfuel = 2.0; break;
			case 6:
				htfuel = 2.5; break;
			case 7:
				htfuel = 2.5; break;
			case 8:
				htfuel = 0.2; break;
			case 9:
				htfuel = 0.2; break;
			case 10:
				htfuel = 1.0; break;
			case 11:
				htfuel = 1.0; break;
			case 12:
				htfuel = 2.3; break;
			case 13:
				htfuel = 3.0; break;
			default:
				htfuel = GetFuelDepth(ld.fuel);  // retrieve from cust models
				if (htfuel == 0.0)
					htfuel = 0.01;
				break;
			}
		}
		else
			htfuel = 0.01;		// no fuel so height is essentially zero
		htflame = htfuel;		// from Baughman and Albini 6th conf. FFM 1980
		m1 = (1.0 + 0.36 * (htfuel / htflame)) /
			(log((20.0 + 0.36 * htfuel) / (0.13 * htfuel)));
		m2 = log(((htflame / htfuel + 0.36) / .13)) - 1.0;
		mwindspd = m1 * m2 * twindspd;
	}
	else
	{
		ffactor = ((double) ld.cover / 100.0) * 0.33333; // volume ratio of cone to cylinder
		ffactor *= PI / 4.0;						// area ratio of circle to square

		m1 = 0.555 /
			(sqrt(canopyht * ffactor) * log((20.0 + 0.36 * canopyht) /
											(0.13 * canopyht)));
		if (m1 > 1.0)
			m1 = 1.0;
		mwindspd = m1 * twindspd;
	}
}


//FireEnvironment2 Functions



//============================================================================
FireEnvironment2::FireEnvironment2()
{ //FireEnvironment2::FireEnvironment2
	long i, j;

	Stations = 0;
	NumStations = 0;
	LastDate = -1;
	CloudCount = 0;
	fmsthread = 0;
	NumFmsThreads = 0;
	SimStart = -1.0;

	for( i = 0; i < NUM_FUEL_SIZES; i++ ) {
		for( j = 0; j < NUM_FUEL_SIZES; j++ ) {
			CurHist[i][j] = 0;
			NextHist[i][j] = 0;
			FirstHist[i][j] = 0;
		}
	}
} //FireEnvironment2::FireEnvironment2

//============================================================================
FireEnvironment2::~FireEnvironment2()
{ //FireEnvironment2::~FireEnvironment2
  FreeStations();
} //FireEnvironment2::~FireEnvironment2

//============================================================================
bool FireEnvironment2::CalcMapFuelMoistures( double SimTime )
{ //FireEnvironment2::CalcMapFuelMoistures
  long i, j, k, PrevNumAlloc;
  bool UpDated = false, NeedToRecalculate = true;

  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::CalcMapFuelMoistures:1\n",
            CallLevel, "" );

  PrevNumAlloc = MxDesc.NumAlloc[0][SIZECLASS_10HR];
  if( NumStations < GetNumStations() ) {
    AllocStations( GetNumStations() );
    for( i = 0; i < NUM_FUEL_SIZES; i++ ) ResetData( i );
  }

  //Make sure that starttime is same as calculations.
  NormalizeMoistureStartTime();

  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::CalcMapFuelMoistures:2\n",
            CallLevel, "" );

	for( i = 0; i < NumStations; i++ ) {
    if( Verbose > CallLevel )
      printf( "%*sfsxwfms2:FireEnvironment2::CalcMapFuelMoistures:2a\n",
              CallLevel, "" );
		StationNumber = i;
		for( k = 1; k < NUM_FUEL_SIZES; k++ ) {
      if( Verbose > CallLevel )
        printf( "%*sfsxwfms2:FireEnvironment2::CalcMapFuelMoistures:2a1\n",
                CallLevel, "" );
			if( Stations[i].ReAllocFuels(k) || EnvironmentChanged(GETVAL, i, k) ) {
				FreeHistory( i, k );
        if( k == 1 ) {
          FreeHistory(i, 0);
					PrevNumAlloc = 0;
				}
				RefreshHistoryDescription(i, k);
				ResetData(k);
				for( j = 0; j < Stations[i].NumFuels[k]; j++ ) {
					Stations[i].FMS[k][j]->FirstTime = 0.0;
					Stations[i].FMS[k][j]->LastTime = 0.0;
				}
			} // will only happen if moistures not calculated in .FMM file
      else if( Stations[i].NumFuels[k] > 0 && MxDesc.NumAlloc[i][k] == 0 ) {
				FreeHistory(i, k);
				if (k == 1)
					FreeHistory(i, 0);
				RefreshHistoryDescription(i, k);
				ResetData(k);
				for (j = 0; j < Stations[i].NumFuels[k]; j++)
				{
					Stations[i].FMS[k][j]->FirstTime = 0.0;
					Stations[i].FMS[k][j]->LastTime = 0.0;
				}
			}
      if( Verbose > CallLevel )
        printf( "%*sfsxwfms2:FireEnvironment2::CalcMapFuelMoistures:2a2\n",
                CallLevel, "" );

      //If the moistures have not already been calculated.
      if( ! CheckMoistureHistory(i, k, SimTime) ) {
        if( Verbose > CallLevel )
          printf( "%*sfsxwfms2:FireEnvironment2::CalcMapFuelMoistures:2a2a\n",
                  CallLevel, "" );
        for( j = 0; j < Stations[i].NumFuels[k]; j++ )
          RunFmsThreads( SimTime, i, j, k );
      }
			else if( k == 1 ) {
        if( Verbose > CallLevel )
          printf( "%*sfsxwfms2:FireEnvironment2::CalcMapFuelMoistures:2a2b\n",
                  CallLevel, "" );
				LastDate = -1;
				CloudCount = 0;
				WindLoc[StationNumber] = 0;
				CheckMoistureHistory(i, SIZECLASS_1HR, SimTime);
			}
      if( Verbose > CallLevel )
        printf( "%*sfsxwfms2:FireEnvironment2::CalcMapFuelMoistures:2a3\n",
                CallLevel, "" );

      EnvironmentChanged( false, i, k );
      if( k == 1 ) EnvironmentChanged( false, i, 0 );

      if( Verbose > CallLevel )
        printf( "%*sfsxwfms2:FireEnvironment2::CalcMapFuelMoistures:2a4\n",
                CallLevel, "" );
		}

    if( Verbose > CallLevel )
      printf( "%*sfsxwfms2:FireEnvironment2::CalcMapFuelMoistures:2b\n",
              CallLevel, "" );
	}

  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::CalcMapFuelMoistures:3\n",
            CallLevel, "" );

  if( PrevNumAlloc > 0 ) {
		if (UpDated == true)
			NeedToRecalculate = true;	   //Must recalc if data have been changed;
		else
			NeedToRecalculate = false;  //Don't have to recalc if data already exist
	}

  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::CalcMapFuelMoistures:4\n",
            CallLevel, "" );
  CallLevel--;

	return NeedToRecalculate;
} //FireEnvironment2::CalcMapFuelMoistures


//============================================================================

void FireEnvironment2::ResetData(long FuelSize)
{ //ResetData
  //AAA long i, j; In JAS! ver only

	LastDate = -1;
	CloudCount = 0;
  memset(WindLoc, 0x0, 5*sizeof(long));//WindLoc = 0;
	SimStart = - 1.0;

  //AAA for(i=0; i<NumStations; i++)                      In JAS! ver only
  //AAA{ for(j=0; j<Stations[i].NumFuels[FuelSize]; j++)  In JAS! ver only
  //AAA{  Stations[i].FMS[FuelSize][j]->FirstTime=0.0;    In JAS! ver only
  //AAA   Stations[i].FMS[FuelSize][j]->LastTime=0.0;     In JAS! ver only
  //AAA }                                                 In JAS! ver only
  //AAA}                                                  In JAS! ver only
} //ResetData

//============================================================================
bool FireEnvironment2::AllocStations( long Num )
{ //FireEnvironment2::AllocStations
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::AllocStations:1 Num=%ld\n",
            CallLevel, "", Num );

  FreeStations();
  long i, j;

  if( Num > 0 ) {
    Stations = new FuelMoistureMap[Num];
    if( Stations == NULL ) {
      if( Verbose > CallLevel )
        printf( "%*sfsxwfms2:FireEnvironment2::AllocStations:1a\n",
                CallLevel, "" );
      CallLevel--;
      return false;
    }

    for( i = 0; i < Num; i++ ) {
      Stations[i].AllocFuels( SIZECLASS_10HR );
      Stations[i].AllocFuels( SIZECLASS_100HR );
      Stations[i].AllocFuels( SIZECLASS_1000HR );
    }

    NumStations = Num;
  }
  else {
      if( Verbose > CallLevel )
        printf( "%*sfsxwfms2:FireEnvironment2::AllocStations:2b\n",
                CallLevel, "" );
      CallLevel--;
    return false;
  }

  for( i = 0; i < MAX_NUM_STATIONS; i++ ) {
    for( j = 1; j < NUM_FUEL_SIZES; j++ )
      RefreshHistoryDescription( i, j );  //First call to this
  }

  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::AllocStations:2\n",
            CallLevel, "" );
  CallLevel--;

  return true;
} //FireEnvironment2::AllocStations

//============================================================================
void FireEnvironment2::FreeStations()
{ //FireEnvironment2::FreeStations
  if( Stations == NULL ) return;

  long i, j;
  for( i = 0; i < NumStations; i++ ) {
    for( j = 0; j < NUM_FUEL_SIZES; j++ ) Stations[i].FreeFuels(j);
  }

  delete[] Stations;
  Stations = 0;
  NumStations = 0;

  for( i = 0; i < MAX_NUM_STATIONS; i++ ) {
    for( j = 0; j < NUM_FUEL_SIZES; j++ )
      FreeHistory(i, j);
  }
  CloseFmsThreads();
  SimStart = -1.0;
} //FireEnvironment2::FreeStations

bool FireEnvironment2::AllocFmsThreads()
{
	if (NumFmsThreads == GetMaxThreads())
		return true;

	CloseFmsThreads();
	fmsthread = new FmsThread[GetMaxThreads()];

	if (!fmsthread)
	{
		NumFmsThreads = 0;

		return false;
	}

	NumFmsThreads = GetMaxThreads();

	return true;
}


void FireEnvironment2::ResetAllThreads()
{
	CloseFmsThreads();
}


void FireEnvironment2::CloseFmsThreads()
{
     /*
	long m;

	if (NumFmsThreads == 0)
		return;
	for (m = 0; m < NumFmsThreads; m++)
		fmsthread[m].SetRange(0.0, 0, 0, 0, 0, 0, 0, 0, -1);
	for (m = 0; m < NumFmsThreads; m++)
		fmsthread[m].StartFmsThread(m, 0, &MxDesc, false);
     */
     
	FreeFmsThreads();
}


void FireEnvironment2::FreeFmsThreads()
{
	if (fmsthread)
		delete[] fmsthread;

	fmsthread = 0;
	NumFmsThreads = 0;
}


void FireEnvironment2::HumTemp(long date, long hour, double* tempref,
	double* humref, long* elevref, double* rain, double* humidmx,
	double* humidmn, double* tempmx, double* tempmn)
{
	// Finds and interpolates Humidity and temperature for Current Hour

	long count = -1, month, day, hmorn, haft, Tmin, Tmax, Hmax, Hmin;
	long elref, hx, garbage, ppt;
	double h1, h2, dtprime, dtmxmn, humid, temp, tempf, humf, sign;
	double Thalf, Hhalf;
	//	double dewptref, dewpt;

	while (LastDate != date)
	{
		count++;
		month = GetWeatherMonth(StationNumber, count);
		day = GetWeatherDay(StationNumber, count);
		LastDate = day + GetJulianDays(month);
		LastCount = count;
		if (LastDate > 365)
			count = -1;
	}
	hmorn = GetWeatherTime1(StationNumber, LastCount);
	haft = GetWeatherTime2(StationNumber, LastCount);
	ppt = (long)GetWeatherRain(StationNumber, LastCount);
	Tmin = (long)GetWeatherTemp1(StationNumber, LastCount);
	Tmax = (long)GetWeatherTemp2(StationNumber, LastCount);
	Hmax = (long)GetWeatherHumid1(StationNumber, LastCount);
	Hmin = (long)GetWeatherHumid2(StationNumber, LastCount);
	elref = (long)GetWeatherElev(StationNumber, LastCount);
	ppt = (long)GetWeatherRain(StationNumber, LastCount);
	count = LastCount;	// only meaningful if LastDate==date;
	if (hour > haft)
	{
		count++;
		garbage = (long)GetWeatherElev(StationNumber, count);
		hmorn = (long)GetWeatherTime1(StationNumber, count);
		Tmin = (long)GetWeatherTemp1(StationNumber, count);
		Hmax = (long)GetWeatherHumid1(StationNumber, count);
		if (garbage != elref)
		{
			tempf = (double) Tmin;
			humf = (double) Hmax;
			fmsthread[0].SiteSpecific(elref - garbage, tempf, &temp, humf,
							&humid);	/* adjusts to site specific temp and relhum at a given pixel */
			Tmin = (int) temp;
			Hmax = (int) humid;
		}
	}
	else if (hour < hmorn)
	{
		count--;
		if (count < 0)
			count = 0;
		garbage = (long)GetWeatherElev(StationNumber, count);
		haft = (long)GetWeatherTime2(StationNumber, count);
		Tmax = (long)GetWeatherTemp2(StationNumber, count);
		Hmin = (long)GetWeatherHumid2(StationNumber, count);
		if (garbage != elref)
		{
			tempf = Tmax;
			humf = Hmin;
			fmsthread[0].SiteSpecific(elref - garbage, tempf, &temp, humf,
							&humid);	/* adjusts to site specific temp and relhum at a given pixel */
			Tmax = (int) temp;
			Hmin = (int) humid;
		}
	}
	hx = hmorn / 100;
	hx *= 100;
	if (hx < hmorn)
		h1 = (double) hx + (10.0 * (hmorn - hx) / 6.0);
	else
		h1 = (double) hmorn;
	hx = haft / 100;
	hx *= 100;
	if (hx < haft)
		h2 = (double) hx + (10.0 * (haft - hx) / 6.0);
	else
		h2 = (double) haft;
	dtmxmn = (2400 - h2) + h1;		/* this section interpolates temperature */
	if (hour >= h1 && hour <= h2)		/* and humidity from high and low obs */
	{
		//if(hour==h2)
		//	garbage=elref;
		dtprime = (double) hour - h1;	/* and their time of observation */
		dtmxmn = 2400 - dtmxmn;
		sign = 1.0;
	}
	else
	{
		if (hour > h2)
			dtprime = double (hour) - h2;   // time since the maximum
		else
			dtprime = (2400 - h2) + hour;    // time since the maximum
		sign = -1.0;
	}
	Thalf = ((double) Tmax - (double) Tmin) / 2.0 * sign;
	Hhalf = ((double) Hmin - (double) Hmax) / 2.0 * sign;

	*tempref = ((double) (Tmax + Tmin)) /
		2.0 +
		(Thalf * sin(PI * (dtprime / dtmxmn - 0.5)));
	*humref = ((double) (Hmax + Hmin)) /
		2.0 +
		(Hhalf * sin(PI * (dtprime / dtmxmn - 0.5)));

	//FILE *mout=fopen("mout.txt", "a");
	//fprintf(mout, "%ld %ld %ld %ld %ld %ld %ld %lf %lf %lf %lf\n", hmorn, haft, hour, Tmin, Tmax, Hmax, Hmin, Thalf, Hhalf, *tempref, *humref);
	//fclose(mout);

	*elevref = elref;
	*rain = ((double) ppt) / 100.0;
	*tempmx = (double) Tmax;
	*tempmn = (double) Tmin;
	*humidmx = (double) Hmax;
	*humidmn = (double) Hmin;
}


long FireEnvironment2::GetClouds(long date, double hour)
{
	// FINDS MOST CURRENT WINDSPD AND CALCULATES MIDFLAME WINDSPDS FROM 20-FT WS (MPH)

	long cloud;
	long count, month, xmonth, xday, day, hhour, xdate;

	count = CloudCount--;
	do
	{
		count++;
		month = GetWindMonth(StationNumber, count);
		day = GetWindDay(StationNumber, count);
		if (month < 13)
			xmonth = GetJulianDays(month);
		else
		{
			day = 0;
			xmonth = date;
		}
		xdate = xmonth + day;
	}
	while (xdate != date);
	if (month != 13)						// if hit end of windspeed data
	{
		xday = day;
		hhour = GetWindHour(StationNumber, count);
		while (hour >= hhour)
		{
			count++;
			day = GetWindDay(StationNumber, count);
			xmonth = GetWindMonth(StationNumber, count);
			hhour = GetWindHour(StationNumber, count);
			if (day > xday || xmonth > month)
				break;
		}
	}
	count--;
	cloud = GetWindCloud(StationNumber, count);
	CloudCount = count;

	return cloud;
}

/*
long	FireEnvironment2::GetAtmClouds(long curdate, double hours)
{
	long count=-1, ddate, month, day, hhour, xmonth, xday;
	short cloud_s;

	 do
	 {    count++;
	 	month=GetAtmosphereGrid()->GetAtmMonth(count);
		  if(month==-1)					// hit end of ATMDATA
		  	break;
		  day=GetAtmosphereGrid()->GetAtmDay(count);
		  ddate=day+GetJulianDays(month);
	} while(ddate!=curdate);
	 if(month!=-1)						// if hit end of ATMDATA data
	{	do
		{    xmonth=GetAtmosphereGrid()->GetAtmMonth(count);
			xday=GetAtmosphereGrid()->GetAtmDay(count);
			hhour=GetAtmosphereGrid()->GetAtmHour(count);
			if(xday>day || xmonth>month)
				break;
			   count++;
		} while(hours>=hhour);
		  --count;
		  GetAtmosphereGrid()->GetAtmosphereValue(ATMCLOUD, XLocation, YLocation, count, &cloud_s);
	}

	 return (long) cloud_s;
}
*/


//============================================================================

double FireEnvironment2::GetMx( double Time, long fuel, long elev, long slope,
                                double aspectf, long cover, double* equil,

                                double* solrad, long FuelSize )	// millivolts
{ //FireEnvironment2::GetMx
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::GetMx:1 "
            "Time=%lf fuel=%ld elev=%ld FuelSize=%ld\n",
            CallLevel, "", Time, fuel, elev, FuelSize );

  // Time = minutes
  // fuel = index to fuel model, not zero-based
  // elev = land elevation, meters
  // radiation = millivolts

  // equil =equilibrium moisture content

  long i, Aspect;
  long sn = StationNumber;
  long ElevIndex, ElevIndexN, SlopeIndex, SlopeIndexN;
  long AspIndex, AspIndexN, CovIndex, CovIndexN;
  double loc;
  double efract, sfract, afract, cfract, ipart;
  double mx[16];
  double mxavg[8], timeavg1, timeavg2;
  double tmxavg, timefract, LastTime, FirstTime;

  if( fuel < 1 ) {
    CallLevel--;
    return 0.0;
  }

  if( MxDesc.NumFuels[sn][FuelSize] == 0 ) {
    CallLevel--;
    return 0.0;
  }

  if( Verbose >= CallLevel ) {
    printf( "%*sfsxwfms2:FireEnvironment2::GetMx:2 "
            "sn=%ld Time=%lf fuel=%ld FuelSize=%ld (%ld)\n",
            CallLevel, "", sn, Time, fuel, FuelSize,
            Stations[sn].FuelKey[FuelSize][fuel- 1] - 1 );
  }
  if( *solrad == 1 ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwfms2:FireEnvironment2::GetMx:2a\n", CallLevel, "" );
    CheckMoistureHistory( sn, FuelSize, Time );
    if( FuelSize == SIZECLASS_10HR )
      CheckMoistureHistory( sn, SIZECLASS_1HR, Time );
  }
  else if( Stations[sn].FMS[FuelSize][
                    Stations[sn].FuelKey[FuelSize][fuel- 1] - 1]->FirstTime >
           Time ||
           Stations[sn].FMS[FuelSize][
                    Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->LastTime <
           Time ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwfms2:FireEnvironment2::GetMx:2b\n", CallLevel, "" );
    CheckMoistureHistory( sn, FuelSize, Time );
    if( FuelSize == SIZECLASS_10HR )
      CheckMoistureHistory( sn, SIZECLASS_1HR, Time );
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::GetMx:3\n", CallLevel, "" );
  if( GetTheme_Units(E_DATA) == 0 )
    loc = ( (double) (elev - GetLoElev())) /
            (double) GetMoistCalcInterval(FuelSize, FM_INTERVAL_ELEV);
  else
    loc = ( (double) (elev - GetLoElev() * 0.3048)) /
            (double) GetMoistCalcInterval(FuelSize, FM_INTERVAL_ELEV);
  efract = modf( loc, &ipart );
  ElevIndex = (long) ipart;
  if( ElevIndex < 0 ) ElevIndex = 0;
  ElevIndexN = ElevIndex;

  if( ElevIndex > MxDesc.NumElevs[sn][FuelSize] - 1 )
    ElevIndex = MxDesc.NumElevs[sn][FuelSize] - 1;
  else ElevIndexN++;
  if( ElevIndexN > MxDesc.NumElevs[sn][FuelSize] - 1 )
    ElevIndexN = MxDesc.NumElevs[sn][FuelSize] - 1;

  SlopeIndexN = SlopeIndex
              = slope / GetMoistCalcInterval(FuelSize, FM_INTERVAL_SLOPE);
  if( SlopeIndex > MxDesc.NumSlopes[sn][FuelSize] - 1 )
    SlopeIndex = MxDesc.NumSlopes[sn][FuelSize] - 1;
  else SlopeIndexN++;
  if( SlopeIndexN > MxDesc.NumSlopes[sn][FuelSize] - 1 )
    SlopeIndexN = MxDesc.NumSlopes[sn][FuelSize] - 1;
  loc = (double) (slope -
         (SlopeIndex * GetMoistCalcInterval(FuelSize, FM_INTERVAL_SLOPE))) /
         (double) GetMoistCalcInterval(FuelSize, FM_INTERVAL_SLOPE);
  sfract = modf( loc, &ipart );

  Aspect = (long) (aspectf / PI * 180.0);
  AspIndexN = AspIndex
            = Aspect / GetMoistCalcInterval(FuelSize, FM_INTERVAL_ASP);

  if( AspIndex > MxDesc.NumAspects[sn][FuelSize] - 1 )
    AspIndex = MxDesc.NumAspects[sn][FuelSize] - 1;
  else AspIndexN++;
  if( AspIndexN > MxDesc.NumAspects[sn][FuelSize] - 1 )
    AspIndexN = MxDesc.NumAspects[sn][FuelSize] - 1;

  loc = (double) (Aspect -
         (AspIndex * GetMoistCalcInterval(FuelSize, FM_INTERVAL_ASP))) /
         (double) GetMoistCalcInterval(FuelSize, FM_INTERVAL_ASP);
  afract = modf( loc, &ipart );

  CovIndexN = CovIndex
            = cover / GetMoistCalcInterval(FuelSize, FM_INTERVAL_COV);
  if( CovIndex > MxDesc.NumCovers[sn][FuelSize] - 1 )
    CovIndex = MxDesc.NumCovers[sn][FuelSize] - 1;
  else CovIndexN++;
  if( CovIndexN > MxDesc.NumCovers[sn][FuelSize] - 1 )
    CovIndexN = MxDesc.NumCovers[sn][FuelSize] - 1;
  loc = (double) (cover -
         (CovIndex * GetMoistCalcInterval(FuelSize, FM_INTERVAL_COV))) /
         (double) GetMoistCalcInterval(FuelSize, FM_INTERVAL_COV);
  cfract = modf( loc, &ipart );

  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::GetMx:4\n", CallLevel, "" );
	mx[0] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->LastMx[CovIndex];
	mx[1] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->LastMx[CovIndexN];
	mx[2] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->LastMx[CovIndex];
	mx[3] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->LastMx[CovIndexN];
	mx[4] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->LastMx[CovIndex];
	mx[5] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->LastMx[CovIndexN];
	mx[6] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->LastMx[CovIndex];
	mx[7] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->LastMx[CovIndexN];
	mx[8] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->LastMx[CovIndex];
	mx[9] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->LastMx[CovIndexN];
	mx[10] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->LastMx[CovIndex];
	mx[11] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->LastMx[CovIndexN];
	mx[12] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->LastMx[CovIndex];
	mx[13] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->LastMx[CovIndexN];
	mx[14] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->LastMx[CovIndex];
	mx[15] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->LastMx[CovIndexN];

	// interpolate Cover
	for (i = 0; i < 8; i++)
	{
		mxavg[i] = mx[i * 2] - cfract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Aspect
	for (i = 0; i < 4; i++)
	{
		mxavg[i] = mx[i * 2] - afract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Slope
	for (i = 0; i < 2; i++)
	{
		mxavg[i] = mx[i * 2] - sfract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	timeavg1 = mx[i] - efract * (mx[i] - mx[i + 1]);

	mx[0] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->NextMx[CovIndex];
	mx[1] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->NextMx[CovIndexN];
	mx[2] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->NextMx[CovIndex];
	mx[3] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->NextMx[CovIndexN];
	mx[4] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->NextMx[CovIndex];
	mx[5] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->NextMx[CovIndexN];
	mx[6] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->NextMx[CovIndex];
	mx[7] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->NextMx[CovIndexN];
	mx[8] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->NextMx[CovIndex];
	mx[9] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->NextMx[CovIndexN];
	mx[10] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->NextMx[CovIndex];
	mx[11] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->NextMx[CovIndexN];
	mx[12] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->NextMx[CovIndex];
	mx[13] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->NextMx[CovIndexN];
	mx[14] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->NextMx[CovIndex];
	mx[15] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->NextMx[CovIndexN];

	// interpolate Cover
	for (i = 0; i < 8; i++)
	{
		mxavg[i] = mx[i * 2] - cfract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Aspect
	for (i = 0; i < 4; i++)
	{
		mxavg[i] = mx[i * 2] - afract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Slope
	for (i = 0; i < 2; i++)
	{
		mxavg[i] = mx[i * 2] - sfract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Elevation
	timeavg2 = mx[i] - efract * (mx[i] - mx[i + 1]);

	// interpolate over timeinterval
	FirstTime = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->FirstTime;
	LastTime = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->LastTime;
	if (LastTime - FirstTime > 1e-6)
		timefract = (Time - FirstTime) / (LastTime - FirstTime);
	else
		timefract = 1.0;
	if (timefract > 1.0)
		timefract = 1.0;
	tmxavg = timeavg1 - timefract * (timeavg1 - timeavg2);

	if (FuelSize != SIZECLASS_10HR) {
          CallLevel--;
		return tmxavg;
        }

	// finds mean equilibrium moisture content for use in 1-hour fuels
	mx[0] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->LastEq[CovIndex];
	mx[1] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->LastEq[CovIndexN];
	mx[2] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->LastEq[CovIndex];
	mx[3] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->LastEq[CovIndexN];
	mx[4] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->LastEq[CovIndex];
	mx[5] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->LastEq[CovIndexN];
	mx[6] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->LastEq[CovIndex];
	mx[7] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->LastEq[CovIndexN];
	mx[8] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->LastEq[CovIndex];
	mx[9] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->LastEq[CovIndexN];
	mx[10] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->LastEq[CovIndex];
	mx[11] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->LastEq[CovIndexN];
	mx[12] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->LastEq[CovIndex];
	mx[13] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->LastEq[CovIndexN];
	mx[14] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->LastEq[CovIndex];
	mx[15] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->LastEq[CovIndexN];


	// interpolate Cover
	for (i = 0; i < 8; i++)
	{
		mxavg[i] = mx[i * 2] - cfract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Aspect
	for (i = 0; i < 4; i++)
	{
		mxavg[i] = mx[i * 2] - afract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Slope
	for (i = 0; i < 2; i++)
	{
		mxavg[i] = mx[i * 2] - sfract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	timeavg1 = mx[i] - efract * (mx[i] - mx[i + 1]);

	mx[0] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->NextEq[CovIndex];
	mx[1] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->NextEq[CovIndexN];
	mx[2] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->NextEq[CovIndex];
	mx[3] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->NextEq[CovIndexN];
	mx[4] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->NextEq[CovIndex];
	mx[5] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->NextEq[CovIndexN];
	mx[6] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->NextEq[CovIndex];
	mx[7] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->NextEq[CovIndexN];
	mx[8] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->NextEq[CovIndex];
	mx[9] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->NextEq[CovIndexN];
	mx[10] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->NextEq[CovIndex];
	mx[11] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->NextEq[CovIndexN];
	mx[12] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->NextEq[CovIndex];
	mx[13] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->NextEq[CovIndexN];
	mx[14] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->NextEq[CovIndex];
	mx[15] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->NextEq[CovIndexN];
//printf("AAA fsxwfms2:FireEnvironment2::GetMx 2 sn=%d FuelSize=%d fuel=%d ElevIndex=%d SlopeIndex=%d AspIndexN=%d CovIndex=%d CovIndexN=%d\n",sn,FuelSize,fuel,ElevIndex,SlopeIndex,AspIndexN,CovIndex,CovIndexN);

//printf("AAA fsxwfms2:FireEnvironment2::GetMx 3 mx[2]=%f mx[3]=%f\n",mx[2],mx[3]);


	// interpolate Cover
	for (i = 0; i < 8; i++)
	{
		mxavg[i] = mx[i * 2] - cfract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Aspect
	for (i = 0; i < 4; i++)
	{
		mxavg[i] = mx[i * 2] - afract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Slope
	for (i = 0; i < 2; i++)
	{
		mxavg[i] = mx[i * 2] - sfract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Elevation
	timeavg2 = mx[i] - efract * (mx[i] - mx[i + 1]);

	*equil = timeavg1 - timefract * (timeavg1 - timeavg2);	// average over elevation


	if (*solrad == 0) {
          CallLevel--;
		return tmxavg;
        }

	// interpolate solarradiation
	mx[0] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->SolRad;
	mx[1] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->SolRad;
	mx[2] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->SolRad;
	mx[3] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->SolRad;
	mx[4] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->SolRad;
	mx[5] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->SolRad;
	mx[6] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->SolRad;
	mx[7] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndex]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->SolRad;

	// interpolate Cover
	for (i = 0; i < 4; i++)
	{
		mxavg[i] = mx[i * 2] - cfract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Aspect
	for (i = 0; i < 2; i++)
	{
		mxavg[i] = mx[i * 2] - afract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Slope
	timeavg1 = mx[i * 2] - sfract * (mx[i * 2] - mx[i * 2 + 1]);

	mx[0] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->SolRad;
	mx[1] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndex]->SolRad;
	mx[2] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->SolRad;
	mx[3] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndex]->Fms_Aspect[AspIndexN]->SolRad;
	mx[4] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->SolRad;
	mx[5] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndex]->SolRad;
	mx[6] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->SolRad;
	mx[7] = Stations[sn].FMS[FuelSize][Stations[sn].FuelKey[FuelSize][fuel - 1] - 1]->Fms_Elev[ElevIndexN]->Fms_Slope[SlopeIndexN]->Fms_Aspect[AspIndexN]->SolRad;

	// interpolate Cover
	for (i = 0; i < 4; i++)
	{
		mxavg[i] = mx[i * 2] - cfract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Aspect
	for (i = 0; i < 2; i++)
	{
		mxavg[i] = mx[i * 2] - afract * (mx[i * 2] - mx[i * 2 + 1]);
		mx[i] = mxavg[i];
	}
	// interpolate Slope
	timeavg2 = mx[i * 2] - sfract * (mx[i * 2] - mx[i * 2 + 1]);

	*solrad = timeavg1 - timefract * (timeavg1 - timeavg2);

  CallLevel--;
  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::GetMx:5\n", CallLevel, "" );

  return tmxavg;
} //FireEnvironment2::GetMx


bool FireEnvironment2::AllocHistory(long Station, long FuelSize)
{
	if (FirstHist[Station][FuelSize] == 0)
	{
		FirstHist[Station][FuelSize] = new DeadMoistureHistory;
		CurHist[Station][FuelSize] = FirstHist[Station][FuelSize];
	}
	else
	{
		while (CurHist[Station][FuelSize]->next != NULL)
		{
			NextHist[Station][FuelSize] = (DeadMoistureHistory *)
				CurHist[Station][FuelSize]->next;
			CurHist[Station][FuelSize] = NextHist[Station][FuelSize];
		};
		CurHist[Station][FuelSize]->next = (DeadMoistureHistory *) new DeadMoistureHistory;
		NextHist[Station][FuelSize] = (DeadMoistureHistory *)
			CurHist[Station][FuelSize]->next;
		CurHist[Station][FuelSize] = NextHist[Station][FuelSize];
	}

	CurHist[Station][FuelSize]->Moisture = new double[MxDesc.NumAlloc[Station][FuelSize]];
	if (CurHist[Station][FuelSize]->Moisture == NULL)
		return false;
	memset(CurHist[Station][FuelSize]->Moisture, 0x0,
		MxDesc.NumAlloc[Station][FuelSize] * sizeof(double));
	CurHist[Station][FuelSize]->next = 0;

	MxDesc.NumHist[Station][FuelSize]++;

	return true;
}


void FireEnvironment2::FreeHistory(long Station, long FuelSize)
{
	long i, j;

	i = FuelSize;
	CurHist[Station][i] = FirstHist[Station][i];
	for (j = 0; j < MxDesc.NumHist[Station][i]; j++)
	{
		delete[] CurHist[Station][i]->Moisture;
		NextHist[Station][i] = (DeadMoistureHistory *)
			CurHist[Station][i]->next;
		delete CurHist[Station][i];
		CurHist[Station][i] = NextHist[Station][i];
	}
	FirstHist[Station][i] = 0;
	CurHist[Station][i] = 0;
	NextHist[Station][i] = 0;
	MxDesc.ResetDescription(Station, FuelSize);
}


void FireEnvironment2::TerminateHistory()
{
	FreeStations();
}

//============================================================================
void FireEnvironment2::RefreshHistoryDescription(long Station, long FuelSize)
{ //FireEnvironment2::RefreshHistoryDescription
	if (NumStations == 0) return;

	MxDesc.NumStations = NumStations;
	if (Stations[0].NumFuels[FuelSize] == 0) return;

	MxDesc.NumElevs[Station][FuelSize] = Stations[0].FMS[FuelSize][0]->NumElevs;
	MxDesc.NumSlopes[Station][FuelSize] = Stations[0].FMS[FuelSize][0]->Fms_Elev[0]->NumSlopes;
	MxDesc.NumAspects[Station][FuelSize] = Stations[0].FMS[FuelSize][0]->Fms_Elev[0]->Fms_Slope[0]->NumAspects;
	MxDesc.NumCovers[Station][FuelSize] = Stations[0].FMS[FuelSize][0]->Fms_Elev[0]->Fms_Slope[0]->Fms_Aspect[0]->NumFms;
	MxDesc.NumFuels[Station][FuelSize] = Stations[0].NumFuels[FuelSize];
	memcpy(MxDesc.FuelKey[Station][FuelSize],
		Stations[0].FuelKey[FuelSize], MAXNUM_FUEL_MODELS * sizeof(long));
	MxDesc.NumAlloc[Station][FuelSize] = MxDesc.NumFuels[Station][FuelSize] * MxDesc.NumElevs[Station][FuelSize] * MxDesc.NumSlopes[Station][FuelSize] * MxDesc.NumAspects[Station][FuelSize] * MxDesc.NumCovers[Station][FuelSize];
	MxDesc.EndTime[Station][FuelSize] = 0.0;

	if (FuelSize == 1) {
		MxDesc.NumElevs[Station][0] = MxDesc.NumElevs[Station][1];
		MxDesc.NumSlopes[Station][0] = MxDesc.NumSlopes[Station][1];
		MxDesc.NumAspects[Station][0] = MxDesc.NumAspects[Station][1];
		MxDesc.NumCovers[Station][0] = MxDesc.NumCovers[Station][1];
		MxDesc.NumFuels[Station][SIZECLASS_1HR] = MxDesc.NumFuels[Station][1];
		MxDesc.NumAlloc[Station][0] = MxDesc.NumAlloc[Station][SIZECLASS_10HR];
	}

	if( MxDesc.fms ) delete[] MxDesc.fms;
	MxDesc.fms = 0;

} //FireEnvironment2::RefreshHistoryDescription


void FireEnvironment2::NormalizeMoistureStartTime()
{
	if (UseConditioningPeriod(GETVAL) == false)
		return;

	if (MxDesc.NumAlloc[0][1] == 0)
		return;

	long i, j, k;
	double StartTime, SimDiff;

	StartTime = ConvertActualTimeToSimtime(GetStartMonth(), GetStartDay(),
					GetStartHour(), GetStartMin(), true);
	if (SimStart == -1.0)
	{
		SimStart = StartTime;

		return;
	}

	if (SimStart == StartTime)
		return;

	SimDiff = SimStart - StartTime;
	SimStart = StartTime;

	for (i = 0; i < NumStations; i++)
	{
		for (j = 0; j < NUM_FUEL_SIZES; j++)
		{
			CurHist[i][j] = FirstHist[i][j];
			for (k = 0; k < MxDesc.NumHist[i][j]; k++)
			{
				CurHist[i][j]->LastTime += SimDiff;
				CurHist[i][j]->SimTime += SimDiff;
				NextHist[i][j] = (DeadMoistureHistory *) CurHist[i][j]->next;
				CurHist[i][j] = NextHist[i][j];
			}
			CurHist[i][j] = FirstHist[i][j];
		}
	}
}

//============================================================================
bool FireEnvironment2::CheckMoistureTimes( double SimTime )
{ //FireEnvironment2::CheckMoistureTimes
  for( long i = 0; i < NumStations; i++ ) {
    for( long j = 1; j < NUM_FUEL_SIZES; j++ ) {
      CheckMoistureHistory( i, j, SimTime );
      if( j == 1 ) CheckMoistureHistory( i, 0, SimTime );
    }
  }

  return true;
} //FireEnvironment2::CheckMoistureTimes

//============================================================================
bool FireEnvironment2::CheckMoistureHistory(long Station, long FuelSize,
	double SimTime)
{
	//bool COPY_ALL=false;
	long i;

	if (EnvironmentChanged(GETVAL, Station, FuelSize))
		return false;

	if (SimTime > MxDesc.EndTime[Station][FuelSize])
		return false;

	//if(SimTime==GetActualTimeStep())
	//{	CopyMoistureHistory(Station, FuelSize);
	//
	//     return true;
	//}

	if (MxDesc.NumFuels[Station][FuelSize] == 0)
		return true;

	if (MxDesc.NumHist[Station][FuelSize] == 0)
		return false;

	//if(FuelSize==0)
	//{	if(SimTime>=CurHist[Station][SIZECLASS_10HR]->LastTime && SimTime<=CurHist[Station][SIZECLASS_10HR]->SimTime)	// if within current time span
	//		return true;
	//}
	//else
	//{

	if (SimTime >= CurHist[Station][FuelSize]->LastTime &&
		SimTime <= CurHist[Station][FuelSize]->SimTime)	// if within current time span
		return true;
	//}

	//FILE *mxout;
	DeadMoistureHistory* lasthist, * thishist;

	// if SimTime is later than current, move forward until current or exceed data
	if (SimTime > CurHist[Station][FuelSize]->SimTime)
	{
		//if(SimTime-CurHist[Station][FuelSize]->SimTime<=GetActualTimeStep())
		//	COPY_ALL=true;
		CurHist[Station][FuelSize] = lasthist = FirstHist[Station][FuelSize];
		for (i = 0; i < MxDesc.NumHist[Station][FuelSize]; i++)
		{
			if (SimTime >= CurHist[Station][FuelSize]->LastTime &&
				SimTime <= CurHist[Station][FuelSize]->SimTime)
			{
				//if(COPY_ALL) // only do this 1st time
				//{
				thishist = CurHist[Station][FuelSize];
				CurHist[Station][FuelSize] = lasthist;
				CopyMoistureHistory(Station, FuelSize);
				CurHist[Station][FuelSize] = thishist;
				//     COPY_ALL=false;
				//}
				CopyMoistureHistory(Station, FuelSize);
				break;
			}
			lasthist = CurHist[Station][FuelSize];
			NextHist[Station][FuelSize] = (DeadMoistureHistory *)
				CurHist[Station][FuelSize]->next;
			CurHist[Station][FuelSize] = NextHist[Station][FuelSize];
		}
		if (i == MxDesc.NumHist[Station][FuelSize])	// this will indicate if SimTime is greater than the
			return false;				// data in the History
	}

	// if SimTime is earlier than current, start from 0 and work forward until current
	else if (SimTime < CurHist[Station][FuelSize]->LastTime)
	{
		CurHist[Station][FuelSize] = lasthist = FirstHist[Station][FuelSize];
		for (i = 0; i < MxDesc.NumHist[Station][FuelSize]; i++)
		{
			if (SimTime >= CurHist[Station][FuelSize]->LastTime &&
				SimTime <= CurHist[Station][FuelSize]->SimTime)
			{
				//if(COPY_ALL)
				//{
				thishist = CurHist[Station][FuelSize];
				CurHist[Station][FuelSize] = lasthist;
				CopyMoistureHistory(Station, FuelSize);
				CurHist[Station][FuelSize] = thishist;
				//    COPY_ALL=false;
				//}
				CopyMoistureHistory(Station, FuelSize);
				break;
			}
			lasthist = CurHist[Station][FuelSize];
			NextHist[Station][FuelSize] = (DeadMoistureHistory *)
				CurHist[Station][FuelSize]->next;
			CurHist[Station][FuelSize] = NextHist[Station][FuelSize];
		}
	}

	return true;
}

//============================================================================
void FireEnvironment2::CopyMoistureHistory( long Station, long FuelSize )
{ //FireEnvironment2::CopyMoistureHistory
	long i, j, k, m, n, p, loc;
	long NFuel;
	double Moist;


	NFuel = MxDesc.NumFuels[Station][FuelSize];
	i = Station;
	switch (FuelSize)
	{
	case SIZECLASS_1HR:
		//1hour
		//for(i=0; i<NumStations; i++)
		//{
		for (j = 0; j < NFuel; j++)
		{
			for (k = 0; k < MxDesc.NumElevs[i][FuelSize]; k++)
			{
				for (m = 0; m < MxDesc.NumSlopes[i][FuelSize]; m++)
				{
					for (n = 0; n < MxDesc.NumAspects[i][FuelSize]; n++)
					{
						for (p = 0; p < MxDesc.NumCovers[i][FuelSize]; p++)
						{
							if (CurHist[i][FuelSize]->LastTime == 0.0 &&
								UseConditioningPeriod(GETVAL) == false)
								Stations[i].FMS[SIZECLASS_10HR][j]->Fms_Elev[k]->Fms_Slope[m]->
								Fms_Aspect[n]->LastEq[p] = ((double)
									GetInitialFuelMoisture(Stations[i].FMS[SIZECLASS_10HR][j]->Fuel,
										FuelSize)) /
									100.0;
							else
								Stations[i].FMS[SIZECLASS_10HR][j]->Fms_Elev[k]->Fms_Slope[m]->
								Fms_Aspect[n]->LastEq[p] = Stations[i].FMS[SIZECLASS_10HR][j]->Fms_Elev[k]->Fms_Slope[m]->
								Fms_Aspect[n]->NextEq[p];
							loc = //i*MxDesc.NumFuels[i][1]*MxDesc.NumElevs[i][FuelSize]*MxDesc.NumSlopes[i][FuelSize]*MxDesc.NumAspects[i][FuelSize]*MxDesc.NumCovers[i][FuelSize]+
							j * MxDesc.NumElevs[i][FuelSize] * MxDesc.NumSlopes[i][FuelSize] * MxDesc.NumAspects[i][FuelSize] * MxDesc.NumCovers[i][FuelSize] +
								k * MxDesc.NumSlopes[i][FuelSize] * MxDesc.NumAspects[i][FuelSize] * MxDesc.NumCovers[i][FuelSize] +
								m * MxDesc.NumAspects[i][FuelSize] * MxDesc.NumCovers[i][FuelSize] +
								n * MxDesc.NumCovers[i][FuelSize] +
								p;

							Stations[i].FMS[SIZECLASS_10HR][j]->Fms_Elev[k]->Fms_Slope[m]->
							Fms_Aspect[n]->NextEq[p] = CurHist[i][FuelSize]->Moisture[loc];

							//Stations[i].FMS[SIZECLASS_10HR][j]->FirstTime=CurHist[i][FuelSize]->LastTime;
							//Stations[i].FMS[SIZECLASS_10HR][j]->LastTime=CurHist[i][FuelSize]->SimTime;
						}
					}
				}
			}
		}
		//}
		break;
	default:
		//10,,100, 1000 hr
		//for(i=0; i<NumStations; i++)
		//{
		for (j = 0; j < NFuel; j++)
		{
			for (k = 0; k < MxDesc.NumElevs[i][FuelSize]; k++)
			{
				for (m = 0; m < MxDesc.NumSlopes[i][FuelSize]; m++)
				{
					for (n = 0; n < MxDesc.NumAspects[i][FuelSize]; n++)
					{
						for (p = 0; p < MxDesc.NumCovers[i][FuelSize]; p++)
						{
							if (CurHist[i][FuelSize]->LastTime == 0.0 &&
								UseConditioningPeriod(GETVAL) == false)
							{
								if (FuelSize < 3)
									Stations[i].FMS[FuelSize][j]->Fms_Elev[k]->Fms_Slope[m]->
									Fms_Aspect[n]->LastMx[p] = ((double)
										GetInitialFuelMoisture(Stations[i].FMS[FuelSize][j]->Fuel,
											FuelSize)) /
										100.0;
								else
								{
                  Moist = Get1000HrMoisture(Stations[i].FMS[FuelSize][j]->Fuel, true) /100.0; //AAA In FromG5 ver

									if (Moist <= 0.10)
										Moist = 0.10;
									Stations[i].FMS[FuelSize][j]->Fms_Elev[k]->Fms_Slope[m]->
									Fms_Aspect[n]->LastMx[p] = Moist;
								}
							}
							else
								Stations[i].FMS[FuelSize][j]->Fms_Elev[k]->Fms_Slope[m]->
								Fms_Aspect[n]->LastMx[p] = Stations[i].FMS[FuelSize][j]->Fms_Elev[k]->Fms_Slope[m]->
								Fms_Aspect[n]->NextMx[p];
							loc = //i*MxDesc.NumFuels[i][1]*MxDesc.NumElevs[i][FuelSize]*MxDesc.NumSlopes[i][FuelSize]*MxDesc.NumAspects[i][FuelSize]*MxDesc.NumCovers[i][FuelSize]+
							j * MxDesc.NumElevs[i][FuelSize] * MxDesc.NumSlopes[i][FuelSize] * MxDesc.NumAspects[i][FuelSize] * MxDesc.NumCovers[i][FuelSize] +
								k * MxDesc.NumSlopes[i][FuelSize] * MxDesc.NumAspects[i][FuelSize] * MxDesc.NumCovers[i][FuelSize] +
								m * MxDesc.NumAspects[i][FuelSize] * MxDesc.NumCovers[i][FuelSize] +
								n * MxDesc.NumCovers[i][FuelSize] +
								p;

							Stations[i].FMS[FuelSize][j]->Fms_Elev[k]->Fms_Slope[m]->
							Fms_Aspect[n]->NextMx[p] = CurHist[i][FuelSize]->Moisture[loc];
							Stations[i].FMS[FuelSize][j]->FirstTime = CurHist[i][FuelSize]->LastTime;
							Stations[i].FMS[FuelSize][j]->LastTime = CurHist[i][FuelSize]->SimTime;
							if (FuelSize == SIZECLASS_10HR)
							{
								elevref = CurHist[i][FuelSize]->Elevation;
								tempref = CurHist[i][FuelSize]->AirTemperature;
								humref = CurHist[i][FuelSize]->RelHumidity;
								rain = CurHist[i][FuelSize]->Rainfall;
							}
						}
					}
				}
			}
		}
		//}
		break;
	}
} //FireEnvironment2::CopyMoistureHistory


bool FireEnvironment2::ExportMoistureData(char* FileName, char* Desc)
{
	// write out all moisture history data here.
	long i, j, k, NumNodes;
	unsigned long MaxAlloc = 0;
	char Version[16] = "Version4.1";
	char LcpName[256] = "";

	FILE* fout = fopen(FileName, "wb");
	if (fout == NULL)
		return false;

	char RevCopy[256];
	char pathdiv[] = "/";              		// search for path
	strcpy(RevCopy, GetLandFileName());	// copy file path string
	strrev(RevCopy);						// reverse file path string
	int length = strcspn(RevCopy, pathdiv);   // compute length without path
	strncpy(LcpName, RevCopy, length);		// copy length w/o path
	strrev(LcpName);						// reverse to get filename only

	fwrite(Version, sizeof(char), 16, fout);
	fwrite(Desc, sizeof(char), 512, fout);
	fwrite(&NumStations, sizeof(long), 1, fout);
	fwrite(LcpName, sizeof(char), 256, fout);

	for (i = 0; i < NumStations; i++)
	{
		for (j = 0; j < NUM_FUEL_SIZES; j++)
		{
			fwrite(&MxDesc.NumAlloc[i][j], sizeof(unsigned long), 4, fout);
			fwrite(&MxDesc.NumElevs[i][j], sizeof(long), 4, fout);
			fwrite(&MxDesc.NumSlopes[i][j], sizeof(long), 4, fout);
			fwrite(&MxDesc.NumAspects[i][j], sizeof(long), 4, fout);
			fwrite(&MxDesc.NumCovers[i][j], sizeof(long), 4, fout);
			fwrite(&MxDesc.NumFuels[i][j], sizeof(long), 4, fout);
			fwrite(&MxDesc.FuelKey[i][j], sizeof(long), MAXNUM_FUEL_MODELS,
				fout);
			fwrite(&MxDesc.NumHist[i][j], sizeof(long), 4, fout);
			fwrite(&MxDesc.EndTime[i][j], sizeof(double), 4, fout);
		}
	}

	for (i = 0; i < NumStations; i++)
	{
		for (j = 0; j < NUM_FUEL_SIZES; j++)
		{
			if (MxDesc.NumAlloc[i][j] > MaxAlloc)
				MaxAlloc = MxDesc.NumAlloc[i][j];
		}
	}
	if (MxDesc.fms)
		delete[] MxDesc.fms;
	MxDesc.fms = new Fms * [MaxAlloc];

	for (i = 0; i < NumStations; i++)
	{
		for (j = 1; j < NUM_FUEL_SIZES; j++)
		{
			CopyFmsData(i, j, true);
			for (k = 0; k < (long) MxDesc.NumAlloc[i][j]; k++)
			{
				fwrite(MxDesc.fms[k], sizeof(Fms), 1, fout);
				NumNodes = MxDesc.fms[k]->n;
				fwrite(MxDesc.fms[k]->x, sizeof(double), NumNodes, fout);
				fwrite(MxDesc.fms[k]->d, sizeof(double), NumNodes, fout);
				fwrite(MxDesc.fms[k]->t, sizeof(double), NumNodes, fout);
				fwrite(MxDesc.fms[k]->s, sizeof(double), NumNodes, fout);
				fwrite(MxDesc.fms[k]->w, sizeof(double), NumNodes, fout);
				fwrite(MxDesc.fms[k]->v, sizeof(double), NumNodes, fout);
			}
		}
	}
	long NumHist;
	unsigned long NumAlloc;
	for (i = 0; i < NumStations; i++)
	{
		for (j = 0; j < NUM_FUEL_SIZES; j++)
		{
			NumHist = MxDesc.NumHist[i][j];
			NumAlloc = MxDesc.NumAlloc[i][j];
			CurHist[i][j] = FirstHist[i][j];
			for (k = 0; k < NumHist; k++)
			{
				fwrite(&CurHist[i][j]->LastTime, sizeof(double), 2, fout);
				fwrite(&CurHist[i][j]->Elevation, sizeof(long), 1, fout);
				fwrite(&CurHist[i][j]->AirTemperature, sizeof(double), 2, fout);
				fwrite(&CurHist[i][j]->CloudCover, sizeof(long), 1, fout);
				fwrite(&CurHist[i][j]->Rainfall, sizeof(double), 1, fout);
				fwrite(CurHist[i][j]->Moisture, sizeof(double), NumAlloc, fout);
				NextHist[i][j] = (DeadMoistureHistory *) CurHist[i][j]->next;
				CurHist[i][j] = NextHist[i][j];
			}
			CurHist[i][j] = FirstHist[i][j];
		}
	}
	fclose(fout);
	if (MxDesc.fms)
		delete[] MxDesc.fms;
	MxDesc.fms = 0;

	return true;
}


bool FireEnvironment2::ImportMoistureData(char* FileName, char* Desc)
{
	long i, j, k, NumNodes, FileVersion;
	unsigned long MaxAlloc = 0;
	double* ptr[6];
	char* cptr;
	char Version[16] = "";
	char Name[256] = "";

	FILE* fin = fopen(FileName, "rb");
	if (fin == NULL)
		return false;

	// copy in all moisture history data here.

	fread(Version, sizeof(char), 16, fin);
	if (!strcmp(Version, "Version4.0"))
		FileVersion = 1;
	else
		FileVersion = 2;
	fread(Desc, sizeof(char), 512, fin);
	FreeStations();
	fread(&NumStations, sizeof(long), 1, fin);
	MxDesc.NumStations = NumStations;
	AllocStations(NumStations);
	fread(&Name, sizeof(char), 256, fin);


	for (i = 0; i < NumStations; i++)
	{
		for (j = 0; j < NUM_FUEL_SIZES; j++)
			FreeHistory(i, j);
	}

	for (i = 0; i < NumStations; i++)
	{
		for (j = 0; j < NUM_FUEL_SIZES; j++)
		{
			fread(&MxDesc.NumAlloc[i][j], sizeof(unsigned long), 4, fin);
			fread(&MxDesc.NumElevs[i][j], sizeof(long), 4, fin);
			fread(&MxDesc.NumSlopes[i][j], sizeof(long), 4, fin);
			fread(&MxDesc.NumAspects[i][j], sizeof(long), 4, fin);
			fread(&MxDesc.NumCovers[i][j], sizeof(long), 4, fin);
			fread(&MxDesc.NumFuels[i][j], sizeof(long), 4, fin);
			if (FileVersion == 1)
				fread(&MxDesc.FuelKey[i][j], sizeof(long), 100, fin);
			else
				fread(&MxDesc.FuelKey[i][j], sizeof(long), 256, fin);
			fread(&MxDesc.NumHist[i][j], sizeof(long), 4, fin);
			fread(&MxDesc.EndTime[i][j], sizeof(double), 4, fin);
		}
		//for(j=1; j<NUM_FUEL_SIZES; j++)
		//{	if(Stations[i].NumFuels[j]!=MxDesc.NumFuels[i][j])
		//	{	FreeStations();
		//     	fclose(fin);
		//		return false;
		//     }
		//}
	}

	for (i = 0; i < NumStations; i++)
	{
		for (j = 0; j < NUM_FUEL_SIZES; j++)
		{
			if (MxDesc.NumAlloc[i][j] > MaxAlloc)
				MaxAlloc = MxDesc.NumAlloc[i][j];
		}
	}

	if (MxDesc.fms)
		delete[] MxDesc.fms;
	MxDesc.fms = new Fms * [MaxAlloc * NUM_FUEL_SIZES];

	for (i = 0; i < NumStations; i++)
	{
		for (j = 1; j < NUM_FUEL_SIZES; j++)
		{
			CopyFmsData(i, j, false);	// copy the pointers from the site data to the MxDesc struct
			for (k = 0; k < (long) MxDesc.NumAlloc[i][j]; k++)
			{
				ptr[0] = MxDesc.fms[k]->x;  	 // must store allocated pointers in current fms struct
				ptr[1] = MxDesc.fms[k]->d;
				ptr[2] = MxDesc.fms[k]->t;
				ptr[3] = MxDesc.fms[k]->s;
				ptr[4] = MxDesc.fms[k]->w;
				ptr[5] = MxDesc.fms[k]->v;
				cptr = MxDesc.fms[k]->name;
				fread(MxDesc.fms[k], sizeof(Fms), 1, fin);
				NumNodes = MxDesc.fms[k]->n;
				MxDesc.fms[k]->x = ptr[0];  	 // restore allocated pointers
				MxDesc.fms[k]->d = ptr[1];
				MxDesc.fms[k]->t = ptr[2];  												
				MxDesc.fms[k]->s = ptr[3];
				MxDesc.fms[k]->w = ptr[4];
				MxDesc.fms[k]->v = ptr[5];
				MxDesc.fms[k]->name = cptr;
				fread(MxDesc.fms[k]->x, sizeof(double), NumNodes, fin);
				fread(MxDesc.fms[k]->d, sizeof(double), NumNodes, fin);
				fread(MxDesc.fms[k]->t, sizeof(double), NumNodes, fin);
				fread(MxDesc.fms[k]->s, sizeof(double), NumNodes, fin);
				fread(MxDesc.fms[k]->w, sizeof(double), NumNodes, fin);
				fread(MxDesc.fms[k]->v, sizeof(double), NumNodes, fin);
			}
		}
	}
	long NumHist;
	unsigned long NumAlloc;
	for (i = 0; i < NumStations; i++)
	{
		for (j = 0; j < NUM_FUEL_SIZES; j++)
		{
			NumHist = MxDesc.NumHist[i][j];
			NumAlloc = MxDesc.NumAlloc[i][j];
			MxDesc.NumHist[i][j] = 0;
			for (k = 0; k < NumHist; k++)
			{
				AllocHistory(i, j);
				fread(&CurHist[i][j]->LastTime, sizeof(double), 2, fin);
				fread(&CurHist[i][j]->Elevation, sizeof(long), 1, fin);
				fread(&CurHist[i][j]->AirTemperature, sizeof(double), 2, fin);
				fread(&CurHist[i][j]->CloudCover, sizeof(long), 1, fin);
				fread(&CurHist[i][j]->Rainfall, sizeof(double), 1, fin);
				fread(CurHist[i][j]->Moisture, sizeof(double), NumAlloc, fin);
				if (k == 0)
					FirstHist[i][j] = CurHist[i][j];
			}
			CurHist[i][j] = FirstHist[i][j];
		}
	}
	fclose(fin);
	if (MxDesc.fms)
		delete[] MxDesc.fms;
	MxDesc.fms = 0;

	return true;
}


void FireEnvironment2::CopyFmsData( long Station, long FuelSize, bool ToHistory )
{
  long i, j, k, m, n, p, q, loc;
  Fms* fms;

  if( ToHistory && Stations[0].FMS[1][0]->Fms_Elev[0]->Fms_Slope[0]->
      Fms_Aspect[0]->fms[0] == NULL )    // if no history to copy
    return;

  i = Station;
  q = FuelSize;
  for( j = 0; j < MxDesc.NumFuels[i][q]; j++ ) {
    for( k = 0; k < MxDesc.NumElevs[i][q]; k++ ) {
      for( m = 0; m < MxDesc.NumSlopes[i][q]; m++ ) {
        for( n = 0; n < MxDesc.NumAspects[i][q]; n++ ) {
          for( p = 0; p < MxDesc.NumCovers[i][q]; p++ ) {
            loc = j * MxDesc.NumElevs[i][q] * MxDesc.NumSlopes[i][q] *
                   MxDesc.NumAspects[i][q] * MxDesc.NumCovers[i][q] +
                  k * MxDesc.NumSlopes[i][q] * MxDesc.NumAspects[i][q] *
                   MxDesc.NumCovers[i][q] +
                  m * MxDesc.NumAspects[i][q] * MxDesc.NumCovers[i][q] +
                  n * MxDesc.NumCovers[i][q] + p;

            if( ToHistory ) {    // copy pointer from site data to MxDes.fms
              fms = Stations[i].FMS[q][j]->Fms_Elev[k]->Fms_Slope[m]->Fms_Aspect[n]->fms[p];
              MxDesc.fms[loc] = fms;
            }
            else {   // copy pointer from MxDesc to site data
              fms = Stations[i].FMS[q][j]->Fms_Elev[k]->Fms_Slope[m]->Fms_Aspect[n]->fms[p];
              if( fms != NULL )
                delete fms;

              switch( q ) {
                case SIZECLASS_1HR:
                  break;
                case SIZECLASS_10HR:
                  MxDesc.fms[loc] =
                   Stations[i].FMS[q][j]->Fms_Elev[k]->Fms_Slope[m]->Fms_Aspect[n]->fms[p] =
                   Fms_Create10Hour( (char *) "10hr" );
                  break;
                case SIZECLASS_100HR:
                  MxDesc.fms[loc] =
                   Stations[i].FMS[q][j]->Fms_Elev[k]->Fms_Slope[m]->Fms_Aspect[n]->fms[p] =
                   Fms_Create100Hour( (char *) "100hr" );
                  break;
                case SIZECLASS_1000HR:
                  MxDesc.fms[loc] =
                   Stations[i].FMS[q][j]->Fms_Elev[k]->Fms_Slope[m]->Fms_Aspect[n]->fms[p] =
                   Fms_Create1000Hour( (char *) "1000hr" );
                  break;
              }
            }
          }
        }
      }
    }
  }

  if( ! ToHistory )
    EnvironmentChanged( false, i, FuelSize );
}

//============================================================================
void FireEnvironment2::RunFmsThreads( double SimTime, long sn, long FuelType,
                                      long FuelSize )
{ //FireEnvironment2::RunFmsThreads
	bool StartAtBeginning = false;
	long i, j;//, FuelSize=1;
	long date, hour, tr1, tr2;
	long Cloud, MoistLoc;
	long begin, end, threadct, range;
	long mo, dy, hr, mn, min1, min2;
	double fract, interval, ipart, RainRate, Interval, r1, r2;
	double lasttime, checktime, hours;
	DeadMoistureHistory** hist = 0;

  if( MxDesc.NumFuels[sn][FuelSize] == 0 ) return;

  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::RunFmsThreads:1\n",
            CallLevel, "" );

	AllocFmsThreads();
	i = FuelType;

	interval = ((double) Stations[sn].FMS[FuelSize][i]->NumElevs) /
		((double) NumFmsThreads);
	fract = modf(interval, &ipart);
	range = (long) interval;
	if( fract > 0.0 ) range++;

  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::RunFmsThreads:2\n",
            CallLevel, "" );

  if( Stations[sn].FMS[FuelSize][FuelType]->LastTime == 0.0 ||
      MxDesc.NumHist[sn][FuelSize] == 0 ) {
    if( Verbose > CallLevel )
      printf( "%*sfsxwfms2:FireEnvironment2::RunFmsThreads:2a\n",
              CallLevel, "" );

    hist = new DeadMoistureHistory * [NUM_FUEL_SIZES];
		if( FuelType == 0 ) {
			switch( FuelSize ) {
        case SIZECLASS_100HR:
          AllocHistory( sn, SIZECLASS_100HR );  // 100hr fuels
          break;
        case SIZECLASS_1000HR:
          AllocHistory( sn, SIZECLASS_1000HR );  // 1000hr fuels
          break;
        default:
          AllocHistory( sn, SIZECLASS_1HR );  // 1hr fuels
          AllocHistory( sn, SIZECLASS_10HR );  // 10hr fuels
          break;
      }
    }
    else {
      CurHist[sn][FuelSize] = FirstHist[sn][FuelSize];
      if( FuelSize == SIZECLASS_10HR )
				CurHist[sn][SIZECLASS_1HR] = FirstHist[sn][SIZECLASS_1HR];
			StartAtBeginning = true;
		}
		hist[FuelSize] = CurHist[sn][FuelSize];
    if( FuelSize == SIZECLASS_10HR ) hist[0] = CurHist[sn][0];
    if( Verbose > CallLevel )
      printf( "%*sfsxwfms2:FireEnvironment2::RunFmsThreads:2b\n",
              CallLevel, "" );

		LastDate = -1;
		LastCount = -1;
		date = Chrono( GetMoistCalcInterval(FuelSize, FM_INTERVAL_TIME), &hour,
				&hours, true );  // get hours days etc.
		if (AtmosphereGridExists())
			Cloud = 0;//GetAtmClouds(date, hours);
		else
			Cloud = GetClouds(date, hours);
		HumTemp(date, hour, &tempref, &humref, &elevref, &rain, &humidmx,
			&humidmn, &tempmx, &tempmn);
    if( Verbose > CallLevel )
      printf( "%*sfsxwfms2:FireEnvironment2::RunFmsThreads:2c\n",
              CallLevel, "" );

    if( rain > 0.0 ) {
      GetWeatherRainTimes( sn, LastCount, &tr1, &tr2 );
      if( tr2 == 0 ) {  //Use actual rain duration
        tr2 = 2400;
        tr1 = 0;
      }
      ConvertSimtimeToActualTime( GetMoistCalcInterval(FuelSize,
                                                       FM_INTERVAL_TIME),
                                  &mo, &dy, &hr, &mn, true );
      if( Verbose > CallLevel )
        printf( "%*sfsxwfms2:FireEnvironment2::RunFmsThreads:2c1 dy=%ld\n",
                CallLevel, "", dy );
			min1 = ((long) (tr1 / 100.0)) * 100;
			mn = tr1 - min1;
			r1 = ConvertActualTimeToSimtime(mo, dy, min1, mn, true);
			min2 = ((long) (tr2 / 100.0)) * 100;
			mn = tr2 - min2;
			r2 = ConvertActualTimeToSimtime(mo, dy, min2, mn, true);

			RainRate = rain / (r2 - r1);
			lasttime = 0.0;
			Interval = GetMoistCalcInterval(FuelSize, FM_INTERVAL_TIME);
			if (r1 >= lasttime || r2 < lasttime - Interval) 		// if interval starts before r1 or after r2
				rain = 0.0;
			else if (r1 > lasttime - Interval && r1 < lasttime)	// if r1 starts inside interval
			{
				if (r2 > lasttime)					// if interval ends before r2
					rain = RainRate * (lasttime - r1);
				else									// if r1 && r2 inside interval
					rain = RainRate * (r2 - r1);
			}
			else if (lasttime - Interval > r1 && r2 >= lasttime) //if interval between r1 && r2
				rain = RainRate * Interval;
			else if (r2 >= lasttime - Interval && lasttime > r2) //if interval brackets r2
				rain = RainRate * (r2 - (lasttime - Interval));
			if (rain > 0.0)
			{
				humref = 90.0;
				Cloud = 100;
			}
		}

    if( Verbose > CallLevel )
      printf( "%*sfsxwfms2:FireEnvironment2::RunFmsThreads:2d\n",
              CallLevel, "" );

		rain *= 2.54;			// rain in cm
		Stations[sn].CuumRain = rain;//rain*(hour/100.0)/24.0;
		CurHist[sn][FuelSize]->AirTemperature = tempref;
		CurHist[sn][FuelSize]->RelHumidity = humref;
		CurHist[sn][FuelSize]->Elevation = elevref;
		CurHist[sn][FuelSize]->CloudCover = Cloud;
		CurHist[sn][FuelSize]->Rainfall = Stations[sn].CuumRain;
		if (FuelSize == SIZECLASS_10HR)
		{
			CurHist[sn][0]->AirTemperature = tempref;
			CurHist[sn][0]->RelHumidity = humref;
			CurHist[sn][0]->Elevation = elevref;
			CurHist[sn][0]->CloudCover = Cloud;
			CurHist[sn][0]->Rainfall = Stations[sn].CuumRain;
		}
		begin = threadct = 0;
		for (j = 0; j < NumFmsThreads; j++)
		{
			end = begin + range;
			if (begin >= Stations[sn].FMS[FuelSize][i]->NumElevs)
				continue;
			if (end > Stations[sn].FMS[FuelSize][i]->NumElevs)
				end = Stations[sn].FMS[FuelSize][i]->NumElevs;
			fmsthread[j].SetRange(SimTime, date, hour, StationNumber,
							FuelType, Stations, hist, begin, end);
			threadct++;
			begin = end;
		}

		for (j = 0; j < threadct; j++)
			fmsthread[j].StartFmsThread(j, FuelSize, &MxDesc, true);
		//WaitForMultipleObjects(threadct, hMoistureEvent, TRUE, INFINITE);
	}

  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::RunFmsThreads:3\n",
            CallLevel, "" );

	LastDate = -1;
	LastCount = -1;
	if (hist)
		delete[] hist;
	hist = 0;
	if (Stations[sn].FMS[FuelSize][i]->LastTime < SimTime)//while(Stations[sn].FMS[i].LastTime<SimTime)
	{
		//if(sn==0)	// will allocate all stations, so only do for 1st station
		//{
		hist = new DeadMoistureHistory * [NUM_FUEL_SIZES];
		j = 0;
		lasttime = Stations[sn].FMS[FuelSize][i]->LastTime;
		do
		{
			if (i == 0)  // FuelType is the first one
			{
				switch (FuelSize)
				{
				case SIZECLASS_100HR:
					AllocHistory(sn, SIZECLASS_100HR);  				// 100hr fuels
					break;
				case SIZECLASS_1000HR:
					AllocHistory(sn, SIZECLASS_1000HR); 			 	// 1000hr fuels
					break;
				default:
					AllocHistory(sn, SIZECLASS_10HR);   	   		// 10hr fuels
					AllocHistory(sn, SIZECLASS_1HR);		  		// 1hr fuels
					break;
				}
			}
			else
			{
				if (StartAtBeginning)
				{
					NextHist[sn][FuelSize] = (DeadMoistureHistory *)
						CurHist[sn][FuelSize]->next;
					CurHist[sn][FuelSize] = NextHist[sn][FuelSize];
					if (FuelSize == SIZECLASS_10HR)
					{
						NextHist[sn][0] = (DeadMoistureHistory *)
							CurHist[sn][0]->next;
						CurHist[sn][0] = NextHist[sn][0];
					}
				}
				else if (j == 0)  // first time through
				{
					MoistLoc = MxDesc.NumAlloc[sn][FuelSize] /
						MxDesc.NumFuels[sn][FuelSize] * FuelType;
					CurHist[sn][FuelSize] = FirstHist[sn][FuelSize];
					while (CurHist[sn][FuelSize]->Moisture[MoistLoc] > 0.0)
					{
						NextHist[sn][FuelSize] = (DeadMoistureHistory *)
							CurHist[sn][FuelSize]->next;
						CurHist[sn][FuelSize] = NextHist[sn][FuelSize];
					};
					if (FuelSize == SIZECLASS_10HR)
					{
						MoistLoc = MxDesc.NumAlloc[sn][0] /
							MxDesc.NumFuels[sn][0] * FuelType;
						CurHist[sn][0] = FirstHist[sn][0];
						while (CurHist[sn][0]->Moisture[MoistLoc] > 0.0)
						{
							NextHist[sn][0] = (DeadMoistureHistory *)
								CurHist[sn][0]->next;
							CurHist[sn][0] = NextHist[sn][0];
						};
					}
				}
				else
				{
					NextHist[sn][FuelSize] = (DeadMoistureHistory *)
						CurHist[sn][FuelSize]->next;
					CurHist[sn][FuelSize] = NextHist[sn][FuelSize];
					MoistLoc = MxDesc.NumAlloc[sn][FuelSize] /
						MxDesc.NumFuels[sn][FuelSize] * FuelType;
					while (CurHist[sn][FuelSize]->Moisture[MoistLoc] > 0.0)
					{
						NextHist[sn][FuelSize] = (DeadMoistureHistory *)
							CurHist[sn][FuelSize]->next;
						CurHist[sn][FuelSize] = NextHist[sn][FuelSize];
					};
					if (FuelSize == SIZECLASS_10HR)
					{
						NextHist[sn][0] = (DeadMoistureHistory *)
							CurHist[sn][0]->next;
						CurHist[sn][0] = NextHist[sn][0];
						MoistLoc = MxDesc.NumAlloc[sn][0] /
							MxDesc.NumFuels[sn][0] * FuelType;
						while (CurHist[sn][0]->Moisture[MoistLoc] > 0.0)
						{
							NextHist[sn][0] = (DeadMoistureHistory *)
								CurHist[sn][0]->next;
							CurHist[sn][0] = NextHist[sn][0];
						};
					}
				}
			}
			if (j == 0)
			{
				hist[FuelSize] = CurHist[sn][FuelSize];
				if (FuelSize == SIZECLASS_10HR)
					hist[0] = CurHist[sn][0];
			}
			j++;
			CurHist[sn][FuelSize]->LastTime = lasttime;
			if (FuelSize == SIZECLASS_10HR)
				CurHist[sn][0]->LastTime = lasttime;
			lasttime += GetMoistCalcInterval(FuelSize, FM_INTERVAL_TIME);
			CurHist[sn][FuelSize]->SimTime = lasttime;
			if (FuelSize == SIZECLASS_10HR)
				CurHist[sn][0]->SimTime = lasttime;
			checktime = lasttime + GetConditMinDeficit();
			date = Chrono(checktime, &hour, &hours, true);				// get hours days etc.
			if (AtmosphereGridExists())
				Cloud = 0;//GetAtmClouds(date, hours);
			else
				Cloud = GetClouds(date, hours);
			HumTemp(date, hour, &tempref, &humref, &elevref, &rain, &humidmx,
				&humidmn, &tempmx, &tempmn);
			if (rain > 0.0)
			{
				GetWeatherRainTimes(sn, LastCount, &tr1, &tr2);
				if (tr2 == 0)		// set rain duration for all day
				{
					tr2 = 2400;
					tr1 = 0;
				}
				ConvertSimtimeToActualTime(lasttime, &mo, &dy, &hr, &mn, true);
				min1 = ((long) (tr1 / 100.0)) * 100;
				mn = tr1 - min1;
				r1 = ConvertActualTimeToSimtime(mo, dy, min1, mn, true);
				min2 = ((long) (tr2 / 100.0)) * 100;
				mn = tr2 - min2;
				r2 = ConvertActualTimeToSimtime(mo, dy, min2, mn, true);

				RainRate = rain / (r2 - r1);
				Interval = GetMoistCalcInterval(FuelSize, FM_INTERVAL_TIME);

				if (r1 >= lasttime || r2 < lasttime - Interval) 		// if interval starts before r1 or after r2
					rain = 0.0;
				else if (r1 > lasttime - Interval && r1 < lasttime)	// if r1 starts inside interval
				{
					if (r2 > lasttime)					// if interval ends before r2
						rain = RainRate * (lasttime - r1);
					else									// if r1 && r2 inside interval
						rain = RainRate * (r2 - r1);
				}
				else if (lasttime - Interval > r1 && r2 >= lasttime) //if interval between r1 && r2
					rain = RainRate * Interval;
				else if (r2 >= lasttime - Interval && lasttime > r2) //if interval brackets r2
					rain = RainRate * (r2 - (lasttime - Interval));
				if (rain > 0.0)
				{
					humref = 90.0;
					Cloud = 100;
				}
				//}
			}
			rain *= 2.54;		// rain in cm
			Stations[sn].CuumRain += rain;//(double) GetMoistCalcInterval(FuelSize, FM_INTERVAL_TIME)/24.0*rain;	// convert hundreds to cm
			CurHist[sn][FuelSize]->AirTemperature = tempref;
			CurHist[sn][FuelSize]->RelHumidity = humref;
			CurHist[sn][FuelSize]->Elevation = elevref;
			CurHist[sn][FuelSize]->CloudCover = Cloud;
			CurHist[sn][FuelSize]->Rainfall = Stations[sn].CuumRain;
			if (FuelSize == SIZECLASS_10HR)
			{
				CurHist[sn][0]->AirTemperature = tempref;
				CurHist[sn][0]->RelHumidity = humref;
				CurHist[sn][0]->Elevation = elevref;
				CurHist[sn][0]->CloudCover = Cloud;
				CurHist[sn][0]->Rainfall = Stations[sn].CuumRain;
			}
		}
		while (lasttime < SimTime);
		//}

		begin = threadct = 0;
		for (j = 0; j < NumFmsThreads; j++)
		{
			end = begin + range;
			if (begin >= Stations[sn].FMS[FuelSize][i]->NumElevs)
				continue;
			if (end > Stations[sn].FMS[FuelSize][i]->NumElevs)
				end = Stations[sn].FMS[FuelSize][i]->NumElevs;
			fmsthread[j].SetRange(SimTime, date, hour, StationNumber,
							FuelType, Stations, hist, begin, end);
			threadct++;
			begin = end;
		}

		for (j = 0; j < threadct; j++)
			fmsthread[j].StartFmsThread(j, FuelSize, &MxDesc, false);
		//WaitForMultipleObjects(threadct, hMoistureEvent, TRUE, INFINITE);
		MxDesc.EndTime[sn][FuelSize] = Stations[sn].FMS[FuelSize][i]->LastTime = lasttime;
		if (FuelSize == SIZECLASS_10HR)
			MxDesc.EndTime[sn][0] = MxDesc.EndTime[sn][FuelSize];
	}

  if( hist ) delete[] hist;

  if( Verbose > CallLevel )
    printf( "%*sfsxwfms2:FireEnvironment2::RunFmsThreads:4\n",
            CallLevel, "" );  
  CallLevel--;
} //FireEnvironment2::RunFmsThreads


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//   FuelMoistureMap functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------



FuelMoistureMap::FuelMoistureMap()
{
	long i;

	for (i = 0; i < 4; i++)
	{
		NumFuels[i] = 0;
//		ZeroMemory(FuelKey[i], MAXNUM_FUEL_MODELS * sizeof(long));
		memset(FuelKey[i],0x0, MAXNUM_FUEL_MODELS * sizeof(long));
		FMS[i] = 0;
	}
	FmTolerance[0][(int)0] = 1;
	FmTolerance[0][(int)1] = 2;
	FmTolerance[0][(int)2] = 4;
	FmTolerance[1][(int)0] = 3;
	FmTolerance[1][(int)1] = 4;
	FmTolerance[1][2] = 8;
	FmTolerance[2][0] = 5;
	FmTolerance[2][1] = 6;
	FmTolerance[2][2] = 10;
	FmTolerance[3][0] = 8;
	FmTolerance[3][1] = 10;
	FmTolerance[3][2] = 15;

	CuumRain = 0.0;
}


FuelMoistureMap::~FuelMoistureMap()
{
	long i;

	for (i = 0; i < 4; i++)
		FreeFuels(i);
}


void FuelMoistureMap::SearchCondenseFuels(long FuelSize)
{
	// condense the number of fuel types used based on tolerance for the
	// initial moisture content of each size class
	//-> Can probably make the criteria sensitive to the timespan of the simulation
	//-> where differences in 1hr fuels are ignored after ~6 hours and diff in 10hr fuels
	//-> are ignored after 3 days or so.

	long i, j, num, FuelOrder;
	long t10;
	long t10a;
	long FmTol[3];

	// don't need to worry about initial conditions if using a conditioning period
	//if(UseConditioningPeriod(GETVAL))
	//	memcpy(FmTol, FmTolerance[3], 3*sizeof(long));
	//else	 // if no conditioning period, then tolerances are needed
	//{
	switch (GetFmTolerance())
	{
	case FM_HARDASS:
		memcpy(FmTol, FmTolerance[0], 3 * sizeof(long));
		break;
	case FM_MODERATE:
		memcpy(FmTol, FmTolerance[1], 3 * sizeof(long));
		break;
	case FM_LIBERAL:
		memcpy(FmTol, FmTolerance[2], 3 * sizeof(long));
		break;
	case FM_SLOPPY:
		memcpy(FmTol, FmTolerance[3], 3 * sizeof(long));
		break;
	}
	//}

	FuelOrder = 1;

	if (FuelSize < SIZECLASS_1000HR)
	{
		for (i = 0; i < MAXNUM_FUEL_MODELS; i++)
		{
			if (FuelKey[FuelSize][i] < FuelOrder)
				continue;
			FuelKey[FuelSize][i] = FuelOrder;
			num = i + 1;	  // Fuel Model Number is not zero based
			t10 = GetInitialFuelMoisture(num, FuelSize);

			for (j = 0; j < MAXNUM_FUEL_MODELS; j++)
			{
				if (j == i)
					continue;
				if (FuelKey[FuelSize][j] < FuelOrder)
					continue;
				num = j + 1;
				t10a = GetInitialFuelMoisture(num, FuelSize);
				if (t10 > t10a - FmTol[FuelSize] &&
					t10 < t10a + FmTol[FuelSize])
				{
					FuelKey[FuelSize][j] = FuelKey[FuelSize][i];	 // set to same
					NumFuels[FuelSize]--;
				}
			}
			FuelOrder++;
			if (FuelOrder > NumFuels[FuelSize])
				break;
		}
	}
	else
	{
		for (i = 0; i < MAXNUM_COARSEWOODY_MODELS; i++)
		{
			if (FuelKey[FuelSize][i] < FuelOrder)
				continue;
			FuelKey[FuelSize][i] = FuelOrder;
			num = (long)i + 1;	  // Fuel Model Number is not zero based

      t10 = (long)Get1000HrMoisture(num, true); //AAA In FromG5 ver

			for (j = 0; j < MAXNUM_COARSEWOODY_MODELS; j++)
			{
				if (j == i)
					continue;
				if (FuelKey[FuelSize][j] < FuelOrder)
					continue;
				num = (long)j + 1;
        t10a = (long)Get1000HrMoisture(num, true); //AAA In FromG5 ver

				if (t10 > t10a - FmTol[SIZECLASS_100HR] &&
					t10 < t10a + FmTol[SIZECLASS_100HR])
				{
					FuelKey[FuelSize][j] = FuelKey[FuelSize][i];	 // set to same
					NumFuels[FuelSize]--;
				}
			}
			FuelOrder++;
			if (FuelOrder > NumFuels[FuelSize])
				break;
		}
	}
}

bool FuelMoistureMap::ReAllocFuels(long FuelSize)
{
	bool UPDATE;
	long i, j, k, fueltype, oldnumfuels, numfuels;
	long NewKey[MAXNUM_FUEL_MODELS];

	//Reindex the new fuel types based on fuel moisture
  if( FuelSize < SIZECLASS_1000HR ) {
    numfuels = GetLandscapeTheme()->NumAllCats[3];
    if( GetLandscapeTheme()->AllCats[3][numfuels] == 99 ) numfuels--;
    if( GetLandscapeTheme()->AllCats[3][numfuels] == 98 ) numfuels--;
  }
  else {
    numfuels = GetLandscapeTheme()->NumAllCats[9];
    if( numfuels == 0 ) return false;
  }
  memcpy( NewKey, FuelKey[FuelSize], MAXNUM_FUEL_MODELS * sizeof(long) );

//	ZeroMemory(FuelKey[FuelSize], MAXNUM_FUEL_MODELS * sizeof(long));
	memset(FuelKey[FuelSize],0x0, MAXNUM_FUEL_MODELS * sizeof(long));

	k = 1;

  for( i = 0; i <= numfuels; i++ ) {
    if( FuelSize < SIZECLASS_1000HR ) {
      fueltype = GetLandscapeTheme()->AllCats[3][i];
      fueltype = GetFuelConversion(fueltype);   // see if there are conversions
      if( fueltype<MAXNUM_FUEL_MODELS+1 && fueltype>0 ) {
        if( FuelKey[FuelSize][fueltype - 1] == 0 )
          FuelKey[FuelSize][fueltype - 1] = k++;
      }
    }
    else {
      fueltype = GetLandscapeTheme()->AllCats[9][i];
      if( fueltype<MAXNUM_COARSEWOODY_MODELS && fueltype>0 ) {
				if (FuelKey[FuelSize][fueltype - 1] == 0)
					FuelKey[FuelSize][fueltype - 1] = k++;
			}
		}
	}
	numfuels = k - 1;				  	// new potential number, not condensed


  // reorder all of the fuels in the fuel key             //AAA In FromG5 ver only

  k=1;                                                    //AAA In FromG5 ver only

  for(i=0; i<MAXNUM_FUEL_MODELS; i++) {                   //AAA In FromG5 ver only

    if(FuelKey[FuelSize][i]>0) FuelKey[FuelSize][i]=k++;  //AAA In FromG5 ver only

  }                                                       //AAA In FromG5 ver only


	oldnumfuels = NumFuels[FuelSize]; 	// save the previous number

	NumFuels[FuelSize] = numfuels;   	// set new number for condensation scan

	SearchCondenseFuels(FuelSize);

	UPDATE = false;

	if (oldnumfuels != NumFuels[FuelSize])// if original number is not the new number
		UPDATE = true;
	else
	{
		for (i = 0; i < MAXNUM_FUEL_MODELS; i++)
		{
			if (FuelKey[FuelSize][i] != NewKey[i])
			{
				UPDATE = true;
				break;
			}
		}
	}

	if (!UPDATE)
		return false;
	else
	{
		numfuels = NumFuels[FuelSize];		// save new number
		NumFuels[FuelSize] = oldnumfuels;	// replace old one for freeing
		memcpy(NewKey, FuelKey[FuelSize],
			MAXNUM_FUEL_MODELS * sizeof(long));
		FreeFuels(FuelSize);		 		// Free Fuels erases all fuel types
		memcpy(FuelKey[FuelSize], NewKey,
			MAXNUM_FUEL_MODELS * sizeof(long));
		NumFuels[FuelSize] = numfuels;  	// restore new number of fuel types
		FMS[FuelSize] = new FMS_Elevations * [numfuels];
		long next = 1;
		for (i = 0; i < NumFuels[FuelSize]; i++)
		{
			FMS[FuelSize][i] = new FMS_Elevations(FuelSize);
			for (j = 0; j < MAXNUM_FUEL_MODELS; j++)
			{
				if (FuelKey[FuelSize][j] == next)
				{
					FMS[FuelSize][i]->SetFuel(j + 1);
					FMS[FuelSize][i]->AllocElevations();
					next++;
					break;
				}
				else if (next > NumFuels[FuelSize])
					break;
			}
		}
	}

	return true;
}

//============================================================================
bool FuelMoistureMap::AllocFuels( long FuelSize )
{ //FuelMoistureMap::AllocFuels
  CallLevel++;
  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FuelMoistureMap:AllocFuels:1\n", CallLevel, "" );

  long i, j, k, next, fueltype;

  FreeFuels( FuelSize );

  if( FuelSize < SIZECLASS_1000HR ) {
    NumFuels[FuelSize] = GetLandscapeTheme()->NumAllCats[3];
    if( GetLandscapeTheme()->AllCats[3][NumFuels[FuelSize]] == 99 )
      NumFuels[FuelSize]--;

    if( GetLandscapeTheme()->AllCats[3][NumFuels[FuelSize]] == 98 )
      NumFuels[FuelSize]--;
  }
  else {
    NumFuels[FuelSize] = GetLandscapeTheme()->NumAllCats[9];
    if( NumFuels[FuelSize] == 0 ) {
      if( Verbose >= CallLevel )
        printf( "%*sfsxwfms2:FuelMoistureMap:AllocFuels:1a\n", CallLevel, "" );
      CallLevel--;
      return false;
    }
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FuelMoistureMap:AllocFuels:2 "
            "FuelSize=%ld NumFuels[FuelSize]=%ld\n", CallLevel, "",
            FuelSize, NumFuels[FuelSize] );
  k = 1;
  for( i = 0; i <= NumFuels[FuelSize]; i++ ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwfms2:FuelMoistureMap:AllocFuels:2a "
              "FuelSize=%ld SIZECLASS_1000HR=%d\n", CallLevel, "",
              FuelSize, SIZECLASS_1000HR );
    if( FuelSize < SIZECLASS_1000HR ) {
      fueltype = GetLandscapeTheme()->AllCats[3][i];
      if( Verbose >= CallLevel )
        printf( "%*sfsxwfms2:FuelMoistureMap:AllocFuels:2a1 "
                "fueltype=%ld\n", CallLevel, "", fueltype );
      //See if there are conversions.
      fueltype = GetFuelConversion( fueltype );
      if( Verbose >= CallLevel )
        printf( "%*sfsxwfms2:FuelMoistureMap:AllocFuels:2a2 "
                "fueltype=%ld\n", CallLevel, "", fueltype );
      if( fueltype < 257 && fueltype > 0 ) {
        if( FuelKey[FuelSize][fueltype - 1] == 0 )
          FuelKey[FuelSize][fueltype - 1] = k++;
        if( Verbose >= CallLevel )
          printf( "%*sfsxwfms2:FuelMoistureMap:AllocFuels:2a2a "
                  "FuelSize=%ld fueltype=%ld FuelKey[%ld][%ld]=%ld\n",
                  CallLevel, "", FuelSize, fueltype, FuelSize, fueltype-1,
                  FuelKey[FuelSize][fueltype-1] );
      }
    }
    else {
      fueltype = GetLandscapeTheme()->AllCats[9][i];
      if( Verbose >= CallLevel )
        printf( "%*sfsxwfms2:FuelMoistureMap:AllocFuels:2a3 "
                "fueltype=%ld\n", CallLevel, "", fueltype );
      if( fueltype<MAXNUM_COARSEWOODY_MODELS && fueltype>0 ) {
        if( FuelKey[FuelSize][fueltype - 1] == 0 )
          FuelKey[FuelSize][fueltype - 1] = k++;
      }
    }
  }

  NumFuels[FuelSize] = k - 1;

  //Reorder all of the fuels in the fuel key.
  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FuelMoistureMap:AllocFuels:3 "
            "FuelSize=%ld MAXNUM_FUEL_MODELS=%d\n", CallLevel, "",
            FuelSize, MAXNUM_FUEL_MODELS );
  k=1;
  for( i=0; i<MAXNUM_FUEL_MODELS; i++ ) {
    if( FuelKey[FuelSize][i] > 0 ) FuelKey[FuelSize][i]=k++;
  }
  SearchCondenseFuels( FuelSize );
  if( NumFuels[FuelSize] == 0 ) {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwfms2:FuelMoistureMap:AllocFuels:3a\n", CallLevel, "" );
    CallLevel--;
    return true;
  }

  next = 1;
  FMS[FuelSize] = new FMS_Elevations * [NumFuels[FuelSize]];
  for( i = 0; i < NumFuels[FuelSize]; i++ ) {
    FMS[FuelSize][i] = new FMS_Elevations( FuelSize );
    for( j = 0; j < MAXNUM_FUEL_MODELS; j++ ) {
      if( FuelKey[FuelSize][j] == next ) {
        FMS[FuelSize][i]->SetFuel( j + 1 );
        FMS[FuelSize][i]->AllocElevations();
        next++;
        break;
      }
    }
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsxwfms2:FuelMoistureMap:AllocFuels:4\n", CallLevel, "" );
  CallLevel--;
  return true;
} //FuelMoistureMap::AllocFuels


void FuelMoistureMap::FreeFuels(long FuelSize)
{
  long i;

  for ( i = 0; i < NumFuels[FuelSize]; i++ ) {
    //if(FMS[FuelSize][i]->Fuel==0)
    //  continue;
    FMS[FuelSize][i]->FreeElevations();
    delete FMS[FuelSize][i];
  }

  if( NumFuels[FuelSize] > 0 ) {
    if( FMS[FuelSize] )
      delete[] FMS[FuelSize];
      FMS[FuelSize] = 0;
  }
  NumFuels[FuelSize] = 0;
  memset( FuelKey[FuelSize],0x0, MAXNUM_FUEL_MODELS * sizeof(long) );
  CuumRain = 0.0;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//   FMS_Elevation functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------



FMS_Elevations::FMS_Elevations(long fuelsize)
{
	Fms_Elev = 0;
	NumElevs = 0;
	Fuel = 0;
	FirstTime = 0.0;
	LastTime = 0.0;
	FuelSize = fuelsize;
}

//============================================================================
bool FMS_Elevations::AllocElevations()
{ //FMS_Elevations::AllocElevations
  FreeElevations();

  long   i;
  double conv = 1.0;

  if( GetTheme_Units(E_DATA) == 1 ) conv = 0.3048; //If feet convert to meters

  NumElevs = (long)( ((GetHiElev() - GetLoElev()) * conv) /
                     GetMoistCalcInterval(FuelSize, FM_INTERVAL_ELEV) + 1 );
  if( NumElevs < 1 ) {  //Error checking
    printf( "## fsxwfms2:FMS_Elevations::AllocElevations:           ##\n"
            "## Invalid value for NumElevs (%8ld). Check values ##\n"
            "## in Landscape file. Cannot allocate Fms_Elev.        ##\n",
            NumElevs );
    return false;
  }
  Fms_Elev = new FMS_Slopes * [NumElevs];
  if( Fms_Elev == NULL ) return false;

  for( i = 0; i < NumElevs; i++ ) {
    Fms_Elev[i] = new FMS_Slopes( FuelSize );
    if( Fms_Elev[i] == NULL ) return false;
    else {
      Fms_Elev[i]->Elev = (long)( i * GetMoistCalcInterval(FuelSize,
                                  FM_INTERVAL_ELEV) + GetLoElev() * conv );
      Fms_Elev[i]->Fuel = (long)Fuel;
    }
    if( (Fms_Elev[i]->AllocSlopes()) == false ) return false;
  }

  return true;
} //FMS_Elevations::AllocElevations


void FMS_Elevations::SetFuel(long fuel)
{
	//     long i;

	Fuel = fuel;
	//     for(i=0; i<NumElevs; i++)
	//     	Fms_Elev[i]->Fuel=Fuel;
}


void FMS_Elevations::FreeElevations()
{
	if (Fms_Elev == 0)
		return;

	long i;

	for (i = 0; i < NumElevs; i++)
	{
		if (Fms_Elev[i] == 0)
			continue;
		Fms_Elev[i]->FreeSlopes();
		delete Fms_Elev[i];
		Fms_Elev[i] = 0;
	}
	delete[] Fms_Elev;//GlobalFree(Fms_Elev);
	Fms_Elev = 0;
	NumElevs = 0;
	Fuel = 0;
	FirstTime = 0.0;
	LastTime = 0.0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//   FMS_Slope functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


FMS_Slopes::FMS_Slopes(long fuelsize)
{
	NumSlopes = 0;
	Fms_Slope = 0;
	FuelSize = fuelsize;
}


bool FMS_Slopes::AllocSlopes()
{
	long i;

	LoVal = GetTheme_LoValue(S_DATA);
	HiVal = GetTheme_HiValue(S_DATA);

	double fraction, ipart;
	double slopef;

	if (GetTheme_Units(S_DATA) == 1)
	{
		slopef = atan((double) LoVal / 100.0) / PI * 180.0;
		LoVal = (long)slopef;
		fraction = (long) modf(slopef, &ipart);
		if (fraction >= 0.5)
			LoVal++;
		slopef = atan((double) HiVal / 100.0) / PI * 180.0;
		HiVal = (long)slopef;
		fraction = (long)modf(slopef, &ipart);
		if (fraction >= 0.5)
			HiVal++;
	}

	if (LoVal > 50)
		LoVal = 50;
	if (HiVal > 50)
		HiVal = 50;
	NumSlopes = (HiVal - LoVal) / GetMoistCalcInterval(FuelSize,
									FM_INTERVAL_SLOPE) + 1;

	if (NumSlopes == 0)
		return false;

	Fms_Slope = new FMS_Aspects * [NumSlopes];
	if (Fms_Slope == 0)
		return false;

	for (i = 0; i < NumSlopes; i++)
	{
		Fms_Slope[i] = new FMS_Aspects(FuelSize);
		if (Fms_Slope[i] != 0)
		{
			Fms_Slope[i]->Slope = i * GetMoistCalcInterval(FuelSize,
										FM_INTERVAL_SLOPE) + LoVal;
			Fms_Slope[i]->Elev = Elev;
			Fms_Slope[i]->Fuel = Fuel;
		}
		else
			return false;
		if ((Fms_Slope[i]->AllocAspects()) == false)
			return false;
	}

	return true;
}

void FMS_Slopes::FreeSlopes()
{
	long i;


	for (i = 0; i < NumSlopes; i++)
	{
		Fms_Slope[i]->FreeAspects();

		delete Fms_Slope[i];
	}

	delete[] Fms_Slope;
}




//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//   FMS_Aspect functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


FMS_Aspects::FMS_Aspects(long fuelsize)
{
	NumAspects = 0;
	Fms_Aspect = 0;
	FuelSize = fuelsize;
}


bool FMS_Aspects::AllocAspects()
{
	long i;

	LoVal = 0;//GetTheme_LoValue(A_DATA);
	HiVal = 360;//GetTheme_HiValue(A_DATA);
	NumAspects = (HiVal - LoVal) / GetMoistCalcInterval(FuelSize,
									FM_INTERVAL_ASP) + 1;

	if (NumAspects == 0)
		return false;

	Fms_Aspect = new FMS_Cover * [NumAspects];
	if (Fms_Aspect == 0)
		return false;

	for (i = 0; i < NumAspects; i++)
	{
		Fms_Aspect[i] = new FMS_Cover(FuelSize);
		if (Fms_Aspect[i] != 0)
		{
			Fms_Aspect[i]->Aspect = i * GetMoistCalcInterval(FuelSize,
											FM_INTERVAL_ASP) + LoVal;
			Fms_Aspect[i]->Slope = Slope;
			Fms_Aspect[i]->Elev = Elev;
			Fms_Aspect[i]->Fuel = Fuel;
		}
		else
			return false;
		if ((Fms_Aspect[i]->AllocCover()) == false)
			return false;
	}

	return true;
}


void FMS_Aspects::FreeAspects()
{
	long i;


	for (i = 0; i < NumAspects; i++)
	{
		Fms_Aspect[i]->FreeCover();

		delete Fms_Aspect[i];
	}

	delete[] Fms_Aspect;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//   FMS_Cover functions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------



FMS_Cover::FMS_Cover(long fuelsize)
{
	NumFms = 0;
	fms = 0;
	LastMx = 0;
	NextMx = 0;
	LastEq = 0;
	NextEq = 0;
	Elev = -1;
	FuelSize = fuelsize;
}

bool FMS_Cover::AllocCover()
{
	long i;

	LoVal = GetTheme_LoValue(C_DATA);
	HiVal = GetTheme_HiValue(C_DATA);
	if (HiVal > 80)
		HiVal = 80; //percent
	NumFms = (HiVal - LoVal) /
		GetMoistCalcInterval(FuelSize, FM_INTERVAL_COV) +
		1;

	fms = new Fms * [NumFms];//(Fms **) GlobalAlloc(GMEM_FIXED, NumFms*sizeof(Fms*));
	if (fms == NULL)
	{
		NumFms = 0;
		return false;
	}
	for (i = 0; i < NumFms; i++)
		fms[i] = 0;

	LastMx = new double[NumFms];//(double *) GlobalAlloc(GMEM_FIXED, NumFms*sizeof(double));
	if (LastMx == NULL)
	{
		NumFms = 0;
		return false;
	}
	for (i = 0; i < NumFms; i++)
		LastMx[i] = 0.0;

	NextMx = new double[NumFms];//(double *) GlobalAlloc(GMEM_FIXED, NumFms*sizeof(double));
	if (NextMx == NULL)
	{
		NumFms = 0;
		return false;
	}
	for (i = 0; i < NumFms; i++)
		NextMx[i] = 0.0;
	LastEq = new double[NumFms];//(double *) GlobalAlloc(GMEM_FIXED, NumFms*sizeof(double));
	if (LastEq == NULL)
	{
		NumFms = 0;
		return false;
	}
	for (i = 0; i < NumFms; i++)
		LastEq[i] = 0.0;
	NextEq = new double[NumFms];//(double *) GlobalAlloc(GMEM_FIXED, NumFms*sizeof(double));
	if (NextEq == NULL)
	{
		NumFms = 0;
		return false;
	}
	for (i = 0; i < NumFms; i++)
		NextEq[i] = 0.0;

	return true;
}

void FMS_Cover::FreeCover()
{
	if (fms)
	{
		long i;
		for (i = 0; i < NumFms; i++)
		{
			if (fms[i])
				Fms_Destroy(fms[i]);
		}
		delete[] fms;//GlobalFree(fms);
	}
	if (LastMx)
		delete[] LastMx;//GlobalFree(LastMx);
	if (NextMx)
		delete[] NextMx;//GlobalFree(NextMx);
	if (LastEq)
		delete[] LastEq;//GlobalFree(LastEq);
	if (NextEq)
		delete[] NextEq;//GlobalFree(NextEq);
	fms = 0;
	LastMx = 0;
	NextMx = 0;
	LastEq = 0;
	NextEq = 0;
	NumFms = 0;
}



//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
//   FmsThread Functions
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------



FmsThread::FmsThread()
{
	Begin = End = 0;

	CurHist = 0;

	ThreadOrder = -1;

	//hFmsEvent=0;
}



FmsThread::~FmsThread()
{
}


void FmsThread::StartFmsThread(long ID, long sizeclass,
	DeadMoistureDescription* mxdesc, bool firsttime)
{

  FuelSize = sizeclass;

  FirstTime = firsttime;

  MxDesc = mxdesc;

  RunFmsThread(this);
}



unsigned FmsThread::RunFmsThread(void* fmsthread)
{
	static_cast <FmsThread*>(fmsthread)->UpdateMoistures();


	return 1;
}



void FmsThread::UpdateMoistures()
{
	do {
		if (End < 0) break;

		UpdateMapMoisture();

    //need this if multithreading is restored
		//WaitForSingleObject(hFmsEvent, INFINITE);
		//ResetEvent(hFmsEvent);
          // remove break if multithreading is restored
          break;
  } while (End > -1);
}



void FmsThread::SetRange(double simtime, long date, long hour,
	long stationnumber, long fueltype, FuelMoistureMap* map,
	DeadMoistureHistory** hist, long begin, long end)
{
	SimTime = simtime;

	Date = date;

	Hour = hour;

	StationNumber = stationnumber;

	FuelType = fueltype;

	Stations = map;

	CurHist = hist;

	Begin = begin;

	End = end;
}


//============================================================================
void FmsThread::UpdateMapMoisture()
{
  long i, j, k, m, n, loc;
  long sn = StationNumber;
  long ElevInc, ElevDiff;
  long Slope, Aspect, Cover;
  double Radiate, temp, Ctemp, humid, mx, emult = 3.2808;
  double ElevRef, TempRef, HumidRef, Rain;
  long Cloud;
  FMS_Cover* cov;
  DeadMoistureHistory* curhist, * nexthist, * onehist;

  i = FuelType;
  curhist = CurHist[FuelSize];
  if( FuelSize == SIZECLASS_10HR )
    onehist = CurHist[0];
  if( FirstTime ) {
    ElevRef = curhist->Elevation;
    TempRef = curhist->AirTemperature;
    HumidRef = curhist->RelHumidity;
    Rain = curhist->Rainfall;
    Cloud = curhist->CloudCover;
    if( GetTheme_Units(E_DATA) == 1 )
      ElevInc = (long)(GetLoElev() - GetMoistCalcInterval(FuelSize, FM_INTERVAL_ELEV) * emult);
      // must be less, so that increment will start at LoElev
    else
      ElevInc = (long)((GetLoElev() - GetMoistCalcInterval(FuelSize, FM_INTERVAL_ELEV)) * emult);

    for( j = 0; j < Begin; j++ )
      ElevInc += (long)(GetMoistCalcInterval(FuelSize, FM_INTERVAL_ELEV) * emult);

    // get starting fuel moisture
    if( FuelSize < SIZECLASS_1000HR )
      mx = ((double) GetInitialFuelMoisture(Stations[sn].FMS[FuelSize][i]->Fuel, FuelSize)) / 100.0;
    else {
      mx = Get1000HrMoisture(Stations[sn].FMS[FuelSize][i]->Fuel, true)/100.0; //AAA In FromG5 ver only
      if( mx <= 0.10 )
        mx = 0.10;
    }

    for( j = Begin; j < End; j++ ) {   // for each elevation
      ElevInc += (long)(GetMoistCalcInterval(FuelSize, FM_INTERVAL_ELEV) * emult);
      ElevDiff = (long)(ElevRef - ElevInc);
      SiteSpecific(ElevDiff, TempRef, &temp, HumidRef, &humid); // adjusts to site specific temp and relhum
      Ctemp = temp - 32.0;  // convert to C
      Ctemp /= 1.8;
      humid /= 100.0;
      for( k = 0; k < Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->NumSlopes; k++ ) {
        Slope = Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->LoVal +
                k * GetMoistCalcInterval(FuelSize, FM_INTERVAL_SLOPE);
        for( m = 0; m < Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->Fms_Slope[k]->NumAspects; m++ ) {
          Aspect = Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->Fms_Slope[k]->LoVal +
                   m * GetMoistCalcInterval(FuelSize, FM_INTERVAL_ASP);
          for( n = 0; n < Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->Fms_Slope[k]->Fms_Aspect[m]->NumFms; n++ ) {
            Cover = Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->Fms_Slope[k]->Fms_Aspect[m]->LoVal +
                    n * GetMoistCalcInterval(FuelSize, FM_INTERVAL_COV);

            cov = Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->Fms_Slope[k]->Fms_Aspect[m];
            cov->LastMx[n] = mx;
            cov->LastEq[n] = mx;
            if( cov->fms[n] == NULL ) {
              switch( FuelSize ) {
                case SIZECLASS_10HR:
                  cov->fms[n] = Fms_Create10Hour( (char *) "10hr" ); break;
                case SIZECLASS_100HR:
                  cov->fms[n] = Fms_Create100Hour( (char *) "100hr" ); break;
                case SIZECLASS_1000HR:
                  cov->fms[n] = Fms_Create1000Hour( (char *) "1000hr" ); break;
              }
            }

            Radiate = SimpleRadiation(Date, Hour, Cloud, ElevInc, Slope, Aspect, Cover);

            Radiate *= 697.8;    // Watts/m2

            cov->SolRad = Radiate;

            Fms_Initialize( cov->fms[n], Ctemp, humid, Radiate,
                            Stations[sn].CuumRain, Ctemp, humid, mx );    // go directly to Fms_Update, not Fms_UpdateAt

            Stations[sn].CuumRain += (double)
                                      GetMoistCalcInterval(FuelSize, FM_INTERVAL_TIME) / 24.0 * Rain;

            Fms_Update( cov->fms[n], GetMoistCalcInterval(FuelSize, FM_INTERVAL_TIME) / 60.0,
                        Ctemp, humid, Radiate, Rain );    // go directly to Fms_Update, not Fms_UpdateAt

            loc = i * MxDesc->NumElevs[sn][FuelSize] * MxDesc->NumSlopes[sn][FuelSize] *
                   MxDesc->NumAspects[sn][FuelSize] * MxDesc->NumCovers[sn][FuelSize] +
                  j * MxDesc->NumSlopes[sn][FuelSize] * MxDesc->NumAspects[sn][FuelSize] *
                   MxDesc->NumCovers[sn][FuelSize] +
                  k * MxDesc->NumAspects[sn][FuelSize] * MxDesc->NumCovers[sn][FuelSize] +
                  m * MxDesc->NumCovers[sn][FuelSize] + n;
            cov->NextMx[n] = Fms_MeanWtdMoisture(cov->fms[n]);
            curhist->Moisture[loc] = cov->NextMx[n];
            cov->NextEq[n] = cov->fms[n]->sem;

            if( FuelSize == SIZECLASS_10HR )
              onehist->Moisture[loc] = cov->NextEq[n];
          }
        }
      }
    }

    Stations[sn].FMS[FuelSize][i]->FirstTime = - ((double) GetConditMinDeficit());
    curhist->LastTime = Stations[sn].FMS[FuelSize][i]->FirstTime;
    LastTime = (long)curhist->LastTime;
    if( FuelSize == SIZECLASS_10HR )
      onehist->LastTime = LastTime;

    LastTime += GetMoistCalcInterval(FuelSize, FM_INTERVAL_TIME);  // force constant time interval between calcs.
    Stations[sn].FMS[FuelSize][i]->LastTime = curhist->SimTime = MxDesc->EndTime[sn][FuelSize] = LastTime;

    if( FuelSize == SIZECLASS_10HR )
      onehist->SimTime = MxDesc->EndTime[sn][0] = LastTime;

    return;
  }

  LastTime = (long)Stations[sn].FMS[FuelSize][i]->LastTime;
  while( LastTime < SimTime ) {
    ElevRef = curhist->Elevation;
    TempRef = curhist->AirTemperature;
    HumidRef = curhist->RelHumidity;
    Rain = curhist->Rainfall;
    Cloud = curhist->CloudCover;
    if( GetTheme_Units(E_DATA) == 1 )
      // must be less, so that increment will start at LoElev
      ElevInc = (long)(GetLoElev() - GetMoistCalcInterval((long)FuelSize, FM_INTERVAL_ELEV) * emult);
    else
      ElevInc = (long)((GetLoElev() - GetMoistCalcInterval((long)FuelSize, FM_INTERVAL_ELEV)) * emult);

    for( j = 0; j < Begin; j++ )
      ElevInc += (long)((GetMoistCalcInterval((long)FuelSize, FM_INTERVAL_ELEV) * emult));

    for( j = Begin; j < End; j++ ) {   // for each elevation
      ElevInc += (long)(GetMoistCalcInterval((long)FuelSize, FM_INTERVAL_ELEV) * emult);
      ElevDiff = (long)(ElevRef - ElevInc);
      SiteSpecific(ElevDiff, TempRef, &temp, HumidRef, &humid); // adjusts to site specific temp and relhum
      Ctemp = temp - 32.0;    // convert to C
      Ctemp /= 1.8;
      humid /= 100.0;

      for( k = 0; k < Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->NumSlopes; k++ ) {
        Slope = Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->LoVal +
                k * GetMoistCalcInterval(FuelSize, FM_INTERVAL_SLOPE);
        for( m = 0; m < Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->Fms_Slope[k]->NumAspects; m++ ) {
          Aspect = Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->Fms_Slope[k]->LoVal +
                   m * GetMoistCalcInterval(FuelSize, FM_INTERVAL_ASP);

          for( n = 0; n < Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->Fms_Slope[k]->Fms_Aspect[m]->NumFms; n++ ) {
            Cover = Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->Fms_Slope[k]->Fms_Aspect[m]->LoVal +
                    n * GetMoistCalcInterval(FuelSize, FM_INTERVAL_COV);
            cov = Stations[sn].FMS[FuelSize][i]->Fms_Elev[j]->Fms_Slope[k]->Fms_Aspect[m];
            cov->LastMx[n] = cov->NextMx[n];
            cov->LastEq[n] = cov->NextEq[n];

            Radiate = SimpleRadiation(Date, Hour, Cloud, ElevInc, Slope, Aspect, Cover);
            Radiate *= 697.8;    // Watts/m2
            cov->SolRad = Radiate;

            Fms_Update( cov->fms[n], GetMoistCalcInterval(FuelSize, FM_INTERVAL_TIME) / 60.0,
                        Ctemp, humid, Radiate, Rain );    // go directly to Fms_Update, not Fms_UpdateAt

            loc = i * MxDesc->NumElevs[sn][FuelSize] * MxDesc->NumSlopes[sn][FuelSize] *
                   MxDesc->NumAspects[sn][FuelSize] * MxDesc->NumCovers[sn][FuelSize] +
                  j * MxDesc->NumSlopes[sn][FuelSize] * MxDesc->NumAspects[sn][FuelSize] *
                   MxDesc->NumCovers[sn][FuelSize] +
                  k * MxDesc->NumAspects[sn][FuelSize] * MxDesc->NumCovers[sn][FuelSize] +
                  m * MxDesc->NumCovers[sn][FuelSize] + n;
            curhist->Moisture[loc] = cov->NextMx[n] = Fms_MeanWtdMoisture(cov->fms[n]);
            if( FuelSize == SIZECLASS_10HR ) {
              onehist->Moisture[loc] = cov->NextEq[n] = cov->fms[n]->sem;
            }
          }
        }
      }
    }

    curhist->LastTime = Stations[sn].FMS[FuelSize][i]->FirstTime = LastTime;
    if( FuelSize == SIZECLASS_10HR )
      onehist->LastTime = LastTime;
    LastTime += GetMoistCalcInterval(FuelSize, FM_INTERVAL_TIME);
    curhist->SimTime = MxDesc->EndTime[sn][FuelSize] = Stations[sn].FMS[FuelSize][i]->LastTime = LastTime;
    if( FuelSize == SIZECLASS_10HR ) {
      onehist->SimTime = MxDesc->EndTime[sn][0] = LastTime;
      nexthist = (DeadMoistureHistory *) onehist->next;
      onehist = nexthist;
    }
    nexthist = (DeadMoistureHistory *) curhist->next;
    curhist = nexthist;
  }
}


//============================================================================

double FmsThread::SimpleRadiation( long date, double hour, long cloud,
                                   long elev, long slope, long aspect,
                                   long cover ) {
  // calculates irradiance from ROTHERMEL ET AL. 1986 but excludes tree orientation geometry

  double sangle, I, Im, M, atm, row, delta, ztan, zcos, zeta;
  double tc, A, Z, flatitude, fslope, fhour;
  double tt, tn, Ia, Id, aspectf;

  row = 0.8;
  flatitude = double (GetLatitude()) / 180.0 * PI;

  fslope = double (slope) / 180.0 * PI;
  aspectf = ((double) aspect) / 180.0 * PI;

  delta = 23.45 * sin((.9863 * (284 + date)) / 180 * PI);
  delta = delta / 180 * PI;
  fhour = (PI / 12.0) * (hour / 100.0 - 6.0);  /* hour angle from 6 AM */

  sangle = sin(fhour) * cos(delta) * cos(flatitude) +
           sin(delta) * sin(flatitude);  /* sine of solar angle */
  if( sangle > 0.0 ) {
    A = asin(sangle);
    ztan = (sin(fhour) * cos(delta) * sin(flatitude) -
           sin(delta) * cos(flatitude)) /
           (cos(fhour) * cos(delta));
    zcos = cos(fhour) * (cos(delta) / cos(A));
    Z = atan(ztan);
    if( zcos >= 0.0 ) Z += PI / 2.0;
    else Z += 3.0 * PI / 2.0;
    if( Z > 2.0 * PI ) Z -= 2.0 * PI;

    if( slope > 0 ) {
      zeta = atan(tan(fslope) * cos(Z - aspectf)) * -1.0;
      sangle = sin(A - zeta) * (cos(fslope) / cos(zeta));
    }
    if( sangle > 0.0 ) {
      atm = 1000.0 * exp(-.0000448 * elev);  /* atmospheric pressure at elev */
			M = atm / 1000.0 * (1.0 / sangle);
//printf("AAA fsxwfms2:FmsThread::SimpleRadiation 1 a atm=%f sangle=%f\n",atm,sangle);

			if (M < 200.0)					// don't underflow Im
				Im = 1.98 * pow(row, M);			/* cal/cm2/min */
			else
				Im = 0;
			tc = ((double) (100.0 - cloud) / 100.0);

			tt = (100.0 - (double) cover) / 100.0;
			tn = tc * tt;
			Ia = Im * tn;
//printf("AAA fsxwfms2:FmsThread::SimpleRadiation 1 b Im=%f tc=%f tt=%f sangle=%f\n",Im,tc,tt,sangle);

			Id = Ia * sangle;
      if( Id < 0.001 ) I = 0.0;
      else I = (double) Id;
//printf("AAA fsxwfms2:FmsThread::SimpleRadiation 1 c I=%f\n",I);

    }
    else I = 0.0;
  }

  else I = 0.0;

  return I;
}


void FmsThread::SiteSpecific(long ElevDiff, double tempref, double* temp,
	double humref, double* humid)
{
	// FROM ROTHERMEL, WILSON, MORRIS, & SACKETT  1986
	double dewptref, dewpt;
	// ElevDiff is always in feet, temp is in Fahrenheit, humid is %

	dewptref = -398.0 - 7469.0 /
		(log(humref / 100.0) - 7469.0 / (tempref + 398.0));
	*temp = tempref + (double) ElevDiff / 1000.0 * 5.5; 	  	// Stephenson 1988 found summer adiabat at 3.07 F/1000ft
	dewpt = dewptref + (double) ElevDiff / 1000.0 * 1.1; 		// new humidity, new dewpt, and new humidity
	*humid = 7469.0 * (1.0 / (*temp + 398.0) - 1.0 / (dewpt + 398.0));
	*humid = exp(*humid) * 100.0;				// convert from ln units
	if (*humid > 99.0)
		*humid = 99.0;
}
