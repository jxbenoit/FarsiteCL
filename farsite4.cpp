/*============================================================================
  farsite4.cpp

  RFL version  
  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  See LICENSE.TXT file for license information.

  * 200906 - JWB
    Added TotalAreaBurned to TFarsiteInterface class. This provides a
    convenient way to get total area burned for each iteration in
    TFarsiteInterface::FarsiteSimulationLoop().
  ============================================================================
*/
#include<sys/stat.h>
#include<string.h>
#include"globals.h"
#include"farsite4.h"
#include"portablestrings.h"

#define SIMREQ_NULL              0
#define SIMREQ_RESUMESUSPEND     3
#define SIMREQ_RESET             4

char ElTime[12], CurTime[14];
PFrontData pfdata;  //Global current post frontal stuff
FILE* CurrentFile;  //Global file pointer, for landscape file

//============================================================================
TFarsiteInterface::TFarsiteInterface( const char* farsitedirectory )
{ //TFarsiteInterface::TFarsiteInterface
  NumSimThreads = 0;
  SimRequest = SIMREQ_NULL;
  SIM_SUSPENDED = false;
  SIMULATE_GO = false;
  FARSITE_GO = false;
  NextFireAfterInterrupt = 0;

  int i;

  //Initialize strings for filenames.
  for( i = 0; i < 5; i++ ) {
    memset( Inputs.WeatherFile[i], 0x0, sizeof Inputs.WeatherFile[i] );
    memset( Inputs.WindFile[i], 0x0, sizeof Inputs.WindFile[i] );
    strcpy( Inputs.MBWtr[i], "Load Weather" );
    strcpy( Inputs.MBWnd[i], "Load Winds" );
  }
  strcpy( Inputs.MBLand, "Load Landscape" );
  sprintf( ElTime, "%02d %02d%s%02d", 0, 0, ":", 0 );

  NumWData = 0;         //Initialize wind grid storage for xorpen
  WData = 0;
  gaat = 0;    //Group air attack pointer

  //Set rate of spread adjustments to 1.0.
  for( i = 0; i < 257; i++ ) RosRed[i] = 1.0;

  memset( FarsiteDirectory, 0x0, sizeof(FarsiteDirectory) );
  strcpy( FarsiteDirectory, farsitedirectory );

  for( i = 0; i < 10000; i++ ) {
    sprintf( IgFileName, "%signition%d.ign", FarsiteDirectory, i );
    if( ! IsWritable(IgFileName) ) break;
  }

  FILE *iout = fopen( IgFileName, "w" );
  fclose( iout );

  ResetNewFuels();

  //Set model params.
  SetActualTimeStep( 30.0 );
  SetVisibleTimeStep( 30.0 );
  SetSecondaryVisibleTimeStep( -1.0 );

  double metric_convert = 1.0; //If using English, metric_convert = 0.914634

  //Keep perimeter res bigger than distance res!
  SetPerimRes( 90.0 * metric_convert );
  SetDistRes( 60.0 * metric_convert );
} //TFarsiteInterface::TFarsiteInterface

//============================================================================
TFarsiteInterface::~TFarsiteInterface() {}

//============================================================================
bool TFarsiteInterface::SetInputsFromFile( char* settingsfilename )
{ //TFarsiteInterface::SetInputsFromFile
  CallLevel++;

  char Line[1000];
  char* name;
  char* value;

  FILE* IN = fopen( settingsfilename, "r" );
  if( ! IN ) {
    printf( "## Cannot open %s ##\n", settingsfilename );
    CallLevel--;
    return false;
  }

  Inputs.ResetData();
 
  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::SetInputsFromFile:1\n",
            CallLevel, "" );

  while( ! feof(IN) ) {
    memset( Line, 0x0, 1000 * sizeof(char) );
    fgets( Line, 999, IN );

    if( strlen(Line) == 0 || ! strncmp(Line, "\n", 1) ) continue;

    if( Line[0] == '#' ) continue;

    name = Line;
    value = strchr( Line, '=' );
    if( value != NULL ) {
      value[0]='\0';
      value++;
    }
    else continue;

    trim( name );
    trim( value );
    if( strlen(value) == 0 ) continue;

    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::SetInputsFromFile:2 "
              "Processing name=%s value=%s\n", CallLevel, "", name, value );

    ProcessNameValue( name, value );
  }

  fclose( IN );

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::SetInputsFromFile:3\n",
            CallLevel, "" );

  bool bValid = ValidateSettings();
  if( bValid ) {
    SetSimulationDuration( 0 );  //Reset Duration
    SetStartDate( GetJulianDays(StartMonth) + StartDay + StartHour );
    bValid = Execute_InitiateTerminate();
  }

  CallLevel--;

  return bValid;
} //TFarsiteInterface::SetInputsFromFile

