/*fsxwatm.cpp
  See LICENSE.TXT file for license information.
*/
//#include"fsxwatm.h"
#include"globals.h"
#include"fsglbvar.h"
#include"portablestrings.h"

//----------------------------------------------------------------------------
//AtmosphereGrid Structure Functions
//----------------------------------------------------------------------------

//============================================================================
AtmosphereGrid::AtmosphereGrid( long numgrids )
{ //AtmosphereGrid::AtmosphereGrid
  NumGrids = numgrids;
  switch ( NumGrids ) {
    case 0:
      StartGrid = 6;
      AtmGridWND = false;
      AtmGridWTR = false;
      break;
    case 3:
      StartGrid = 3;
      AtmGridWND = true;
      AtmGridWTR = false;
      break;
    case 6:
      StartGrid = 0;
      AtmGridWND = true;
      AtmGridWTR = true;
      break;
    default:
      StartGrid = 6;
      AtmGridWND = false;
      AtmGridWTR = false;
      break;
  }
  TimeIntervals = 0;
  Month = 0;
  Day = 0;
  Hour = 0;
  Metric = 0;
} //AtmosphereGrid::AtmosphereGrid

//============================================================================
AtmosphereGrid::~AtmosphereGrid()
{ //AtmosphereGrid::~AtmosphereGrid
  long i;

  for( i = StartGrid; i < 6; i++ ) atmosphere[i].FreeAtmGrid();
  if( Month ) delete[] Month;
  if( Day ) delete[] Day;
  if( Hour ) delete[] Hour;
} //AtmosphereGrid::~AtmosphereGrid

//============================================================================
void AtmosphereGrid::FreeAtmData()
{ //AtmosphereGrid::FreeAtmData
  long i;

  for( i = StartGrid; i < 6; i++ ) atmosphere[i].FreeAtmGrid();
  if( Month ) delete[] Month;
  if( Day ) delete[] Day;
  if( Hour ) delete[] Hour;
  Month = 0;
  Day = 0;
  Hour = 0;
  Metric = 0;
} //AtmosphereGrid::FreeAtmData

