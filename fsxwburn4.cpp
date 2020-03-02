/*fsxwburn4.cpp
  Burn model functions for FARSITE.
  Copyright 1994, 1995, 1996
  Mark A. Finney, Systems for Environemntal Management
  See LICENSE.TXT file for license information.
*/
#include<string.h>
#include"globals.h"
#include"fsx4.h"
#include"fsxwattk.h"
#include"fsairatk.h"
#include"fsxpfront.h"
#include"fsglbvar.h"

//============================================================================
BurnThread::BurnThread( FireEnvironment2* Env )
{ //BurnThread::BurnThread
  FireIsUnderAttack = false;
  ThreadID = 0;
  Begin = End = -1;
  turn = 0;
  hBurnThread = 0;
  fe = new FELocalSite();
  env = Env;
  firering = 0;
  embers.SetFireEnvironmentCalls(env, fe);
  Started = false;
  DoSpots = false;
  CuumPoint = 0;
} //BurnThread::BurnThread

//============================================================================
BurnThread::~BurnThread()
{ //BurnThread::~BurnThread
  delete fe;
} //BurnThread::~BurnThread

//============================================================================
void BurnThread::SetRange( long currentfire, double SimTime,
 	                         double cuumtimeincrement, double timeincrement,
                           double timemaxrem, long begin, long end, long Turn,
                           bool attack, FireRing* Ring )
{ //BurnThread::SetRange
  if( begin > -1 ) {
    Begin = begin;
    End = end;
  }
  CurrentFire = currentfire;
  SIMTIME = SimTime;
  CuumTimeIncrement = cuumtimeincrement;
  TimeMaxRem = timemaxrem;
  TimeIncrement = EventTimeStep = timeincrement;
  turn = Turn;
  FireIsUnderAttack = attack;
  firering = Ring;
  CanStillBurn = StillBurning = false;
  SimTimeOffset = CuumTimeIncrement;
} //BurnThread::SetRange

//============================================================================
long BurnThread::StartBurnThread( long ID )
{ //BurnThread::StartBurnThread
  RunBurnThread( this );
  return hBurnThread;
} //BurnThread::StartBurnThread

//============================================================================
unsigned BurnThread::RunBurnThread( void* bt )
{ //BurnThread::RunBurnThread
  static_cast <BurnThread*>(bt)->PerimeterThread();

  return 1;
} //BurnThread::RunBurnThread

//============================================================================
void BurnThread::PerimeterThread()
{ //BurnThread::PerimeterThread
  long begin, end;
  long i, TestPointL, TestPointN;
  bool ComputeSpread;
  double oldfli;
  double FliL, FliN;
  double mx[20];   //Fuel moistures
  CallLevel++;

  begin = Begin;
  end = End;
  Started = true; //Means that the thread has been started

  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1 "
            "StillBurning=%d CanStillBurn=%d\n",
            CallLevel, "", StillBurning, CanStillBurn );
  do {
    if( End < 0 ) break;

    if( Verbose >= CallLevel )
      printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1a\n", CallLevel, "" );
    if( CurrentFire >= 0 ) {
      if( turn == 0 ) distchek( CurrentFire );
      else if( DistanceCheckMethod(GETVAL) == 0 ) distchek( CurrentFire );
    }
    if( Verbose >= CallLevel )
      printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b begin=%ld end=%ld\n",
              CallLevel, "", begin, end );

    long Pos, NumBadPositions = 0; //Track if perim is beyond landscape (JWB)

    //FOR ALL POINTS ON EACH FIRE.
    for( CurrentPoint = begin; CurrentPoint < end; CurrentPoint++ ) {
      timerem = TimeIncrement;
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b1 "
                "timerem=%lf ld.fuel=%d\n",
                CallLevel, "", timerem, ld.fuel );
      GetPoints( CurrentFire, CurrentPoint );
      Pos = fe->GetLandscapeData( xpt, ypt, ld );
      if( Verbose >= CallLevel ) {
        printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b2 "
                "(%lf, %lf) ld.fuel=%d ld.elev=%d sizeof(ld)=%ld "
                "timerem=%lf\n",
                CallLevel, "", xpt, ypt, ld.fuel, ld.fuel, sizeof(ld),
                timerem );
        printf( "%*s                                         "
                "sizeof(celldata)=%ld\n", CallLevel, "", sizeof(celldata));
        printf( "%*s                                         "
                "sizeof(grounddata)=%ld\n", CallLevel, "", sizeof(grounddata));
        printf( "%*s                                         "
                "sizeof(crowndata)=%ld\n", CallLevel, "", sizeof(crowndata));
        printf( "%*s                                         "
                "sizeof(CanopyCharacteristics)=%ld\n", CallLevel, "",
                sizeof(CanopyCharacteristics));
        printf( "%*s                                         "
                "sizeof(LandscapeStruct)=%ld\n", CallLevel, "",
                sizeof(LandscapeStruct));
        printf( "%*s                                                   "
                " ls.elev=%ld\n", CallLevel, "", (long)ld.elev );
        printf( "%*s                                                   "
                " ls.slope=%ld\n", CallLevel, "", (long)ld.slope );
        printf( "%*s                                                   "
                " ls.aspect=%ld\n", CallLevel, "", (long)ld.aspect );
        printf( "%*s                                                   "
                " ls.fuel=%ld\n", CallLevel, "", (long)ld.fuel );
      }

      if( Pos <= 0 ) NumBadPositions++;  //JWB

      if( ld.fuel > 0 && ld.elev != -9999 ) {  //If not a rock or lake etc.
        if( Verbose >= CallLevel )
          printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b2a\n",
                  CallLevel, "" );

        fli = FliFinal = oldfli = GetPerimeter2Value( CurrentPoint, FLIVAL );
        if( Verbose >= CallLevel )
          printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b2b\n",
                  CallLevel, "" );

        if( fli < 0.0 ) {
          StillBurning = true;
          if( FireIsUnderAttack ) {
            if( turn ) {
              ComputeSpread = false;
              for( i = 1; i < 3; i++ ) {
                TestPointL = CurrentPoint - i;
                TestPointN = CurrentPoint + i;
                if( TestPointL < 0 )
                  TestPointL += GetNumPoints( CurrentFire );
                if( TestPointN >= GetNumPoints(CurrentFire) )
                  TestPointN -= GetNumPoints( CurrentFire );
                FliL = GetPerimeter2Value( TestPointL, FLIVAL );
                FliN = GetPerimeter2Value( TestPointN, FLIVAL );
                if( FliL > 0.0 || FliN > 0.0 ) {
                  ComputeSpread = true;
                  break;
                }
              }
            }
            else ComputeSpread = true;
          }
          else ComputeSpread = false;
        }
        else ComputeSpread = true;

        if( Verbose >= CallLevel )
          printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b2c\n",
                  CallLevel, "" );
        if( ComputeSpread ) {
          if( Verbose >= CallLevel )
            printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b2c1\n",
                    CallLevel, "" );
          EmberCoords();        //Compute ember source coordinates
          CanStillBurn = true;  //Has fuel, can still burn

          if( Verbose >= CallLevel )
            printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b2c2\n",
                    CallLevel, "" );
          fe->GetFireEnvironment( env, SIMTIME + SimTimeOffset, false );
          if( Verbose >= CallLevel )
            printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b2c3\n",
                    CallLevel, "" );

          if( EventMinimumTimeStep(GETVAL) < EventTimeStep &&
              EventMinimumTimeStep(GETVAL) > 0.0 )
            EventTimeStep = EventMinimumTimeStep( GETVAL );
          if( Verbose >= CallLevel )
            printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b2c4\n",
                    CallLevel, "" );

          //mechcalls function to transfrm orientation of point with slope.
          NormalizeDirectionWithLocalSlope();
          //Get forward ROS from last timestep or last substep.
          RosT = GetPerimeter2Value( CurrentPoint, ROSVAL );
          if( Verbose >= CallLevel )
            printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b2c5\n",
                    CallLevel, "" );

          SurfaceFire();
          if( oldfli < 0.0 ) {
            fli *= -1.0;
            FliFinal *= -1.0;
            react = 0.0;
          }
          if( Verbose >= CallLevel )
            printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b2c6\n",
                    CallLevel, "" );

          if( ld.cover > 0 && ld.height > 0 )  //If there IS a tree canopy
            CrownFire();  //If turn==1 then start try spot fires, else no
          else cf.CrownLoadingBurned = 0.0;

          if( Verbose >= CallLevel )
            printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b2c7 "
                    "timerem=%lf\n", CallLevel, "", timerem );

          //Tolerance for simulation 1/1000 min.
          if( oldfli >= 0.0 && timerem > 0.001 && timerem > TimeMaxRem )
            TimeMaxRem = timerem;  //LARGEST TIME REMAINING,
                                   //MEANS FASTEST SPREAD RATE
        }
        else RosT1 = RosT = GetPerimeter2Value( CurrentPoint, ROSVAL );
        if( Verbose >= CallLevel )
          printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b2d\n",
                  CallLevel, "" );
      }
      else {
        RosT = RosT1 = 0.0;
        fli = FliFinal = 0.0;
        react = 0.0;
      }
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1b3\n",
                CallLevel, "" );

      if( turn == 1 ) {
        if( FireIsUnderAttack && ! ComputeSpread ) FliFinal *= -1.0;
        prod.cuumslope[0] += ( double ) ld.slope;  //CUMULATIVE SLOPE ANGLES
        prod.cuumslope[1] += 1;  //CUMULATIVE NUMBER OF PERIMETER POINTS
        SetPerimeter1( CurrentFire, CurrentPoint, xpt, ypt );
        SetFireChx( CurrentFire, CurrentPoint, RosT1, FliFinal );
        SetReact( CurrentFire, CurrentPoint, react );

         //Beginning of timestep.
        if( DistanceCheckMethod(GETVAL) == 1 && CuumTimeIncrement == 0.0 )
          SetElev( CurrentPoint, ld.elev );

        //Store current fire perim in ring.
        if( DistanceCheckMethod(GETVAL) == 0 && CheckPostFrontal(GETVAL) ) {
          SetElev( CurrentPoint, ld.elev );
          GetCurrentFuelMoistures( ld.fuel, ld.woody, (double *) &gmw, mx,
                                   7 );
          AddToCurrentFireRing( firering, CurrentPoint, ld.fuel, ld.woody,
                                ld.duff, mx, cf.CrownLoadingBurned * 1000.0 );
        }
      }
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1c "
              "StillBurning=%d CanStillBurn=%d\n",
              CallLevel, "", StillBurning, CanStillBurn );
    if( Verbose >= CallLevel && NumBadPositions >= 1 )
      printf( "## fsxwburn4:BurnThread::PerimeterThread: "
              "Fire may have burned outside of landscape domain ##\n" );

    if( End < 0 ) break;

    prod.cuumslope[0] = 0.0;  //Reset after they have been read by burn::
    prod.cuumslope[1] = 0;
    begin = Begin;  //Restore local copies from Class data
    end = End;
    if( Verbose >= CallLevel )
      printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1d\n", CallLevel, "" );

    if( DoSpots ) {
      CurrentFire = -1;
      if( embers.NumEmbers > 0 ) {
        //Current time to Flight is iter*actual, but really (iter+1)*actual.
        embers.Flight( SIMTIME, SIMTIME + GetActualTimeStep() );
        //Check ember resting positions for overlap with burned areas.
        if( embers.NumSpots > 0 && EnableSpotFireGrowth(GETVAL) )
          embers.Overlap();
      }
      begin = 0;  //Go back to the beginning
      end = 0;
      DoSpots = false;
    }
    if( Verbose >= CallLevel )
      printf( "%*sfsxwburn4:BurnThread:PerimeterThread:1e\n", CallLevel, "" );

    break; //Eliminate if multithreading is restored
  } while( End > -1 );
  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:BurnThread:PerimeterThread:2 "
            "StillBurning=%d CanStillBurn=%d\n",
            CallLevel, "", StillBurning, CanStillBurn );

  CallLevel--;
} //BurnThread::PerimeterThread

