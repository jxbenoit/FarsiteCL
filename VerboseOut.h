/*============================================================================
  VerboseOut.h

  A class that allows indented printing of statements, mainly for debugging.
  ============================================================================
*/
#include<string>

using namespace std;

class VerboseOut {
  private:
    int    Verbosity;
    string S;         //String to be printed out
    bool   Indent;    //Toggle between indent & not indent modes

  public:
    int CallLevel;
    VerboseOut();
    VerboseOut( int V );
    void SetVerbosity( int V ) { Verbosity = V; };
    bool CanPrint();

    friend VerboseOut& operator<<( VerboseOut& out, const string & mesg );
    friend VerboseOut& operator<<( VerboseOut& out, const int mesg );
};
