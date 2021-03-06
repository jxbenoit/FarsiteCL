#include<fstream>
#include<cstring>
#include<string>
#include<stdio.h>
#include"LCPAnalyzer.h"

using namespace std;

//----------------------------------------------------------------------------
LCPAnalyzer::LCPAnalyzer()
{ //LCPAnalyzer::LCPAnalyzer
  FileName = "";
  Analyzed = false;  //Flag telling if file was analyzed
  HeaderSize = NumErrors = 0;
  CalculatedHeaderSizeGood = false;
  CalculatedDataSizeGood = false;
} //LCPAnalyzer::LCPAnalyzer

//----------------------------------------------------------------------------
LCPAnalyzer::LCPAnalyzer( const char * FileName )
{ //LCPAnalyzer::LCPAnalyzer
  this->FileName = FileName;
  Analyzed = false;  //Flag telling if file was analyzed
  HeaderSize = NumErrors = 0;
  CalculatedHeaderSizeGood = false;
  CalculatedDataSizeGood = false;
} //LCPAnalyzer::LCPAnalyzer

//----------------------------------------------------------------------------
LCPAnalyzer::~LCPAnalyzer()
{ //LCPAnalyzer::~LCPAnalyzer
  LCP.close();
} //LCPAnalyzer::~LCPAnalyzer

//----------------------------------------------------------------------------
void LCPAnalyzer::SetFileName( const char * FileName )
{ //LCPAnalyzer::SetFileName
  this->FileName = FileName;
  Analyzed = false;  //Flag telling if file was analyzed
  HeaderSize = NumErrors = 0;
  CalculatedHeaderSizeGood = false;
  CalculatedDataSizeGood = false;
} //LCPAnalyzer::SetFileName

//----------------------------------------------------------------------------
bool LCPAnalyzer::Analyze()
{ //LCPAnalyzer::Analyze
  if( FileName.length() == 0 ) return false;
  Analyze( FileName.c_str() );
  return true;
} //LCPAnalyzer::Analyze