/*============================================================================
  BurnThread::SurfaceFire
  Does surface fire calculations of spread rate and intensity.
*/
void BurnThread::SurfaceFire( void )
{ //BurnThread::SurfaceFire
  LoadGlobalFEData( fe );
  fros = spreadrate( ld.slope, m_windspd, ld.fuel );

  if( fros > 0.0 ) {  //If rate of spread is >0
    StillBurning = 1;  //Allows fire to continue burning
    GetAccelConst();   //Get acceleration constants

    VecSurf();         //Vector wind with slope

    ellipse( vecros, ivecspeed );  //Compute elliptical dimensions

    grow( vecdir );  //Compute time derivatives xt and yt for perimeter point
    AccelSurf1();    //Compute new equilibrium ROS and Avg. ROS in remaining
                     //timestep
    SlopeCorrect( 1 );
    SubTimeStep = timerem;

    limgrow();       //Limits HORIZONTAL growth to mindist

    AccelSurf2();    //Calcl ros & fli for SubTimeStep Kw/m from BTU/ft/s*/
  }
  else {
    timerem = 0;
    RosT = 0;
    fli = 0;
    FliFinal = 0;
  }
} //BurnThread::SurfaceFire

//============================================================================
void BurnThread::CrownFire(void)
{ //BurnThread::CrownFire
	// does crown fire calculations of spread rate and intensity (Van Wagner's)
	//double FabsFli=fabs(fli);
	double FabsFli = 3.4613 * (384.0 * (react / 0.189275) * (ExpansionRate /
		.30480060960) /
		(60.0 * savx));	// ending forward fli in timestep
	double ExpRate = ExpansionRate; 	// attempting to account for ws variation

	if (EnableCrowning(GETVAL))			   		   // global flag for disabling crowning
	{
		if (FabsFli > 0.0)  					  // use average fli
			cf.CrownIgnite(ld.height, ld.base, ld.density); 		  // calculate critical intensity from van Wagner
		else
			cf.Io = 1;
		if (FabsFli >= cf.Io)			   // if sufficient energy to ignite crown
		{
			//cf.CrownBurn(avgros, FabsFli, A);
      if( GetCrownFireCalculation() == 0 ) {                 //AAA FromG5 ver

        cf.CrownBurn( ExpRate, FabsFli, A );                 //AAA FromG5 ver

        R10=spreadrate( ld.slope, m_twindspd*0.4, 10 );      //AAA FromG5 ver

      }                                                      //AAA FromG5 ver

      else R10 = cf.CrownBurn2( ExpRate, FabsFli, A, this ); //AAA FromG5 ver

      //AAA cf.CrownBurn(ExpRate, FabsFli, A);   JAS! ver

      //AAA R10 = spreadrate(ld.slope, m_twindspd * 0.4, 10); JAS! ver


      if( R10 == 0.0 ) R10 = avgros;
			VecCrown(); 				// get vectored crown spread rate

			ellipse(vecros, ivecspeed); // determine elliptical dims with vectored winds not m_twindspd
			grow(vecdir);    	   	   // compute time derivatives xt & yt for perimeter pt.
			SpreadCorrect();   	   	   // correct vecros for directional spread
			//cros=cf.CrownSpread(avgros, vecros);	// wind-driven crown fire rate of spread Rothermel 1991
			cros = cf.CrownSpread(ExpRate, ExpansionRate);	// wind-driven crown fire rate of spread Rothermel 1991
			if (cros > 0.0)			   // if active crown fire
			{
				timerem = SubTimeStep;   // reset timerem again to before surface spread began in last substep
				A = cf.A;   			 // use crown fire acceleration rate from Crown::CrownBurn()
				AccelCrown1();
				SlopeCorrect(0);
				limgrow();   		   // limits HORIZONTAL growth to mindist
				AccelCrown2();
				SubTimeStep = SubTimeStep - timerem;  // actual subtimestep after crowning
				cf.FlameLength = 0;
				cf.CrownIntensity(cros, &fli);    // calc flamelength and FLI from crownfire
				//	AVGFlameLength=AVGFlameLength+FlameLength/SubTimeStep;  // fl/unittime
				//   CrownDuration=CrownDuration+SubTimeStep;   			 // total time spent crowning
				if (EnableSpotting(GETVAL) && FliFinal > 0.0)    // no spotting from dead perimeter
					SpotFire(0);	  // spotting from active crown fire
			}
			else						// if passive crown fire
			{
				cf.CrownIntensity(avgros, &fli);	// calc flamelength and FLI for passive crown fire
				if (EnableSpotting(GETVAL) && FliFinal > 0.0)    // no spotting from dead perimeter
				{
					cf.FlameLength = 0;
					SpotFire(1);	  // spotting from torching trees
				}
			}
			if (fli > fabs(FliFinal))
			{
				if (FliFinal < 0.0)
					FliFinal = fli * -1.0;
				else
					FliFinal = fli;
			}
		}
		else
			cf.CrownLoadingBurned = 0.0;	// need to set because of post-frontal
	}
	else
		cf.CrownLoadingBurned = 0.0;	// need to set because of post-frontal
} //BurnThread::CrownFire