/*============================================================================
  TFarsiteInterface::ProcessNameValue
  Process the name-value pair from the Settings file.
  SOME validation is done here -- mainly to optional variables.
*/
void TFarsiteInterface::ProcessNameValue( char* name, char* value )
{ //TFarsiteInterface::ProcessNameValue
  char buff[64];
  char numset[] = ".1234567890";
  int i;

  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::ProcessNameValue:1 Processing "
            "name=%s value=%s\n", CallLevel, "", name, value );

  if( strlen(name) == 0 || strlen(value) == 0 ) {
    CallLevel--;
    return;
  }

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::ProcessNameValue:2\n",
            CallLevel, "" );

  name = strlwr( name );  //Convert the name string to lowercase

  if( strcmp(name, "version") == 0 ) Inputs.version = atoi( value );
  else if( strcmp(name, "projectfilename") == 0 ) {
    strcpy( Inputs.ProjectFile, value );
    Inputs.TestProjectFileVersion();
  }
  else if( strcmp(name, "ignitionfile") == 0 )
    strcpy( Ignition.ifile, value );
  else if( strcmp(name, "ignitionfiletype") == 0 ) {
    if( strcmp(strlwr(value), "polygon") == 0 )
      Ignition.SelectFileInputCmdL( 1 );
    else if( strcmp(strlwr(value), "line") == 0 )
      Ignition.SelectFileInputCmdL( 2 );
    else if( strcmp(strlwr(value), "point") == 0 )
      Ignition.SelectFileInputCmdL( 3 );
    else
      printf( "## Unknown ignition file type - must be either polygon, line, "
              "or point! ##" );
  }
  else if( strcmp(name, "landscapefile") == 0 )
    Inputs.setLandscapeFile( value );
  else if( strcmp(name, "adjustmentfile") == 0 )
    Inputs.loadUpAdjustmentsFile( value );
  else if( strcmp(name, "fuelmoisturefile") == 0 )
    Inputs.loadUpMoistureFile( value );
  else if( strcmp(name, "coarsewoodyfile") == 0 ) {
    strcpy( Inputs.CoarseWoody, value );
    Inputs.loadUpCoarseWoodyFile( value );
  }
  else if( strcmp(name, "weatherfile0") == 0 )
    Inputs.setWeatherFile( 0, value );
  else if( strcmp(name, "weatherfile1") == 0 )
    Inputs.setWeatherFile( 1, value );
  else if( strcmp(name, "weatherfile2") == 0 )
    Inputs.setWeatherFile( 2, value );
  else if( strcmp(name, "weatherfile3") == 0 )
    Inputs.setWeatherFile( 3, value );
  else if( strcmp(name, "weatherfile4") == 0 )
    Inputs.setWeatherFile( 4, value );
  else if( strcmp(name, "windfile0") == 0 )
    Inputs.setWindFile( 0, value );
  else if( strcmp(name, "windfile1") == 0 )
    Inputs.setWindFile( 1, value );
  else if( strcmp(name, "windfile2") == 0 )
    Inputs.setWindFile( 2, value );
  else if( strcmp(name, "windfile3") == 0 )
    Inputs.setWindFile( 3, value );
  else if( strcmp(name, "windfile4") == 0 )
    Inputs.setWindFile( 4, value );
  else if( strcmp(name, "fuelmodelfile") == 0 )
    Inputs.loadUpFuelModelFile( value );
  else if( strcmp(name, "convertfile") == 0 )
    Inputs.loadUpConversionFile( value );
  else if( strcmp(name, "burnperiodfile") == 0 )
    Inputs.loadUpBurnPeriodFile( value );
  else if( strcmp(name, "grideastnorth") == 0 )
    Inputs.saveGridEastNorth( value );
  else if( strcmp(name, "gridrow") == 0 )
    Inputs.saveGridRow( value );
  else if( strcmp(name, "landscapethemefile") == 0 )
    Inputs.loadUpLandscapeTheme( value );
  else if( strcmp(name, "shapedeffile") == 0 )
    Inputs.loadUpShapeFile( value );
  else if( strcmp(name, "storedvector") == 0 )
    Inputs.loadUpStoredVector( value );
  else if( strcmp(name, "north") == 0 )
    Inputs.loadUpSetDirection( value, 'N' );
  else if( strcmp(name, "south") == 0 )
    Inputs.loadUpSetDirection( value, 'S' );
  else if( strcmp(name, "east") == 0 )
    Inputs.loadUpSetDirection( value, 'E' );
  else if( strcmp(name, "west") == 0 )
    Inputs.loadUpSetDirection( value, 'W' );
  else if( strcmp(name, "canopycheckvalues") == 0 )
    Inputs.loadUpCanopyCheck( value );
  else if( strcmp(name, "useconditioningperiod") == 0 ) {
    if( strcmp(strlwr(value), "true") == 0 ) UseConditioningPeriod( 1 );
    else UseConditioningPeriod( 0 );
  }
  else if( strcmp(name, "conditmonth") == 0 )
    ConditMonth = atoi( value );
  else if( strcmp(name, "conditday") == 0 )
    ConditDay = atoi( value );
  else if( strcmp(name, "startmonth") == 0 )
    StartMonth = atoi( value );
  else if( strcmp(name, "startday") == 0 )
    StartDay = atoi( value );
  else if( strcmp(name, "starthour") == 0 )
    StartHour = atoi( value );
  else if( strcmp(name, "startmin") == 0 )
    StartMin = atoi( value );
  else if( strcmp(name, "endmonth") == 0 )
    EndMonth = atoi( value );
  else if( strcmp(name, "endday") == 0 )
    EndDay = atoi( value );
  else if( strcmp(name, "endhour") == 0 )
    EndHour = atoi( value );
  else if( strcmp(name, "endmin") == 0 )
    EndMin = atoi( value );
  else if( strcmp(name, "rastmake") == 0 ) {
    bool bValue = false;
    if( strcmp(strlwr(value), "true") == 0 ) bValue = true;
    SetRastMake( bValue );
  }
  else if( strcmp(name, "vectmake") == 0 ) {
    bool bValue = false;
    if( strcmp(strlwr(value), "true") == 0 ) bValue = true;
    SetVectMake( bValue );
  }
  else if( strcmp(name, "shapemake") == 0 ) {
    bool bValue = false;
    if( strcmp(strlwr(value), "true") == 0 ) bValue = true;
    SetShapeMake( bValue );
  }
  else if( strcmp(name, "shapefile") == 0 )
    SetShapeFileChx( value, 1, 0, 1 );
  else if( strcmp(name, "rasterfilename") == 0 )
    SetRasterFileName( value );
  else if( strcmp(name, "vectorfilename") == 0 )
    SetVectorFileName( value );
  else if( strcmp(name, "rast_arrivaltime") == 0 ) {
    bool bValue = false;
    if( strcmp(strlwr(value), "true") == 0 ) bValue = true;
    SetFileOutputOptions( RAST_ARRIVALTIME, bValue );
  }
  else if( strcmp(name, "rast_fireintensity") == 0 ) {
    bool bValue = false;
    if( strcmp(strlwr(value), "true") == 0 ) bValue = true;
    SetFileOutputOptions( RAST_FIREINTENSITY, bValue );
  }
  else if( strcmp(name, "rast_spreadrate") == 0 ) {
    bool bValue = false;
    if( strcmp(strlwr(value), "true") == 0 ) bValue = true;
    SetFileOutputOptions( RAST_SPREADRATE, bValue );
  }
  else if( strcmp(name, "rast_flamelength") == 0 ) {
    bool bValue = false;
    if( strcmp(strlwr(value), "true") == 0 ) bValue = true;
    SetFileOutputOptions( RAST_FLAMELENGTH, bValue );
  }
  else if( strcmp(name, "rast_heatperarea") == 0 ) {
    bool bValue = false;
    if( strcmp(strlwr(value), "true") == 0 ) bValue = true;
    SetFileOutputOptions( RAST_HEATPERAREA, bValue );
  }
  else if( strcmp(name, "rast_crownfire") == 0 ) {
    bool bValue = false;
    if( strcmp(strlwr(value), "true") == 0 ) bValue = true;
    SetFileOutputOptions( RAST_CROWNFIRE, bValue );
  }
  else if( strcmp(name, "rast_firedirection") == 0 ) {
    bool bValue = false;
    if( strcmp(strlwr(value), "true") == 0 ) bValue = true;
    SetFileOutputOptions( RAST_FIREDIRECTION, bValue );
  }
  else if( strcmp(name, "rast_reactionintensity") == 0 ) {
    bool bValue = false;
    if( strcmp(strlwr(value), "true") == 0 ) bValue = true;
    SetFileOutputOptions( RAST_REACTIONINTENSITY, bValue );
  }
  else if( strcmp(name, "timestep") == 0 ) {
    //Timestep is only valid from 1min to 59min or 1.00 hrs to 6.00 hrs.
    memset( buff, 0x0, 64 );
    i = strspn( value, numset );
    strncpy( buff, value, i );
    if( strchr(value,'m') == NULL ) SetActualTimeStep( atof(buff) * 60 );
    else SetActualTimeStep( (atof(buff)) );
  }
  else if( strcmp(name, "visiblestep") == 0 ) {
    //Visible step only valid from 1hrs to 4.17 days.
    memset( buff, 0x0, 64 );
    i = strspn( value, numset );
    strncpy( buff, value, i );
    if( strchr(value,'m') == NULL ) {
      if( strchr(value,'h') == NULL ) {
        //Value is specified in days,
        SetVisibleTimeStep( atof(buff) * 60 * 24 );
      }
      else {
        //Value is specified in hrs.
        SetVisibleTimeStep( atof(buff) * 60 );
      }
    }
    else {
      //Value is specified in minutes.
      SetVisibleTimeStep( atof(buff) );
    }
  } //End parsing visiblestep
  else if( strcmp(name, "secondaryvisiblestep") == 0 ) {
    //Secondary visible step only valid from 1hrs to 4.17 days.
    memset( buff, 0x0, 64 );
    i = strspn( value, numset );
    strncpy( buff, value, i );
    if( strchr(value,'m') == NULL ) {
      if( strchr(value,'h') == NULL ) {
        //value is specified in days
        SetSecondaryVisibleTimeStep( atof(buff) * 60 * 24 );
      }
      else {
        //value is specified in hrs
        SetSecondaryVisibleTimeStep( atof(buff) * 60 );
      }
    }
    else {
      //value is specified in minutes
      SetSecondaryVisibleTimeStep( atof(buff) );
    }
  } //end parsing secondaryvisiblestep
  else if( strcmp(name, "perimeterresolution") == 0 )
    //perimeter resolution only valid from 1 to 500m default is 60m
    SetPerimRes( atof(value) );
  else if( strcmp(name, "distanceresolution") == 0 )
    //distance resolution only valid from 1 to 500m default is 60m
    SetDistRes( atof(value) );
  else if( strcmp(name, "units") == 0 ) {}
    //model units either english or metric 
  else if( strcmp(name, "enablecrownfire") == 0 ) {
    //only valid value for the following is either true or false
    if( strcmp(strlwr(value), "true") == 0 ) EnableCrowning( 1 );
    else EnableCrowning( 0 );
  }
  else if( strcmp(name, "linkcrowndensityandcover") == 0 ) {
    if( strcmp(strlwr(value), "true") == 0 ) LinkDensityWithCrownCover( 1 );
    else LinkDensityWithCrownCover( 0 );
  }
  else if( strcmp(name, "embersfromtorchingtrees") == 0 ) {
    //only valid value for the following is either true or false
    if( strcmp(strlwr(value), "true") == 0 ) EnableSpotting( 1 );
    else EnableSpotting( 0 );
  }
  //can't find this in original Farsite4 code...
  else if( strcmp(name, "embersfromsurfacefires") == 0 ) {}
  else if( strcmp(name, "enablespotfiregrowth") == 0 ) {
    if( strcmp(strlwr(value), "true") == 0 ) EnableSpotFireGrowth( 1 );
    else EnableSpotFireGrowth( 0 );
  }
  else if( strcmp(name, "ignitionfrequency") == 0 )
    //ignition frequency and ignition delay are only used
    //if enable spot fire growth is true
    //ignition frequency is from 0 to 100 percent
    PercentIgnition( atof(value) );
  else if( strcmp(name, "ignitiondelay") == 0 )
    //ignition delay 0 to 320 minutes
    IgnitionDelay( atof(value) );
  else if( strcmp(name,"nwnsbackingros") == 0 ) {
    bool bValue = false;
    if( strcmp(strlwr(value), "true") == 0 ) bValue = true;
    ConstantBackingSpreadRate( bValue );
  }
  else if( strcmp(name, "expansioncorrection") == 0 ) {
    if( strcmp(strlwr(value), "true") == 0 ) CheckExpansions( 1 );
    else CheckExpansions( 0 );
  }
  else if( strcmp(name, "distancechecking") == 0 ) {
    //Distance checking is either fire level =1 or sim level =0.
    if( strcmp(strlwr(value), "simlevel") == 0 )
      DistanceCheckMethod( 0 );
    else if( strcmp(strlwr(value), "firelevel") == 0 )
      DistanceCheckMethod( 1 );
    else
      printf( "## Invalid Distance Checking! "
              "Method must be either Simlevel or Firelevel. ##\n" );
  }
  else if( strcmp(name, "fireacceleration") == 0 ) {
    //fire accleration is either on or off
    bool bValue = false;
    if( strcmp(strlwr(value), "on") == 0 ) bValue = true;
    SetAccelerationON( bValue );
  }
  else if( strcmp(name, "fireacclerationloadfile") == 0 ) {}
  else if( strcmp(name, "fireacclerationsavefile") == 0 ) {}
  //fire fuel is either model or types
  else if( strcmp(name, "firefuel") == 0 ) {}
  //fire model value may be between 1-50
  else if( strcmp(name, "firemodelvalue") == 0 ) {}
  //fire fuel value may be one of grass
  //shrub, timber, slash, default
  else if( strcmp(name, "firefuelvalue") == 0 ) {}
  else if( strcmp(name, "accelerationpointvalueA") == 0 ) {}
  else if( strcmp(name, "accelerationpointvalueB") == 0 ) {}
  else if( strcmp(name, "accelerationlinevalueA") == 0 ) {}
  else if( strcmp(name, "accelerationlinevalueB") == 0 ) {}
  //acceleration transition value is between 10-500 meters
  else if( strcmp(name, "accelerationtranstion") == 0 ) {}
  else if( strcmp(name, "simulatepostfrontalcombustion") == 0 ) {
    if( strcmp(strlwr(value), "true") == 0 ) CheckPostFrontal( 1 );
    else CheckPostFrontal( 0 );
  }
  else if( strcmp(name, "fuelinputoption") == 0 ) {
    //fuel input options are 
    //  1 Never combine surface and cwd fuels
    //  2 Always combine surface and cwd fuels
    //  3 Use surface fuels when CWD absent (default)
    if( strcmp(strlwr(value), "1") == 0 ) WoodyCombineOptions( 1 );
    else if( strcmp(strlwr(value), "2") == 0 ) WoodyCombineOptions( 2 );
    else if( strcmp(strlwr(value), "3") == 0 ) WoodyCombineOptions( 3 );
    else if( strcmp(strlwr(value), "never") == 0 ) WoodyCombineOptions( 1 );
    else if( strcmp(strlwr(value), "always") == 0 ) WoodyCombineOptions( 2 );
    else if( strcmp(strlwr(value), "absent") == 0 ) WoodyCombineOptions( 3 );
    else
      printf( "## Invalid Fuel Option specified! "
              "Valid options are never, always, or absent. ##\n" );
  }
  else if( strcmp(name, "calculationprecision") == 0 ) {
    //calculation precision options are normal or high
    if( strcmp(strlwr(value), "high") == 0 ) burn.post.BurnupPrecision( 1 );
    else burn.post.BurnupPrecision( 0 );
  }
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //Suppression keywords added Aug30, 2010.
  else if( strcmp(name, "suppressionresourcesfilename") == 0 )
    InitSuppressionResources( value );
  else if( strcmp(name, "setgroundattack") == 0 ) {
    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::ProcessNameValue:2a %s\n",
              CallLevel, "", value );

    int    AttackType;
    long   ResourceNum;
    double *Coords = new double [200];
    long NumCoords = ProcessAttackString( value, GROUND, &ResourceNum,
                                          &AttackType, Coords );
    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::ProcessNameValue:2b "
              "ResourceNum=%ld AttackType=%d NumCoords=%ld\n",
              CallLevel, "", ResourceNum, AttackType, NumCoords );

    //Note: Always attack the first fire (in the function calls below).
    if( AttackType == DIRECT ) SetupDirectAttack( ResourceNum, 0, Coords );
    else if( AttackType == INDIRECT )
      SetupIndirectAttack( ResourceNum, Coords, NumCoords );


    delete [] Coords;

    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::ProcessNameValue:2c\n",
              CallLevel, "" );
  }
  else if( strcmp(name, "setairattack") == 0 ) {
    int    AttackType;
    long   ResourceNum;
    double *Coords = new double [200];
    ProcessAttackString( value, AIR, &ResourceNum, &AttackType, Coords );
    long CoverageLevel = 3;   //HARDCODED
    long Duration = 8;       //HARDCODED (8 seconds)
//    SetupAirAttack( ResourceNum, CoverageLevel, Duration, Coords );
    char GroupName[10];
    sprintf( GroupName, "Group%ld", ResourceNum );
    SetupGroupAirAttack( ResourceNum, CoverageLevel, Duration, Coords, -2,
                         GroupName );
    gaat = new GroupAirAttack();
    delete [] Coords;
  }
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::ProcessNameValue:3\n",
            CallLevel, "" );

  CallLevel--;
} //TFarsiteInterface::ProcessNameValue

/*============================================================================
  TFarsiteInterface::ValidateSettings
  Used in TFarsiteInterface::SetInputsFromFile() to do error-checking on
  input settings. If bad values are found warn the user. (JWB 200907)
  This is probably best kept separate from ProcessNameValue() above because
  we must check that ALL needed settings are set. E.g. if the user didn't set
  start or end times, we can capture that here.
  Returns true if settings are valid, false otherwise.
*/
bool TFarsiteInterface::ValidateSettings()
{ //TFarsiteInterface::ValidateSettings
  bool bValid = true;

  if( StartMonth < 1 || StartMonth > 12 ) {
    printf( "## StartMonth is out of range or not set: %ld ##\n", StartMonth );
    bValid = false;
  }
  if( StartDay < 1 || StartDay > 31 ) {
    printf( "## StartDay is out of range or not set: %ld ##\n", StartDay );
    bValid = false;
  }
  if( StartHour < 0 || StartHour > 2500 || StartHour % 100 != 0 ) {
    printf( "## StartHour is out of range or not set: %ld ##\n", StartHour );
    bValid = false;
  }
  if( StartMin < 0 || StartMin > 59 ) {
    printf( "## StartMin is out of range or not set: %ld ##\n", StartMin );
    bValid = false;
  }
  if( EndMonth < 1 || EndMonth > 12 ) {
    printf( "## EndMonth is out of range or not set: %ld ##\n", EndMonth );
    bValid = false;
  }
  if( EndDay < 1 || EndDay > 31 ) {
    printf( "## EndDay is out of range or not set: %ld ##\n", EndDay );
    bValid = false;
  }
  if( EndHour < 0 || EndHour > 2500 || EndHour % 100 != 0 ) {
    printf( "## EndHour is out of range or not set: %ld ##\n", EndHour );
    bValid = false;
  }
  if( EndMin < 0 || EndMin > 59 ) {
    printf( "## EndMin is out of range or not set: %ld ##\n", EndMin );
    bValid = false;
  }

  //Check Optional variables.
  if( ConditMonth < 1 || ConditMonth > 12 )
    printf( "## ConditMonth is out of range: %ld ##\n", ConditMonth );
  if( ConditDay < 1 || ConditDay > 31 )
    printf( "## ConditDay is out of range: %ld ##\n", ConditDay );

  return bValid;
} //TFarsiteInterface::ValidateSettings

