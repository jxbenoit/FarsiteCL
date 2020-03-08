#include<fstream>
#include<string>

using namespace std;

/*============================================================================
  LCPAnalyzer  -- Mar 4, 2020 -- J.Benoit
  This class provides analysis data for a given LCP (Landscape) file.
  The LCP file format is known not to be robust across different operating
  systems and computing platforms. This class gives hints to the true size of
  the LCP header and to where the grid data actually starts in the file.
*/
class LCPAnalyzer
{ //LCPAnalysyzer
  private:
    string FileName, Messages;
    bool Analyzed, CalculatedHeaderSizeGood, CalculatedDataSizeGood;
    unsigned int HeaderSize, CellSize, ShortSize, LongSize, DoubleSize;
    unsigned long TotalFileSize;
    bool   ReadHeader( fstream &In, unsigned int header_size,
                       unsigned int short_size, unsigned int long_size,
                       unsigned int double_size );
    long   ExtractAsLong( const char *Mem, unsigned long Pos,
                          unsigned int LongSize );
    double ExtractAsDouble( const char *Mem, unsigned long Pos,
                            unsigned int DoubleSize );
    unsigned long CalcHeaderSize();
    unsigned long CalcDataSize();

    //Header attributes
    long CrownFuels, GroundFuels, Latitude, NumEast, NumNorth;
    double LoEast, HiEast, LoNorth, HiNorth,
           EastUTM, WestUTM, NorthUTM, SouthUTM;

  public:
    LCPAnalyzer();
    LCPAnalyzer( const char * FileName );
    void SetFileName( const char * FileName );
    bool Analyze();
    bool Analyze( const char * FileName );

    //Getters
    string GetFileName() { return FileName; }
    string GetMessages() { return Messages; }
    bool   IsAnalyzed() { return Analyzed; }
    bool   IsHeaderSizeGood() { return CalculatedHeaderSizeGood; }
    bool   IsDataSizeGood() { return CalculatedDataSizeGood; }
    unsigned int  GetHeaderSize() { return HeaderSize; }
    unsigned int  GetCellSize() { return CellSize; }
    unsigned int  GetShortSize() { return ShortSize; }
    unsigned int  GetLongSize() { return LongSize; }
    unsigned int  GetDoubleSize() { return DoubleSize; }
    unsigned long GetFileSize() { return TotalFileSize; }
    long   GetCrownFuels() { return CrownFuels; }
    long   GetGroundFuels() { return GroundFuels; }
    long   GetLatitude() { return Latitude; }
    long   GetNumEast() { return NumEast; }
    long   GetNumNorth() { return NumNorth; }
    double GetEastUTM() { return EastUTM; }
    double GetWestUTM() { return WestUTM; }
    double GetNorthUTM() { return NorthUTM; }
    double GetSouthUTM() { return SouthUTM; }
};//LCPAnalyzer