//============================================================================
void BurnThread::SpotFire( int SpotSource )
{
	// calls spot fire functions for lofting embers
	if (turn && fli > 0.0)  								  // if check method 2 and second pass
	{
		if (turn == 1)
			CurrentTime = CuumTimeIncrement + SIMTIME;   	// timerem will be ~zero with method 2
		else
			CurrentTime = (GetActualTimeStep() - timerem) + SIMTIME;		// timerem will represent time remaining in actual TS
		embers.SpotSource = SpotSource;
		embers.Loft(cf.FlameLength, cf.CrownFractionBurned, ld.height,
				cf.CrownLoadingBurned, HorizSpread, SubTimeStep, CurrentTime);
	}
}

//============================================================================
void BurnThread::EmberCoords()
{
	// copies coordinates for perimeter segment that generates embers
	embers.Fcoord.x = xpt;
	embers.Fcoord.y = ypt;			// STORE PERIMETER SEGMENT FOR SPOTTING
	embers.Fcoord.xl = xptl;
	embers.Fcoord.xn = xptn;
	embers.Fcoord.yl = yptl;
	embers.Fcoord.yn = yptn;			// natural units (english or metric grid units)
	embers.Fcoord.e = (double) ld.elev; 	// meters
	embers.Fcoord.cover = ld.cover; 		// transfer cover to
}

//----------------------------------------------------------------------------
//Burn:: Functions
//----------------------------------------------------------------------------

//============================================================================
Burn::Burn()
{ //Burn::Burn
  SIMTIME = 0;
  CuumTimeIncrement = 0.0;
  burnthread = 0;
  NumPerimThreads = 0;
  fe = new FELocalSite();
  env = new FireEnvironment2();

  NumSpots = 0;
  ExNumPts = 0;
} //Burn::Burn

//============================================================================
Burn::~Burn()
{ //Burn::~Burn
  CloseAllPerimThreads();
  FreeAllFirePerims();
  FreeElev();
  delete fe;
  delete env;
} //Burn::~Burn

//============================================================================
void Burn::BurnIt( long count )
{ //Burn::BurnIt
  CallLevel++;

  //Controls access to different burn methods and output options.
  CurrentFire = count;  //Local copy of currentfire

  if( Verbose > CallLevel )
    printf( "%*sfsxwburn4:Burn:BurnIt:1 count=%ld SIMTIME=%lf\n",
            CallLevel, "", count, SIMTIME );

  if( DistanceCheckMethod(GETVAL) ) {
    if( Verbose > CallLevel )
      printf( "%*sfsxwburn4:Burn:BurnIt:1a\n", CallLevel, "" );

    BurnMethod1();  //Distance checking on individual fire basis

    if( Verbose > CallLevel )
      printf( "%*sfsxwburn4:Burn:BurnIt:1b\n", CallLevel, "" );
  }
  else {
    if( Verbose > CallLevel )
      printf( "%*sfsxwburn4:Burn:BurnIt:1c\n", CallLevel, "" );

    BurnMethod2();   //Distance checking on Simulation basis
  }

  CallLevel--;
} //Burn::BurnIt

//============================================================================
void Burn::ResetAllPerimThreads()
{
	CloseAllPerimThreads();
}

//============================================================================
bool Burn::AllocPerimThreads()
{ //Burn::AllocPerimThreads
  if( NumPerimThreads == GetMaxThreads() ) return true;

  CloseAllPerimThreads();
  burnthread = ( BurnThread * * ) new BurnThread * [GetMaxThreads()];

	long i;
	if( burnthread ) {
		NumPerimThreads = GetMaxThreads();
		NumSpots = new long[NumPerimThreads];
		for (i = 0; i < NumPerimThreads; i++)
			burnthread[i] = new BurnThread( env );
		return true;
	}

	return false;
} //Burn::AllocPerimThreads

//============================================================================
void Burn::CloseAllPerimThreads()
{ //Burn::CloseAllPerimThreads
  if( burnthread ) {
    for( int i = 0; i < NumPerimThreads; i++ ) delete burnthread[i];
    delete[] burnthread;
    if( NumSpots ) delete[] NumSpots;
  }

  burnthread = 0;
  NumPerimThreads = 0;
  NumSpots = 0;
} //Burn::CloseAllPerimThreads

//============================================================================
void Burn::ResumeSpotThreads(long threadct)
{
	long i;

	env->CheckMoistureTimes(SIMTIME); //must do here because ember flight uses: GetFireEnvt()
	for (i = 0; i < threadct; i++)
	{
		burnthread[i]->SetRange( 0, SIMTIME, CuumTimeIncrement, TimeIncrement,
						TimeMaxRem, 0, 0, 0, FireIsUnderAttack, firering);
		if( ! burnthread[i]->Started ) {
			burnthread[i]->StartBurnThread( i );
		}
	}
	for (i = 0; i < threadct; i++)
	{
		burnthread[i]->DoSpots = true;
		burnthread[i]->StartBurnThread( i );
	}
}

//============================================================================
void Burn::ResumePerimeterThreads( long threadct )
{ //Burn::ResumePerimeterThreads
  long i;

  for( i = 0; i < threadct; i++ )
    burnthread[i]->SetRange( CurrentFire, SIMTIME, CuumTimeIncrement,
                             TimeIncrement, TimeMaxRem, -1, 0, Turn,
                             FireIsUnderAttack, firering );
  for( i = 0; i < threadct; i++ ) burnthread[i]->StartBurnThread( i );
} //Burn::ResumePerimeterThreads

//============================================================================
long Burn::StartPerimeterThreads_Equal()
{ //Burn::StartPerimeterThreads_Equal
  long i;
  long begin, end, threadct, range;
  double fract, interval, ipart;
  CallLevel++;

  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:Burn:StartPerimeterThreads_Equal:1\n",
            CallLevel, "" );
  AllocPerimThreads();
  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:Burn:StartPerimeterThreads_Equal:2\n",
            CallLevel, "" );

  interval = ( (double) GetNumPoints(CurrentFire) ) /
              ( (double) NumPerimThreads );
  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:Burn:StartPerimeterThreads_Equal:3 interval=%lf\n",
            CallLevel, "", interval );

  fract = modf( interval, &ipart );
  range = ( long ) interval;
  if( fract > 0.0 ) range++;

  begin = threadct = 0;

  for( i = 0; i < NumPerimThreads; i++ ) {
    end = begin + range;
    if( begin >= GetNumPoints(CurrentFire) ) continue;
    if( end > GetNumPoints(CurrentFire) ) end = GetNumPoints(CurrentFire);

    burnthread[i]->SetRange( CurrentFire, SIMTIME, CuumTimeIncrement,
                             TimeIncrement, TimeMaxRem, begin, end, Turn,
                             FireIsUnderAttack, firering );

    threadct++;
    begin = end;
  }
  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:Burn:StartPerimeterThreads_Equal:4\n",
            CallLevel, "" );

  for( i = 0; i < threadct; i++ ) burnthread[i]->StartBurnThread( i );
  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:Burn:StartPerimeterThreads_Equal:5 \n",
            CallLevel, "" );

  CallLevel--;
  return threadct;
} //Burn::StartPerimeterThreads_Equal

