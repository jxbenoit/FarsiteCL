/*============================================================================
  See LICENSE.TXT file for license information.
  ============================================================================
*/
#include<string.h>
#include"globals.h"
#include"fsxwignt.h"
#include"portablestrings.h"

//============================================================================
IgnitionFile::IgnitionFile() { }

//============================================================================
IgnitionFile::~IgnitionFile() { }

//============================================================================
bool IgnitionFile::ShapeInput() 
{ //IgnitionFile::ShapeInput
  SHPHandle hSHP;
  SHPObject* pSHP;
  int    nShapeType, nEntities;
  long   i, j, k, m, start, end, NewFire, NumPts, OldNumPts;
  double xpt, ypt;
  double xdiff, ydiff, midx, midy;
  double MinBound[4], MaxBound[4];

  hSHP = SHPOpen( ifile, "rb" );
  if( hSHP == NULL ) return false;

  SHPGetInfo( hSHP, &nEntities, &nShapeType, MinBound, MaxBound );
  for( i = 0; i < nEntities; i++ ) {
    pSHP = SHPReadObject( hSHP, i );
    if( pSHP->nVertices <= 0 ) {
      SHPDestroyObject( pSHP );
      continue;
    }

    NewFire = GetNewFires();
    switch( nShapeType ) {
      case SHPT_POINT:
      case SHPT_POINTZ:
      case SHPT_POINTM:
      case SHPT_MULTIPOINT:
      case SHPT_MULTIPOINTZ:
      case SHPT_MULTIPOINTM:
        for( j = 0; j < pSHP->nVertices; j++ ) {
          NewFire = GetNewFires();
          AllocPerimeter1( NewFire, 11 );
          CenterX = ConvertUtmToEastingOffset( pSHP->padfX[j] );
          CenterY = ConvertUtmToNorthingOffset( pSHP->padfY[j] );
          for( k = 0; k <= 9; k++ ) {
            angle = ( double (k) * (PI / 5.0) + PI / 2.0 );
            xpt = CenterX + ( cos(angle) );
            ypt = CenterY + ( sin(angle) );
            SetPerimeter1( GetNewFires(), k, xpt, ypt );
            SetFireChx( GetNewFires(), k, 0.0, 0.0 );
            SetReact( GetNewFires(), k, 0.0 );
          }
          SetNumPoints( GetNewFires(), 10 );
          SetInout( NewFire, 1 );
          IncNewFires( 1 );
          BoundingBox( NewFire );  //Find bounding box for line source
        }
        break;
      case SHPT_ARC:
      case SHPT_ARCZ:
      case SHPT_ARCM:
        for( m = 0; m < pSHP->nParts; m++ ) {
          NewFire = GetNewFires();
          start = pSHP->panPartStart[m];
          if( m < pSHP->nParts - 1 )
            end = pSHP->panPartStart[m + 1];
          else end = pSHP->nVertices;
          if( (end - start) == 2 ) {
            AllocPerimeter1( GetNewFires(), 5 );  //1 more for bounding box
            CenterX = ConvertUtmToEastingOffset( pSHP->padfX[0] );
            CenterY = ConvertUtmToNorthingOffset( pSHP->padfY[0] );
            xpt = ConvertUtmToEastingOffset( pSHP->padfX[1] );
            ypt = ConvertUtmToNorthingOffset( pSHP->padfY[1] );
            SetPerimeter1( GetNewFires(), 0, CenterX, CenterY );
            //100% of equilibrium spread.
            SetFireChx( GetNewFires(), 0, -1.0, 0.0 );
            SetReact( GetNewFires(), 0, 0.0 );
            SetPerimeter1( GetNewFires(), 2, xpt, ypt );
            //100% of equilibrium spread.
            SetFireChx( GetNewFires(), 2, -1.0, 0.0 );
            SetReact( GetNewFires(), 2, 0.0 );
            xdiff = CenterX - xpt;
            ydiff = CenterY - ypt;
            midx = CenterX - xdiff / 2.0;
            midy = CenterY - ydiff / 2.0;
            SetPerimeter1( GetNewFires(), 1, midx, midy );
            //100% of equilibrium spread.
            SetFireChx( GetNewFires(), 1, -1.0, 0.0 );
            SetReact( GetNewFires(), 1, 0.0 );
            SetPerimeter1( GetNewFires(), 3, midx, midy );
            //100% of equilibrium spread.
            SetFireChx( GetNewFires(), 3, -1.0, 0.0 );
            SetReact( GetNewFires(), 3, 0.0 );
            SetNumPoints( GetNewFires(), 4 );
          }
          else if( (end - start) > 0 ) {
            AllocPerimeter1( GetNewFires(), 2 * (end - start) - 1 );
            count2 = 2 * ( end - start ) - 3;
            for( j = start; j < end; j++ ) {
              CenterX = ConvertUtmToEastingOffset( pSHP->padfX[j] );
              CenterY = ConvertUtmToNorthingOffset( pSHP->padfY[j] );
              SetPerimeter1( GetNewFires(), j - start, CenterX, CenterY );
              //100% of equilibrium spread.
              SetFireChx( GetNewFires(), j - start, -1.0, 0.0 );
              SetReact( GetNewFires(), j - start, 0.0 );
              //write points backward in array.
              if( j > start && k < end - 1 ) {
                SetPerimeter1( GetNewFires(), count2, CenterX, CenterY );
                //100% of equilibrium spread.
                SetFireChx( GetNewFires(), count2, -1.0, 0.0 );
                SetReact( GetNewFires(), count2, 0.0 );
                --count2;
              }
            }
            SetNumPoints( GetNewFires(), 2 * (end - start) - 2 );
          }
          SetInout( GetNewFires(), 1 );
          IncNewFires( 1 );
          BoundingBox( NewFire );  //Find bounding box for line source
        }
        break;
      case SHPT_POLYGON:
      case SHPT_POLYGONZ:
      case SHPT_POLYGONM:
      case SHPT_MULTIPATCH:
        for( m = 0; m < pSHP->nParts; m++ ) {
          NewFire = GetNewFires();
          start = pSHP->panPartStart[m];
          if( m < pSHP->nParts - 1 )
            end = pSHP->panPartStart[m + 1];
          else end = pSHP->nVertices;
          if( (end - start) <= 0 ) continue;
          AllocPerimeter1( NewFire, (end - start) + 1 );
          for( j = start; j < end; j++ ) {
            xpt = ConvertUtmToEastingOffset( pSHP->padfX[j] );
            ypt = ConvertUtmToNorthingOffset( pSHP->padfY[j] );
            SetPerimeter1( NewFire, j - start, xpt, ypt );
            SetFireChx( NewFire, j - start, -1.0, 0.0 );
            SetReact( NewFire, j - start, 0.0 );
          }
          SetNumPoints( NewFire, end - start );
          SetInout( NewFire, 1 );
          IncNewFires( 1 );
          if( arp() < 0 ) ReversePoints( 1 );
          BoundingBox( NewFire );  //Find bounding box for line source
        }
        break;
    }
    SHPDestroyObject( pSHP );
    RemoveIdenticalPoints( NewFire );

    xpt = GetPerimeter1Value( NewFire, GetNumPoints(NewFire), XCOORD );
    ypt = GetPerimeter1Value( NewFire, GetNumPoints(NewFire), YCOORD );
    midx = GetPerimeter1Value( NewFire, GetNumPoints(NewFire), ROSVAL );
    midy = GetPerimeter1Value( NewFire, GetNumPoints(NewFire), FLIVAL );
    do {
      OldNumPts = GetNumPoints( NewFire );
      DensityControl( NewFire );
      NumPts = GetNumPoints( NewFire );
      FreePerimeter1( NewFire );
      AllocPerimeter1( NewFire, NumPts + 1 );
      SwapFirePerims( NewFire, -NumPts );
    } while( NumPts > OldNumPts );
    SetPerimeter1( NewFire, NumPts, xpt, ypt );  //Replace bounding box
    SetFireChx( NewFire, NumPts, midx, midy );
    SetReact( NewFire, NumPts, 0.0 );
  }
  SHPClose( hSHP );

  return true;
} //IgnitionFile::ShapeInput

