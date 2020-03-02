/*batch.cpp
  See LICENSE.TXT file for license information.
*/
#include<iostream>
#include"globals.h"
#include"farsite4.h"
#include"portablestrings.h"

using namespace std;

//Prototypes
bool ProcessArgs( int argc, char* argv[], char **Filename );

//============================================================================
int main( int argc, char* argv[] )
{ //main
  char *InputSettingsFilename = NULL;

  //Process command-line arguments. Print help page, if needed.
  bool args_ok = ProcessArgs( argc, argv, &InputSettingsFilename );
  if( ! args_ok ) {
    cout << "\nFarsiteCL - Version " << VERSION << "\n"
         << "Usage: FarsiteCL [-h] [-v level] file\n\n"
         << "    -h       : this help page\n"
         << "    -v level : set the verbosity (level = 0-7)\n\n"
         << "Note that the name of an input settings file must be included "
         << "as an argument.\n" << endl;

    return 0;
  }

  if( Verbose > CallLevel )
    printf( "%*sbatch:main:1 Verbosity set to %d\n", CallLevel, "", Verbose );

  if( ! Exists(InputSettingsFilename) ) {
    printf( "## Can't access %s! ##\n", InputSettingsFilename );

    return 1;
  }

  char CurrDir[MAX_CUR_DIR_STR_LEN];
  GetCurDir( CurrDir, MAX_CUR_DIR_STR_LEN );
  if( Verbose > CallLevel )
    printf( "%*sbatch:main:2 CurrDir = %s\n", CallLevel, "", CurrDir );
  if( CurrDir == NULL ) {
    printf( "## Can't get current working directory! ##\n" );

    return 2;
  }

  TFarsiteInterface farsite = TFarsiteInterface( CurrDir );

  if( Verbose > CallLevel )
    printf( "%*sbatch:main:3\n", CallLevel, "" );

  if( farsite.SetInputsFromFile( InputSettingsFilename ) ) {
    if( Verbose > CallLevel )
      printf( "%*sbatch:main:4 Starting main procedure....\n", CallLevel, "" );

    farsite.FlatOpenProject();

    if( Verbose > CallLevel )
      printf( "%*sbatch:main:5 Done\n", CallLevel, "" );
  }
  else {
    printf( "Bailing out, due to invalid settings....\n" );
    return 3;
  }

  fflush( stdout );

  delete [] InputSettingsFilename;

  return 0;
} //main

//============================================================================
bool ProcessArgs( int argc, char* argv[], char **Filename )
{ //ProcessArgs
  bool bSwitch = false, bVerbose = false, bFilenameSet = false;
  int i;

  for( i = 1; i < argc; i++ ) {
    char *p = argv[i];

    if( *p == '-' ) {
      bSwitch = true;
      if( *++p == 0 ) continue;
    }

    if( bSwitch ) {
      switch( *p ) {
        case 'h': case 'H': case '?':
          return false;
          break;
        case 'v': case 'V':
          bVerbose = true;         //Verbose level being set
          break;
      }

      bSwitch = false;          //Reset switch flag
    }
    else if( bVerbose ) {
      //(Remember to check for invalid number values for verbose level here).
      long v = atol( p );
      if( v >= 0 && v <= 12 ) Verbose = (int) v;
      else printf( "## Bad setting for Verbose argument ##\n" );
      bVerbose = false;
    }
    else {           //Treat as a input settings filename
      if( ! bFilenameSet ) {
        *Filename = new char [ strlen(argv[i]) + 1 ];
        strcpy( *Filename, argv[i] );

        bFilenameSet = true;
      }
    }
  }

  if( bFilenameSet ) return true;
  else return false;
} //ProcessArgs