/*============================================================================
  TFarsiteInterface::ProcessAttackString
*/
long TFarsiteInterface::ProcessAttackString( char *value, int ResourceType,
                                             long *ResourceNum,
                                             int *AttackType, double *Coords )
{ //TFarsiteInterface::ProcessAttackString
  CallLevel++;
  char *name = strtok( value, " \t\n\r" );
  name = strlwr( name );

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::ProcessAttackString:1 "
            "value=%s\n", CallLevel, "", value );

  char s[300];
  int i;
  if( ResourceType == GROUND ) {
    for( i = 0; i < NumCrews; i++ ) {
      strcpy( s, crew[i]->CrewName );
      strlwr( s );
      if( ! strcmp(name, s) ) {
        *ResourceNum = i;
        break;
      }
    }
  }
  else {
    for( i = 0; i < NumAircraft; i++ ) {
      strcpy( s, aircraft[i]->AirCraftName );
      strlwr( s );
      if( ! strcmp(name, s) ) {
        *ResourceNum = i;
        break;
      }
    }
  }

  char *attack = strtok( NULL, " \t\n\r" );
  strlwr( attack );
  if( ! strcmp(attack, "direct") ) *AttackType = DIRECT;
  else if( ! strcmp(attack, "indirect") ) *AttackType = INDIRECT;
  else printf( "## Bad Attack Type value given in string: %s ##\n", value );
  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::ProcessAttackString:2 "
            "AttackType=%d name=%s attack=%s\n",
            CallLevel, "", *AttackType, name, attack );

  //Get resource name (first arg in string).
  char *ps = strtok( NULL, " \t\n\r" );
  if( ! ps ) {
    printf( "## TFarsiteInterface::ProcessAttackString: Missing or invalid "
            "coordinates given for attack resource %s ##\n", name );

    CallLevel--;
    return 0;
  }

  Coords[0] = atof( ps );

  //Get coords.
  i = 1;
  do {
    ps = strtok( NULL, " \t\n\r" );
    if( ps ) {
      Coords[i] = atof( ps );
      i++;
    }
  } while ( ps );

  CallLevel--;
  return (long)( i / 2.0 );  //Return number of coords read.
} //TFarsiteInterface::ProcessAttackString

/*============================================================================
  TFarsiteInterface::Execute_InitiateTerminate
  Displays landscape window and enables/disables menu selections.
*/
bool TFarsiteInterface::Execute_InitiateTerminate()
{ //TFarsiteInterface::Execute_InitiateTerminate
  bool bValid = false;

  if( ! Inputs.LandID )
    printf( "## Load Landscape file (.lcp). Data input is incomplete. ##\n" );
  else if( ! Inputs.WtrID )
    printf( "## Load Weather file (.wtr). Data input is incomplete. ##\n" );
  else if( ! Inputs.WndID )
    printf( "## Load Wind file (.wnd). Data input is incomplete. ##\n" );
  else if( ! Inputs.FuelMoisID )
    printf( "## Load Initial Fuel Moistures (.fms). Data input is "
            "incomplete. ##\n" );
  else if( ! Inputs.AdjID )
    printf( "## Load Adjustment file (.adj). Data input is incomplete. ##"
            "\n" );
  else if( NeedCustFuelModels() && ! HaveCustomFuelModels() )
    printf( "## Load Custom Fuel Models (.fmd). Data input is incomplete. ##"
            "\n" );
  else if( NeedConvFuelModels() && ! HaveFuelConversions() )
    printf( "## Load Fuel Conversions (.cnv). Data input is incomplete ##"
            "\n" );
  else {
    a1 = 0;
    p1 = 0;
    p2 = 0;
    sarea = 0;
    firesizeh = 0;
    firesizes = 0;  // for area and perimeter calculations
    PrimaryVisEqual = 0;
    PrimaryVisCount = 0;
    SecondaryVisEqual = -1;
    SecondaryVisCount = 0;
    CurrentFire = 0;

    ResetVisPerimFile();  // reset visperim file to 0
    for( long i = 0; i < 5; i++ ) WStat[i].tws = -1.0;
    LastFMCalcTime = 0;
    //SYSTEMTIME tb;
    //GetSystemTime(&tb);
    //srand(tb.wMilliseconds);
    idum = -( (rand() % 20) + 1 );  // initialize random num gen
    bValid = true;
  }
  StepThrough = false;

  SaveSettingsToFile( "./OutputSettings.txt" );

  return bValid;
} //TFarsiteInterface::Execute_InitiateTerminate

//============================================================================
void TFarsiteInterface::FlatOpenProject()
{ //TFarsiteInterface::FlatOpenProject
  CallLevel++;

  if( Verbose > CallLevel )
     printf( "%*sfarsite4:TFarsiteInterface::FlatOpenProject:1 "
             "Setting Duration....\n", CallLevel, "" );

  SimulationDuration =
   ConvertActualTimeToSimtime( EndMonth, EndDay, EndHour, EndMin, false );

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FlatOpenProject:2 "
            "Executing 'Start/Restart Simulation'....\n", CallLevel, "" );

  Execute_StartRestart();

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FlatOpenProject:3 Exiting....\n",
            CallLevel, "" );

  CallLevel--;
} //TFarsiteInterface::FlatOpenProject

/*============================================================================
  TFarsiteInterface::Execute_StartRestart
  This function makes starts simulation process loop.
*/
void TFarsiteInterface::Execute_StartRestart()
{ //TFarsiteInterface::Execute_StartRestart
  CallLevel++;

  long i, RealFires = 0;
  for( i = 0; i < GetNewFires(); i++ ) {
    if( GetInout(i) > 0 && GetInout(i) < 3 ) {
      RealFires = 1;
      break;
    }
  }

  if( RealFires > 0 ) {
    if( SimulationDuration == 0.0 ) {
      StepThrough = false;
      printf( "## No Duration set for simulation ##\n" );
    }
    else if( GetActualTimeStep() == 0 ) {
      StepThrough = false;
      printf( "## Set Model | Parameters before Starting the Simulation!"
              " Time- and Space-resolutions not set ##\n" );
    }
    else {
      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface::Execute_StartRestart:1 "
                "Initializing simulation....\n", CallLevel, "" );

      LandscapeTheme* grid = GetLandscapeTheme();
      long i;
      for( i = 1; i <= grid->NumAllCats[3]; i++ ) {
        if( Verbose > CallLevel )
          printf( "%*sfarsite4:TFarsiteInterface::Execute_StartRestart:1a "
                  "grid->AllCats[3][%ld]=%ld (%d)\n", CallLevel, "",
                  i, grid->AllCats[3][i],
                  GetFuelConversion(grid->AllCats[3][i]) );
        if( GetFuelConversion(grid->AllCats[3][i]) > 0 &&
            GetFuelConversion(grid->AllCats[3][i]) < 257 ) {
          if( ! InitialFuelMoistureIsHere(GetFuelConversion(
                grid->AllCats[3][i])) ) {
            printf( "## Fuel Model %ld Has No Initial Fuel Moisture. "
                    "Update File %s before continuing. ##\n",
                    (long) GetFuelConversion(grid->AllCats[3][i]),
                    Inputs.MBMois );
            CallLevel--;
            return;
          }
        }
      }

      if( HaveGroundFuels() ) {
        if( GetTheme_Units(W_DATA) != 0 && Inputs.CwdID == false &&
            CheckPostFrontal(GETVAL) ) {
          printf( "## Post-frontal combustion enabled, but missing CWD "
                   "file. Add .CWD File to Project before continuing. ##" );
          CallLevel--;
          return;
        }
      }

      SaveIgnitions();
      SIMULATE_GO = true;
      FARSITE_GO = true;
      burn.SIMTIME = 0.0;    //Reset start of FARSITE iterations
      burn.CuumTimeIncrement = 0.0;
      maximum = 0;
      numspots = 0;
      ProcNum = 1;    //First process can begin
      //Set distance check method.
      burn.DistMethod = DistanceCheckMethod( GETVAL );
      AddDownTime( -1.0 );    //Set down time to 0.0;
      ConvertAbsoluteToRelativeBurnPeriod();
      NextFireAfterInterrupt = 0;
      smolder = flaming = 0.0;

      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface::Execute_StartRestart:2 "
                "Starting Farsite simulation loop ....\n", CallLevel, "" );

      //Start FARSITE process and check message loop between iterations.
      FarsiteSimulationLoop();

      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface::Execute_StartRestart:3 "
                "Exited Farsite simulation loop.\n", CallLevel, "" );
    }
  }
  else {
    StepThrough = false;
    printf( "## Input ignition(s) on Landscape! ##\n" );
  }

  CallLevel--;
} //TFarsiteInterface::Execute_StartRestart

//============================================================================
bool TFarsiteInterface::OpenLandscapeFile()
{ //TFarsiteInterface::OpenLandscapeFile
  if( Inputs.LandID == true ) {
    strcpy( Inputs.MBLand, "Load Landscape" );
    CloseLandFile();
    memset( Inputs.LandscapeFile, 0x0, sizeof(Inputs.LandscapeFile) );
    Inputs.LandID = false;
  }

  if( OpenLandFile() ) {
    if( TestForLCPVersion1() ) {
      if( ! ConvertLCPFileToVersion2() ) {
        CloseLandFile();
        return false;
      }
      else Inputs.InitializeLandscapeFile();
    }
    else if( TestForLCPVersion2() ) {
      if( ! ConvertLCPFile2ToVersion4() ) {
        CloseLandFile();
        return false;
      }
      else Inputs.InitializeLandscapeFile();
    }
    else Inputs.InitializeLandscapeFile();
  }
  else return false;

  return true;
} //TFarsiteInterface::OpenLandscapeFile

//============================================================================
void TFarsiteInterface::Execute_ResumeSuspend()
{ //TFarsiteInterface::Execute_ResumeSuspend
  if( SIMULATE_GO ) {
    if( FARSITE_GO && SIM_SUSPENDED == false ) {
      StepThrough = false;
      //FARSITE_GO=false;   // don't do it with mt version
      SIM_SUSPENDED = true;
    }
    else {
      if( GetSimulationDuration() == 0.0 ) {
        StepThrough = false;
        printf( "## Verify or Reset Duration - Weather/Wind streams changed "
                "##\n" );
      }
      else {
        SIM_SUSPENDED = false;
        SimRequest = SIMREQ_NULL;
        FARSITE_GO = true;
        ConvertAbsoluteToRelativeBurnPeriod();
        FarsiteSimulationLoop() ; //For single thread. [BLN]
      }
    }
  }
  else printf( "## START Simulation first - Can't Resume! ##\n" );
} //TFarsiteInterface::Execute_ResumeSuspend

