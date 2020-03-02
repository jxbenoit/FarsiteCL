/*gridthem.cpp
  See LICENSE.TXT file for license information.
*/
#include<math.h>
#include"themes.h"

//----------------------------------------------------------------------------
//Color Legend stuff for FARSITE Window
//----------------------------------------------------------------------------

//============================================================================
GridTheme::GridTheme()
{ //GridTheme::GridTheme
  RedVal = 50;
  GreenVal = 0;
  BlueVal = 0;
  VarVal = 0;
  NumColors = 12;
  MaxBrite = 255;
  LegendNum = -1;
  OnOff = false;
  OnOff3d = false;
  WantNewRamp = true;
  CatsOK = false;
  Changed3d = false;
  Priority = 0;
} //GridTheme::GridTheme

//============================================================================
GridTheme::~GridTheme() {}

//============================================================================
void GridTheme::CreateRamp()
{ //GridTheme::CreateRamp
  if( ! Continuous ) {
    NumColors = NumCats;
    VarVal = 18;
  }
} //GridTheme::CreateRamp