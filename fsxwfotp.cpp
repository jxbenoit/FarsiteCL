/*fsxwfotp.cpp
  See LICENSE.TXT file for license information.
*/
#include<sys/stat.h>
#include<string.h>
#include"fsglbvar.h"
#include"globals.h"

OutputFile::OutputFile()
{
	filepos = 0;
	NumRastAlloc = 0;   
	NumRastData = 0;
};


OutputFile::~OutputFile()
{
	FreeRastData();
};


void OutputFile::SelectOutputs(long OutputFormat)
{
  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfsxwfotp:OutputFile::SelectOutputs:1\n", CallLevel, "" );

  if( OutputFormat == 1 || OutputFormat == 5 ) WriteRastMemFiles();
  else if( OutputFormat > 1 ) {
    FileOutput = OutputFormat;	// specify vector file for optional
    OptionalOutput(false);
  }

  if( Verbose > CallLevel )
    printf( "%*sfsxwfotp:OutputFile::SelectOutputs:2\n", CallLevel, "" );

  CallLevel--;
}


void OutputFile::SelectMemOutputs(long OutputFormat)
{
  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfsxwfotp:OutputFile::SelectMemOutputs:1\n", CallLevel, "" );

  setHeaderType(OutputFormat);     //AAA In FromG5 ver only

  if( OutputFormat == 1 || OutputFormat == 5 ) WriteRastMemFiles();
  else if( OutputFormat > 1 ) {
    FileOutput = OutputFormat;  // specify vector file for optional
    OptionalOutput(true);
  }

  //AAANumRastData = 0;                                    //AAA In RAS! ver only
  if( Verbose > CallLevel )
    printf( "%*sfsxwfotp:OutputFile::SelectMemOutputs:2\n", CallLevel, "" );

  CallLevel--;
}


void OutputFile::WriteRastMemFiles()
{
  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfsxwfotp:OutputFile::WriteRastMemFiles:1\n", CallLevel, "" );
//AAA JAS! ver AAA	RastMemFile(RAST_ARRIVALTIME);
//AAA JAS! ver AAA	if (GetFileOutputOptions(RAST_FIREINTENSITY))
//AAA JAS! ver AAA		RastMemFile(RAST_FIREINTENSITY);
//AAA JAS! ver AAA	if (GetFileOutputOptions(RAST_FLAMELENGTH))
//AAA JAS! ver AAA		RastMemFile(RAST_FLAMELENGTH);
//AAA JAS! ver AAA	if (GetFileOutputOptions(RAST_SPREADRATE))
//AAA JAS! ver AAA		RastMemFile(RAST_SPREADRATE);
//AAA JAS! ver AAA	if (GetFileOutputOptions(RAST_HEATPERAREA))
//AAA JAS! ver AAA		RastMemFile(RAST_HEATPERAREA);
//AAA JAS! ver AAA	if (GetFileOutputOptions(RAST_REACTIONINTENSITY))
//AAA JAS! ver AAA		RastMemFile(RAST_REACTIONINTENSITY);
//AAA JAS! ver AAA	if (GetFileOutputOptions(RAST_CROWNFIRE))
//AAA JAS! ver AAA		RastMemFile(RAST_CROWNFIRE);
//AAA JAS! ver AAA	if (GetFileOutputOptions(RAST_FIREDIRECTION))
//AAA JAS! ver AAA		RastMemFile(RAST_FIREDIRECTION);

  //AAA FromG5 ver below
  WriteFile(RAST_ARRIVALTIME);

  if( GetFileOutputOptions(RAST_FIREINTENSITY) )
    WriteFile(RAST_FIREINTENSITY);

  if( GetFileOutputOptions(RAST_FLAMELENGTH) )
    WriteFile(RAST_FLAMELENGTH);

  if( GetFileOutputOptions(RAST_SPREADRATE) )
    WriteFile(RAST_SPREADRATE);

  if( GetFileOutputOptions(RAST_HEATPERAREA) )
    WriteFile(RAST_HEATPERAREA);

  if( GetFileOutputOptions(RAST_REACTIONINTENSITY) )
    WriteFile(RAST_REACTIONINTENSITY);

  if( GetFileOutputOptions(RAST_CROWNFIRE) )
    WriteFile(RAST_CROWNFIRE);

  if( GetFileOutputOptions(RAST_FIREDIRECTION) )
    WriteFile(RAST_FIREDIRECTION);
  //AAA End FromG5 ver

  if( Verbose > CallLevel )
    printf( "%*sfsxwfotp:OutputFile::WriteRastMemFiles:2\n", CallLevel, "" );

  CallLevel--;
}