//============================================================================
bool AtmosphereGrid::ReadInputTable( char* InputFileName )
{ //AtmosphereGrid::ReadInputTable
  char InputTemp[256];
  char InputHumid[256];
  char InputRain[256];
  char InputCloud[256];
  char InputSpd[256];
  char InputDir[256];
  char FileName[256];
  char UnitsString[256] = "";
  long i, j, fpos;

  CallLevel++;

  TimeIntervals = 0;
  memset( ErrMsg, 0x0, sizeof(ErrMsg) );

  if( Verbose > CallLevel )
    printf( "%*sfsxwatm:AtmosphereGrid:ReadInputTable:1 Attempting to open "
            "%s....\n", CallLevel, "", InputFileName );

  if( (InputTable = fopen(InputFileName, "r")) != NULL ) {
    fscanf( InputTable, "%s", InputTemp );  //Get by header
    fscanf( InputTable, "%s", UnitsString );
    fpos = ftell( InputTable );

    //English default.
    if( ! strcmp(strlwr(UnitsString), "metric") ) Metric = 1;
    else if( ! strcmp(strlwr(UnitsString), "english") ) Metric = 0;
    else {
      rewind( InputTable );
      fscanf( InputTable, "%s", InputTemp );  //Get by header
      fpos = ftell( InputTable );
    }

    while( ! feof(InputTable) ) {    //Count number of time intervals
      fscanf( InputTable, "%ld", &month );
      if( feof(InputTable) ) break;
      if( NumGrids < 6 ) {
        fscanf( InputTable, "%ld %ld %s %s %s ", &day, &hour, InputSpd,
                InputDir, InputCloud );
        if( ! IsReadable(InputSpd) )
          sprintf( ErrMsg, "%s %s", InputSpd, "Can't Be Opened or Read" );
        if( ! IsReadable(InputDir) )
          sprintf( ErrMsg, "%s %s", InputDir, "Can't Be Opened or Read" );
        if( ! IsReadable(InputCloud) )
          sprintf( ErrMsg, "%s %s", InputCloud, "Can't Be Opened or Read" );
      }
      else {
        fscanf( InputTable, "%ld %ld %s %s %s %s %s %s", &day, &hour,
                InputTemp, InputHumid, InputRain, InputSpd, InputDir,
                InputCloud );
        if( ! IsReadable(InputTemp) )
          sprintf( ErrMsg, "%s %s", InputTemp, "Can't Be Opened or Read" );
        if( ! IsReadable(InputHumid) )
          sprintf( ErrMsg, "%s %s", InputHumid, "Can't Be Opened or Read" );
        if( ! IsReadable(InputRain) )
          sprintf( ErrMsg, "%s %s", InputRain, "Can't Be Opened or Read" );
        if( ! IsReadable(InputSpd) )
          sprintf( ErrMsg, "%s %s", InputSpd, "Can't Be Opened or Read" );
        if( ! IsReadable(InputDir) )
          sprintf( ErrMsg, "%s %s", InputDir, "Can't Be Opened or Read" );
        if( ! IsReadable(InputCloud) )
          sprintf( ErrMsg, "%s %s", InputCloud, "Can't Be Opened or Read" );
      }
      if( strlen(ErrMsg) > 0 ) {
        fclose( InputTable );
        CallLevel--;
        return false;
      }
      TimeIntervals++;
    };
    Month = new long[TimeIntervals];
    Day = new long[TimeIntervals];
    Hour = new long[TimeIntervals];

    fseek( InputTable, fpos, SEEK_SET );
    if( NumGrids < 6 )
      fscanf( InputTable, "%ld %ld %ld %s %s %s ", &month, &day, &hour,
              InputSpd, InputDir, InputCloud );
    else
      fscanf( InputTable, "%ld %ld %ld %s %s %s %s %s %s", &month, &day,
              &hour, InputTemp, InputHumid, InputRain, InputSpd, InputDir,
              InputCloud );
    for( j = StartGrid; j < 6; j++ ) {   //Set header information in each file
      memset( FileName, 0x0, sizeof(FileName) );
      switch( j ) {
        case 0: strcpy( FileName, InputTemp );  break;
        case 1: strcpy( FileName, InputHumid );  break;
        case 2: strcpy( FileName, InputRain );  break;
        case 3: strcpy( FileName, InputSpd );  break;
        case 4: strcpy( FileName, InputDir );  break;
        case 5: strcpy( FileName, InputCloud );  break;
      }
      if( (ThisFile = fopen(FileName, "r")) == NULL ) {
        sprintf( ErrMsg, "%s %s", FileName, "Can't Be Opened or Read" );
        fclose( InputTable );

        CallLevel--;
        return false;
      }
      else if( ! ReadHeaders(j) ) {
        sprintf( ErrMsg, "%s %s", FileName, "Header Not GRASS or GRID" );
        fclose( InputTable );

        CallLevel--;
        return false;
      }
      else if( ! atmosphere[j].AllocAtmGrid(TimeIntervals) ) {
        sprintf( ErrMsg, "%s %s", FileName, 
                 "Memory Not Sufficient for File" );
        for( i = StartGrid; i < j; i++ ) atmosphere[i].FreeAtmGrid();
        fclose( InputTable );

        CallLevel--;
        return false;
      }
    }
    for( i = 0; i < TimeIntervals; i++ ) {
      Month[i] = month;
      Day[i] = day;
      Hour[i] = hour;
      for( j = StartGrid; j < 6; j++ ) {
        memset( FileName, 0x0, sizeof(FileName) );
        switch( j ) {
          case 0: strcpy( FileName, InputTemp );  break;
          case 1: strcpy( FileName, InputHumid );  break;
          case 2: strcpy( FileName, InputRain );  break;
          case 3: strcpy( FileName, InputSpd );  break;
          case 4: strcpy( FileName, InputDir );  break;
          case 5: strcpy( FileName, InputCloud );  break;
        }
        if( (ThisFile = fopen(FileName, "r")) == NULL ) {
          sprintf( ErrMsg, "%s %s", FileName,
                   "File Not Found or Cannot Be Read" );
          fclose( InputTable );

          CallLevel--;
          return false;
        }
        if( ! CompareHeader(j) ) {
          sprintf( ErrMsg, "%s %s", FileName,
                   "Header Not Same For File Type" );
          fclose( InputTable );

          CallLevel--;
          return false;
        }
        if( ! SetAtmosphereValues(i, j) ) {
          sprintf( ErrMsg, "%s %s", FileName, "Error Reading File Values" );
        fclose( InputTable );

          CallLevel--;
          return false;
        }
      }
      if( NumGrids == 6 )
        fscanf( InputTable, "%ld %ld %ld %s %s %s %s %s %s", &month, &day,
                &hour, InputTemp, InputHumid, InputRain, InputSpd, InputDir,
                InputCloud );
      else
        fscanf( InputTable, "%ld %ld %ld %s %s %s", &month, &day, &hour,
                InputSpd, InputDir, InputCloud );
    };

    fclose( InputTable );
  }
  else {
    printf( "%*sfsxwatm:AtmosphereGrid:ReadInputTable:## Can't open "
            "%s! ##\n", CallLevel, "", InputFileName );
    CallLevel--;
    return false;
  }

  CallLevel--;
  return true;
} //AtmosphereGrid::ReadInputTable