/*============================================================================
  TFarsiteInterface::FarsiteSimulationLoop
  This function activates the FARSITE loop and checks for messages between
  iterations.
*/
void TFarsiteInterface::FarsiteSimulationLoop()
{ //TFarsiteInterface::FarsiteSimulationLoop
  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:1\n",
            CallLevel, "" );

  CheckSteps();    //Check visual and actual timestep for changes

  while( burn.SIMTIME <= maximum ) {
    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:2 "
              "Sim time = %f\n", CallLevel, "", burn.SIMTIME );

    if( SimRequest != SIMREQ_NULL ) break;

    CheckSteps();

    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:3 "
              "\n", CallLevel, "" );

    if( FARSITE_GO ) {
      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:3a "
                "\n", CallLevel, "" );

      PreCalculateFuelMoisturesNoUI();
      if( ! FARSITE_GO ) {  //Can happen if moistures are canceled
        FARSITE_GO = true;
        SimRequest = SIMREQ_RESUMESUSPEND;
        continue;
      }
      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:3b "
                "\n", CallLevel, "" );

      if( ProcNum < 3 ) {  //If all inputs ready for FARSITE model
        if( NextFireAfterInterrupt == 0 ) CountFires();
        if( Verbose > CallLevel )
          printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:3b1 "
                  "ProcNum=%d\n", CallLevel, "", ProcNum );
        if( ProcNum == 1 ) {
          if( FarsiteProcess1() ) {  //Do another iteration of FARSITE process
            if( Verbose > CallLevel )
              printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:"
                      "3b1a Total area burned = %f\n", CallLevel, "",
                      TotalAreaBurned );

            while( FarsiteProcess2() )
              { if( ! FarsiteProcessSpots() ) break; }
          }
          else break;
        }
        else if( ProcNum == 2 ) {
          if( FarsiteProcessSpots() ) {
            while( FarsiteProcess2() )
              { if( ! FarsiteProcessSpots() ) break; }
          }
        }
      }
      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:3c "
                "\n", CallLevel, "" );

      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:3d "
                "\n", CallLevel, "" );
      if( ProcNum == 3 ) {
        if( Verbose > CallLevel )
          printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:3d1 "
                  "\n", CallLevel, "" );
        FarsiteProcess3();  //Mergers between fires and increment iteration
        ProcNum = 1;
        if( ! OutputVectsAsGrown && NextFireAfterInterrupt == 0 ) {
          for( CurrentFire = 0; CurrentFire < GetNumFires(); CurrentFire++ ) {
            if( GetInout(CurrentFire) == 0 ) continue;
            //Output perimeters to screen and/or vector file.
            WriteVectorOutputs();
          }
        }
        if( Verbose > CallLevel )
          printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:3d2 "
                  "\n", CallLevel, "" );

        if( NextFireAfterInterrupt == 0 ) {
          if( burn.SIMTIME > 0.0 && PrimaryVisCount == PrimaryVisEqual ) {
            if( DistanceCheckMethod(GETVAL) == DISTCHECK_SIMLEVEL ) {
              if( burn.CuumTimeIncrement == 0.0 ) {
                if( CheckPostFrontal(GETVAL) ) {}
                //Write areas and perimeters to data structures and screen.
                WriteOutputs( -1 );
                PrimaryVisCount = 1;
              }
            }
            else {
              //Write areas and perimeters to data structures and screen.
              WriteOutputs( -1 );
              PrimaryVisCount = 1;
            }
          }
          else {
            if( DistanceCheckMethod(GETVAL) == DISTCHECK_SIMLEVEL ) {
              if( burn.CuumTimeIncrement == 0.0 ) {
                WriteOutputs( 1 );
                PrimaryVisCount++;
              }
            }
            else {
              WriteOutputs( 1 );
              PrimaryVisCount++;
            }
          }

          if( burn.SIMTIME > 0.0 ) {
            if( SecondaryVisCount >= SecondaryVisEqual )
              SecondaryVisCount = 1;
            else {
              if( DistanceCheckMethod(GETVAL) == DISTCHECK_SIMLEVEL ) {
                if (burn.CuumTimeIncrement == 0.0) SecondaryVisCount++;
              }
              else SecondaryVisCount++;
            }
          }
          else SecondaryVisCount = 1;
        }
        if( Verbose > CallLevel )
          printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:3d3 "
                  "\n", CallLevel, "" );
      }
    }
    else break;   //Check visual and actual timesteps for changes

    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:4 "
              "Sim time = %f maximum=%ld\n",
              CallLevel, "", burn.SIMTIME, maximum );

    if( burn.SIMTIME > maximum ) {
      FARSITE_GO = false;
      if( GetRastMake() ) WriteGISLogFile( 0 );
      if( GetVectMake() ) WriteGISLogFile( 1 );
      if( GetShapeMake() ) WriteGISLogFile( 2 );
      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:4a "
                "SIMULATION COMPLETED\n", CallLevel, "" );
      SimRequest = SIMREQ_NULL;
      SIM_SUSPENDED = true;
      StepThrough = false;

      CheckSteps();
    }

    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:5 "
              "Sim time = %f maximum=%ld\n",
              CallLevel, "", burn.SIMTIME, maximum );
  }  //End while loop

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:6 "
            "Exited main loop\n", CallLevel, "" );

  if( gaat ) delete gaat;

  if( SimRequest == SIMREQ_RESUMESUSPEND ) {
    SimRequest = SIMREQ_NULL;
    Execute_ResumeSuspend();
  }

  //Write raster files at end of timestep...
  if( GetRastMake() )
    burn.rast.SelectMemOutputs( GetRastFormat() );  // write to raster file

  if( burn.SIMTIME > maximum ) {
    FARSITE_GO = false;
    if( GetRastMake() ) WriteGISLogFile( 0 );
    if( GetVectMake() ) WriteGISLogFile( 1 );
    if( GetShapeMake() ) WriteGISLogFile( 2 );
    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::FarsiteSimulationLoop:7 "
              "SIMULATION COMPLETED\n", CallLevel, "" );
  }

  CallLevel--;
} //TFarsiteInterface::FarsiteSimulationLoop()

//============================================================================
void TFarsiteInterface::WriteGISLogFile( long LogType )
{
  char LogFile[256];
  memset(LogFile, 0x0, sizeof(LogFile));
  long len;

  if( LogType == 0 ) {
    len = strlen(GetRasterFileName(0));
    strncpy(LogFile, GetRasterFileName(0), len - 3);
    strcat(LogFile, "LGR");
  }
  else if( LogType == 1 ) {
    len = strlen(GetVectorFileName());
    strncpy(LogFile, GetVectorFileName(), len - 3);
    strcat(LogFile, "LGV");
  }
  else if( LogType == 2 ) {
    if( ! shape.GetShapeFileName() ) return;
    len = strlen( shape.GetShapeFileName() );
    strncpy( LogFile, shape.GetShapeFileName(), len - 3 );
    strcat( LogFile, "LGS" );
  }
  else return;

  FILE* logfile = fopen( LogFile, "w" );
  if( logfile == NULL ) {
    SetFileMode( LogFile, FILE_MODE_READ | FILE_MODE_WRITE );
    logfile = fopen( LogFile, "w" );
  }
  fprintf( logfile, "%s %s\n", "Log File: ", LogFile );

  if( LogType < 2 ) {
    if( AccessOutputUnits(GETVAL) == 0 )
      fprintf(logfile, "File units: Metric\n");
    else fprintf( logfile, "File units: English\n" );
  }
  fprintf(logfile, "%s %s\n", "Landscape File: ", Inputs.LandscapeFile);
  for( len = 0; len < GetNumStations(); len++ ) {
    fprintf( logfile, "%s %ld: %s\n", "Weather File",
             len + 1, Inputs.WeatherFile[len] );
    fprintf( logfile, "%s %ld: %s\n", "Wind File",
             len + 1, Inputs.WindFile[len] );
    if( len > 4 ) break;
  }
  fprintf( logfile, "%s %s\n", "Adjustment File: ", Inputs.AdjustmentFile );
  fprintf( logfile, "%s %s\n", "Fuel Moisture File: ",
           Inputs.FuelMoistureFile );
  if( HaveFuelConversions() )
    fprintf(logfile, "%s %s\n", "Conversion File: ",
            Inputs.ConvertFile);
  else fprintf(logfile, "%s %s\n", "Conversion File: ", "None");
		if (HaveCustomFuelModels())
			fprintf(logfile, "%s %s\n", "Custom Fuel Model File: ",
				Inputs.FuelModelFile);
		else
			fprintf(logfile, "%s %s\n", "Custom Fuel Model File: ", "None");
		if (EnableCrowning(GETVAL))
		{
			fprintf(logfile, "%s\n", "Crown Fire: Enabled");
			if (LinkDensityWithCrownCover(GETVAL))
				fprintf(logfile, "%s\n", "Crown Density LINKED to Crown Cover");
			else
				fprintf(logfile, "%s\n",
					"Crown Density NOT LINKED to Crown Cover");
		}
		else
			fprintf(logfile, "%s\n", "Crown Fire: Disabled");
		if (EnableSpotting(GETVAL))
		{
			fprintf(logfile, "%s\n", "Ember Generation: Enabled");
			if (EnableSpotFireGrowth(GETVAL))
				fprintf(logfile, "%s\n", "Spot Growth: Enabled");
			else
				fprintf(logfile, "%s\n", "Spot Growth: Disabled");
		}
		else
			fprintf(logfile, "%s\n", "Ember Generation: Disabled");
    if( ConstantBackingSpreadRate(GETVAL) )
      fprintf( logfile, "%s\n",
               "Backing Spread: Calculated from No Wind/No Slope" );
    else
      fprintf( logfile, "%s\n",
               "Backing Spread: Calculated from Elliptical Dimensions" );
		if (AccelerationON())
		{
			/*	if (strlen(TransAccelData.Dat.AccelLoad) > 0)
			{
				if (strlen(TransAccelData.Dat.AccelSave) > 0)
					fprintf(logfile, "%s %s\n", "Acceleration File Used: ",
						TransAccelData.Dat.AccelSave);
				else
					fprintf(logfile, "%s %s\n", "Acceleration File Used: ",
						TransAccelData.Dat.AccelLoad);
			}
			else
				fprintf(logfile, "%s %s\n", "Acceleration File Used: ",
					"Default Values");
						  */
		}
		else
			fprintf(logfile, "%s %s\n", "Acceleration File Used: ",
				"None (feature toggled off)");
		fprintf(logfile, "\n");
		fprintf(logfile, "%s %ld%s%ld %ld:00\n",
			"Simulation Started (Day Hour:Min):", GetStartMonth(), "/",
			GetStartDay(), GetStartHour() / 100);
		fprintf(logfile, "%s %s\n", "Simulation Ended (Day Hour:Min):",
			CurTime);
		fprintf(logfile, "%s %s\n\n", "Elapsed Time (Days Hours:Mins):",
			ElTime);
		fprintf(logfile, "%s %lf\n", "Actual Time Step (min):",
			GetActualTimeStep());
		fprintf(logfile, "%s %lf\n", "Visible Time Step (min):",
			GetVisibleTimeStep());
		fprintf(logfile, "%s %lf\n", "Perimeter Resolution (m):",
			GetPerimRes());
		fprintf(logfile, "%s %lf\n", "Distance Resolution (m):", GetDistRes());
		fclose(logfile);
		//free(LogFile);
}

/*============================================================================
  TFarsiteInterface::SaveIgnitions
  For saving ignitions before restarting.
*/
void TFarsiteInterface::SaveIgnitions()
{ //TFarsiteInterface::SaveIgnitions
  long i, j, k;
  double xpt, ypt, ros, fli;
  FILE* IgFile;

  if( (IgFile = fopen(IgFileName, "w")) == NULL ) {
    SetFileMode( IgFileName, FILE_MODE_READ | FILE_MODE_WRITE );
    IgFile = fopen(IgFileName, "w");
  }

  if( IgFile ) {
    fprintf( IgFile, "%ld\n", GetNewFires() );
    for( i = 0; i < GetNewFires(); i++ ) {
      k = GetNumPoints(i) + 1;
      if( k > 0 ) {
        fprintf( IgFile, "%ld %ld %ld\n", i, k, GetInout(i) );
        for( j = 0; j < k; j++ ) {
          xpt = GetPerimeter1Value( i, j, XCOORD );
          ypt = GetPerimeter1Value( i, j, YCOORD );
          ros = GetPerimeter1Value( i, j, ROSVAL );
          fli = GetPerimeter1Value( i, j, FLIVAL );
          fprintf( IgFile, "%lf %lf %lf %lf\n", xpt, ypt, ros, fli );
        }
      }
    }
    fclose( IgFile );
  }
  else
    printf( "## Check File Attributes and Change From 'READ ONLY'!"
            " Could Not Write File ##\n" );
} //TFarsiteInterface::SaveIgnitions