/*AAA JAS! ver  AAA
bool OutputFile::RastFile(long Type)
{
	char RasterCopy[256] = "";
	double OutDoubleData;
	long FileLoc, OutLongData;

	memset(RasterCopy, 0x0, sizeof RasterCopy);
	switch (Type)
	{
	case RAST_ARRIVALTIME:
		OutDoubleData = t;
		break;
	case RAST_FIREINTENSITY:
		OutDoubleData = f / convf1;
		break;
	case RAST_FLAMELENGTH:
		Calcs(0);					// calculate flame length
		OutDoubleData = l;
		break;
	case RAST_SPREADRATE:
		OutDoubleData = r * convf2;
		break;
	case RAST_HEATPERAREA:
		Calcs(1);   				  // calculate heat/unit area
		OutDoubleData = h;
		break;
	case RAST_REACTIONINTENSITY:
		Calcs(1);   				  // calculate heat/unit area
		OutDoubleData = rx;
		break;
	case RAST_CROWNFIRE:
		Calcs(3);
		OutLongData = c;
		break;
	case RAST_FIREDIRECTION:
		OutLongData = d;
		break;
	}
	strcpy(RasterCopy, GetRasterFileName(Type));
	FileLoc = GetRasterFileLocation(Type);
	otpfile = fopen(RasterCopy, "r+");
	if (otpfile == NULL)
		return false;
	//rewind(otpfile);
	fseek(otpfile, FileLoc, SEEK_SET);
	if (Type == RAST_ARRIVALTIME)
	{
		double OldTime;
		fscanf(otpfile, "%lf", &OldTime);
		if (OldTime == -1.0 || OutDoubleData < OldTime)
		{
			//rewind(otpfile);
			fseek(otpfile, FileLoc, SEEK_SET);
			fprintf(otpfile, "%09.3lf", OutDoubleData);
			fclose(otpfile);
			return true;		  // ok to write other files
		}
		fclose(otpfile);
		return false;
	}
	else if (Type == RAST_FIREINTENSITY ||
		Type == RAST_HEATPERAREA ||
		Type == RAST_REACTIONINTENSITY)
		fprintf(otpfile, "%09.3lf", OutDoubleData);
	else if (Type == RAST_FLAMELENGTH || Type == RAST_SPREADRATE)
		fprintf(otpfile, "%05.2lf", OutDoubleData);
	else
		fprintf(otpfile, "%03ld", OutLongData);
	fclose(otpfile);

	return false;
}

bool OutputFile::RastMemFile(long Type)
{
	char RasterCopy[256] = "";
	double OutDoubleData;
	long i, FileLoc, PrevLoc = 0, RelLoc, StartLoc, OutLongData;

	memset(RasterCopy, 0x0, sizeof(RasterCopy));
	strcpy(RasterCopy, GetRasterFileName(Type));
	otpfile = fopen(RasterCopy, "r+");
	if (otpfile == NULL)
		return false;

	for (i = 0; i < NumRastData; i++)
	{
		if (Type != RAST_ARRIVALTIME)
		{
			if (rd[i].Write == false)
				continue;
		}
		PrevLoc = ftell(otpfile);
		x = rd[i].xpt;
		y = rd[i].ypt;
		t = rd[i].Time;
		f = rd[i].Fli;
		r = rd[i].Ros;
		rx = rd[i].Rcx;
		d = rd[i].Dir;
		switch (Type)
		{
		case RAST_ARRIVALTIME:
			OutDoubleData = t;
			break;
		case RAST_FIREINTENSITY:
			OutDoubleData = f / convf1;
			break;
		case RAST_FLAMELENGTH:
			Calcs(0);					// calculate flame length
			OutDoubleData = l;
			break;
		case RAST_SPREADRATE:
			OutDoubleData = r * convf2;
			break;
		case RAST_HEATPERAREA:
			Calcs(1);   				  // calculate heat/unit area
			OutDoubleData = h;
			break;
		case RAST_REACTIONINTENSITY:
			Calcs(1);   				  // calculate heat/unit area
			OutDoubleData = rx;
			break;
		case RAST_CROWNFIRE:
			Calcs(3);
			OutLongData = c;
			break;
		case RAST_FIREDIRECTION:
			OutLongData = d;
			break;
		}
		FileLoc = GetRasterFileLocation(Type);
		RelLoc = FileLoc - PrevLoc;

		fseek(otpfile, RelLoc, SEEK_CUR);
		if (Type == RAST_ARRIVALTIME)
		{
			double OldTime;
			StartLoc = ftell(otpfile);
			fscanf(otpfile, "%lf", &OldTime);
			if (OldTime == -1.0 || OutDoubleData < OldTime)
			{
				RelLoc = StartLoc - ftell(otpfile);
				fseek(otpfile, RelLoc, SEEK_CUR);
				fprintf(otpfile, "%09.3lf", OutDoubleData);
				rd[i].Write = true;
			}
			else
				rd[i].Write = false;
		}
		else if (Type == RAST_FIREINTENSITY ||
			Type == RAST_HEATPERAREA ||
			Type == RAST_REACTIONINTENSITY)
			fprintf(otpfile, "%09.3lf", OutDoubleData);
		else if (Type == RAST_FLAMELENGTH || Type == RAST_SPREADRATE)
			fprintf(otpfile, "%05.2lf", OutDoubleData);
		else
			fprintf(otpfile, "%03ld", OutLongData);
	}
	fclose(otpfile);

	return false;
}
AAA*/