//============================================================================
bool AtmosphereGrid::ReadHeaders( long FileNumber )
{ //AtmosphereGrid::ReadHeaders
  char   TestString[256];
  char   CompGrass[] = "north:";
  char   CompGrid[] = "NCOLS";
  double north, south, east, west, xres, yres;
  long   rows, cols;

  fscanf( ThisFile, "%s", TestString );
  if( ! strcmp(TestString, CompGrass) ) {    //Grass file
    fscanf( ThisFile, "%lf", &north );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%lf", &south );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%lf", &east );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%lf", &west );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%ld", &rows );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%ld", &cols );
    xres = ( east - west ) / (double) cols;
    yres = ( north - south ) / (double) rows;
    atmosphere[FileNumber].SetHeaderInfo( north, south, east, west, xres,

                                          yres );
  }
  else if( ! (strcmp(strupr(TestString), CompGrid)) ) {
    fscanf( ThisFile, "%ld", &cols );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%ld", &rows );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%lf", &west );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%lf", &south );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%lf", &xres);
    yres = xres;
    east = west + (double) cols * xres;
    north = south + (double) rows * yres;
    atmosphere[FileNumber].SetHeaderInfo( north, south, east, west, xres,
                                          yres );
  }

  else {
    fclose( ThisFile );

    return false;
  }

  fclose( ThisFile );

  return true;
} //AtmosphereGrid::ReadHeaders