/*============================================================================
  TFarsiteInterface::FarsiteProcess1
  This function contains the Farsite process control.
*/
bool TFarsiteInterface::FarsiteProcess1()
{ //TFarsiteInterface::FarsiteProcess1
  long InOut, FireNum;
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface:FarsiteProcess1:1 "
            "burn.CuumTimeIncrement=%lf\n",
            CallLevel, "", burn.CuumTimeIncrement );

  IN_BURNPERIOD = true;

  //Check if simlevel timestep and/or burnout enabled.
  if( DistanceCheckMethod(GETVAL) == DISTCHECK_SIMLEVEL ) {
    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface:FarsiteProcess1:1a\n",
              CallLevel, "" );

    if( NextFireAfterInterrupt == 0 ) burn.DetermineSimLevelTimeStep();
    if( ! InquireInBurnPeriod(burn.SIMTIME) ) {
      if( burn.prod.cuumslope[1] > 1e-9 )
        sarea = ( a1 - firesizeh ) /
                ( cos(atan((PI * burn.prod.cuumslope[0] / 180.0) /
                  burn.prod.cuumslope[1])) );
      else sarea = 0.0;
      sarea = firesizes + sarea;
      firesizeh = a1;
      firesizes = sarea;
      AddDownTime( GetTemporaryTimeStep() );
      burn.CuumTimeIncrement += GetTemporaryTimeStep();
      IN_BURNPERIOD = false;

      CallLevel--;
      return true;
    }
    else if( NextFireAfterInterrupt == 0 && burn.CuumTimeIncrement == 0.0 ) {
      a1 = 0.0;
      p1 = 0.0;
      p2 = 0.0;
    }

    for( CurrentFire = NextFireAfterInterrupt; CurrentFire < GetNumFires();
         CurrentFire++ ) {   //For all fires

      if( (InOut = GetInout(CurrentFire)) == 0 ) continue;

      burn.BurnIt( CurrentFire );
      if( InOut < 3 && burn.CuumTimeIncrement == 0.0 ) {
        a1 += burn.prod.area;     // +area of all fires (&& -inward fire area)
        p1 += burn.prod.perimeter;    // +perimeter of all fires)
        p2 += burn.prod.sperimeter;   // +slope perimeter of all fires
      }
    }

    if( ! FARSITE_GO ) {
      CallLevel--;
      return false;
    }

    NextFireAfterInterrupt = 0;
    AddDownTime( -1.0 );
  }
  else {
    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface:FarsiteProcess1:1b\n",
              CallLevel, "" );

    if( ! InquireInBurnPeriod(burn.SIMTIME) ) {
      if( burn.prod.cuumslope[1] > 1e-9 )
        sarea = ( a1 - firesizeh ) /
                 ( cos(atan((PI * burn.prod.cuumslope[0] / 180.0) /
                                             burn.prod.cuumslope[1])) );
      else sarea = 0.0;

      sarea = firesizes + sarea;
      firesizeh = a1;
      firesizes = sarea;
      AddDownTime( GetActualTimeStep() );
      for( CurrentFire = NextFireAfterInterrupt; CurrentFire < GetNumFires();
           CurrentFire++ ) {  // for all fires
        //Output perimeters to screen and/or vector file.
        if( OutputVectsAsGrown ) WriteVectorOutputs();
      }
      IN_BURNPERIOD = false;

      CallLevel--;
      return true;
    }
    else if( NextFireAfterInterrupt == 0 ) {
      a1 = 0.0;
      p1 = 0.0;
      p2 = 0.0;
    }

    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface:FarsiteProcess1:1c\n",
              CallLevel, "" );

    TotalAreaBurned = 0; //Init
    for( CurrentFire = NextFireAfterInterrupt; CurrentFire < GetNumFires();
         CurrentFire++ ) {    //For all fires
      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface:FarsiteProcess1:1c1\n",
                CallLevel, "" );

      if( (InOut = GetInout(CurrentFire)) == 0 ) continue;

      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface:FarsiteProcess1:1c2\n",
                CallLevel, "" );

      if( OutputVectsAsGrown )
        WriteVectorOutputs(); //Output perimeters to screen and/or vector file

      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface:FarsiteProcess1:1c3 "
                "SIMTIME=%lf\n", CallLevel, "", burn.SIMTIME );

      burn.BurnIt( CurrentFire );   //Burn the fire
      TotalAreaBurned += burn.prod.area;

      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface:FarsiteProcess1:1c4\n",
                CallLevel, "" );

      if( InOut < 3 ) {
        a1 += burn.prod.area;     // +area of all fires (&& -inward fire area)
        p1 += burn.prod.perimeter;   // +perimeter of all fires)
        p2 += burn.prod.sperimeter;  // +slope perimeter of all fires
      }
    }

    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface:FarsiteProcess1:1d\n",
              CallLevel, "" );

    if( ! FARSITE_GO ) {
      CallLevel--;
      return false;
    }

    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface:FarsiteProcess1:1e\n",
              CallLevel, "" );

    NextFireAfterInterrupt = 0;
    AddDownTime( -1.0 );
  }
  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface:FarsiteProcess1:2\n",
            CallLevel, "" );

  //Condense array of fire perimeter pointers.
  while( GetNumFires() < GetNewFires() ) {
    CurrentFire = GetNumFires();
    if( GetInout(CurrentFire) != 0 ) {
      FireNum = 0;
      while( GetInout(FireNum) > 0 ) FireNum++;
      if( FireNum < CurrentFire ) {
        FireNum = CurrentFire;
        burn.RePositionFire( &CurrentFire );
        if( CheckPostFrontal(GETVAL) )
          SetNewFireNumber( FireNum, CurrentFire,
                            burn.post.AccessReferenceRingNum(1, GETVAL) );
      }
    }
    IncNumFires( 1 );   //Increment number of active fires
  }
  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface:FarsiteProcess1:3\n",
            CallLevel, "" );

  IncNumFires( -GetSkipFires() );    //Subtract merged or extinguished fires
  SetNewFires( GetNumFires() );
  SetSkipFires( 0 );  //Reset skipfire to 0

  if( burn.prod.cuumslope[1] > 1e-9 )
    sarea = ( a1 - firesizeh ) /
            ( cos(atan((PI * burn.prod.cuumslope[0] / 180.0) /
              burn.prod.cuumslope[1])) );
  else sarea = 0.0;
  sarea = firesizes + sarea;
  firesizeh = a1;
  firesizes = sarea;

  CallLevel--;
  return true;
} //TFarsiteInterface::FarsiteProcess1

/*============================================================================
  TFarsiteInterface::FarsiteProcessSpots
  This function contains the Farsite process control.
*/
bool TFarsiteInterface::FarsiteProcessSpots()
{ //TFarsiteInterface::FarsiteProcessSpots
  CallLevel++;
  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcessSpots:1 "
            "burn.SIMTIME=%lf\n",
            CallLevel, "", burn.SIMTIME );

  if( ! IN_BURNPERIOD ) {
    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcessSpots:1a "
              "burn.SIMTIME=%lf\n",
              CallLevel, "", burn.SIMTIME );
    CallLevel--;

    return true;
  }

  double Actual;
  double simtime = burn.SIMTIME;
  double cuumtime = burn.CuumTimeIncrement;
  double TempStep = 0.0;
  bool SpotActivityChanged = false;
  long InOut, TempTime, CurrentSpot;
  Embers::spotdata* curspot;

  if( EnableSpotting(GETVAL) ) {
    EnableSpotting( false );
    SpotActivityChanged = true;
  }

  burn.SetSpotLocation( 0 );
  Actual = GetActualTimeStep();
  if( DistanceCheckMethod(GETVAL) == DISTCHECK_SIMLEVEL )
    TempStep = GetTemporaryTimeStep();

  //For all fires.
  for( CurrentFire = 0; CurrentFire < burn.TotalSpots; CurrentFire++) { 
    curspot = burn.GetSpotData( Actual );
    if( curspot == NULL ) continue;
    if( curspot->TimeRem < 0.10 ) curspot->TimeRem = 0.10;

    CurrentSpot = InsertSpotFire( curspot->x, curspot->y );
    if( CurrentSpot < 0 ) continue;
    if( (InOut = GetInout(CurrentSpot)) == 0 ) continue;
    //Truncate to nearest 0.1 minutes for easier precision in Burn::.
    curspot->TimeRem *= 10.0;
    TempTime = ( long ) curspot->TimeRem;
    curspot->TimeRem = ( double ) TempTime / 10.0;

    burn.SIMTIME += ( Actual - curspot->TimeRem );
    if( burn.SIMTIME < 0.0 ) burn.SIMTIME = 0.0;

    if( DistanceCheckMethod(GETVAL) == DISTCHECK_FIRELEVEL ) {
      burn.CuumTimeIncrement = 0.0;
      SetActualTimeStep( curspot->TimeRem );
    }
    else {
      burn.CuumTimeIncrement = Actual - curspot->TimeRem;
      SetTemporaryTimeStep( curspot->TimeRem );
    }

    if( ! InquireInBurnPeriod(burn.SIMTIME) ) {
      burn.SIMTIME = simtime;
      continue;
    }

    burn.BurnIt( CurrentSpot );   //Burn the fire
    if( InOut < 3 ) {
      a1 += burn.prod.area;        //+area of all fires (&& -inward fire area)
      p1 += burn.prod.perimeter;   //+perimeter of all fires)
      p2 += burn.prod.sperimeter;  //+slope perimeter of all fires
    }
    burn.SIMTIME = simtime;
    burn.CuumTimeIncrement = cuumtime;
  }
  NextFireAfterInterrupt = 0;
  long FireNum;
  //Condense array of fire perimeter pointers.
  while( GetNumFires() < GetNewFires() ) { 
    CurrentFire = GetNumFires();
    if( GetInout(CurrentFire) != 0 ) {
      FireNum = 0;
      while( GetInout(FireNum) > 0 ) FireNum++;
      if( FireNum < CurrentFire ) {
        FireNum = CurrentFire;
        burn.RePositionFire( &CurrentFire );
        if( CheckPostFrontal(GETVAL) )
          SetNewFireNumber( FireNum, CurrentFire,
                            burn.post.AccessReferenceRingNum(1, GETVAL) );
        }
    }
    IncNumFires( 1 );       //Increment number of active fires
  }
  IncNumFires( -GetSkipFires() );  //Subtract merged or extinguished fires
  SetNewFires( GetNumFires() );
  SetSkipFires( 0 );  //Reset skipfire to 0
  if( burn.prod.cuumslope[1] > 1e-9 )
    sarea = ( a1 - firesizeh ) /
             ( cos(atan((PI * burn.prod.cuumslope[0] / 180.0) /
               burn.prod.cuumslope[1])) );
  else sarea = 0.0;
  sarea = firesizes + sarea;
  firesizeh = a1;
  firesizes = sarea;
  if( DistanceCheckMethod(GETVAL) == DISTCHECK_FIRELEVEL )
    SetActualTimeStep( Actual );
  else SetTemporaryTimeStep( TempStep );
  if( SpotActivityChanged ) EnableSpotting( true );
  burn.SetSpotLocation( -1 );     //Elimate all spots

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcessSpots:2 "
            "burn.SIMTIME=%lf\n",
            CallLevel, "", burn.SIMTIME );
  CallLevel--;

  return true;
} //TFarsiteInterface::FarsiteProcessSpots