//============================================================================
void Burn::BurnMethod1()
{ //Burn::BurnMethod1
  CallLevel++;

  //In-TimeStep growth of new inward fires 12/27/1994.
  bool INFIRE = false;
  long i;
  long NewFires;
  long NewPts;
  long NewInFires;
  long ThisInFire;
  long InFiresBurned = 0;  //Number of new inward burning fires

  //Save fire type in case inward fire burns out in this timestep.
  long FireType = GetInout( CurrentFire );
  long NumStartAttack;
  long NumLastAttack;
  bool GroupDirectAttack;
  double DownTime;
  double xpt, ypt;

  GroupAirAttack* gaat;
  AttackData* atk;    //Pointer to AttackData struct
  Attack Atk;         //Instance of Attack

  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:Burn:BurnMethod1:1 SIMTIME=%lf\n",
            CallLevel, "", SIMTIME );
  NewInFires = NewFires = GetNewFires();  //Copy of new fire number
  NumInFires = 0;   //Reset BURN::NumInFires;
  Turn = 0;
  StillBurning = false;     //Start each fire out as burning (changed)
  CanStillBurn = false;     //Must test to see if fire is still active
  InitRect( CurrentFire );  //Resets hi's and lo's for bounding rectangle
  TimeIncrement = EventTimeStep = GetActualTimeStep();
  CuumTimeIncrement = 0.0;
  AllocElev( CurrentFire );  //Alloc space for elevations
  tranz( CurrentFire, 0 );   //Transfer pts to perimeter2 array for next turn
  TimeMaxRem = 0.0;          //Maximum time remaining
  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:Burn:BurnMethod1:2\n", CallLevel, "" );

  if( (atk = GetAttackForFireNumber(CurrentFire, 0, &NumLastAttack)) != 0 )
    FireIsUnderAttack = true;
  else FireIsUnderAttack = false;
  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:Burn:BurnMethod1:3\n", CallLevel, "" );

  if( (GetGroupAttackForFireNumber(CurrentFire, 0, &NumLastAttack)) != 0 ) {
    GroupDirectAttack = true;
    gaat = new GroupAirAttack();
  }
  else {
    GroupDirectAttack = false;
    gaat = 0;
  }

  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:Burn:BurnMethod1:4 CuumTimeIncrement=%lf\n",
            CallLevel, "", CuumTimeIncrement );

  do {
    if( Verbose >= CallLevel )
      printf( "%*sfsxwburn4:Burn:BurnMethod1:4a StillBurning=%d "
              "numpts[%ld]=%ld numfires=%ld\n",
              CallLevel, "", StillBurning, CurrentFire,
              GetNumPoints(CurrentFire), numfires );
    ExNumPts = 0;
    if( GetInout(CurrentFire) == 3 ) {
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4a1\n", CallLevel, "" );
      CrossFires( 0, &CurrentFire );
      CanStillBurn = true;
      TimeIncrement = 0.0;
      CuumTimeIncrement = GetActualTimeStep();
      break;
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwburn4:Burn:BurnMethod1:4b StillBurning=%d "
              "TimeIncrement=%lf EventTimeStep=%lf CuumTimeIncrement=%lf "
              "TimeMaxRem=%lf numpts[%ld]=%ld\n",
              CallLevel, "", StillBurning, TimeIncrement, EventTimeStep,
              CuumTimeIncrement, TimeMaxRem, CurrentFire,
              GetNumPoints(CurrentFire) );

    /*
    //Air attack, must be here because it is attacking perim1.
    if( turn == 0 && GroupDirectAttack ) {
      NumStartAttack=0;
      while( (GetGroupAttackForFireNumber(CurrentFire, NumStartAttack,
              &NumLastAttack)) != 0 ) {
        NumStartAttack = NumLastAttack + 1;
        gaat->GetCurrentGroup();
        if( gaat->CheckSuspendState(GETVAL) ) continue;
        gaat->ExecuteAttacks( 0.0 );  //For each aircraft,
      }    //Execute, but if not==TimeIncrement, just dec waittimes
      CrossesWithBarriers( CurrentFire );  //Merge with barriers if present
    }
    */

    //Multithreading stuff.
    NumStartAttack = 0;
    if( Turn == 0 ) {
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4b1\n", CallLevel, "" );
      env->CheckMoistureTimes( SIMTIME + CuumTimeIncrement );
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4b2\n", CallLevel, "" );

      ThreadCount = StartPerimeterThreads_Equal();
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4b3 "
                "burnthread[0]->StillBurning=%d\n",
                CallLevel, "", burnthread[0]->StillBurning );
    }
    else {
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4b4 "
                "burnthread[0]->StillBurning=%d\n",
                CallLevel, "", burnthread[0]->StillBurning );
      ResumePerimeterThreads( ThreadCount );
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4b5 "
                "burnthread[0]->StillBurning=%d\n",
                CallLevel, "", burnthread[0]->StillBurning );

      CanStillBurn = burnthread[0]->CanStillBurn;
      StillBurning = burnthread[0]->StillBurning;
      prod.cuumslope[0] += burnthread[0]->prod.cuumslope[0];
      prod.cuumslope[1] += burnthread[0]->prod.cuumslope[1];

      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4b6 "
                "burnthread[0]->StillBurning=%d\n",
                CallLevel, "", burnthread[0]->StillBurning );
      for( i = 1; i < ThreadCount; i++ ) {
        if( burnthread[i]->CanStillBurn ) CanStillBurn = true;
        if( burnthread[i]->StillBurning ) StillBurning = true;
        prod.cuumslope[0] += burnthread[i]->prod.cuumslope[0];
        prod.cuumslope[1] += burnthread[i]->prod.cuumslope[1];
      }
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4b7\n", CallLevel, "" );
    }
    //------------------------------------------------------------------------
    if( Verbose >= CallLevel )
      printf( "%*sfsxwburn4:Burn:BurnMethod1:4c "
              "StillBurning=%d TimeIncrement=%lf numpts[%ld]=%ld\n",
              CallLevel, "", StillBurning, TimeIncrement,
              CurrentFire, GetNumPoints(CurrentFire) );

    if( Turn ) {
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4c1\n", CallLevel, "" );

      for( i = 0; i < GetNumPoints(CurrentFire); i++ ) {
        xpt = GetPerimeter1Value( CurrentFire, i, XCOORD );
        ypt = GetPerimeter1Value( CurrentFire, i, YCOORD );
        DetermineHiLo( xpt, ypt );
      }
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4c2\n", CallLevel, "" );

      if( CuumTimeIncrement == 0.0 && INFIRE == false ) {
        //Should only happen first time through.
        prod.arp( CurrentFire );   //Calc area of previous fire perimeter
        DownTime = GetDownTime();  //Time spent out of active burn period
      }
      else DownTime = 0.0;
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4c3\n", CallLevel, "" );

      NewFires = GetNewFires();
      while( (atk = GetAttackForFireNumber(CurrentFire, NumStartAttack,
                                           &NumLastAttack)) != 0 ) {
        if( Verbose >= CallLevel )
          printf( "%*sfsxwburn4:Burn:BurnMethod1:4c3a\n", CallLevel, "" );

        NumStartAttack = NumLastAttack + 1;
        if( atk->Indirect == 2 ) {
          if( ! Atk.ParallelAttack(atk, TimeIncrement + DownTime) )
            CancelAttack( atk->AttackNumber );
        }
        //Perform attack on this fire in timeincrement.
        else if( ! Atk.DirectAttack(atk, TimeIncrement + DownTime) )
          CancelAttack( atk->AttackNumber );
      }
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4c4\n", CallLevel, "" );

      //Air attack, must be here because it is attacking perim1.
      if( GroupDirectAttack ) {
        if( Verbose >= CallLevel )
          printf( "%*sfsxwburn4:Burn:BurnMethod1:4c4a\n",
                  CallLevel, "" );

        NumStartAttack = 0;
        while( (GetGroupAttackForFireNumber(CurrentFire,
               NumStartAttack, &NumLastAttack)) != 0 ) {
          if( Verbose >= CallLevel )
            printf( "%*sfsxwburn4:Burn:BurnMethod1:4c4a1\n",
                    CallLevel, "" );

          NumStartAttack = NumLastAttack + 1;
          gaat->GetCurrentGroup();
          if( gaat->CheckSuspendState(GETVAL) ) continue;
          gaat->ExecuteAttacks( 0.0 ); //For each aircraft,
        }     //execute, but if not==TimeIncrement, just dec waittimes
      }

      WriteHiLo( CurrentFire );
      if( FireIsUnderAttack ) {
        RestoreDeadPoints( CurrentFire );
        BoundingBox( CurrentFire );
      }

      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4c5 StillBurning=%d "
                "numpts[%ld]=%ld\n",
                CallLevel, "", StillBurning, CurrentFire,
                GetNumPoints(CurrentFire) );

      //If inward fire not eliminated && still burning....
      if( GetInout(CurrentFire) != 0 && StillBurning ) {
        if( Verbose >= CallLevel )
          printf( "%*sfsxwburn4:Burn:BurnMethod1:4c5a\n",
                  CallLevel, "" );

        if( rastmake )      //Compute raster information
          rast.rasterinit( CurrentFire, 0, SIMTIME, TimeIncrement,
                           CuumTimeIncrement + TimeIncrement );

        if( Verbose >= CallLevel )
          printf( "%*sfsxwburn4:Burn:BurnMethod1:4c5b "
                  "\n",
                  CallLevel, "" );

        if( FireIsUnderAttack ) {
          while( GetNewFires() > NewFires ) {
            if( GetInout(NewFires) != 1 ) {
              NewFires++;
              continue;
            }
            ReorderPerimeter( NewFires, FindExternalPoint(NewFires, 0) );
            FindOuterFirePerimeter( NewFires );
            NewPts = GetNumPoints( NewFires );
            FreePerimeter1( NewFires );
            if( NewPts > 0 ) {
              AllocPerimeter1( NewFires, NewPts + 1 );
              tranz( NewFires, NewPts );
            }
            else {
              SetNumPoints( NewFires, 0 );
              SetInout( NewFires, 0 );
              IncSkipFires( 1 );
            }
            NewFires++;
          }
        }

        if( Verbose >= CallLevel )
          printf( "%*sfsxwburn4:Burn:BurnMethod1:4c5c "
                  "numpts[%ld]=%ld\n",
                  CallLevel, "", CurrentFire, GetNumPoints(CurrentFire) );

        CrossFires( 0, &CurrentFire );  //Clip cross-overs and rediscretize

        if( Verbose >= CallLevel )
          printf( "%*sfsxwburn4:Burn:BurnMethod1:4c5d numpts[%ld]=%ld\n",
                  CallLevel, "", CurrentFire, GetNumPoints(CurrentFire) );

        if( GetInout(CurrentFire) == 2 ) {  //If inward burning fire
          if( arp(1, CurrentFire) >= 0.0 )  //Eliminates inward fire if area>0
            EliminateFire( CurrentFire );
        }

        if( Verbose >= CallLevel )
          printf( "%*sfsxwburn4:Burn:BurnMethod1:4c5e numpts[%ld]=%ld\n",
                  CallLevel, "", CurrentFire, GetNumPoints(CurrentFire) );

        CrossesWithBarriers( CurrentFire );  //Merge with barriers if present
        while( GetNewFires() > NewInFires ) {
          ThisInFire = NewInFires++;   //Copy before incrementing newinfires
          if( GetInout(ThisInFire) == 2 ) {
            //Transfer points to perimeter2 in case it is removed.
            tranz(ThisInFire, 0);

            CrossFires(0, &ThisInFire);    //Clip cross-overs and rediscretize
            if( GetInout(ThisInFire) == 2 ) {
              if( arp(1, ThisInFire) >= 0.0 )  //Elims inward fire if area>0
                EliminateFire( ThisInFire );
              else if( (GetActualTimeStep() - CuumTimeIncrement +
                        TimeIncrement) > 0.0 )
                AllocNewInFire( ThisInFire,
                                GetActualTimeStep() - CuumTimeIncrement - 
                                TimeIncrement );
            }
          }
        }
      }
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4c6\n", CallLevel, "" );

      CuumTimeIncrement += TimeIncrement;  //Accumulate timeincrements

      //If inward fire has now been eliminated....
      if( GetInout(CurrentFire) == 0 || ! StillBurning ) {
        if( FireType == 2 && StillBurning ) {   //Inward fire was extinguished
          if( rastmake )  //Make rasterizing accurate to the nearest time step
            rast.rasterinit( CurrentFire, ExNumPts, SIMTIME,
                             CuumTimeIncrement, GetActualTimeStep() );
          //Compute raster information.
        }
        TimeIncrement = 0.0;  //Zero time left in timestep
      }
      else {
        TimeIncrement = GetActualTimeStep() - CuumTimeIncrement;
        EventTimeStep = TimeIncrement;  //Reset Event drivent Time Step
        //Transfer points to perimeter2 array for next turn.
        tranz( CurrentFire, 0 );
        Turn = 0;
      }
    }
    else {
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4c7 TimeIncrement=%lf\n",
                CallLevel, "", TimeIncrement );

      TimeMaxRem = burnthread[0]->TimeMaxRem;

      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4c8 TimeMaxRem=%lf\n",
                CallLevel, "", TimeMaxRem );

      EventTimeStep = burnthread[0]->EventTimeStep;
      for( i = 1; i < ThreadCount; i++ ) {
        if( burnthread[i]->TimeMaxRem > TimeMaxRem )
          TimeMaxRem = burnthread[i]->TimeMaxRem;

        if( burnthread[i]->EventTimeStep < EventTimeStep )
          EventTimeStep = burnthread[i]->EventTimeStep;
      }
      TimeIncrement -= TimeMaxRem;  //Calculate distance limited TS

      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4c9 TimeIncrement=%lf\n",
                CallLevel, "", TimeIncrement );

      if( GroupDirectAttack ) {
        while( (GetGroupAttackForFireNumber(CurrentFire,
                                    NumStartAttack, &NumLastAttack)) != 0 ) {
          NumStartAttack = NumLastAttack + 1;
          gaat->GetCurrentGroup();   //Get current group attack
          if( gaat->CheckSuspendState(GETVAL) ) continue;
          EventTimeStep = gaat->GetNextAttackTime( EventTimeStep );
        }
      }
      //Check to see if event limited TS < distance limited TS.
      if( EventTimeStep < TimeIncrement && EventTimeStep > 0.01 )
        TimeIncrement = EventTimeStep;
      if( CuumTimeIncrement > 0.0 && GroupDirectAttack )
        gaat->IncrementWaitTimes( TimeIncrement );  //Subtract last timestep
      Turn = 1;
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod1:4c10 "
                "TimeIncrement=%lf Just set turn = 1\n",
                CallLevel, "", TimeIncrement );
    }

    if( Verbose >= CallLevel )
      printf( "%*sfsxwburn4:Burn:BurnMethod1:4d "
              "SIMTIME=%lf TimeIncrement=%lf numpts[%ld]=%ld\n",
              CallLevel, "", SIMTIME, TimeIncrement, CurrentFire,
              GetNumPoints(CurrentFire) );

    TimeMaxRem = 0.0;
    if( TimeIncrement == 0.0 && NumInFires > InFiresBurned ) {
      do {
        //AAA if (InFiresBurned <= NumInFires)  //JAS! was strictly <  JAS!
        if( InFiresBurned < NumInFires ) {  //BLN original  In FromG5 ver
          GetNewInFire( InFiresBurned );
          InFiresBurned++;
        }
        else TimeIncrement = -1.0;

        if( TimeIncrement > 0.0 ) {
          CuumTimeIncrement = GetActualTimeStep() - TimeIncrement;
          EventTimeStep = TimeIncrement; //Reset EventTimeStep
          AllocElev( CurrentFire );  //Alloc space for elevations
          //Transfer points to perimeter2 array for next turn.
          tranz( CurrentFire, 0 );
          InitRect(CurrentFire); //Resets hi's and lo's for bounding rectangle
          CanStillBurn = false;  //Reset ability to burn as false
          FireType = 2;  //Set local var. FireType to inward fire
          Turn = 0;      //Set turn to 1st time through
          INFIRE = true;
        }
      } while( TimeIncrement == 0.0 );
    }
  } while( TimeIncrement > 0.0 );  //While time remaining in time step

  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:Burn:BurnMethod1:5 CuumTimeIncrement=%lf\n",
            CallLevel, "", CuumTimeIncrement );

  //Eliminates fire perimeter around rock or lake islands.
  if( ! StillBurning && ! CanStillBurn && GetInout(CurrentFire) == 2 ) {
    if( PreserveInactiveEnclaves(GETVAL) == false ) //But not in fuel
      EliminateFire( CurrentFire );
  }

  FreeNewInFires();
  if( gaat ) delete gaat;    //Group air attack

  if( Verbose >= CallLevel )
    printf( "%*sfsxwburn4:Burn:BurnMethod1:6\n", CallLevel, "" );
  CallLevel--;
} //Burn::BurnMethod1