//============================================================================
bool AtmosphereGrid::CompareHeader( long FileNumber )
{ //AtmosphereGrid::CompareHeader
  char   TestString[256];
  char   CompGrass[] = "north:";
  char   CompGrid[] = "NCOLS";
  double north, south, east, west, xres, yres;
  long   rows, cols;

  fscanf( ThisFile, "%s", TestString );
  if( ! strcmp(TestString, CompGrass)) {    //Grass file
    fscanf( ThisFile, "%lf", &north );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%lf", &south );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%lf", &east );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%lf", &west );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%ld", &rows );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%ld", &cols );
    xres = (east - west) / (double) cols;
    yres = (north - south) / (double) rows;
    if( ! atmosphere[FileNumber].CompareHeader(north, south, east, west, xres,

        yres) ) {
      fclose( ThisFile );

      return false;
    }
  }
  else if( ! (strcmp(strupr(TestString), CompGrid)) ) {
    fscanf( ThisFile, "%ld", &cols );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%ld", &rows );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%lf", &west );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%lf", &south );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%lf", &xres );
    fscanf( ThisFile, "%s", TestString );
    fscanf( ThisFile, "%s", TestString );
    yres = xres;
    east = west + (double) cols * xres;
    north = south + (double) rows * yres;
    if( ! atmosphere[FileNumber].CompareHeader(north, south, east, west,
        xres, yres) ) {
      fclose( ThisFile );

      return false;
    }
  }
  else {
    fclose( ThisFile );

    return false;
  }
  if( FileNumber == 3 ) {  //Set gridded weather dimensions for wind display
    double SouthDiff = fabs( south - GetSouthUtm() );
    double WestDiff = fabs( west - GetWestUtm() );
    double SouthOffset = SouthDiff - ( (long) (SouthDiff / yres) ) * yres;
    double WestOffset = WestDiff - ( (long) (WestDiff / xres) ) * xres;
    long   EastNumber, NorthNumber;

    if( GetEastUtm() < east )
      EastNumber = (long)((GetEastUtm() - GetWestUtm() - WestOffset) / xres);
    else
      EastNumber = (long)((east - GetWestUtm() - WestOffset) / xres);
    if( GetNorthUtm() < north )
      NorthNumber = (long)((GetNorthUtm() - GetSouthUtm() - SouthOffset) /
                    yres);
    else
      NorthNumber = (long)((north - GetSouthUtm() - SouthOffset) / yres);
    if( EastNumber < atmosphere[FileNumber].XNumber ) EastNumber++;
    if( NorthNumber < atmosphere[FileNumber].YNumber ) NorthNumber++;
    SetGridNorthOffset( SouthOffset );
    SetGridEastOffset( WestOffset );
    SetGridEastDimension( EastNumber );
    SetGridNorthDimension( NorthNumber );
    //SetGridEastDimension(atmosphere[FileNumber].XNumber);
    //SetGridNorthDimension(atmosphere[FileNumber].YNumber);
  }

  return true;
} //AtmosphereGrid::CompareHeader

//============================================================================
bool AtmosphereGrid::SetAtmosphereValues( long timeinterval, long filenumber )
{ //AtmosphereGrid::SetAtmosphereValues
  double value;

  for( long i = 0; i < atmosphere[filenumber].NumCellsPerGrid; i++ ) {
    fscanf( ThisFile, "%lf", &value );
    if( Metric ) {
      switch( filenumber ) {
        case 0:
          value *= 1.0;
          value += 32.0;
          break;
        case 2:
          value *= 3.93;
          break;
        case 3:
          value *= .5402;  //Convert 10m to 20ft, and kmph to miph
          break;
      }
    }
    if( ! atmosphere[filenumber].SetAtmValue(i, timeinterval, (short) value) )
    {
      fclose( ThisFile );

      return false;
    }
  }
  fclose( ThisFile );

  return true;
} //AtmosphereGrid::SetAtmosphereValues

//============================================================================
bool AtmosphereGrid::GetAtmosphereValue( long FileNumber, double xpt,
                                         double ypt, long time, short* value )
{ //AtmosphereGrid::GetAtmosphereValue
  if( ! atmosphere[FileNumber].GetAtmValue(xpt, ypt, time, value) ) {
    switch( FileNumber ) {
      case 0: *value = 70; break;  //Default values for ATM vars if no data
      case 1: *value = 40; break;
     default: *value = 0; break;
    }

    return false;
  }

  return true;
} //AtmosphereGrid::GetAtmosphereValue

//============================================================================
void AtmosphereGrid::GetResolution( long FileNumber, double* resx,

                                    double* resy )
{ //AtmosphereGrid::GetResolution
  atmosphere[FileNumber].GetResolution( resx, resy );
} //AtmosphereGrid::GetResolution

//============================================================================
long AtmosphereGrid::GetTimeIntervals()
{ //AtmosphereGrid::GetTimeIntervals
  return TimeIntervals;
} //AtmosphereGrid::GetTimeIntervals

//============================================================================

long AtmosphereGrid::GetAtmMonth( long count )
{ //AtmosphereGrid::GetAtmMonth
  if( count < TimeIntervals ) return Month[count];

  return -1;
} //AtmosphereGrid::GetAtmMonth

//============================================================================

long AtmosphereGrid::GetAtmDay( long count )
{ //AtmosphereGrid::GetAtmDay
  if( count < TimeIntervals ) return Day[count];

  return -1;
} //AtmosphereGrid::GetAtmDay

//============================================================================

long AtmosphereGrid::GetAtmHour( long count )
{ //AtmosphereGrid::GetAtmHour
  if( count < TimeIntervals ) return Hour[count];

  return -1;
} //AtmosphereGrid::GetAtmHour