//AAA FromG5 ver below
bool OutputFile::RastMemFile(long Type)
{
  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfsxwfotp:OutputFile::RastMemFile:1 numrows=%ld numcols=%ld\n",
            CallLevel, "", numrows, numcols );

  double OutDoubleData;
  long OutLongData;

  //Loop over all rows and all columns
  for( long j=0; j < numrows; j++ ) { 
    for( long i=0; i < numcols; i++ ) {  
      //Check to see if this cell has raster data written to it.
      coordinate testxy = std::make_pair(i,j) ; 
      RasterMap::iterator data = rd.find(testxy); 

      //If this cell has no data yet, just print a -1 
      if( data == rd.end() ) {
        //output -1 as either a float or a long.
        if( Type == RAST_FIREINTENSITY ||
            Type == RAST_HEATPERAREA ||
            Type == RAST_REACTIONINTENSITY || 
            Type == RAST_ARRIVALTIME ||
            Type == RAST_FLAMELENGTH || 
            Type == RAST_SPREADRATE ) 
          fprintf(otpfile, "%09.3lf\n", -1.0);
        else fprintf(otpfile, "%03ld\n", -1l) ; 

        if( Verbose > CallLevel+1 ) printf( "_ " );
      }
      else {
        if( Type != RAST_ARRIVALTIME ) {
          if( (data->second).Write == false )
            continue;
        }
        x = (data->second).x;
        y = (data->second).y;
        t = (data->second).Time;
        f = (data->second).Fli;
        r = (data->second).Ros;
        rx = (data->second).Rcx;
        d = (data->second).Dir;

        switch( Type ) {
          case RAST_ARRIVALTIME:
            OutDoubleData = t;
            break;

          case RAST_FIREINTENSITY:
            OutDoubleData = f / convf1;
            break;

          case RAST_FLAMELENGTH:
            Calcs(FLAME_LENGTH);    // calculate flame length
            OutDoubleData = l;
            break;

          case RAST_SPREADRATE:
            OutDoubleData = r * convf2;
            break;

          case RAST_HEATPERAREA:
            Calcs(HEAT_PER_AREA);   // calculate heat/unit area
            OutDoubleData = h;
            break;

          case RAST_REACTIONINTENSITY:
            Calcs(HEAT_PER_AREA);   // calculate heat/unit area
            OutDoubleData = rx;
            break;

          case RAST_CROWNFIRE:
            Calcs(CROWNFIRE);
            OutLongData = c;
            break;

          case RAST_FIREDIRECTION:
            OutLongData = d;
            break;
        }

        if( Type == RAST_ARRIVALTIME ) {
          fprintf(otpfile, "%09.3lf\n", OutDoubleData);
          (data->second).Write = true;
          if( Verbose > CallLevel+1 ) printf( "%lf ", OutDoubleData );
        }
        else if( Type == RAST_FIREINTENSITY ||
                 Type == RAST_HEATPERAREA ||
                 Type == RAST_REACTIONINTENSITY )
          fprintf(otpfile, "%09.3lf\n", OutDoubleData);
        else if( Type == RAST_FLAMELENGTH || Type == RAST_SPREADRATE )
          fprintf(otpfile, "%05.2lf\n", OutDoubleData);
        else
          fprintf(otpfile, "%03ld\n", OutLongData);
      }
    } //End loop through cols
    if( Verbose > CallLevel+1 ) printf( "\n" );
  } //End loop through rows

  if( Verbose > CallLevel )
    printf( "%*sfsxwfotp:OutputFile::RastMemFile:2\n", CallLevel, "" );

  CallLevel--;

  return false;
}