//============================================================================
bool IgnitionFile::GrassFile() 
{ //IgnitionFile::GrassFile
  char   Head[30];
  long   count, NewFire, OldNumPts, NumPts;
  double xpt, ypt;
  double xdiff, ydiff, midx, midy;

  do {
    fscanf( IFile, "%s", Head );
  } while( strcmp(Head, "VERTI:") );
  while( ! feof(IFile) ) {
    fscanf( IFile, "%s %li", Head, &NumVertex );
    NewFire = GetNewFires();
    if( ! strcmp(strupr(Head), "A") ) {
      AllocPerimeter1( GetNewFires(), NumVertex + 1 );
      for( count = 0; count < NumVertex; count++ ) {
        fscanf( IFile, "%lf %lf", &ypt, &xpt );  //Grass reverses x & y
        xpt = ConvertUtmToEastingOffset( xpt );
        ypt = ConvertUtmToNorthingOffset( ypt );
        SetPerimeter1( GetNewFires(), count, xpt, ypt );
        SetFireChx( GetNewFires(), count, -1.0, 0.0 );
        SetReact( GetNewFires(), count, 0.0 );
      }
      SetNumPoints( GetNewFires(), count );
      SetInout( GetNewFires(), 1 );
      IncNewFires( 1 );
      if( arp() < 0 ) ReversePoints( 1 );
      BoundingBox( GetNewFires() - 1 );  //Find bounding box for line source
    }
    else if( ! strcmp(strupr(Head), "L") ) {
      if( NumVertex == 2 ) {
        AllocPerimeter1( GetNewFires(), 5 );  //1 more for bounding box
        fscanf( IFile, "%lf %lf", &CenterY, &CenterX );
        fscanf( IFile, "%lf %lf", &ypt, &xpt );
        CenterX = ConvertUtmToEastingOffset( CenterX );
        CenterY = ConvertUtmToNorthingOffset( CenterY );
        xpt = ConvertUtmToEastingOffset( xpt );
        ypt = ConvertUtmToNorthingOffset( ypt );
        SetPerimeter1( GetNewFires(), 0, CenterX, CenterY );
        //100% of equilibrium spread.
        SetFireChx( GetNewFires(), 0, -1.0, 0.0 );
        SetReact( GetNewFires(), 0, 0.0 );
        SetPerimeter1( GetNewFires(), 2, xpt, ypt );
        //100% of equilibrium spread.
        SetFireChx( GetNewFires(), 2, -1.0, 0.0 );
        SetReact( GetNewFires(), 2, 0.0 );
        xdiff = CenterX - xpt;
        ydiff = CenterY - ypt;
        midx = CenterX - xdiff / 2.0;
        midy = CenterY - ydiff / 2.0;
        SetPerimeter1( GetNewFires(), 1, midx, midy );
        //100% of equilibrium spread.
        SetFireChx( GetNewFires(), 1, -1.0, 0.0 );
        SetReact( GetNewFires(), 1, 0.0 );
        SetPerimeter1( GetNewFires(), 3, midx, midy );
        //100% of equilibrium spread.
        SetFireChx( GetNewFires(), 3, -1.0, 0.0 );
        SetReact( GetNewFires(), 3, 0.0 );
        SetNumPoints( GetNewFires(), 4 );
      }
      else {
        //1 more for bounding box.
        AllocPerimeter1( GetNewFires(), 2 * NumVertex - 1 );
        count2 = 2 * NumVertex - 3;
        for( count = 0; count < NumVertex; count++ ) {
          fscanf( IFile, "%lf %lf", &CenterY, &CenterX );
          CenterX = ConvertUtmToEastingOffset( CenterX );
          CenterY = ConvertUtmToNorthingOffset( CenterY );
          SetPerimeter1( GetNewFires(), count, CenterX, CenterY );
          //100% of equilibrium spread.
          SetFireChx( GetNewFires(), count, -1.0, 0.0 );
          SetReact( GetNewFires(), count, 0.0 );
          //Write points backward in array.
          if( count > 0 && count < NumVertex - 1 ) {  
            SetPerimeter1( GetNewFires(), count2, CenterX, CenterY );
            //100% of equilibrium spread.
            SetFireChx( GetNewFires(), count2, -1.0, 0.0 );
            SetReact( GetNewFires(), count2, 0.0 );
            --count2;
          }
        }
        SetNumPoints( GetNewFires(), 2 * NumVertex - 2 );
      }
      SetInout( GetNewFires(), 1 );
      IncNewFires( 1 );
      BoundingBox( GetNewFires() - 1 );  //Find bounding box for line source
      fscanf( IFile, "%s", TestEnd );
    }
    RemoveIdenticalPoints( NewFire );
    xpt = GetPerimeter1Value( NewFire, GetNumPoints(NewFire), XCOORD );
    ypt = GetPerimeter1Value( NewFire, GetNumPoints(NewFire), YCOORD );
    midx = GetPerimeter1Value( NewFire, GetNumPoints(NewFire), ROSVAL );
    midy = GetPerimeter1Value( NewFire, GetNumPoints(NewFire), FLIVAL );
    do {
      OldNumPts = GetNumPoints( NewFire );
      DensityControl( NewFire );
      NumPts = GetNumPoints( NewFire );
      FreePerimeter1( NewFire );
      AllocPerimeter1( NewFire, NumPts + 1 );
      SwapFirePerims( NewFire, -NumPts );
    } while( NumPts > OldNumPts );
    SetPerimeter1( NewFire, NumPts, xpt, ypt );  //Replace bounding box
    SetFireChx( NewFire, NumPts, midx, midy );
    SetReact( NewFire, NumPts, 0.0 );
  }

  return true;
} //IgnitionFile::GrassFile

