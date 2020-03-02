/*fsxwbar.h
  Use of Vector Barriers to Fire Spread
  Copyright 1994, 1995, 1996
  Mark A. Finney, Systems for Environemntal Management
  See LICENSE.TXT file for license information.
*/
#ifndef FSXWBAR_H
#define FSXWBAR_H

//============================================================================
class VectorBarrier
{ //VectorBarrier
  long   NumVertices;
  double DiffRes;
  double* Barrier;
  double* BufBarrier;
  double DistanceResolution;
  double BarrierDistance;

  bool ReDiscretizeBarrier();
  void ReBuffer( long BarrierNumber );
  void FreeBarrier();

public:
  VectorBarrier();
  ~VectorBarrier();
  bool AllocBarrier( long VertNumber );
  void SetBarrierVertex( long NumVecs, double xpt, double ypt );
  void BufferBarrier( double DistanceMultiplier );
  bool ReBufferBarriers();  //Buffer the barrier by given width
  bool TransferBarrier( long NumFire );
};//VectorBarrier

#endif