//AAA End FromG5 ver


/*AAA RAS! ver
void OutputFile::OptionalOutput(bool FromMemory)
{
	char RasterCopy[256] = "";
	long i;

	if (FileOutput == 3)
		strcpy(RasterCopy, GetRasterFileName(0));
	else
		strcpy(RasterCopy, GetVectorFileName());
	if (filepos == 0)
	{
//AAA		if (access(RasterCopy, F_OK) == 0)
    if ( _access(RasterCopy, 0) == 0)  //AAA Visual C++ uses _access() instead of access()

			chmod(RasterCopy, S_IWRITE);
		otpfile = fopen(RasterCopy, "w");
		filepos = 1;
	}
	else
		otpfile = fopen(RasterCopy, "a");

	if (otpfile == NULL)
		return;
	if (FromMemory)
	{
		for (i = 0; i < NumRastData; i++)
		{
			x = rd[i].xpt;
			y = rd[i].ypt;
			t = rd[i].Time;
			f = rd[i].Fli;
			r = rd[i].Ros;
			rx = rd[i].Rcx;
			d = rd[i].Dir;
			WriteOptionalFile();
			fprintf(otpfile, "\n");
		}
	}
	else
	{
		WriteOptionalFile();
		fprintf(otpfile, "\n");
	}
	fclose(otpfile);
}
AAA*/





/*AAA FromG5 ver below

 * \brief encloses the logic to write either a single value to the "optional"

 * output file, or writes the entire accumulated array to the file.  THis 

 * does not output in a gridded format, but rather just dumps the accumulated

 * points to the file.

 */

//============================================================================
void OutputFile::OptionalOutput( bool FromMemory )
{ //OutputFile::OptionalOutput
  char RasterCopy[256] = "";

  if( FileOutput == 3 ) strcpy( RasterCopy, GetRasterFileName(0) );
  else strcpy( RasterCopy, GetVectorFileName() );

  if( filepos == 0 ) {
    if( Exists(RasterCopy) ) SetFileMode( RasterCopy, FILE_MODE_WRITE );
    otpfile = fopen( RasterCopy, "w" );
    filepos = 1;
  }
  else otpfile = fopen( RasterCopy, "a" );

  if( otpfile == NULL ) return;

  if( FromMemory ) {
    RasterMap::const_iterator data ; 
    for( data = rd.begin(); data != rd.end(); ++data ) {
      x = (data->second).x;
      y = (data->second).y;
      t = (data->second).Time;
      f = (data->second).Fli;
      r = (data->second).Ros;
      rx = (data->second).Rcx;
      d = (data->second).Dir;
      WriteOptionalFile();
      fprintf( otpfile, "\n" );
    }
  }
  else {
    WriteOptionalFile();
    fprintf( otpfile, "\n" );
  }

  fclose( otpfile );
} //OutputFile::OptionalOutput

//AAA End FromG5 ver



/**

 * \brief Writes a single value to the already opened, already positioned 

 * output file.  

 */

void OutputFile::WriteOptionalFile()
{
	x = ConvertEastingOffsetToUtm(x);
	y = ConvertNorthingOffsetToUtm(y);
	fprintf(otpfile, "%010.4lf %010.4lf %010.4lf", x, y, t);
	if (GetFileOutputOptions(RAST_FIREINTENSITY))
		fprintf(otpfile, " %010.4lf", f / convf1);
	if (GetFileOutputOptions(RAST_FLAMELENGTH))
	{
		//AAA Calcs(0);       RAS! ver
		Calcs(FLAME_LENGTH);  //AAA FromG5 ver

		fprintf(otpfile, " %010.4lf", l);
	}
	if (GetFileOutputOptions(RAST_SPREADRATE))
		fprintf(otpfile, " %010.4lf", r * convf2);
	if (GetFileOutputOptions(RAST_HEATPERAREA))
	{
		//AAACalcs(1);        RAS! ver

		Calcs(HEAT_PER_AREA);  //AAA FromG5 ver
		fprintf(otpfile, " %010.4lf", h);
	}
	if (GetFileOutputOptions(RAST_REACTIONINTENSITY))
	{
		//AAACalcs(1);       RAS! ver

    Calcs(HEAT_PER_AREA);  //AAA FromG5 ver
		fprintf(otpfile, " %010.4lf", rx);
	}
	if (GetFileOutputOptions(RAST_CROWNFIRE))
	{
		//AAACalcs(3);       RAS! ver

    Calcs(CROWNFIRE);     //AAA FromG5 ver
		fprintf(otpfile, " %ld", c);
	}
	if (GetFileOutputOptions(RAST_FIREDIRECTION))
		fprintf(otpfile, " %ld", d);
}