//============================================================================
void Burn::DetermineSimLevelTimeStep()
{ //Burn::DetermineSimLevelTimeStep
  TimeMaxRem = 0.0;
  TimeIncrement = EventTimeStep = GetActualTimeStep() - CuumTimeIncrement;
  CuumPoint = 0;
  for( CurrentFire = 0; CurrentFire < GetNumFires(); CurrentFire++ ) {
    if( GetInout(CurrentFire) == 3 || GetInout(CurrentFire) == 0 ) continue;
    PreBurn();
  }
  TimeIncrement -= TimeMaxRem;
 
  //Check to see if event limited TS < distance limited TS.
  if( EventTimeStep < TimeIncrement && EventTimeStep > 0.01 )
    TimeIncrement = EventTimeStep;
  SetTemporaryTimeStep( TimeIncrement );
  CuumPoint = 0;
} //Burn::DetermineSimLevelTimeStep

//============================================================================
void Burn::PreBurn()
{
	// just finds out what is the fastest spreading point among all fires ==> sets timestep
	bool GroupDirectAttack; //ComputeSpread,
	long i, NumStartAttack = 0, NumLastAttack; //i, TestPointL, TestPointN,
	GroupAirAttack* gaat;

	AllocElev(CurrentFire);				// alloc space for elevations
	tranz(CurrentFire, 0);				// transfer points to perimeter2 array for next turn
  if( (GetAttackForFireNumber(CurrentFire, 0, &NumLastAttack)) != 0 )
    FireIsUnderAttack = true;
  else FireIsUnderAttack = false;
  if( (GetGroupAttackForFireNumber(CurrentFire, 0, &NumLastAttack)) != 0 ) {
    GroupDirectAttack = true;
    gaat = new GroupAirAttack();
  }
  else {
    GroupDirectAttack = false;
    gaat = 0;
  }

	//--------------------------------------------------------------------------
	Turn = 0;
	env->CheckMoistureTimes(SIMTIME + CuumTimeIncrement);
	ThreadCount = StartPerimeterThreads_Equal();

	TimeMaxRem = burnthread[0]->TimeMaxRem;

	EventTimeStep = burnthread[0]->EventTimeStep;
	for( i = 1; i < ThreadCount; i++ ) {
		if( burnthread[i]->TimeMaxRem > TimeMaxRem )
			TimeMaxRem = burnthread[i]->TimeMaxRem;

    if( burnthread[i]->EventTimeStep < EventTimeStep )
			EventTimeStep = burnthread[i]->EventTimeStep;
	}
	//--------------------------------------------------------------------------
  if( GroupDirectAttack ) {
    while( (GetGroupAttackForFireNumber(CurrentFire, NumStartAttack,
          &NumLastAttack)) != 0 ) {
			NumStartAttack = NumLastAttack + 1;
			gaat->GetCurrentGroup();   // get current group attack
			if (gaat->CheckSuspendState(GETVAL))
				continue;
			EventTimeStep = gaat->GetNextAttackTime(EventTimeStep);
		}
	}
}