/*----------------------------------------------------------------------------
  LCPAnalyzer::Analyze
  Inspect a landscape file (.LCP) to find its header size and where cell data
  begins.
  Here, we assume the size of 'char' will always be 1, representing an 8-bit
  byte, no matter what OS or processing-sized hardware this code is running
  on.
*/
bool LCPAnalyzer::Analyze( const char * FileName )
{ //LCPAnalyzer::Analyze
  //Open file as binary & move to end.
  LCP.open( FileName, ios::in | ios::binary | ios::ate );
  
  if( ! LCP.good() ) {
    Messages += "Unable to open file ";
    Messages += FileName;
    Messages += "\n";
    NumErrors++;
    return false;
  }
  else {
    Messages += "File ";
    Messages += FileName;
    Messages += " opened successfully.\n";
  }

  //Get total size.
  TotalFileSize = LCP.tellg();
  char s[80]; //Temp cstring
  sprintf( s, "Total file size: %ld \n", TotalFileSize );
  Messages += s;

  //Initialize CalculatedSizeGood flag. Set this flag to true once we have
  //figured out a header size & grid data size that makes sense.
  CalculatedHeaderSizeGood = false;

  //Now analyze header to get size of data types in file.
  //Initialize data type sizes with minimum assumed size (in bytes).
  //Chars are assumed to always be 1 bytes (8 bits).
  //Shorts are assumed to be 2 bytes I.e. 16 bits
  //Doubles are assumed to be 8 bytes I.e. 16 bits
  LongSize = 4;   //I.e. 32 bits

  bool OutOfIdeas = false;
  int attempt_num = 1;
  do {
    HeaderSize = CalcHeaderSize();
    ReadHeader( LCP, HeaderSize, SHORT_SIZE, LongSize, DOUBLE_SIZE );

    //Check some values in the header.
    int num_bad_values = 0;

    if( Latitude < -90 || Latitude > 90 ) {
      //Bad value, but not a serious error.
      sprintf( s, "Latitude out of range: %ld\n", Latitude );
      Messages += s;
      num_bad_values++;
    }

    if( NumEast <= 0 ) {
      Messages += "Invalid number of columns (\'numeast\'): ";
      num_bad_values++;
      NumErrors++;
    }
    else
      Messages += "number of columns (\'numeast\'): ";
    sprintf( s, "%ld\n", NumEast );
    Messages += s;

    if( NumNorth <= 0 ) {
      Messages += "Invalid number of rows (\'numnorth\'): ";
      num_bad_values++;
      NumErrors++;
    }
    else
      Messages += "number of rows (\'numnorth\'): ";
    sprintf( s, "%ld\n", NumNorth );
    Messages += s;

    if( EastUTM <= WestUTM ) {
      Messages += "Problem: EastUTM <= WestUTM: ";
      num_bad_values++;
      NumErrors++;
    }
    sprintf( s, "WestUTM: %lf\nEastUTM: %lf\n", WestUTM, EastUTM );
    Messages += s;

    if( NorthUTM <= SouthUTM ) {
      Messages += "Problem: NorthUTM <= SouthUTM:";
      num_bad_values++;
      NumErrors++;
    }
    sprintf( s, "SouthUTM: %lf\nNorthUTM: %lf\n", SouthUTM, NorthUTM );
    Messages += s;

    if( num_bad_values > 1 ) { //Set this comparison higher for 'less strict'
      //Try adjusting data type sizes.
      attempt_num++;
      switch (attempt_num) {
        case 2:  //Second attempt; try increasing long size to 8
          LongSize = 8;
          break;

        /*STUB:
          Add cases here to try changing parameters about the header.
          e.g. case 3, case 4, etc.
        */

        default:  //No solution found; bailing out
          OutOfIdeas = true;
      }
    }
    else CalculatedHeaderSizeGood = true;
  } while( ! CalculatedHeaderSizeGood || OutOfIdeas );

  /*Now do some size calculations to see if everything makes sense.
    The size of the grid data section should be based on the size of the
    individual cells. The cells can have different sizes:
       5 * ShortSize   - If just standard elevation, slope, aspect, fuels, &
                         canopy included
       7 * ShortSize   - If including Ground Fuels (but not Crown Fuels)
       8 * ShortSize   - If including Crown Fuels (but not Ground Fuels)
      10 * ShortSize   - If including both Crown & Ground fuels
  */
  CalculatedDataSizeGood = false;
  unsigned long remaining_file_size = TotalFileSize - HeaderSize;

  CellSize = 5 * SHORT_SIZE; //Initial assumed size
  unsigned long calculated_data_size = CalcDataSize();
  if( calculated_data_size > remaining_file_size ) {
    Messages += "File size is too small to contain all the grid data, ";
    sprintf( s, "if NumEast=%ld and NumNorth=%ld\n", NumEast, NumNorth );
    Messages += s;
    NumErrors++;
  }
  else if( calculated_data_size < remaining_file_size ) {
    //Try again, assuming Ground Fuels included.
    CellSize = 7 * SHORT_SIZE;
    calculated_data_size = CalcDataSize();
    if( calculated_data_size > remaining_file_size ) {
      Messages += "File size is too small to contain all the grid data, ";
      sprintf( s, "if NumEast=%ld and NumNorth=%ld", NumEast, NumNorth );
      Messages += s;
      Messages += ", and ground fuels are included.\n";
      NumErrors++;
    }
    else if( calculated_data_size < remaining_file_size ) {
      //Try again, assuming Crown Fuels included.
      CellSize = 8 * SHORT_SIZE;
      calculated_data_size = CalcDataSize();
      if( calculated_data_size > remaining_file_size ) {
        Messages += "File size is too small to contain all the grid data, ";
        sprintf( s, "if NumEast=%ld and NumNorth=%ld", NumEast, NumNorth );
        Messages += s;
        Messages += ", and crown fuels are included.\n";
        NumErrors++;
      }
      else if( calculated_data_size < remaining_file_size ) {
        //Try again, assuming Ground & Crown Fuels included.
        CellSize = 10 * SHORT_SIZE;
        calculated_data_size = CalcDataSize();
        if( calculated_data_size > remaining_file_size ) {
          Messages += "File size is too small to contain all the grid data, ";
          sprintf( s, "if NumEast=%ld and NumNorth=%ld", NumEast, NumNorth );
          Messages += s;
          Messages += ", and both ground & crown fuels are included.\n";
          NumErrors++;
        }
        else if( calculated_data_size < remaining_file_size ) {
          Messages += "File size is too large, ";
          sprintf( s, "if NumEast=%ld and NumNorth=%ld", NumEast, NumNorth );
          Messages += s;
          Messages += ", even if both ground & crown fuels are included.\n";
          CellSize = 0;  //To flag that no good cell size was found
          NumErrors++;
        }
        else CalculatedDataSizeGood = true;  //Data includes Ground & Crown
      }
      else CalculatedDataSizeGood = true;  //Data includes Crown Fuels
    }
    else CalculatedDataSizeGood = true;  //Data includes Ground Fuels
  }
  else CalculatedDataSizeGood = true;  //Data is just main 5 layers

  Analyzed = true;

  return true;
} //LCPAnalyzer::Analyze