void OutputFile::Calcs(CalcType TYPE)
{
	long garbage;
	double Io, Ro;
	celldata cd;
	crowndata rd;
	grounddata gd;

	CellData(x, y, cd, rd, gd, &garbage);
	if (cd.c > 0)
	{
		ld.BaseConvert(rd.b);
		ld.DensityConvert(rd.p);
		c = 1;
		Io = pow(0.010 * ld.ld.base * (460.0 + 25.9 * GetFoliarMC()), 1.5);
		if (f >= Io)
		{
			c = 2;
			if (ld.ld.density > 0.0)
			{

        if(GetCrownFireCalculation()==0)  //AAA In FromG5 ver only
          Ro = 2.8 / ld.ld.density;     // close to the ro+0.9(rac-ro)

        else Ro=3.0/ld.ld.density;        //AAA In FromG5 ver only
				if (r >= Ro)
					c = 3;
			}
		}
	}
	else if (r > 1e-6)
		c = 1;

	switch (TYPE)
	{
  case FLAME_LENGTH: //AAA Was 0: in JAS!
		if (c > 1)
			l = ((0.2 * pow(f / 3.4613, 2.0 / 3.0)) / 3.2808) * convf2;		// fl for crown fire from Thomas in Rothermel 1991 converted to m
		else
			l = 0.0775 * pow(f, 0.46) * convf2;						// fl from Byram for surface fires
		break;
  case HEAT_PER_AREA: //AAA Was 1: in JAS!
		if (r > 1e-6)
			h = (60.0 * f / convf2) / (r * convf1);
		else
			h = 0.0;
		break; 	// just hpua
  case FL_AND_HPA: //AAA Was 2: in JAS!
		if (c > 1)
			l = ((0.2 * pow(f / 3.4613, 2.0 / 3.0)) / 3.2808) * convf2;		// fl for crown fire from Thomas in Rothermel 1991 converted to m
		else
			l = 0.0775 * pow(f, 0.46) * convf2;						// fl from Byram for surface fires
		if (r > 1e-6)
			h = (60.0 * f / convf2) / (r * convf1);
		else
			h = 0.0;
		break;
  case CROWNFIRE: //AAA Was 3: in JAS!
		/*CellData(x, y, cd, rd, gd, &garbage);
			if(cd.c>0)
			{    ld.BaseConvert(rd.b);
			 	ld.DensityConvert(rd.p);
			 	c=1;
			 	Io=pow(0.010*ld.ld.base* (460.0+25.9*GetFoliarMC()),1.5);
			 	if(f>=Io)
			 	{    c=2;
				 		if(ld.ld.density>0.0)
				 		{    Ro=2.8/ld.ld.density;     // close to the ro+0.9(rac-ro)
					  		if(r>=Ro)
								c=3;
				 		}
				  }
			 }
			 else
			 if(r>1e-6)
				  c=1;
			 */
		break;
  case REACTION_INTENSITY: //AAA Was 4: in JAS!
		rx /= convf3;
		break;
	default: 

	   ; // do nothing

	}
}


void OutputFile::ConvF()
{
	convf1 = convf2 = convf3 = 1.0;
	if (AccessOutputUnits(GETVAL))
	{
		convf1 = 3.4613;
		convf2 = 3.2808;
		convf3 = 0.18189275;
	}
}