//============================================================================
bool IgnitionFile::ArcLine() 
{ //IgnitionFile::ArcLine
  long   OldFires, NewFire, NumPts, OldNumPts;
  double xdiff, ydiff, midx, midy;

  OldFires = NewFire = GetNewFires();
  fscanf( IFile, "%s", TestEnd );  //Get past label location
  do {
    NewFire = GetNewFires();
    fposition1 = ftell( IFile );
    NumVertex = 0;
    do {
      fscanf( IFile, "%s", TestEnd );
      if( ! strcmp(TestEnd, "END") ) break;
      fscanf( IFile, "%lf", &CenterY );
      NumVertex++;
    } while( strcmp(TestEnd, "END") );
    if( NumVertex == 2 ) {
      fseek( IFile, fposition1, SEEK_SET );
      AllocPerimeter1( GetNewFires(), 5 );  //1 more for bounding box
      fscanf( IFile, "%lf %lf", &CenterX, &CenterY );
      fscanf( IFile, "%lf %lf", &xpt, &ypt );
      CenterX = ConvertUtmToEastingOffset( CenterX );
      CenterY = ConvertUtmToNorthingOffset( CenterY );
      xpt = ConvertUtmToEastingOffset( xpt );
      ypt = ConvertUtmToNorthingOffset( ypt );
      SetPerimeter1( GetNewFires(), 0, CenterX, CenterY );
      //100% of equilibrium spread.
      SetFireChx( GetNewFires(), 0, -1.0, 0.0 );
      SetReact( GetNewFires(), 0, 0.0 );
      SetPerimeter1( GetNewFires(), 2, xpt, ypt );
      //100% of equilibrium spread.
      SetFireChx( GetNewFires(), 2, -1.0, 0.0 );
      SetReact( GetNewFires(), 2, 0.0 );
      xdiff = CenterX - xpt;
      ydiff = CenterY - ypt;
      midx = CenterX - xdiff / 2.0;
      midy = CenterY - ydiff / 2.0;
      SetPerimeter1( GetNewFires(), 1, midx, midy );
      //100% of equilibrium spread.
      SetFireChx( GetNewFires(), 1, -1.0, 0.0 );
      SetReact( GetNewFires(), 1, 0.0 );
      SetPerimeter1( GetNewFires(), 3, midx, midy );
      //100% of equilibrium spread.
      SetFireChx( GetNewFires(), 3, -1.0, 0.0 );
      SetReact( GetNewFires(), 3, 0.0 );
      count = 3;
      SetNumPoints( GetNewFires(), 4 );
      RemoveIdenticalPoints( NewFire );
    }
    else if( NumVertex > 1 ) {
      fseek( IFile, fposition1, SEEK_SET );
      //1 more for bounding box.
      AllocPerimeter1( GetNewFires(), 2 * NumVertex - 1 );
      count2 = 2 * NumVertex - 3;
      for( count = 0; count < NumVertex; count++ ) {
        fscanf( IFile, "%lf %lf", &CenterX, &CenterY );
        CenterX = ConvertUtmToEastingOffset( CenterX );
        CenterY = ConvertUtmToNorthingOffset( CenterY );
        SetPerimeter1( GetNewFires(), count, CenterX, CenterY );
        //100% of equilibrium spread.
        SetFireChx( GetNewFires(), count, -1.0, 0.0 );
        SetReact( GetNewFires(), count, 0.0 );
        //Write points backward in array.
        if( count > 0 && count < NumVertex - 1 ) {
          SetPerimeter1( GetNewFires(), count2, CenterX, CenterY );
          //100% of equilibrium spread.
          SetFireChx( GetNewFires(), count2, -1.0, 0.0 );
          SetReact( GetNewFires(), count2, 0.0 );
          --count2;
        }
      }
      fscanf( IFile, "%s", TestEnd );
      SetNumPoints( GetNewFires(), 2 * NumVertex - 2 );
      RemoveIdenticalPoints( NewFire );
    }
    if( NumVertex > 1 ) {
      SetInout( GetNewFires(), 1 );
      IncNewFires(1);
      do {
        OldNumPts = GetNumPoints( NewFire );
        DensityControl( NewFire );
        NumPts = GetNumPoints( NewFire );
        FreePerimeter1( NewFire );
        AllocPerimeter1( NewFire, NumPts + 1 );
        SwapFirePerims( NewFire, -NumPts );
      } while( NumPts > OldNumPts );
      BoundingBox( NewFire );
    }  //Find bounding box for line source
    fscanf( IFile, "%s", TestEnd );
  } while( strcmp(TestEnd, "END") );
  if( OldFires == GetNewFires() )
    printf( "Line Ignition Contains only One Point Ignition NOT Imported\n" );

  return true;
} //IgnitionFile::ArcLine