//----------------------------------------------------------------------------
bool LCPAnalyzer::ReadHeader( fstream &In, unsigned int header_size,
                              unsigned int short_size, unsigned int long_size,
                              unsigned int double_size )
{ //LCPAnalyzer::ReadHeader
  //Read in header data.
  if( header_size == 0 )
    return false;

  char *header = new char[header_size];
  In.seekg( 0, ios::beg );  //Reset ptr to start of file.
  In.read( header, header_size );

  //Try to extract attributes.
  CrownFuels = ExtractAsLong( header, 0, long_size );
  GroundFuels = ExtractAsLong( header, long_size, long_size );
  Latitude = ExtractAsLong( header, 2*long_size, long_size );
  NumEast = ExtractAsLong( header, 1033*long_size+4*double_size, long_size );
  NumNorth = ExtractAsLong( header, 1034*long_size+4*double_size, long_size );
  LoEast = ExtractAsDouble( header, 3*long_size, double_size );
  HiEast = ExtractAsDouble( header, 3*long_size+double_size, double_size );
  LoNorth = ExtractAsDouble( header, 3*long_size+2*double_size, double_size );
  HiNorth = ExtractAsDouble( header, 3*long_size+3*double_size, double_size );
  EastUTM = ExtractAsDouble( header, 1035*long_size+4*double_size,
                             double_size );
  WestUTM = ExtractAsDouble( header, 1035*long_size+5*double_size,
                             double_size );
  NorthUTM = ExtractAsDouble( header, 1035*long_size+6*double_size,
                              double_size );
  SouthUTM = ExtractAsDouble( header, 1035*long_size+7*double_size,
                              double_size );

  delete [] header;

  return true;
} //LCPAnalyzer::ReadHeader

/*----------------------------------------------------------------------------
  LCPAnalyzer::ExtractAsLong
  Read a part of a block of memory as if it were bytes of a long data type.
  Pos is the position into Mem to start reading. LongSize is the presumed
  number of bytes that should be used.
*/
long LCPAnalyzer::ExtractAsLong( const char *Mem, unsigned long Pos,
                                 unsigned int LongSize )
{ //LCPAnalyzer::ExtractAsLong
  unsigned int    buf_size = 16;  //Assume the size of the 'longest long'
  char   buffer[buf_size];
  for( unsigned int i = 0; i < buf_size; i++ ) {
    if( i < LongSize )
      buffer[i] = Mem[Pos + i];    //Note: Reading 'backwards' (little-endian)
    else buffer[i] = 0;
  }

  long l;
  memcpy( &l, buffer, buf_size );

  return l;
} //LCPAnalyzer::ExtractAsLong

/*----------------------------------------------------------------------------
  LCPAnalyzer::ExtractAsDouble
  Read a part of a block of memory as if it were bytes of a double data type.
  Pos is the position into Mem to start reading. DoubleSize is the presumed
  number of bytes that should be used.
*/
double LCPAnalyzer::ExtractAsDouble( const char *Mem, unsigned long Pos,
                                     unsigned int DoubleSize )
{ //LCPAnalyzer::ExtractAsDouble
  unsigned int    buf_size = 16;  //Assume the size of the 'longest double'
  char   buffer[buf_size];
  for( unsigned int i = 0; i < buf_size; i++ ) {
    if( i < DoubleSize )
      buffer[i] = Mem[Pos + i];    //Note: Reading 'backwards' (little-endian)
    else buffer[i] = 0;
  }

  double d;
  memcpy( &d, buffer, buf_size );

  return d;
} //LCPAnalyzer::ExtractAsDouble