//============================================================================
void Burn::BurnMethod2()
{ //Burn::BurnMethod2
  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfsxwburn4:Burn:BurnMethod2:1\n", CallLevel, "" );

  //Simulation-level process control, uses time step determind by PreBurn().
  long NewFires;
  long NewPts;
  //Save fire type in case inward fire burns out in this timestep.
  long FireType = GetInout( CurrentFire );
  long NumStartAttack;
  long NumLastAttack;
  bool FireIsUnderAttack;  //=false;
  bool GroupDirectAttack;  //=false;
  long i;
  double xpt, ypt;
  GroupAirAttack* gaat;
  AttackData* atk;  //Pointer to AttackData struct
  Attack Atk;       //Instance of Attack for rasterizing inside of extinct fire
  Turn = 1;         //Always is 'turn' because timestep already determined
  StillBurning = 1; //Start each fire out as burning (changed)
  CanStillBurn = false;     //Must test to see if fire is still active
  InitRect( CurrentFire );  //Resets hi's and lo's for bounding rectangle
  TimeIncrement = GetTemporaryTimeStep();
  tranz( CurrentFire, 0 );  //Transfer points to perimeter2 array for next turn
  if( (atk = GetAttackForFireNumber(CurrentFire, 0, &NumLastAttack)) != 0 )
    FireIsUnderAttack = true;
  else FireIsUnderAttack = false;
  if( (GetGroupAttackForFireNumber(CurrentFire, 0, &NumLastAttack)) != 0 ) {
    GroupDirectAttack = true;
    gaat = new GroupAirAttack();
  }
  else {
    GroupDirectAttack = false;
    gaat = 0;
  }

  if( GetInout(CurrentFire) == 3 ) {
    CrossFires( 0, &CurrentFire );
    CanStillBurn = true;
    TimeIncrement = 0.0;

    if( Verbose > CallLevel )
      printf( "%*sfsxwburn4:Burn:BurnMethod2:1a\n", CallLevel, "" );

    CallLevel--;

    return;
  }

  AllocElev( CurrentFire );  //Alloc space for elevations
  if( CheckPostFrontal(GETVAL) )    //Store current fire perim in ring
    firering = post.SetupFireRing( CurrentFire, SIMTIME + CuumTimeIncrement,
                                 SIMTIME + CuumTimeIncrement + TimeIncrement );

  //-------------------------------------------------------------------------
  NumStartAttack = 0;

  env->CheckMoistureTimes( SIMTIME + CuumTimeIncrement );
  ThreadCount = StartPerimeterThreads_Equal();

  CanStillBurn = burnthread[0]->CanStillBurn;
  StillBurning = burnthread[0]->StillBurning;
  prod.cuumslope[0] += burnthread[0]->prod.cuumslope[0];
  prod.cuumslope[1] += burnthread[0]->prod.cuumslope[1];
  for( i = 1; i < ThreadCount; i++ ) {
    if( burnthread[i]->CanStillBurn ) CanStillBurn = true;
    if( burnthread[i]->StillBurning ) StillBurning = true;
    prod.cuumslope[0] += burnthread[i]->prod.cuumslope[0];
    prod.cuumslope[1] += burnthread[i]->prod.cuumslope[1];
  }

  for( i = 0; i < GetNumPoints(CurrentFire); i++ ) {
    xpt = GetPerimeter1Value( CurrentFire, i, XCOORD );
    ypt = GetPerimeter1Value( CurrentFire, i, YCOORD );
    DetermineHiLo( xpt, ypt );
  }
  //-------------------------------------------------------------------------

  ExNumPts = 0;
  prod.arp( CurrentFire );  //Calc area of previous fire perimeter
  NewFires = GetNewFires();
  while( (atk = GetAttackForFireNumber(CurrentFire, NumStartAttack,
                                       &NumLastAttack)) != 0 ) {
    NumStartAttack = NumLastAttack + 1;
    if( atk->Indirect == 2 ) {
      if( ! Atk.ParallelAttack(atk, TimeIncrement + GetDownTime()) )
        CancelAttack( atk->AttackNumber );
    }
    //Perform attack on this fire in timeincrement
    else if( ! Atk.DirectAttack(atk, TimeIncrement + GetDownTime()) )
      CancelAttack( atk->AttackNumber );
  }
  if( GroupDirectAttack ) {
    NumStartAttack = 0;
    while( (GetGroupAttackForFireNumber(CurrentFire, NumStartAttack,
                                        &NumLastAttack)) != 0 ) {
      if( Verbose >= CallLevel )
        printf( "%*sfsxwburn4:Burn:BurnMethod2:1b1\n",
                CallLevel, "" );

      NumStartAttack = NumLastAttack + 1;
      gaat->GetCurrentGroup();
      if( gaat->CheckSuspendState(GETVAL) ) continue;
      //For each aircraft, execute,
      //but if not==TimeIncrement, just dec waittimes.
      gaat->ExecuteAttacks( TimeIncrement + GetDownTime() );
    }
  }
  WriteHiLo( CurrentFire );
  if( FireIsUnderAttack ) {
    RestoreDeadPoints( CurrentFire );
    BoundingBox( CurrentFire );
  }

  if( FireIsUnderAttack && CheckPostFrontal(GETVAL) )
    post.UpdateAttackPoints( firering, CurrentFire );

  //If inward fire not eliminated && still burning compute raster information.
  if( GetInout(CurrentFire) != 0 && StillBurning ) {
    if( rastmake )
      rast.rasterinit( CurrentFire, ExNumPts, SIMTIME, TimeIncrement,
                       CuumTimeIncrement + TimeIncrement );
    if( FireIsUnderAttack ) {
      while( GetNewFires() > NewFires ) {
        if( GetInout(NewFires) != 1 ) {
          NewFires++;
          continue;
        }
        ReorderPerimeter( NewFires, FindExternalPoint(NewFires, 0) );
        FindOuterFirePerimeter( NewFires );
        NewPts = GetNumPoints( NewFires );
        FreePerimeter1( NewFires );
        if( NewPts > 0 ) {
          AllocPerimeter1( NewFires, NewPts + 1 );
          tranz( NewFires, NewPts );
        }
        else {
          //FreePerimeter2();
          SetNumPoints( NewFires, 0 );
          SetInout( NewFires, 0 );
          IncSkipFires( 1 );
        }
        NewFires++;
      }
    }
    CrossFires( 0, &CurrentFire );        //Clip cross-overs and rediscretize
    if( GetInout(CurrentFire) == 2 ) {    //If inward burning fire
      if( arp(1, CurrentFire) >= 0.0 ) {  //Eliminates inward fire if area>0
        EliminateFire( CurrentFire );
        if( CheckPostFrontal(GETVAL) ) FreeFireRing( GetNumRings() - 1 );
      }
    }
    CrossesWithBarriers( CurrentFire );   //Merge with barriers if present
  }

  //If inward fire has now been eliminated
  if( GetInout(CurrentFire) == 0 || ! StillBurning ) {
    if( FireType == 2 && StillBurning ) {    //Inward fire was extinguished
      if( rastmake )   //Make rasterizing accurate to the nearest time step.
        rast.rasterinit( CurrentFire, ExNumPts, SIMTIME, CuumTimeIncrement,
                         CuumTimeIncrement + TimeIncrement );
          //GetActualTimeStep());
    }
    TimeIncrement = 0.0;      //Zero time left in timestep
  }

  if( ! StillBurning && ! CanStillBurn && GetInout(CurrentFire) == 2) {
    //Eliminates fire perimeter around rock or lake islands
    if( PreserveInactiveEnclaves(GETVAL) == false )    //But not in fuel
      EliminateFire( CurrentFire );
    if( CheckPostFrontal(GETVAL) ) FreeFireRing( GetNumRings() - 1 );
  }
  if( GroupDirectAttack ) //Subtract last timestep
    gaat->IncrementWaitTimes( TimeIncrement + GetDownTime() );

  if( Verbose > CallLevel )
    printf( "%*sfsxwburn4:Burn:BurnMethod2:2\n", CallLevel, "" );

  CallLevel--;
} //Burn::BurnMethod2