//----------------------------------------------------------------------------
//Atmosphere Structure Functions
//----------------------------------------------------------------------------

//============================================================================
Atmosphere::Atmosphere()
{ //Atmosphere::Atmosphere
  North = 0;
  South = 0;
  East = 0;
  West = 0;
  ResolutionX = 0;
  ResolutionY = 0;
  Value = 0;
} //Atmosphere::Atmosphere

//============================================================================
Atmosphere::~Atmosphere() {}

//============================================================================
void Atmosphere::SetHeaderInfo( double north, double south, double east,
                                double west, double resolutionx,

                                double resolutiony )
{ //Atmosphere::SetHeaderInfo
  North = ConvertUtmToNorthingOffset( north );
  South = ConvertUtmToNorthingOffset( south );
  East = ConvertUtmToEastingOffset( east );
  West = ConvertUtmToEastingOffset( west );
  ResolutionX = resolutionx;
  ResolutionY = resolutiony;
  XNumber = (long)( (East - West) / ResolutionX );
  YNumber = (long)( (North - South) / ResolutionY );
  NumCellsPerGrid = (long)( XNumber * YNumber );
} //Atmosphere::SetHeaderInfo

//============================================================================
bool Atmosphere::GetHeaderInfo( double* north, double* south, double* east,
                                double* west, double* resolutionx,

                                double* resolutiony )
{ //Atmosphere::GetHeaderInfo
  *north = North;
  *south = South;
  *east = East;
  *west = West;
  *resolutionx = ResolutionX;
  *resolutiony = ResolutionY;

  return true;
} //Atmosphere::GetHeaderInfo

//============================================================================
void Atmosphere::GetResolution( double* resolutionx, double* resolutiony )
{ //Atmosphere::GetResolution
  *resolutionx = ResolutionX;
  *resolutiony = ResolutionY;
} //Atmosphere::GetResolution

//============================================================================
bool Atmosphere::CompareHeader( double north, double south, double east,
                                double west, double resolutionx,

                                double resolutiony )
{ //Atmosphere::CompareHeader
  if( North != ConvertUtmToNorthingOffset(north) ) return false;
  if( South != ConvertUtmToNorthingOffset(south) ) return false;
  if( East != ConvertUtmToEastingOffset(east) ) return false;
  if( West != ConvertUtmToEastingOffset(west) ) return false;
  if( ResolutionX != resolutionx ) return false;
  if( ResolutionY != resolutiony ) return false;

  return true;
} //Atmosphere::CompareHeader

//============================================================================
bool Atmosphere::AllocAtmGrid( long timeintervals )
{ //Atmosphere::AllocAtmGrid
  long AllocNumber = ( timeintervals + 1 ) * NumCellsPerGrid;

  if( (Value = new short[AllocNumber]) == NULL ) {    //Allocate 2D grid
    Value = 0;
    return false;
  }

  return true;
} //Atmosphere::AllocAtmGrid

//============================================================================

bool Atmosphere::FreeAtmGrid()
{ //Atmosphere::FreeAtmGrid
  if( Value ) delete[] Value;
  else return false;

  Value = 0;

  return true;
} //Atmosphere::FreeAtmGrid

//============================================================================

bool Atmosphere::GetAtmValue( double xpt, double ypt, long time,

                              short* value )
{ //Atmosphere::GetAtmValue
  if( xpt < West || xpt > East ) return false;
  if( ypt < South || ypt > North) return false;
  CellX = ( (long) ((xpt - West) / ResolutionX) );
  CellY = ( (long) ((North - ypt) / ResolutionY) );
  *value = Value[time * NumCellsPerGrid + (CellY * XNumber + CellX)];

  return true;
} //Atmosphere::GetAtmValue

//============================================================================
bool Atmosphere::SetAtmValue( long CellNumber, long time, short value )
{ //Atmosphere::SetAtmValue
  if( Value ) {
    Value[time * NumCellsPerGrid + CellNumber] = (short) value;

    return true;
  }

  return false;
} //Atmosphere::SetAtmValue