/*============================================================================
  ReadLine
  Reads a line from the given file stream. It reads characters until it finds
  a newline, it hits the end-of-file, or the MaxLen limit is reached
  (see fgets() documentation). If it reaches the end of a line, the newline
  character WILL be included in the string.
  Returns true if an entire line has been read. The function returns false if
  a line was only partially read in (we simply read to the MaxLen limit).
  Note that MaxLen must take into account the null character appended to the
  end of the string.
*/
bool ReadLine( FILE *F, char *s, int MaxLen )
{ //ReadLine
  fgets( s, MaxLen, F );
  int n = strlen( s );

  if( n == MaxLen - 1 )
    if( s[n-1] != '\n' && s[n-1] != '\r' ) {
      s[n-1] = 0; //Chop off newline
      return false;
    }

  s[n-1] = 0; //Chop off newline

  return true;
} //ReadLine

/*============================================================================
  IsBlankLine
*/
bool IsBlankLine( char *s )
{ //IsBlankLine
  char *p = s;

  while( *p != 0 ) {
    if( *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' ) return false;
    ++*p;
  }

  return true;
} //IsBlankLine

/*============================================================================
  CountNumbersInStr
  Returns the number of numbers (int, long, float, double, etc.) in the
  string. The numbers can be separated by space, comma, or other non-number
  characters.
*/
long CountNumbersInStr( char *s )
{ //CountNumbersInStr
  char *p = s;
  long count = 0;

  while( *p != 0 ) {
    bool dec = false;

    if( (*p >= '0' && *p <= '9') || *p == '-' || *p == '.' ) {
      if( *p == '.' ) dec = true;

      ++p;
      while( (*p >= '0' && *p <= '9') || *p == '.' ) {
        if( *p == '.' ) {
          if( dec ) break;
          dec = true;
        }
        ++p;
      }

      count++;
    }
    else ++p;
  }

  return count;
} //CountNumbersInStr

/*============================================================================
  ReadDoubleFromStr
  Reads a number from a string & returns it in f.
  Returns a pointer to the position in the string AFTER the number read. This
  is so that successive calls to this function can be made on the same string.
  If pointer **p == *s upon return, no number was readable from the string.
*/
double ReadDoubleFromStr( char *s, char **p )
{ //ReadDoubleFromStr
  double f = 0;
  bool   NumberFound = false;

  *p = s;

  while( **p != 0 ) {
    bool dec = false;

    if( (**p >= '0' && **p <= '9') || **p == '-' || **p == '.' ) {
      if( **p == '.' ) dec = true;

      char *q = *p;
      ++q;
      while( ( (*q >= '0' && *q <= '9') || *q == '.' ) && *q != 0 ) {
        if( *q == '.' ) {
          if( dec ) break;  //Already found a decimal at start of number
          dec = true;
        }
        ++q;
      }

      char *s0 = new char [ q-*p+1 ];
      s0 = strncpy( s0, *p, q-*p );
      s0[q-*p] = 0;
      f = atof( s0 );
      delete [] s0;

      *p = q;

      NumberFound = true;

      break;
    }
    else ++*p;

  }

  if( ! NumberFound ) *p = s; //Reset *p so we know no number was found

  return f;
} //ReadDoubleFromStr

/*============================================================================
  IgnitionFile::ArcPoly
  Reads in points from an ESRI Ungenerate file.
  This version is hopefully a little more robust than the original:
   * It should allow for commas or just whitespace between numbers.
   * More than one pair of numbers can appear in a line, or only one number
     can appear on a line (normally, lines in the Ungenerate file are
     coordinate-pairs).
   * The END statements at the end of the file can be lowercase.
   * Blank lines can appear in the file -- they will be skipped.
   * More error-checking has been added.
*/
bool IgnitionFile::ArcPoly()
{ //IgnitionFile::ArcPoly
  CallLevel++;

  long OldFires, NumPts, NewFire, OldNumPts;
  int  TestEndLen = 30;

  OldFires = NewFire = GetNewFires();
  do {
    if( ! ReadLine(IFile, TestEnd, TestEndLen) )
      printf( "## fsxwignt:IgnitionFile::ArcPoly: Line too long in file "
              "%s: %s ##\n", ifile, TestEnd );
  } while( IsBlankLine(TestEnd) && ! feof(IFile) );

  if( IsBlankLine(TestEnd) )
    printf( "## fsxwignt:IgnitionFile::ArcPoly: File %s is empty ##\n",
            ifile );

  if( Verbose > CallLevel )
    printf( "%*sfsxwignt:IgnitionFile::ArcPoly:1 %s\n",
            CallLevel, "", TestEnd );

  char *p0 = NULL, *p1 = NULL;
  char s[100];
  do {
    NewFire = GetNewFires();
    fposition1 = ftell( IFile );
    NumVertex = 0;

    int RemainderFromPrevLine = 0;
    do {
      //Read in the next non-blank line.
      do {
        if( ! ReadLine(IFile, TestEnd, TestEndLen) )
          printf( "## fsxwignt:IgnitionFile::ArcPoly: Line too long in file "
                  "%s: %s ##\n", ifile, TestEnd );
      } while( IsBlankLine(TestEnd) && ! feof(IFile) );

      strupr( TestEnd );
      if( ! strcmp(TestEnd, "END") ) break;

      long n = CountNumbersInStr( TestEnd );
      if( n > 1 ) NumVertex += ( n / 2 + RemainderFromPrevLine );
      RemainderFromPrevLine = n % 2;
    } while( strcmp(TestEnd, "END") && ! feof(IFile) );

    if( strcmp(TestEnd, "END") )
      printf( "## fsxwignt:IgnitionFile::ArcPoly: No END statement found in "
              "file %s ##\n", ifile );

    if( NumVertex > 2 ) {
      fseek( IFile, fposition1, SEEK_SET );
      //1 more for bounding box
      AllocPerimeter1( GetNewFires(), NumVertex + 1 );

      bool bReadNextLine = true;
      for( count = 0; count < NumVertex; count++ ) {

        //Find the next pair of numbers in the file for CenterX & CenterY.
        bool CenterXFound = false, CenterYFound = false;
        do {
          //Check if the next line in the file should be read. Otherwise,
          //the contents of string buffer s from previous iteration will be
          //scanned for more numbers.
          if( bReadNextLine ) {
            //Read in the next non-blank line; put contents in s.
            do {
              if( ! ReadLine(IFile, s, 100) )
                printf( "## fsxwignt:IgnitionFile::ArcPoly: Line too long in "
                        "file %s: %s ##\n", ifile, s );
            } while( IsBlankLine(s) );

            p0 = s;                //p0 points to start of s
            bReadNextLine = false;
          }

          if( ! CenterXFound ) {
            CenterX = ReadDoubleFromStr( p0, &p1 );
            if( p1 != p0 ) {
              CenterXFound = true;
              p0 = p1;
            }
            else bReadNextLine = true;
          }

          if( ! bReadNextLine ) {
            CenterY = ReadDoubleFromStr( p0, &p1 );
            if( p1 != p0 ) {
              CenterYFound = true;
              p0 = p1;
            }
            else bReadNextLine = true;
          }
        } while( (! CenterXFound || ! CenterYFound) && ! feof(IFile) );
        //Done reading CenterX & CenterY.

        if( feof(IFile) && count != NumVertex )
          printf( "## fsxwignt:IgnitionFile::ArcPoly: Discrepancy between "
                  "number of vertexes counted and read in file %s: %ld vs. "
                  "%ld ##\n", ifile, NumVertex, count );

        if( Verbose > CallLevel )
            printf( "%*sfsxwignt:IgnitionFile::ArcPoly:1 a Read point "
                    "(%lf, %lf)\n", CallLevel, "", CenterX, CenterY );

        CenterX = ConvertUtmToEastingOffset( CenterX );
        CenterY = ConvertUtmToNorthingOffset( CenterY );
        SetPerimeter1( GetNewFires(), count, CenterX, CenterY );
        //100% of equilibrium spread
        SetFireChx( GetNewFires(), count, -1.0, 0.0 );
        SetReact( GetNewFires(), count, 0.0 );
      }

      //Read in the next non-blank line. This *should* be an END statement.
      do {
        if( ! ReadLine(IFile, TestEnd, TestEndLen) )
          printf( "## fsxwignt:IgnitionFile::ArcPoly: Line too long in file "
                  "%s: %s ##\n", ifile, TestEnd );
      } while( IsBlankLine(TestEnd) && ! feof(IFile) );

      SetNumPoints( GetNewFires(), NumVertex );
      SetInout( GetNewFires(), 1 );
      IncNewFires( 1 );
      RemoveIdenticalPoints( NewFire );
      if( arp() < 0 ) ReversePoints( 1 );

      do {
        OldNumPts = GetNumPoints( NewFire );
        DensityControl( NewFire );
        NumPts = GetNumPoints( NewFire );
        FreePerimeter1( NewFire );
        AllocPerimeter1( NewFire, NumPts + 1 );
        SwapFirePerims( NewFire, -NumPts );
      } while( NumPts > OldNumPts );
      BoundingBox( NewFire );
    }  //Find bounding box for line source

    //Read in the next non-blank line.
    do {
      if( ! ReadLine(IFile, TestEnd, TestEndLen) )
        printf( "## fsxwignt:IgnitionFile::ArcPoly: Line too long in file "
                "%s: %s ##\n", ifile, TestEnd );
    } while( IsBlankLine(TestEnd) && ! feof(IFile) );

    strupr( TestEnd );
  } while( strcmp(TestEnd, "END") && ! feof(IFile) );

  if( strcmp(TestEnd, "END") )
    printf( "## fsxwignt:IgnitionFile::ArcPoly: No final END statement found "
            "in file %s ##\n", ifile );

  if( OldFires == GetNewFires() )
    printf( "## Polygon Ignition Contains only One Point! "
            "Ignition NOT Imported ##\n" );

  CallLevel--;

  return true;
} //IgnitionFile::ArcPoly

//============================================================================
bool IgnitionFile::ArcPoint() 
{ //IgnitionFile::ArcPoint
  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sfsxwignt:IgnitionFile::ArcPoint:1\n", CallLevel, "" );

  while( ! feof(IFile) ) {
    fscanf( IFile, "%s", TestEnd );
    if( strcmp(TestEnd, "END") ) {
      fscanf( IFile, "%lf %lf", &CenterX, &CenterY );
      CenterX = ConvertUtmToEastingOffset( CenterX );
      CenterY = ConvertUtmToNorthingOffset( CenterY );
      //10 points, 11th is for bounding box.
      AllocPerimeter1( GetNewFires(), 11 );
      for( count = 0; count <= 9; count++ ) {
        angle = ( double (count) * (PI / 5.0) ) + PI / 2.0;
        xpt = CenterX + cos( angle );
        ypt = CenterY + sin( angle );
        SetPerimeter1( GetNewFires(), count, xpt, ypt );
        SetFireChx( GetNewFires(), count, 0.0, 0.0 );
        SetReact( GetNewFires(), count, 0.0 );
      }
      SetNumPoints( GetNewFires(), count );
      SetInout( GetNewFires(), 1 );
      IncNewFires( 1 );
      BoundingBox( GetNewFires() - 1 );
      fscanf( IFile, "%s", TestEnd );
    }
  }

  if( Verbose > CallLevel )
    printf( "%*sfsxwignt:IgnitionFile::ArcPoint:2\n", CallLevel, "" );

  CallLevel--;

  return true;
} //IgnitionFile::ArcPoint

//============================================================================
IgnitionCorrect::IgnitionCorrect() { }

//============================================================================
IgnitionCorrect::~IgnitionCorrect() { }

//============================================================================
void IgnitionCorrect::ReversePoints( long TYPE )
{ //IgnitionCorrect::ReversePoints
  long   j, AFire = GetNewFires() - 1;
  long   count, BFire = GetNewFires();
  double fxc, fyc, fxc2, fyc2, RosI, RosI2;
  long   halfstop = GetNumPoints( AFire ) / 2;  //Truncated number of points

  switch( TYPE ) {
    case 0:
      //Transfer points in reverse to next array, +1 fires.
      AllocPerimeter1( BFire, GetNumPoints(AFire) );  //Allocate new array
      for( count = 0; count < GetNumPoints(AFire); count++ ) {
        fxc = GetPerimeter1Value( AFire, count, 0 );
        fyc = GetPerimeter1Value( AFire, count, 1 );
        RosI = GetPerimeter1Value( AFire, count, 2 );
        j = GetNumPoints( AFire ) - ( count + 1 );
        SetPerimeter1( BFire, j, fxc, fyc );  //Reverse points
        SetFireChx( BFire, j, RosI, 0 );  //In next available array
        SetReact( BFire, j, 0.0 );
      }    //Set bounding box
      //Reverse points in next available array.
      SetPerimeter1( BFire, count, GetPerimeter1Value(AFire, count, 0),
                     GetPerimeter1Value(AFire, count, 1) );
      SetFireChx( BFire, count, GetPerimeter1Value(AFire, count, 2),
                  GetPerimeter1Value(AFire, count, 3) );
      SetReact( BFire, count, 0.0 );
      SetNumPoints( BFire, count );  //Same number of points in fire
      SetInout( BFire, 1 );  //ID fire as outward burning
      IncNewFires(1);
      break;
    case 1:
      //Reverse points in same array.
      for( count = 0; count < halfstop; count++ ) {
        j = GetNumPoints( AFire ) - count - 1;
        fxc = GetPerimeter1Value( AFire, count, 0 );
        fyc = GetPerimeter1Value( AFire, count, 1 );
        RosI = GetPerimeter1Value( AFire, count, 2 );
        fxc2 = GetPerimeter1Value( AFire, j, 0 );
        fyc2 = GetPerimeter1Value( AFire, j, 1 );
        RosI2 = GetPerimeter1Value( AFire, j, 2 );
        SetPerimeter1( AFire, count, fxc2, fyc2 );
        SetFireChx( AFire, count, RosI2, 0 );
        SetReact( AFire, j, 0.0 );
        SetPerimeter1( AFire, j, fxc, fyc );
        SetFireChx( AFire, j, RosI, 0 );
        SetReact( AFire, j, 0.0 );
      }
      break;
  }
} //IgnitionCorrect::ReversePoints

/*============================================================================
  IgnitionCorrect::arp
  Calculates area and perimeter as a planimeter (with triangles).
*/
double IgnitionCorrect::arp()
{ //IgnitionCorrect::arp
  long   count, count1 = 0, FireNum = GetNewFires() - 1, numx;
  double xpt1, ypt1, xpt2, ypt2, aangle, zangle, DiffAngle;
  double newarea, area = 0.0;

  numx = GetNumPoints( FireNum );
  if( numx < 3 ) return area;

  startx = GetPerimeter1Value( FireNum, 0, 0 );
  starty = GetPerimeter1Value( FireNum, 0, 1 );
  while( count1 < numx ) {    //Make sure that startx,starty!=x[0]y[0]
    count1++;
    xpt1 = GetPerimeter1Value( FireNum, count1, 0 );
    ypt1 = GetPerimeter1Value( FireNum, count1, 1 );
    zangle = direction( xpt1, ypt1 );
    if( zangle != 999.9 ) break;
  }
  count1++;
  for( count = count1; count < numx; count++ ) {
    xpt2 = GetPerimeter1Value( FireNum, count, 0 );
    ypt2 = GetPerimeter1Value( FireNum, count, 1 );
    newarea = .5 * ( startx * ypt1 -
                     xpt1 * starty +
                     xpt1 * ypt2 -
                     xpt2 * ypt1 +
                     xpt2 * starty -
                     startx * ypt2 );
    newarea = fabs( newarea );
    aangle = direction( xpt2, ypt2 );
    if( aangle != 999.9 ) {
      DiffAngle = aangle - zangle;
      if( DiffAngle > PI ) DiffAngle = -( 2.0 * PI - DiffAngle );
      else if( DiffAngle < -PI ) DiffAngle = ( 2.0 * PI + DiffAngle );
      if( DiffAngle > 0.0 ) area -= newarea;
      else if( DiffAngle < 0.0 ) area += newarea;
      zangle = aangle;
    }
    xpt1 = xpt2;
    ypt1 = ypt2;
  }

  return area;
} //IgnitionCorrect::arp

//============================================================================
void StandardizePolygon::RemoveIdenticalPoints( long NumFire )
{ //StandardizePolygon::RemoveIdenticalPoints
  long   i, j, k, NumPts;
  double xpt, ypt, xptn, yptn, xn, yn;
  double xdiff, ydiff, dist, offset;
  double* perimeter1;

  NumPts = GetNumPoints( NumFire );
  for( i = 0; i < NumPts; i++ ) {
    xpt = GetPerimeter1Value( NumFire, i, XCOORD );
    ypt = GetPerimeter1Value( NumFire, i, YCOORD );
    for( j = 0; j < NumPts; j++ ) {
      if( i == j ) continue;
      xptn = GetPerimeter1Value( NumFire, j, XCOORD );
      yptn = GetPerimeter1Value( NumFire, j, YCOORD );
      dist = sqrt( pow2(xpt - xptn) + pow2(ypt - yptn) );
      if( dist < 1e-9 ) {
        k = j + 1;
        if( k > NumPts - 1 ) k = 0;
        xn = GetPerimeter1Value( NumFire, k, XCOORD );
        yn = GetPerimeter1Value( NumFire, k, YCOORD );
        xdiff = xptn - xn;
        ydiff = yptn - yn;
        dist = sqrt( pow2(xdiff) + pow2(ydiff) );
        if( dist < 1e-9 ) {
          perimeter1 = GetPerimeter1Address( NumFire, 0 );
          memcpy( &perimeter1[j * NUMDATA],
                  &perimeter1[(j + 1) * NUMDATA],
                  (NumPts - j) * NUMDATA * sizeof(double) );
          NumPts--;
        }
        else {
          offset = dist * 0.01;
          if( offset > 0.1 ) offset = 0.1;
          xptn = xptn - offset * xdiff / dist;
          yptn = yptn - offset * ydiff / dist;
          SetPerimeter1( NumFire, j, xptn, yptn );
        }
      }
    }
  }
  if( NumPts < GetNumPoints(NumFire) ) SetNumPoints( NumFire, NumPts );
} //StandardizePolygon::RemoveIdenticalPoints

/*============================================================================
  IgnitionFile::SelectFileInputCmdL
  This is processed from the CmdLine for right now
   * - 1 is ArcPoly
   * - 2 is ArcLine
   * - 3 is ArcPoint
  ============================================================================
*/
void IgnitionFile::SelectFileInputCmdL( int type )
{ //IgnitionFile::SelectFileInputCmdL
  CallLevel++;

  if( Verbose > CallLevel )
    printf( "%*sIgnitionFile::SelectFileInputCmdL:1\n", CallLevel, "" );

  char Test[5] = "";
  long len = strlen( ifile );

  Test[0] = ifile[len - 4];
  Test[1] = ifile[len - 3];
  Test[2] = ifile[len - 2];
  Test[3] = ifile[len - 1];
  if( ! strcmp(strlwr(Test), ".shp") ) ShapeInput();
  else if( (IFile = fopen(ifile, "r")) != NULL ) {
    fscanf( IFile, "%s", TestEnd );
    rewind( IFile );
    if( ! strcmp(strupr(TestEnd), "ORGANIZATION:") ) GrassFile();
    else {
      if( type == 1 ) ArcPoly();
      else if( type == 2 ) ArcLine();
      else ArcPoint();
    }
    fclose( IFile );
  }
  else printf( "## Cannot open Ignition File %s for reading ##", ifile );

  if( Verbose > CallLevel )
    printf( "%*sIgnitionFile::SelectFileInputCmdL:2\n", CallLevel, "" );


  CallLevel--;
} //IgnitionFile::SelectFileInputCmdL