void Burn::AllocNewInFire(long NewNum, double TimeInc)
{
	// allocates linked list of new inward fire structures
	if (NumInFires == 0)		// BURN::NumInFires data member
	{
		FirstInFire = (newinfire *) new newinfire;//GlobalAlloc(GMEM_FIXED |  GMEM_ZEROINIT, 1*sizeof(newinfire));
		//TempInNext = (newinfire *)
		//	FirstInFire->next = (newinfire *) new newinfire;//GlobalAlloc(GMEM_FIXED |  GMEM_ZEROINIT, 1*sizeof(newinfire));

		newinfire* t_innext;
		t_innext = new newinfire;
		FirstInFire->next = t_innext;
		TempInNext = t_innext;
		
		TempInFire = FirstInFire;
	}
	TempInFire->FireNum = NewNum;
	TempInFire->TimeInc = TimeInc;
	TempInFire = TempInNext;
	//TempInNext = (newinfire *) TempInFire->next = (newinfire *) new newinfire;//GlobalAlloc(GMEM_FIXED |  GMEM_ZEROINIT, 1*sizeof(newinfire));
	
	newinfire* t_innext;
	t_innext = new newinfire;
	TempInFire->next = t_innext;
	TempInNext = t_innext;

	NumInFires++;
}


void Burn::GetNewInFire(long InFire)
{
	// retrieves data from linked list of new inward fires
	if (InFire == 0)
	{
		CurInFire = FirstInFire;
		NextInFire = (newinfire *) FirstInFire->next;
	}
	CurrentFire = CurInFire->FireNum;   	// BURN::CurrentFire
	TimeIncrement = CurInFire->TimeInc;	   // BURN::TimeIncrement
	CurInFire = NextInFire;
	NextInFire = (newinfire *) CurInFire->next;
}


void Burn::FreeNewInFires()
{
	// frees all linked list for new inward fires
	long i;

	if (NumInFires)
	{
		CurInFire = FirstInFire;
		NextInFire = (newinfire *) CurInFire->next;
		for (i = 0; i < NumInFires; i++)
		{
			delete CurInFire;//GlobalFree(CurInFire);
			CurInFire = NextInFire;
			NextInFire = (newinfire *) CurInFire->next;
		}
		delete CurInFire; //GlobalFree(CurInFire);
		delete NextInFire; //GlobalFree(NextInFire);
		NumInFires = 0;
	}
}


