/*fsxwignt.h
  See LICENSE.TXT file for license information.
*/
#ifndef FSXWIGNT_H
#define FSXWIGNT_H

#include"fsx4.h"
#include"fsglbvar.h"
#include"shapefil.h"

//============================================================================
class IgnitionFile : public IgnitionCorrect
{ //IgnitionFile
  FILE*  IFile;
  long   count, count2;
  long   fposition1, NumVertex;
  double CenterX, CenterY, xpt, ypt, angle;

public:
  char TestEnd[30];
  char ifile[256];

  IgnitionFile();
  ~IgnitionFile();

  bool GrassFile();
  bool ArcLine();
  bool ArcPoly();
  bool ArcPoint();
  bool ShapeInput();
  void SelectFileInputCmdL( int type );
};//IgnitionFile

#endif