/*----------------------------------------------------------------------------
  LCPAnalyzer::CalcHeaderSize
*/
unsigned long LCPAnalyzer::CalcHeaderSize()
{ //LCPAnalyzer::CalcHeaderSize
  return 1036 * LongSize +
           10 * DOUBLE_SIZE +
           10 * SHORT_SIZE +
         3072 * 1; //chars
} //LCPAnalyzer::CalcHeaderSize

/*----------------------------------------------------------------------------
  LCPAnalyzer::CalcDataSize
*/
unsigned long LCPAnalyzer::CalcDataSize()
{ //LCPAnalyzer::CalcDataSize
  return CellSize * NumEast * NumNorth;
} //LCPAnalyzer::CalcDataSize

/*----------------------------------------------------------------------------
  LCPAnalyzer::SetFilePos
*/
bool LCPAnalyzer::SetFilePos( unsigned long Pos )
{ //LCPAnalyzer::SetFilePos
  if( ! LCP.good() )
    return false;
    
  LCP.seekg( Pos, ios::beg );  //Reset from start of file
  return true;
} //LCPAnalyzer::SetFilePos

/*----------------------------------------------------------------------------
  LCPAnalyzer::ExtractInteger
  This extracts the given number of bytes from the LCP file stream and
  converts them to an integer type (int, short, long, etc.).
*/
long LCPAnalyzer::ExtractInteger( int NumBytes )
{ //LCPAnalyzer::ExtractInteger
  char bytes[NumBytes];
  LCP.read( bytes, NumBytes );

  int sys_long_size = sizeof( long );

  //Can't stuff more bytes than can fit in a long!
  if( NumBytes > sys_long_size ) return 0;

  unsigned char b[sys_long_size];  //Array to hold all bytes of a system long
  for( int j = 0; j < sys_long_size; j++ ) {
    if( j < NumBytes ) b[j] = bytes[j];
    else b[j] = 0;
  }

  return *reinterpret_cast<long*>( b );
} //LCPAnalyzer::ExtractInteger

/*----------------------------------------------------------------------------
  LCPAnalyzer::ExtractIntegers
  Similar to ExtractInteger, but fills an array.
    A        - The array to be filled
    Size     - Size of array A
    NumBytes - The number of bytes IN THE FILE that constitute one integer
               value (one element of the array)
*/
bool LCPAnalyzer::ExtractIntegers( long *A, int Size, int NumBytes )
{ //LCPAnalyzer::ExtractIntegers
  char bytes[NumBytes*Size];
  LCP.read( bytes, NumBytes*Size );

  int sys_long_size = sizeof( long );

  //Can't stuff more bytes than can fit in a long!
  if( NumBytes > sys_long_size ) return false;

  int offset = 0;
  unsigned char b[sys_long_size];  //Array to hold all bytes of a system long
  for( int i = 0; i < Size; i++ ) {
    for( int j = 0; j < sys_long_size; j++ ) {
      if( j < NumBytes ) b[j] = bytes[offset+j];
      else b[j] = 0;
    }
    A[i] = *reinterpret_cast<long*>( b );
    offset += NumBytes;
  }

  return true;
} //LCPAnalyzer::ExtractIntegers

/*----------------------------------------------------------------------------
  LCPAnalyzer::ExtractDouble
  For now, we're assuming doubles stay the same size (DOUBLE_SIZE).
*/
double LCPAnalyzer::ExtractDouble()
{ //LCPAnalyzer::ExtractDouble
  char bytes[DOUBLE_SIZE];
  LCP.read( bytes, DOUBLE_SIZE );

  double d = *reinterpret_cast<double *>( bytes );
  return d;
} //LCPAnalyzer::ExtractIntegers

/*----------------------------------------------------------------------------
  LCPAnalyzer::ExtractChars
  Similar to ExtractIntegers, but fills an array of characters.
    A        - The array to be filled
    Size     - Size of array A

  The size of char is assumed to be 1 byte.
*/
void LCPAnalyzer::ExtractChars( char *A, int Size )
{ //LCPAnalyzer::ExtradtChars
  LCP.read( A, Size );
} //LCPAnalyzer::ExtradtChars