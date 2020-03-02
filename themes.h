/*themes.h
  See LICENSE.TXT file for license information.
*/
#ifndef THEMES_H
#define THEMES_H

#include<stdlib.h>
#include<time.h>
#include<sys/timeb.h>

//============================================================================
class GridTheme
{ //GridTheme
public:
  char   Name[256];
  long   Continuous;
  long   RedVal, GreenVal, BlueVal, VarVal, NumColors, MaxBrite, ColorChange;
  bool   WantNewRamp, LcpAscii, OnOff, OnOff3d, Changed3d;
  long   Cats[100], NumCats, CatsOK, Priority;
  long   LegendNum;
  double MaxVal, MinVal;

  GridTheme();
  ~GridTheme();
  void CreateRamp();
};//GridTheme

//============================================================================
class LandscapeTheme : public GridTheme
{ //LandscapeTheme
  void FillCats();
  void SortCats();

public:
  long   NumAllCats[10];
  long   AllCats[10][100];
  double maxval[10], minval[10];

  LandscapeTheme( bool Analyze );
  void CopyStats( long layer );
  void ReadStats();
  void AnalyzeStats();
};//LandscapeTheme

//============================================================================
class RasterTheme : public GridTheme
{ //RasterTheme
  void FillCats();
  void SortCats();

public:
  double* map;
  double rW, rE, rN, rS, rCellSizeX, rCellSizeY, rMaxVal, rMinVal;
  long   rRows, rCols;

  RasterTheme();
  ~RasterTheme();
  bool SetTheme( char* Name );
};//RasterTheme

//============================================================================
struct VectorTheme
{ //VectorTheme
  long VectorNum;
  char FileName[256];
  long FileType;
  long Permanent;
  int  PenStyle;
  int  PenWidth;
  bool OnOff;
  bool OnOff3d;
  bool Changed;
  long Priority;
};//VectorTheme

#endif  //THEMES_H