//globals.h -- Apr 2009 -- J.Benoit
//Code that varies with the operating system / compiler should be put in here.
#ifndef GLOBALS_H
#define GLOBALS_H
#include<math.h>  //for acos();

//Externs
extern int Verbose;
extern int CallLevel;

//Constants & Defines
#define      VERSION                 1.0
const int    FILE_MODE_READ      =   1;  //For SetFileMode()
const int    FILE_MODE_WRITE     =   2;  //For SetFileMode()
const int    MAX_CUR_DIR_STR_LEN = 256;  //For GetCurDir()
const double PI                  = acos( -1.0 );

//Max & Min functions
template <class T> const T &max( const T &a, const T &b )
  { return ( b < a ) ? a : b; }
template <class T> const T &min( const T &a, const T &b )
  { return ( b > a ) ? a : b; }

//Distance functions
template <class T> T CalcDistSq( const T XDist, const T YDist )
  { return ( T )( XDist * XDist + YDist * YDist ); }

//Prototypes
char* GetCurDir( char *Buf, int MaxLen );
bool  ChangeDir( char *Dir );
bool  Exists( char *Filename );
bool  IsReadable( char *Filename );
bool  IsWritable( char *Filename );
void  SetFileMode( char *Filename, int Mode );

#endif