//============================================================================
void OutputFile::WriteFile( long Type )
{ //OutputFile::WriteFile
  double xres, yres;
  char   RasterCopy[256] = "";
  double MetersToKm = 1.0;

  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfsxwfotp:OutputFile::WriteFile:1 Type=%ld\n",
            CallLevel, "", Type );

  GetRastRes( &xres, &yres );
  if( Verbose > CallLevel ) {
    printf( "%*sfsxwfotp:OutputFile::WriteFile:2 East=%lf West=%lf ",
            CallLevel, "", East, West );
    printf( "North=%lf South=%lf\n", North, South );
  }

  numcols = (long) ( (East - West) / xres );
  numrows = (long) ( (North - South) / yres );

  memset( RasterCopy, 0x0, sizeof RasterCopy );
  strcpy( RasterCopy, GetRasterFileName(Type) );

  if( CheckCellResUnits() == 2 ) MetersToKm = 0.001;

  otpfile = fopen( RasterCopy, "w" );

  if( otpfile == NULL ) {
    SetFileMode( RasterCopy, FILE_MODE_READ | FILE_MODE_WRITE );

    otpfile = fopen( RasterCopy, "w" );
  }

  switch( HeaderType ) {
    case 1:
      if( GetNorthUtm() == 0.0 || GetEastUtm() == 0.0 ) {
        if( Verbose > CallLevel )
          printf( "%*sfsxwfotp:OutputFile::WriteFile:1a "
                  "N=%lf S=%lf E=%lf W=%lf\n",
                  CallLevel, "", North, South, East, West );
        fprintf( otpfile, "%s    %lf\n", "north:", North );
        fprintf( otpfile, "%s    %lf\n", "south:", South );
        fprintf( otpfile, "%s     %lf\n", "east:", East );
        fprintf( otpfile, "%s     %lf\n", "west:", West );
      }
      else {
        if( Verbose > CallLevel )
          printf( "%*sfsxwfotp:OutputFile::WriteFile:1b "
                  "N=%lf S=%lf E=%lf W=%lf\n",
                  CallLevel, "", North, South, East, West );
        fprintf( otpfile, "%s    %lf\n", "north:",
                 ConvertNorthingOffsetToUtm(North) );
        fprintf( otpfile, "%s    %lf\n", "south:",
                 ConvertNorthingOffsetToUtm(South) );
        fprintf( otpfile, "%s     %lf\n", "east:",
                 ConvertEastingOffsetToUtm(East) );
        fprintf( otpfile, "%s     %lf\n", "west:",
                 ConvertEastingOffsetToUtm(West) );
      }

      fprintf( otpfile, "%s     %ld\n", "rows:", numrows );
      fprintf( otpfile, "%s     %ld\n", "cols:", numcols );
      break;

    case 5:
      fprintf( otpfile, "%s %ld\n", "NCOLS", numcols );
      fprintf( otpfile, "%s %ld\n", "NROWS", numrows );
      if( GetNorthUtm() == 0.0 || GetEastUtm() == 0.0 ) {
        fprintf( otpfile, "%s %lf\n", "XLLCORNER", West );
        fprintf( otpfile, "%s %lf\n", "YLLCORNER", South );
      }
      else {
        fprintf( otpfile, "%s %lf\n", "XLLCORNER",
                 ConvertEastingOffsetToUtm(West) );
        fprintf( otpfile, "%s %lf\n", "YLLCORNER",
                 ConvertNorthingOffsetToUtm(South) );
      }
      fprintf( otpfile, "%s %lf\n", "CELLSIZE", xres * MetersToKm );
      fprintf( otpfile, "%s %s\n", "NODATA_VALUE", "-1" );
      break;
  }

  //Write the actual data to the file.
  RastMemFile( Type );

  fclose( otpfile );

  if( Verbose > CallLevel )
    printf( "%*sfsxwfotp:OutputFile::WriteFile:3\n", CallLevel, "" );

  CallLevel--;
} //OutputFile::WriteFile


//============================================================================
void OutputFile::GetRasterExtent()
{ //OutputFile::GetRasterExtent
  CallLevel++;

  if( Verbose >= 6 )
    printf( "%*sfsxwfotp:OutputFile::GetRasterExtent:1\n", CallLevel, "" );

  if( CanSetRasterResolution(GETVAL) == 0 ) {
    CallLevel--;
    return;
  }

  if( SetRasterExtentToViewport(GETVAL) ) {
    if( Verbose >= 6 )
      printf( "%*sfsxwfotp:OutputFile::GetRasterExtent:1a\n", CallLevel, "" );
    North = GetViewNorth();
    South = GetViewSouth();
    East = GetViewEast();
    West = GetViewWest();
  }
  else {
    if( Verbose >= 6 )
      printf( "%*sfsxwfotp:OutputFile::GetRasterExtent:1b\n", CallLevel, "" );
    North = GetHiNorth();
    South = GetLoNorth();
    East = GetHiEast();
    West = GetLoEast();
  }

  CallLevel--;
} //OutputFile::GetRasterExtent



