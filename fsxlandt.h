/*
  See LICENSE.TXT file for license information.
*/
#ifndef FSXLANDT_H
#define FSXLANDT_H

typedef struct {
  // structure for holding basic cell information
  short e;  // elevation
  short s;  // slope
  short a;  // aspect
  short f;  // fuel models
  short c;  // canopy cover
} celldata;

typedef struct {
  // structure for holding optional crown fuel information
  short h;  // canopy height
  short b;  // crown base
  short p;  // bulk density
} crowndata;

typedef struct {
  // structure for holding duff and woody fuel information
  short d;  // duff model
  short w;  // coarse woody model
} grounddata;

struct CanopyCharacteristics
{ //CanopyCharacteristics
  // contains average values, landscape wide, temporary until themes are used
  double DefaultHeight;
  double DefaultBase;
  double DefaultDensity;
  double Height;
  double CrownBase;
  double BulkDensity;
  double Diameter;
  double FoliarMC;
  long   Tolerance;
  long   Species;
  CanopyCharacteristics();
};//CanopyCharacteristics

#ifndef OS_X
#pragma pack(push) //JWB 201004: Needs this (at least in VC6) so there's not a
#pragma pack(1)    //byte-boundary reading problem in fread() in ReadHeader().
#endif
typedef struct {
  short  elev;
  short  slope;
  short  aspect;
  short  fuel;
  short  cover;  // Read or derived from landscape data
  double aspectf;
  double height;
  double base;
  double density;
  double duff;
  long   woody;
} LandscapeStruct;
#ifndef OS_X
#pragma pack()  //JWB 2010: See above
#endif

class LandscapeData
{ //LandscapeData
public:
  LandscapeStruct ld;

  void ElevConvert( short );
  void SlopeConvert( short );
  void AspectConvert( short );
  void FuelConvert( short );
  void CoverConvert( short );
  void HeightConvert( short );
  void BaseConvert( short );
  void DensityConvert( short );
  void DuffConvert( short );
  void WoodyConvert( short );
};//LandscapeData

#endif