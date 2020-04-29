/*============================================================================
  main.cpp

  Entry point for FarsiteCL executable.
  RFL version  
  Based on Version 4.1.0 (12/21/2004) by Mark Finney.

  See LICENSE.TXT file for license information.
  ============================================================================
*/
#include<iostream>
#include<string.h>
#include"globals.h"
#include"Farsite.h"

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

  VOut.SetVerbosity( Verbose ); //Set up verbose output stream (global obj)

  if( VOut.CanPrint() )
    VOut << "main:main:1 Verbosity set to " << Verbose << "\n";

  if( ! Exists(InputSettingsFilename) ) {
    cerr << "## Can't access " << InputSettingsFilename << "! ##" << endl;

    return 1;
  }

  char CurrDir[MAX_CUR_DIR_STR_LEN];
  GetCurDir( CurrDir, MAX_CUR_DIR_STR_LEN );

  if( VOut.CanPrint() ) VOut << "main:main:2 CurrDir = " << CurrDir << "\n";

  if( CurrDir[0] == '\0' ) {
    cerr << "## Can't get current working directory! ##" << endl;

    return 2;
  }

  Farsite F( CurrDir );

  if( VOut.CanPrint() ) VOut << "main:main:3\n";

  if( F.SetInputsFromFile( InputSettingsFilename ) ) {
  if( VOut.CanPrint() ) VOut << "main:main:4 Starting main procedure....\n";

    F.FlatOpenProject();

    if( VOut.CanPrint() ) VOut << "main:main:5 Done\n";
  }
  else {
    cerr << "## Bailing out, due to invalid settings.... ##" << endl;
    return 3;
  }

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
      if( v >= 0 && v <= 13 ) Verbose = (int) v;
      else cerr << "## Bad setting for Verbose argument ##\n";
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
