# FarsiteCL
A version of FARSITE derived from the GUI version of FARSITE 4 &amp; command-line version code released downloaded in circa 2006..

## 202003021148: Gave this code to UCSB for batch fire modeling project.  
They were able to compile it on a Linux system. However, there are some known 'classic' issues:  
  * Landscape files are not being read correctly.
  * The simulation of fire they are trying to model (Sharpa) crashes after producing the first perimeter.
  
### 202003061500: Added code to fix LCP header size not being computed correctly.
This problem has existed for quite a while in the original code. I think at the time of development, it was meant to run on a 32-bit Windows computer, with a  certain set of data type sizes.  
With C/C++, there are certain defined sizes for data types, e.g.:  
                Character                        - 1 byte  
                Short integer                    - at least 2 bytes  
                Long integer                     - at least 4 bytes  
                Floating point                   - at least 4 bytes  
                Double precision floating point  - at least 8 bytes  
But this isn’t standard between 32- & 64-bit computers, different operating systems, or different compilers.  
   
So in the case of the Sherpa LCP file, here is what I am seeing. The header data that is supposed to be written as long integers (longs) uses 4 bytes each. Now  the FARSITE code does not actually look at the LCP file itself to figure out how big the header is – it just measures an internal header data structure that *should* be the same size as the one used when the LCP was written. But in my version of the compiled FARSITE code, it assumes longs are 8 bytes – so it actually thinks the header is bigger than it really is.  
  
If you try running the executable with the verbosity set to say 6:  
    farsite.x -v 6 InputSettings.sherpa.txt  
There is a line that says ‘headsize=11464’. The true header size of the file should be 7316.  
(Actually, even 11464 isn’t quite right – should be 11460 – but that’s an issue within this issue! (a byte boundary/alignment problem))  
This is important because the grid cell data follows the header, so if headsize is wrong, FARSITE isn’t using the right cells to calculate firespread.  
  
To calculate the header size, I wrote some code to add to the existing FARSITE code. It opens the actual LCP file & figures out how  big the header is.  
I added LCPAnalyzer.cpp & LCPAnalyzer.h to the FARSITE code. Modified fsglbvar.cpp To use LCPAnalyzer. I modified the makefile include the new code. Hopefully the LCPAnalyzer code will now take care of calculating the true LCP header size.  
  
Now the next issue! The code seems to be crashing with a segmentation fault in FireEnvironment2::GetMx() function in fsxwfms2.cpp. It seems to be trying to calculate  an index for an array contain fuel data, but is getting -1 (which is definitely an error). I don’t think it’s reading all the fuel data into this array somewhere else in the code. I will look into why this is next.  