long TFarsiteInterface::InsertSpotFire(double xpt, double ypt)
{
	long k, newspots;
	long NumVertex;
	double Sxpt, Sypt, NewAngle;
	double Xlo, Xhi, Ylo, Yhi;
	double OutsideX, OutsideY;
	double AngleIncrement, AngleOffset;

	NumVertex = 6;
	GetNumSpots(&newspots, true);	// reserve an address

	Xlo = Xhi = xpt;
	Ylo = Yhi = ypt;									// initialize bounding rectangle
	AngleOffset = ((double) (rand2(&idum) * 1000)) / 999.0 * PI / 2.0;	// randomize orientation of fire polygon
	AngleIncrement = PI / ((double) NumVertex / 2.0);				// to avoid coincident sides
	OutsideX = -1.0;	// no outside points
	OutsideY = -1.0;
	AllocPerimeter1(newspots, NumVertex + 1);
	for (k = 0; k < NumVertex; k++)
	{
		NewAngle = ((double) k) * AngleIncrement + AngleOffset;
		Sxpt = xpt + cos(NewAngle);   	//Stime is the time step for growing spot fires
		Sypt = ypt + sin(NewAngle);
		SetPerimeter1(newspots, k, Sxpt, Sypt); 	 // save head fire ros at time step
		SetFireChx(newspots, k, 0.0, 0.0);
		SetReact(newspots, k, 0.0);
		if (Sxpt < Xlo)
			Xlo = Sxpt;
		else if (Sxpt > Xhi)
			Xhi = Sxpt;
		if (Sypt < Ylo)
			Ylo = Sypt;
		else if (Sypt > Yhi)
			Yhi = Sypt;
	}
	SetPerimeter1(newspots, k, Xlo, Xhi);    // save bounding rectangle
	SetFireChx(newspots, k, Ylo, Yhi);
	for (k = 0; k < NumVertex; k++) 	// check to see if
	{
		Sxpt = GetPerimeter1Value(newspots, k, 0);    // points outside
		Sypt = GetPerimeter1Value(newspots, k, 1);
		if (Sxpt <= GetLoEast())
			OutsideX = Xhi;
		else if (Sxpt >= GetHiEast())
			OutsideX = Xlo;
		if (Sypt >= GetHiNorth())
			OutsideY = Ylo;
		else if (Sypt <= GetLoNorth())
			OutsideX = Yhi;
	}
	if (OutsideX != -1.0 || OutsideY != -1.0)
	{
		FreePerimeter1(newspots);
		newspots = -1;
	}
	else
	{
		SetInout(newspots, 1);
		SetNumPoints(newspots, NumVertex);
	}

	return newspots;
}

//============================================================================
bool TFarsiteInterface::FarsiteProcess2()
{ //TFarsiteInterface::FarsiteProcess2
  if( ! IN_BURNPERIOD ) {
    ProcNum = 3;

    return false;
  }

  if( DistanceCheckMethod(GETVAL) == DISTCHECK_SIMLEVEL ) {
    if( ProcNum == 1 )  //Only first time through
      burn.CuumTimeIncrement += GetTemporaryTimeStep();
    if( burn.CuumTimeIncrement < GetActualTimeStep() ) {
      ProcNum = 3;

      return false;
    }
    else burn.CuumTimeIncrement = GetActualTimeStep();
  }

  long OldNumSpots, Numspots;

  OldNumSpots = numspots;   //Save old number of spots/embers

  //Ember flight and ignition, after all fires have burned this step.
  burn.BurnSpotThreads();

  numspots += OldNumSpots;   //Sum OldNumSpots and new numspots

  GetNumSpots( &Numspots, false );
  SetNumFires( Numspots );
  if( burn.SpotFires ) {
    NextFireAfterInterrupt = GetNumFires() - ( burn.SpotFires );
    burn.SpotFires = 0;
    ProcNum = 2;

    return true;
  }
  ProcNum = 3;
  burn.SetSpotLocation( -2 );

  return false;
} //TFarsiteInterface::FarsiteProcess2

/*============================================================================
  TFarsiteInterface::FarsiteProcess3
  This function processes fire mergers.
*/
void TFarsiteInterface::FarsiteProcess3()
{ //TFarsiteInterface::FarsiteProcess3
  long i, j, NewFires, NewPts;
  double mx[10] =
               { 90.0, 90.0, 90.0, 90.0, 90.0, 90.0, 90.0, 90.0, 90.0, 90.0 };
  AttackData*  atk;
  AirAttackData* aatk;
  FireRing*  firering;

  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcess3:1 "
            "Processing fire mergers....burn.SIMTIME=%lf\n",
            CallLevel, "", burn.SIMTIME );

  // perform indirect attack after all fires have burned
  for( i = 0; i < GetNumAttacks(); i++) {
    if( (atk = GetAttackByOrder(i, true)) != 0 ) {
      NewFires = GetNewFires();		// save value before attack
      if( atk->FireNumber == -1 ) NewFires++;
      if( atk->Indirect == 2 ) Atk.ConductBurnout( atk );
      else if( ! Atk.IndirectAttack(atk, GetActualTimeStep()) ) {
        CancelAttack( atk->AttackNumber );
        i--;  //Need to decrement counter because numattacks has changed
      }       //and order in GetAttackByOrder() has also changed

      if( NewFires < GetNewFires() ) {
        burn.ReorderPerimeter( NewFires,
                               burn.FindExternalPoint(NewFires, 0) );
        burn.FindOuterFirePerimeter( NewFires );
        NewPts = GetNumPoints( NewFires );
        FreePerimeter1( NewFires );
        if( NewPts > 0 ) {
          AllocPerimeter1( NewFires, NewPts + 1 );
          burn.tranz( NewFires, NewPts );
          Ignition.BoundingBox( NewFires );
          if( CheckPostFrontal(GETVAL) ) {
            firering = burn.post.SetupFireRing( NewFires,
                                                burn.SIMTIME +
                                                burn.CuumTimeIncrement,
                                                burn.SIMTIME +
                                                burn.CuumTimeIncrement );
            for( j = 0; j < NewPts; j++ )
              AddToCurrentFireRing( firering, j, 0, 0, 0, mx, 0.0 );
          }
        }
        else {
          SetNumPoints( NewFires, 0 );
          SetInout( NewFires, 0 );
          IncSkipFires( 1 );
        }
      }
      SetNumFires( GetNewFires() );
    }
  }

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcess3:2\n",
            CallLevel, "" );

  //Check air attack effectiveness.
  for( i = 0; i < GetNumAirAttacks(); i++ ) {
    aatk = GetAirAttackByOrder( i );
    if( ! AAtk.CheckEffectiveness(aatk, GetActualTimeStep()) ) {
      --aatk->PatternNumber;
      FreePerimeter1( aatk->PatternNumber );
      SetNumPoints( aatk->PatternNumber, 0 );
      SetInout( aatk->PatternNumber, 0 );
      IncSkipFires( 1 );
      CancelAirAttack( aatk->AirAttackNumber );
      i--;   //Decrement i becuase airattack number has changed.
    }
  }
  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcess3:3\n",
            CallLevel, "" );

  //If object for handling group air attacks exists
  if( gaat ) {
    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcess3:3a "
              "DistanceCheckMethod=%ld\n",
              CallLevel, "", DistanceCheckMethod(GETVAL) );

    if( DistanceCheckMethod(GETVAL) == DISTCHECK_SIMLEVEL )
      gaat->ExecuteAllIndirectAttacks( GetTemporaryTimeStep() );
    else gaat->ExecuteAllIndirectAttacks( GetActualTimeStep() );
  }
  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcess3:4\n",
            CallLevel, "" );

  if( IN_BURNPERIOD == true ) {
    if( GetNumFires() > 1 ) {
      CurrentFire = 0;
      //Check for crosses between fires outward fires.
      burn.CrossFires( 1, &CurrentFire );
      SetNumFires( GetNewFires() );
    }
    CheckStopLocations();
  }

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcess3:5\n",
            CallLevel, "" );

  if( CheckPostFrontal(GETVAL) ) {  //Reset to flag first num
    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcess3:5a\n",
              CallLevel, "" );

    CondenseRings( burn.post.AccessReferenceRingNum(1, GETVAL) );

    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcess3:5b\n",
              CallLevel, "" );

    burn.post.bup.BurnFireRings( burn.post.AccessReferenceRingNum(1, GETVAL),
                                 GetNumRings() );

    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcess3:5c\n",
              CallLevel, "" );

    burn.post.ComputePostFrontal( burn.SIMTIME + burn.CuumTimeIncrement,
                                  &smolder, &flaming );

    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcess3:5d\n",
              CallLevel, "" );

    //Could set to -1 for automatic.
    burn.post.AccessReferenceRingNum( 1, GetNumRings() );
  }
  burn.ResetIntersectionArrays();  //Free allocations for intersections

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcess3:6\n",
            CallLevel, "" );

  if( StepThrough ) {
    if( PrimaryVisCount == PrimaryVisEqual ) {
      if( DistanceCheckMethod(GETVAL) == DISTCHECK_SIMLEVEL ) {
        if( burn.CuumTimeIncrement == GetActualTimeStep() ) {
          SimRequest = SIMREQ_RESUMESUSPEND;
          StepThrough = false;
        }
      }
      else {
        SimRequest = SIMREQ_RESUMESUSPEND;
        StepThrough = false;
      }
    }
  }
  if( DistanceCheckMethod(GETVAL) == DISTCHECK_FIRELEVEL ) {
    burn.SIMTIME += GetActualTimeStep();  //Update simulation time
    burn.CuumTimeIncrement = 0.0;
  }
  else {
    if( GetActualTimeStep() - burn.CuumTimeIncrement <= 0.0 ) {
      //End of timestep.
      burn.SIMTIME += GetActualTimeStep();  //Update simulation time
      burn.CuumTimeIncrement = 0.0;

      pfdata.SetData( burn.SIMTIME, flaming / GetActualTimeStep() * 60.0,
                      smolder / GetActualTimeStep() * 60.0 );
      smolder = 0.0;
      flaming = 0.0;
    }
  }

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface::FarsiteProcess3:7 "
            "burn.SIMTIME=%lf\n",
            CallLevel, "", burn.SIMTIME );

  CallLevel--;
} //TFarsiteInterface::FarsiteProcess3


void TFarsiteInterface::CheckStopLocations()
{
	bool found = false;
	long i, j;
	double xpt, ypt;

	for (j = 0; j < GetNumStopLocations(); j++)
	{
		if (!GetStopLocation(j, &xpt, &ypt))
			continue;
		for (i = 0; i < GetNumFires(); i++)
		{
			if (GetInout(i) != 1)
				continue;
			burn.startx = xpt;
			burn.starty = ypt;
			if (burn.Overlap(i))
			{
				EnableStopLocation(j, false);
				if( ! found ) {
          //Fire encountered Stop Location.
          found = true;
          SimRequest = SIMREQ_RESUMESUSPEND;
				}
			}
		}
	}
}

//============================================================================
void TFarsiteInterface::CountFires()
{ //TFarsiteInterface::CountFires
  VectorBarriers = 0;
  CountInwardFires = 0;
  CountTotalFires = GetNewFires();//GetNumFires();
  for( long i = 0; i < CountTotalFires; i++ ) {
    if( GetInout(i) == 2 ) CountInwardFires++;
    if( GetInout(i) == 3 ) VectorBarriers++;
  }
  CountTotalFires -= VectorBarriers;
} //TFarsiteInterface::CountFires

