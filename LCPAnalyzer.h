#include<fstream>
#include<string>

using namespace std;

/*============================================================================
  LCPAnalyzer  -- Mar10, 2020 -- J.Benoit
  This class provides analysis data for a given LCP (Landscape) file.
  The LCP file format is known not to be robust across different operating
  systems and computing platforms. This class gives hints to the true size of
  the LCP header and to where the grid data actually starts in the file.
*/
class LCPAnalyzer
{ //LCPAnalysyzer
  private:
    static const unsigned int SHORT_SIZE = 2; //Assume shorts are 2 bytes
    static const unsigned int DOUBLE_SIZE = 8;//Assume doubles are 8 bytes
    string FileName, Messages;
    bool Analyzed, CalculatedHeaderSizeGood, CalculatedDataSizeGood;
    unsigned int HeaderSize, CellSize, LongSize, NumErrors;
    unsigned long TotalFileSize;
    fstream LCP;
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
    ~LCPAnalyzer();
    void   SetFileName( const char * FileName );
    bool   Analyze();
    bool   Analyze( const char * FileName );
    bool   SetFilePos( unsigned long Pos = 0 );
    long   ExtractInteger( int NumBytes );
    bool   ExtractIntegers( long *A, int Size, int NumBytes );
    double ExtractDouble();
    void   ExtractChars( char *A, int Size );

    //Getters
    string GetFileName() { return FileName; }
    string GetMessages() { return Messages; }
    int    GetNumErrors() { return NumErrors; }
    bool   IsAnalyzed() { return Analyzed; }
    bool   IsHeaderSizeGood() { return CalculatedHeaderSizeGood; }
    bool   IsDataSizeGood() { return CalculatedDataSizeGood; }
    unsigned int  GetHeaderSize() { return HeaderSize; }
    unsigned int  GetCellSize() { return CellSize; }
    unsigned int  GetLCPShortSize() { return SHORT_SIZE; }
    unsigned int  GetLCPLongSize() { return LongSize; }
    unsigned int  GetLCPDoubleSize() { return DOUBLE_SIZE; }
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