/*AAA In JAS! ver only below
long OutputFile::GetRasterFileLocation(long Type)
{
	//	long RR=0;
	long xpos;
	long ypos;
	long r_by_c;
	double xres, yres;

	GetRastRes(&xres, &yres);
	xpos = (long) ((x - West) / xres);
	ypos = (long) ((North - y) / yres);
	if (xpos < 0)
		xpos = 0;
	else if (xpos > numcols - 1)
		xpos = numcols - 1;
	if (ypos < 0)
		ypos = 0;
	else if (ypos > numrows - 1)
		ypos = numrows - 1;
	r_by_c = (xpos + ypos * numcols);
	if (Type == RAST_ARRIVALTIME ||
		Type == RAST_FIREINTENSITY ||
		Type == RAST_HEATPERAREA ||
		Type == RAST_REACTIONINTENSITY)
		r_by_c *= 11;//12;
	else if (Type == RAST_FLAMELENGTH || Type == RAST_SPREADRATE)
		r_by_c *= 7;//8;
	else				// crownfire or fire dirction azimuth, both integers
		r_by_c *= 5;
	r_by_c += filepos;    // add headerspace

	return r_by_c;
}


bool OutputFile::AllocRastData(long Num)
{
	FreeRastData();
	rd = new RastData[Num];

	if (rd == NULL)
		return false;

	NumRastAlloc = Num;

	return true;
}

bool OutputFile::ReallocRastData(long Num)
{
	RastData* newrast;

	newrast = rd;

	rd = 0;
	rd = new RastData[Num];

	if (rd == NULL)
		return false;

	memcpy(rd, newrast, NumRastAlloc * sizeof(RastData));
	delete[] newrast;
	NumRastAlloc = Num;

	return true;
}
AAA*/




void OutputFile::FreeRastData()
{
  //AAA JAS! ver only AAA if (rd) delete[] rd;
	//AAA rd = 0;  JAS! ver

  rd.clear();  //AAA FromG5 ver
	NumRastAlloc = 0;
	NumRastData = 0;
}




/**

 * \brief Aggregates the provided set of fire behavior characteristics onto

 * the raster.  

 * 

 * <p>

 * The client code supplies a location (xpt, ypt) expressed in real world 

 * coordinates (UTM), and the associated fire behavior characteristics (time, 

 * fli, ros, rcx, dir).  This method translates the location into a grid 

 * cell index (i,j) and stores the fire behavior data for that cell.  

 * </p>

 * 

 * <p>

 * In most cases, if the cell already contains a data value, the old value 

 * is simply overwritten by the new value.  Time of arrival is an exception 

 * to this rule.  The resultant value for the grid cell is the minimum of all

 * the time values assigned.

 * </p>

 */

//============================================================================
bool OutputFile::SetRastData( double xpt, double ypt, double time, double fli,
                              double ros, double rcx, long dir )
{ //OutputFile::SetRastData
  double xres, yres ; 
  coordinate tempxy ; 
  RastData temp ; 

  GetRastRes( &xres, &yres );

  //Compute the grid cell value.
  tempxy.first = (long) ( (xpt - West) / xres );
  tempxy.second = (long) ( (North - ypt) / yres );

  //Check to see if this cell already has an entry.
  RasterMap::iterator oldVal = rd.find(tempxy) ; 

  if( oldVal != rd.end() ) { 
    //Handle special cases where cell already has value.
    temp.Time = min( (oldVal->second).Time, time );
  }
  else {
    //Handle special cases where cell doesn't have value yet.
    temp.Time = time;
  }

  //Handle "standard" case where we blithely replace the value.
  temp.x = xpt;
  temp.y = ypt;
  temp.Fli = fli;
  temp.Ros = ros;
  temp.Rcx = rcx;
  temp.Dir = dir;

  //Add this to the map (if it was already in the map, tell where).
  if( oldVal != rd.end() ) {
    rd.insert( oldVal, std::make_pair(tempxy,temp) );
  }
  else {
    rd.insert( std::make_pair(tempxy,temp) );
  }

  NumRastData = rd.size();

  return true;
} //OutputFile::SetRastData