//============================================================================
void TFarsiteInterface::WriteOutputs( int type )
{ //TFarsiteInterface::WriteOutputs
  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface:WriteOutputs:1\n", CallLevel, "" );

  // if area and perimeter calculations are desired
  //	if (WStation.Number > 0)
  //		CurrentWeather();
  WriteClocks();						// update the clocks if they're visible
  if( GetNumStations() > 1 || AtmosphereGridExists() > 0 ) {}
  else {
			EnvironmentData env;

			burn.fe->GetLandscapeData(GetLoEast() + GetNumEast() / 2,
						GetLoNorth() + GetNumNorth() / 2);
			burn.fe->ld.elev = (short)
				(GetLoElev() + (GetHiElev() - GetLoElev()) / 2);

			burn.fe->ld.slope = 0;
			burn.fe->ld.aspect = 0;
			burn.fe->ld.fuel = -1;//(short) GetFuelConversion(hd.fuels[1]);   // make sure that there is fuel there
			burn.fe->ld.cover = 0;   // make sure that there is fuel there
			burn.fe->GetFireEnvironment(burn.env, burn.SIMTIME, false);
			burn.fe->GetEnvironmentData(&env);
			//WindGauge->tws = env.tws;
			//WindGauge->winddir = env.winddir;
			//	InvalidateRect(WindGauge->HWindow, NULL, true);
  }
     /*
	adata.SetData(a1 * type, sarea * type);    	// type determines the color of the output
	pdata.SetData(p1 * type, p2 * type);			// if visible timestep, then red else blue
	fdata.count = CountInwardFires;
	fdata.count = 0;
	for (int i = 0;
		i < GetNumFires();
		i++)  	// counting is now done in ::CountFires
	{
		if (GetInout(i) == 2)
			fdata.count++;
	}
	fdata.SetData(GetNumFires() * type, numspots * type, fdata.count * type);
     */
	//if (burn.SIMTIME > 0)
	//	fdata.SetData((CountTotalFires + numspots) * type, numspots * type,
	//			fdata.count * type);
	//	else
	//		fdata.SetData(CountTotalFires*type, 0, fdata.count*type);
	//	if(p2>pdata.HIY)						// keep record of largest Y-value
	//		pdata.HIY=p2;
	//if(InquireInBurnPeriod)
	//{	a1=0.0;
	//	p1=0.0;
	//	p2=0.0;					// reset area and perimeter accumulators
	//}
	numspots = 0;

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface:WriteOutputs:2\n", CallLevel, "" );

  CallLevel--;
} //TFarsiteInterface::WriteOutputs



void TFarsiteInterface::StartMoistThread()
{
  TerminateMoist = false;

  RunMoistThread(this);
}


unsigned  TFarsiteInterface::RunMoistThread(void* FarIntFace)
{
  static_cast <TFarsiteInterface*>(FarIntFace)->MoistThread();

  return 1;
}


void TFarsiteInterface::MoistThread()
{
	double Interval;

	//Use the shortest interval, ten hour at present, longer ones will also be taken care of
	MxRecalculated = false;
	MoistSimTime = burn.SIMTIME + GetActualTimeStep();
	Interval = GetMoistCalcInterval(SIZECLASS_10HR, FM_INTERVAL_TIME);
	do
	{
		MoistSimTime += Interval;
		MxRecalculated = burn.env->CalcMapFuelMoistures(MoistSimTime);
		if (TerminateMoist)
			break;
	}
	while (MoistSimTime < maximum);
	TerminateMoist = true;
}


bool TFarsiteInterface::PreCalculateFuelMoistures()
{
	char Line[64] = "";
	long mo, dy, hr, mn;
	double SimTime;

  //Check if simlevel timestep and/or burnout enabled.
  if( DistanceCheckMethod(GETVAL) == DISTCHECK_SIMLEVEL ) {
		GetTemporaryTimeStep();
	}  // else if fire-level timestep
	else if( burn.SIMTIME == 0.0 )
		SimTime = burn.SIMTIME + GetActualTimeStep();
	// use the shortest interval, ten hour at present, longer ones will also be taken care of
	if( EnvironmentChanged(GETVAL, 0, SIZECLASS_10HR) )  // if weather etc. changed
		LastFMCalcTime = 0;
  if( burn.env->CalcMapFuelMoistures(SimTime) ) { }
  else {
    if( LastFMCalcTime >= maximum ) { return false; }
    else {
			ConvertSimtimeToActualTime(burn.SIMTIME, &mo, &dy, &hr, &mn, false);
			sprintf(Line, "%02ld/%02ld %02ld:%02ld", mo, dy, hr, mn);
		}
	}

	StartMoistThread();
  do {
		ConvertSimtimeToActualTime(MoistSimTime, &mo, &dy, &hr, &mn, false);
		sprintf(Line, "%02ld/%02ld %02ld:%02ld", mo, dy, hr, mn);
	}
	while (!TerminateMoist);

	LastFMCalcTime = MoistSimTime;//maximum;

	//Sleep(1000);
	burn.env->CalcMapFuelMoistures(burn.SIMTIME + GetActualTimeStep());
	return true;
}

//============================================================================
bool TFarsiteInterface::WriteClocks()
{
	ElTimeConvert();
	CurTimeConvert();
	return true;
}

//============================================================================
void TFarsiteInterface::CheckSteps()
{ //TFarsiteInterface::CheckSteps
  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface:CheckSteps:1\n", CallLevel, "" );

  if( NextFireAfterInterrupt == 0 ) {
    burn.rastmake = GetRastMake();  //Update decision to make raster file
    SetNumFires( GetNewFires() );  //Newfires set from mouse or from file

    //Recalc PrimaryVisEqual from new visual and actual ts.
    long oldequal = PrimaryVisEqual;
    if( oldequal != (PrimaryVisEqual =
                      (long) (GetVisibleTimeStep() / GetActualTimeStep())) ) {
      PrimaryVisCount = 0;  //Reset visible interval to 0;
      SecondaryVisCount = 0;  //Reset secondary visible interval to 0;
    }
    if( GetSecondaryVisibleTimeStep() != -1 )
      SecondaryVisEqual =
               (long) ( GetSecondaryVisibleTimeStep() / GetActualTimeStep() );
    else SecondaryVisCount = SecondaryVisEqual - 1;

    maximum = (long) GetSimulationDuration();
    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface:CheckSteps:1a maximum=%ld\n",
              CallLevel, "", maximum );

    if( DistanceCheckMethod(GETVAL) == DISTCHECK_FIRELEVEL )
      OutputVectsAsGrown = ShowFiresAsGrown( GETVAL );
    else OutputVectsAsGrown = 0;
    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface:CheckSteps:1b\n",
              CallLevel, "" );

    //Check thread numbers.
    if( NumSimThreads != GetMaxThreads() ) {
      burn.post.ResetAllThreads();
      burn.ResetAllPerimThreads();
      burn.env->ResetAllThreads();
      burn.CloseCrossThreads();
      NumSimThreads = GetMaxThreads();
    }
  }

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface:CheckSteps:2\n", CallLevel, "" );

  CallLevel--;
} //TFarsiteInterface::CheckSteps

//============================================================================
void TFarsiteInterface::WriteVectorOutputs()
{ 
	bool WriteVect = false;

  if (burn.SIMTIME == 0.0 || PrimaryVisCount == PrimaryVisEqual) {  // if visible time step
    //This part was in FromG5 ver but not in JAS! ver.
    if(DistanceCheckMethod(GETVAL)==DISTCHECK_SIMLEVEL) {	
      if(burn.CuumTimeIncrement!=0.0) return;
    }
    //End FromG5 section.
  }

	if (GetVectMake())
	{
		if (!GetVectVisOnly())						  // if write all perimeters
			WriteVect = true;
		else if( burn.SIMTIME == 0.0 || PrimaryVisCount == PrimaryVisEqual )
			WriteVect = true;
		if (ExcludeBarriersFromVectorFiles(GETVAL) &&
			GetInout(CurrentFire) == 3)
			WriteVect = false;
		if (WriteVect)
			vect.VectData(CurrentFire, burn.SIMTIME);
	}
	WriteVect = false;
	if (GetShapeMake())    // if shapefileoutput
	{
		if (!shape.VisOnly)
			WriteVect = true;
		else if (burn.SIMTIME == 0 || PrimaryVisCount == PrimaryVisEqual)
			WriteVect = true;
		if (ExcludeBarriersFromVectorFiles(GETVAL) &&
			GetInout(CurrentFire) == 3)
			WriteVect = false;
		if (WriteVect)
			shape.ShapeData(CurrentFire, burn.SIMTIME);	// write fires
	}
}


//============================================================================
void TFarsiteInterface::ElTimeConvert()
{
	double TEMPTIME = burn.SIMTIME - GetActualTimeStep();
	double Hr, Dy;
	long dy = 0, hr = 0, min;
	char colon[] = ":";

	if (TEMPTIME >= 1440.0)
	{
		Dy = TEMPTIME / 1440.0;
		dy = (long) Dy;
		TEMPTIME -= 1440 * dy;
	}
	if (TEMPTIME >= 60.0)
	{
		Hr = TEMPTIME / 60.0;
		hr = (long) Hr;
		TEMPTIME -= 60 * hr;
	}
	min = (long)TEMPTIME;//*60;
	//TimeKeepElapsed.SetData(dy, hr, min, 0);
	sprintf(ElTime, "%02d %02d%s%02d", (int)dy, (int)hr, colon, (int)min);
}

//============================================================================
void TFarsiteInterface::CurTimeConvert()
{
	double TEMPTIME = burn.SIMTIME - GetActualTimeStep();
	double Hr, Dy;
	long mo = GetStartMonth(), dy = 0, hr = 0, min;
	char colon[] = ":";
	char slash[] = "/";

	if (TEMPTIME >= 1440.0)
	{
		Dy = TEMPTIME / 1440.0;
		dy = (long) Dy;
		TEMPTIME -= (1440 * dy);					 // truncate to nearest day
	}
	if (TEMPTIME >= 60.0)
	{
		Hr = TEMPTIME / 60.0;
		hr = (long) Hr;
		TEMPTIME -= (60 * hr);					 // truncate to nearesat hour
	}
	min = (long) TEMPTIME;  						// minutes left over
	min += GetStartMin();
	if (min >= 60)
	{
		min -= 60;
		hr++;
	}
	hr += (GetStartHour() / 100);
	if (hr >= 24)
	{
		hr -= 24;
		dy++;
	}
	dy += GetStartDay();
	long days, oldmo;
	do
	{
		oldmo = mo;
		switch (mo)
		{
		case 1:
			days = 31; break;			 // days in each month, ignoring leap year
		case 2:
			days = 28; break;
		case 3:
			days = 31; break;
		case 4:
			days = 30; break;
		case 5:
			days = 31; break;
		case 6:
			days = 30; break;
		case 7:
			days = 31; break;
		case 8:
			days = 31; break;
		case 9:
			days = 30; break;
		case 10:
			days = 31; break;
		case 11:
			days = 30; break;
		case 12:
			days = 31; break;
		}
		if (dy > days)
		{
			dy -= days;
			mo++;
			if (mo > 12)
				mo -= 12;
		}
	}
	while (mo != oldmo);						// allows startup of current clock at any time, will find cur month
	//TimeKeepCurrent.SetData(dy, hr, min, mo);
	sprintf(CurTime, "%02ld%s%02ld  %02ld%s%02ld", mo, slash, dy, hr, colon,
		min);
}

//============================================================================
bool TFarsiteInterface::PreCalculateFuelMoisturesNoUI()
{ //TFarsiteInterface::PreCalculateFuelMoisturesNoUI
  long mo, dy, hr, mn;
  double SimTime;

  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface:PreCalculateFuelMoisturesNoUI:1\n",
            CallLevel, "" );
  
  SimTime = burn.SIMTIME + GetActualTimeStep();
  //Use the shortest interval, ten hour at present, longer ones will also be
  //taken care of.
  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface:PreCalculateFuelMoisturesNoUI:2\n",
            CallLevel, "" );

  //If weather etc. changed.
  if( EnvironmentChanged(GETVAL, 0, SIZECLASS_10HR) ) LastFMCalcTime = 0;
  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface:PreCalculateFuelMoisturesNoUI:3 "
            "\n", CallLevel, "" );
  if( burn.env->CalcMapFuelMoistures(SimTime) ) {}
  else {
    if( Verbose > CallLevel )
      printf( "%*sfarsite4:TFarsiteInterface:PreCalculateFuelMoisturesNoUI:3a "
              "\n", CallLevel, "" );
    if( LastFMCalcTime >= maximum ) {
      if( Verbose > CallLevel )
        printf( "%*sfarsite4:TFarsiteInterface:PreCalculateFuelMoisturesNoUI:"
                "3a1\n", CallLevel, "" );
      CallLevel--;
      return false;
    }
    else
      ConvertSimtimeToActualTime( burn.SIMTIME, &mo, &dy, &hr, &mn, false );
  }
  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface:PreCalculateFuelMoisturesNoUI:4\n",
            CallLevel, "" );

  StartMoistThread();
  do {
    ConvertSimtimeToActualTime( MoistSimTime, &mo, &dy, &hr, &mn, false );
  }while ( ! TerminateMoist );

  LastFMCalcTime = MoistSimTime;

  burn.env->CalcMapFuelMoistures(burn.SIMTIME + GetActualTimeStep());

  if( Verbose > CallLevel )
    printf( "%*sfarsite4:TFarsiteInterface:PreCalculateFuelMoisturesNoUI:5\n",
            CallLevel, "" );
  CallLevel--;
  return true;
} //TFarsiteInterface::PreCalculateFuelMoisturesNoUI