void Burn::EliminateFire(long FireNum)
{
	// removes a fire, and cleans up memory for it
	SetNumPoints(FireNum, 0);  		// reset number of points
	SetInout(FireNum, 0);     	 	// reset inward/outward indicator
	FreePerimeter1(FireNum);			// free perimeter array
	IncSkipFires(1);			  	// increment number of extinquished fires
}


void Burn::SetSpotLocation(long loc)
{
	long i;
	if (loc < 0)
	{
		for (i = 0; i < NumPerimThreads; i++)
		{
			if (loc < -1)   					// remove all of them
			{
				burnthread[i]->embers.CarrySpots = 0;
				burnthread[i]->embers.NextSpot = burnthread[i]->embers.FirstSpot;
				if (loc < -2)
					burnthread[i]->embers.EmberReset();
			}
			else if( burnthread[i]->embers.CarrySpots > 0 )
				burnthread[i]->embers.NextSpot = (Embers::spotdata *)
					burnthread[i]->embers.CarrySpot->next;
			else
				burnthread[i]->embers.NextSpot = burnthread[i]->embers.FirstSpot;
			if( burnthread[i]->embers.NumSpots > 0 )
				burnthread[i]->embers.SpotReset(burnthread[i]->embers.NumSpots,
										burnthread[i]->embers.FirstSpot);
			else if( burnthread[i]->embers.SpotFires > 0 )
				burnthread[i]->embers.SpotReset(burnthread[i]->embers.SpotFires -
										burnthread[i]->embers.CarrySpots,
										burnthread[i]->embers.NextSpot);
		}
		SpotCount = 0;

		return;
	}

	CurThread = -1;
	if (EnableSpotFireGrowth(GETVAL))
	{
		for (i = 0; i < NumPerimThreads; i++)
		{
			if (CurThread == -1 && NumSpots[i] > 0)
			{
				CurThread = i;
				burnthread[CurThread]->embers.CurSpot = burnthread[CurThread]->embers.FirstSpot;
				burnthread[CurThread]->embers.CarrySpot = burnthread[CurThread]->embers.FirstSpot;
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < NumPerimThreads; i++)
		{
			if (CurThread == -1 && NumSpots[i] > 0)
			{
				CurThread = i;
				burnthread[CurThread]->embers.CurSpot = burnthread[CurThread]->embers.FirstSpot;
				burnthread[CurThread]->embers.CarrySpot = burnthread[CurThread]->embers.FirstSpot;
				break;
			}
		}
	}
	SpotCount = 0;
}


Embers::spotdata* Burn::GetSpotData(double CurrentTimeStep)
{
	// will add CurrentTimeStep to CurSpot->TimeRem and return 0 if delay is longer than current TimeStep
	if (CurThread >= NumPerimThreads)
		return NULL;

	if (SpotCount >= NumSpots[CurThread])
	{
		SpotCount = 0;
		do
		{
			CurThread++;
			if (CurThread >= NumPerimThreads)
			{
				SpotCount = 0;

				return NULL;
			}
			burnthread[CurThread]->embers.CurSpot = burnthread[CurThread]->embers.FirstSpot;
		}
		while (SpotCount >= NumSpots[CurThread]);
	}
	else if (SpotCount > 0 && SpotCount < NumSpots[CurThread])
	{
		burnthread[CurThread]->embers.NextSpot = (Embers::spotdata *)
			burnthread[CurThread]->embers.CurSpot->next;
		burnthread[CurThread]->embers.CurSpot = burnthread[CurThread]->embers.NextSpot;
	}
	SpotCount++;
	// looks at timeremaining for spot fire and adds time to it for delay if less than 0.0
	// then stores it at the beginning of the linked list
	if (CurrentTimeStep > 0.0)
	{
		if( burnthread[CurThread]->embers.CurSpot->TimeRem <= 0.0 ) {
			if( burnthread[CurThread]->embers.CarrySpots == 0 )
				burnthread[CurThread]->embers.CarrySpot = burnthread[CurThread]->embers.FirstSpot;
			else
			{
				burnthread[CurThread]->embers.NextSpot = (Embers::spotdata *)
					burnthread[CurThread]->embers.CarrySpot->next;
				burnthread[CurThread]->embers.CarrySpot = burnthread[CurThread]->embers.NextSpot;
			}
			burnthread[CurThread]->embers.CurSpot->TimeRem += CurrentTimeStep;
			memcpy( burnthread[CurThread]->embers.CarrySpot,
				burnthread[CurThread]->embers.CurSpot, 3 * sizeof(double));
			if( burnthread[CurThread]->embers.CarrySpots >=
				NumSpots[CurThread])
				burnthread[CurThread]->embers.CarrySpots += 1;
			else
				burnthread[CurThread]->embers.CarrySpots++;

			return NULL;
		}
	}

	return burnthread[CurThread]->embers.CurSpot;
}

//============================================================================
void Burn::BurnSpotThreads()
{ //Burn::BurnSpotThreads
  long   TotalEmbers, dest, * amount, num, excess;
  long   i, j, k, m, range;
  double fract, interval, ipart;

  TotalEmbers = 0;
  for( i = 0; i < NumPerimThreads; i++ )  //For each thread
    TotalEmbers += burnthread[i]->embers.NumEmbers;

  if( TotalEmbers == 0 ) {
    TotalSpots = SpotFires = 0;
    return;
  }

  interval = ( (double) (TotalEmbers) / (double) NumPerimThreads );
  fract = modf( interval, &ipart );
  range = (long) interval;
  if( fract > 0.0 ) range++;

  //This section equalizes the numbers of embers in each thread for flight
  //calculation.
  amount = new long[NumPerimThreads];

  for( i = 0; i < NumPerimThreads; i++ ) {
    for( k = 0; k < NumPerimThreads; k++ ) amount[k] = -1;
    //Find where embers are short.
    if( burnthread[i]->embers.NumEmbers < range ) {
      num = range - burnthread[i]->embers.NumEmbers;
      dest = i;
      //Find the embers from among all burnthreads[].
      for( j = 0; j < NumPerimThreads; j++) {  //For each thread
        if( j == dest ) continue;
        excess = burnthread[j]->embers.NumEmbers - range;
        if( excess > 0 ) {
          if( excess >= num ) {
            amount[j] = num;
            break;
          }
          else {
            amount[j] = excess;
            num -= excess;
          }
        }
      }
      for( k = 0; k < NumPerimThreads; k++ ) {
        if( amount[k] > 0 ) {
          for( m = 0; m < amount[k]; m++ ) {
            emberdata e = burnthread[k]->embers.ExtractEmber(0);
            burnthread[dest]->embers.AddEmber( &e );
          }
        }
      }
    }
  }
  delete[] amount;

  //This section starts the spot threads and synchronizes their completion
  //before returning.
  SpotCount = 0;
  ResumeSpotThreads( NumPerimThreads );

  TotalSpots = SpotFires = 0;
  CurThread = -1;
  if( EnableSpotFireGrowth(GETVAL) ) {
    for( i = 0; i < NumPerimThreads; i++ ) {
      NumSpots[i] = burnthread[i]->embers.SpotFires;
      TotalSpots += NumSpots[i];
      if( CurThread == -1 && NumSpots[i] > 0 ) {
        CurThread = i;
        burnthread[CurThread]->embers.CurSpot =
                                      burnthread[CurThread]->embers.FirstSpot;
      }
      burnthread[i]->embers.CarrySpots = 0;
    }
    SpotFires = TotalSpots;
  }
  else {
    for( i = 0; i < NumPerimThreads; i++ ) {
      NumSpots[i] = burnthread[i]->embers.NumSpots;
      TotalSpots += NumSpots[i];
      if( CurThread == -1 && NumSpots[i] > 0 ) {
        CurThread = i;
        burnthread[CurThread]->embers.CurSpot =
                                      burnthread[CurThread]->embers.FirstSpot;
      }
      burnthread[i]->embers.CarrySpots = 0;
    }
    SpotFires = 0;
  }
} //Burn::BurnSpotThreads