//============================================================================
void TFarsiteInterface::SaveSettingsToFile( const char *fname )
{ //TFarsiteInterface::SaveSettingsToFile
  FILE* outfile;
  char Text[8]="";
  char String[256]="";
  char AclFile[256]="";
  long i;

  outfile = fopen( fname, "w" );
  if( outfile == NULL ) {
    printf( "## Can't to write %s! ##\n", fname );
    return;
  }

  //Inputs
  fprintf( outfile, "Inputs:\n" );
  if( strlen(Inputs.MBLand)>0 )
    fprintf(outfile, "   %s %s\n", "Landscape:", strupr(Inputs.MBLand));
  for( i=0; i<5; i++ ) {
    if( strcmp(Inputs.MBWtr[i], "Load Weather") )
      fprintf(outfile, "   %s %s\n", "Weather:", strupr(Inputs.MBWtr[i]));
  }
  for( i=0; i<5; i++ ) {
    if( strcmp(Inputs.MBWnd[i], "Load Winds") )
      fprintf(outfile, "   %s %s\n", "Winds:", strupr(Inputs.MBWnd[i]));
  }
  if( strlen(Inputs.MBAdj)>0 )
    fprintf(outfile, "   %s %s\n", "Adjustments:", strupr(Inputs.MBAdj));
  if( strlen(Inputs.MBMois)>0 )
    fprintf(outfile, "   %s %s\n", "Fuel Moistures:", strupr(Inputs.MBMois));
  if( strlen(Inputs.MBConv)>0 )
    fprintf(outfile, "   %s %s\n", "Conversions:", strupr(Inputs.MBConv));
  else fprintf( outfile, "   %s %s\n", "Conversions:", "NONE" );

  if( strlen(Inputs.MBFuelMods)>0 )
    fprintf(outfile, "   %s %s\n", "Custom Fuel Models:", strupr(Inputs.MBFuelMods));
  else fprintf( outfile, "   %s %s\n", "Custom Fuel Models:", "NONE" );

  if( strlen(Inputs.MBCwd)>0 )
    fprintf(outfile, "   %s %s\n", "Coarse Woody Fuels:", strupr(Inputs.MBCwd));
  else fprintf( outfile, "   %s %s\n", "Coarse Woody Fuels:", "NONE" );

  if( strlen(Inputs.MBBpd)>0 )
    fprintf(outfile, "   %s %s\n", "Burning Period:", strupr(Inputs.MBBpd));
  else fprintf( outfile, "   %s %s\n", "Burning Period:", "NONE" );

  fprintf( outfile, "   %s %s\n", "Project File:", Inputs.ProjectFile );

  //Outputs
     	fprintf(outfile, "Outputs:\n");

		char RevCopy[256]="", TempCopy[256]="";
		char pathdiv[]="\\";              // search for path
	     long length;

     	if(GetShapeMake())
	     {    long m, n, o;
     		memset(String, 0x0, sizeof(String));
     		strcpy(RevCopy, GetShapeFileChx(&m, &n, &o));	    // copy file path string
	          strrev(RevCopy);			    // reverse file path string
     	     length=strcspn(RevCopy, pathdiv); // compute length without path
          	strncpy(TempCopy, RevCopy, length);  // copy length w/o path
	          strrev(TempCopy);
     	     fprintf(outfile, "   %s: %s\n", "Shapefile", TempCopy);
	     }
     	if(GetVectMake())
	     {    memset(String, 0x0, sizeof(String));
     		strcpy(RevCopy, GetVectorFileName());	    // copy file path string
	          strrev(RevCopy);			    // reverse file path string
     	     length=strcspn(RevCopy, pathdiv); // compute length without path
          	strncpy(TempCopy, RevCopy, length);  // copy length w/o path
	          strrev(TempCopy);
     	     fprintf(outfile, "   %s: %s\n", "Vector file", TempCopy);
	     }

	     if(GetRastMake())
     	{	for(i=20; i<27; i++)
     		{    memset(String, 0x0, sizeof(String));
          		if(GetFileOutputOptions(i))
	          	{    strcpy(String, "Raster File: ");
     	               strcpy(RevCopy, GetRasterFileName(i));	    // copy file path string
					strrev(RevCopy);			    // reverse file path string
					length=strcspn(RevCopy, pathdiv); // compute length without path
					strncpy(TempCopy, RevCopy, length);  // copy length w/o path
					strrev(TempCopy);
     	               strcat(String, TempCopy);
			    	     fprintf(outfile, "   %s: %s\n", "Raster file", String);
               	}
	          }
	     }
     	if(!AccessDisplayUnits(GETVAL))
	     	fprintf(outfile, "   %s\n", "Display Units: METRIC");
	     else
	     	fprintf(outfile, "   %s\n", "Display Units: ENGLISH");
	     if(!AccessOutputUnits(GETVAL))
			fprintf(outfile, "   %s\n", "Output File Units: METRIC");
	     else
			fprintf(outfile, "   %s\n", "Output File Units: ENGLISH");
          switch(OutputFireParam(GETVAL))
          {    case 0:
                    sprintf(String, "Perimeter Color: Fireline Intensity");
                    break;
               case 1:
                    sprintf(String, "Perimeter Color: Spread Rate");
                    break;
               case 2:
                    sprintf(String, "Perimeter Color: Flame Length");
                    break;
               case 3:
                    sprintf(String, "Perimeter Color: Heat/Area");
                    break;
               case 4:
                    sprintf(String, "Perimeter Color: White");
                    break;
               case 5:
                    sprintf(String, "Perimeter Color: Reaction Intensity");
                    break;
          }

  //Model
  fprintf(outfile, "Model:\n");

  fprintf(outfile, "   %s %3.1lf\n", "Parameters: TimeStep", GetActualTimeStep());
  fprintf(outfile, "   %s %3.1lf, %3.1lf\n", "Parameters: Visibles", GetVisibleTimeStep(), GetSecondaryVisibleTimeStep());
  fprintf(outfile, "   %s %3.1lf\n", "Parameters: Perim Res", GetPerimRes());
  fprintf(outfile, "   %s %3.1lf\n", "Parameters: Dist Res", GetDistRes());

	     if(EnableCrowning(GETVAL))
     		fprintf(outfile, "   %s: %s\n", "Options: Crown Fire", "ENABLED");
	     else
     		fprintf(outfile, "   %s: %s\n", "Options: Crown Fire", "DISABLED");
		if(EnableSpotting(GETVAL))
     		fprintf(outfile, "   %s: %s\n", "Options: Spotting", "ENABLED");
	     else
     		fprintf(outfile, "   %s: %s\n", "Options: Spotting", "DISABLED");
		if(EnableSpotFireGrowth(GETVAL))
     		fprintf(outfile, "   %s: %s\n", "Options: Spot Growth", "ENABLED");
	     else
     		fprintf(outfile, "   %s: %s\n", "Options: Spot Growth", "DISABLED");
	     //fprintf(outfile, "   %s: %4.2lf %\n", "Options: Ignition Frequency", (double) PercentIgnition(GETVAL));
		fprintf(outfile, "   %s: %ld mins\n", "Options: Ignition Delay", (long) IgnitionDelay(GETVAL));

		if(DistanceCheckMethod(GETVAL)==0)
			fprintf(outfile, "   %s: %s\n", "Options:", "Sim Level Dist. Check");
	     else
			fprintf(outfile, "   %s: %s\n", "Options", "Fire Level Dist. Check");

	     if(AccelerationON())
     		fprintf(outfile, "   %s: %s\n", "Acceleration", "ON");
	     else
     		fprintf(outfile, "   %s: %s\n", "Acceleration", "OFF");

	     if(strlen(AclFile)>0)
     		fprintf(outfile, "   %s: %s\n", "Acceleration", AclFile);
	     else {
         fprintf(outfile, "   %s: %s\n", "Acceleration", "DEFAULTS");
       }

       if(CheckPostFrontal(GETVAL)) {
         fprintf(outfile, "   %s: %s\n", "Post Frontal", "ON");
       }
       else {
         fprintf(outfile, "   %s: %s\n", "Post Frontal", "OFF");
       }

       fprintf( outfile, "   Dead Fuel Moisture: PRE-CALCULATED\n" );

       //----------------------------------
       //SIMULATE
       //----------------------------------
		fprintf(outfile, "Simulate:\n");

     	fprintf(outfile, "   %s %ld%s%ld\n", "Duration: Conditioning (Mo/Day):",
     		GetConditMonth(), "/", GetConditDay());

	     fprintf(outfile, "   %s %ld%s%ld %ld:%ld\n", "Duration: Starting (Mo/Day Hour:Min):",
     		GetStartMonth(), "/", GetStartDay(), GetStartHour()/100, GetStartMin());

		fprintf( outfile, "   %s %ld%s%ld %ld:%ld\n", "Duration: Ending (Mo/Day Hour:Min):",
             EndMonth, "/", EndDay, EndHour/100, EndMin );

	     if(DurationResetAtRestart(GETVAL))
     		strcpy(Text, "TRUE");
	     else
     		strcpy(Text, "FALSE");
	     fprintf(outfile, "   %s%s%s\n", "Options: ", "Duration Reset: ", Text);

	     if(IgnitionResetAtRestart(GETVAL))
     		strcpy(Text, "TRUE");
	     else
     		strcpy(Text, "FALSE");
	     fprintf(outfile, "   %s%s%s\n", "Options: ", "Restore Ignitions: ", Text);

	     if(RotateSensitiveIgnitions(GETVAL))
     		strcpy(Text, "TRUE");
	     else
     		strcpy(Text, "FALSE");
	     fprintf(outfile, "   %s%s%s\n", "Options: ", "Rotation Sensitive Ignitions: ", Text);

	     if(ShowFiresAsGrown(GETVAL))
     		strcpy(Text, "TRUE");
	     else
     		strcpy(Text, "FALSE");
	     fprintf(outfile, "   %s%s%s\n", "Options: ", "Show Fires as Grown: ", Text);

	     if(AdjustIgnitionSpreadRates(GETVAL))
	     	strcpy(Text, "TRUE");
     	else
     		strcpy(Text, "FALSE");
	     fprintf(outfile, "   %s%s%s\n", "Options: ", "Ignition Spread Rates: ", Text);

	     if(PreserveInactiveEnclaves(GETVAL))
	     	strcpy(Text, "TRUE");
     	else
     		strcpy(Text, "FALSE");
	     fprintf(outfile, "   %s%s%s\n", "Options: ", "Preserve Inactive Enclaves: ", Text);
		fprintf(outfile, "   %s%s%02ld\n", "Options: ", "Simulation Threads: ", GetMaxThreads());

//----------------------------------
//	 ATTACK
//----------------------------------
		fprintf(outfile, "Attack:\n");
//     	if(GetNumCrews()>0)
//		 fprintf(outfile, "   %s: %s\n", "Ground Resources", CrewFile);
//    	 else
//		 fprintf(outfile, "   %s\n", "Ground Resources: NONE");
//     	if(GetNumAirCraft()>0)
//		 fprintf(outfile, "   %s: %s\n", "Air Resources", AirFile);
//     	else
// 	fprintf(outfile, "   %s\n", "Air Resources: NONE");

//----------------------------------
//	 VIEW
//----------------------------------
		fprintf(outfile, "View:\n");

	     if(GetNumViewEast()!=GetNumEast() || GetNumViewNorth()!=GetNumNorth())
     	     fprintf(outfile, "   %s %s\n", "Viewport:", "ZOOMED");
	     else
     	     fprintf(outfile, "   %s %s\n", "Viewport:", "MAXIMIZED");
	     fclose(outfile);
} //TFarsiteInterface::SaveSettingsToFile
