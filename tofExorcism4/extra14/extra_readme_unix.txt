This file provides instruction on the installation and use of
the UNIX version of the library EXTRA v.1.0 extending the 
functionality of the CU Decision Diagram Package CUDD v.2.3.1 


The original version of CUDD is distributed and supported by 
Fabio Somenzi <Fabio@Colorado.EDU>, http://vlsi.colorado.edu/~fabio/
who bears no responsibility for this library.


INSTALLATION OF EXTRA LIBRARY:

(1) Download the file "extraXX.tar.gz" to your computer
    from http://www.ee.pdx.edu/~alanmi/research/extra.htm

(2) Unzip and untar the file into the directory, which will become 
    the home directory of the EXTRA library.

(3) If you do not have Espresso static library project compiled on
    your computer, remove files "hmIrred.c" and "zEspresso.c" from the
    EXTRA library makefile. (The correponding functions will not be 
    available in your version of the library.)
 
(4) Modify makefile to reflect your setting. You may need to provide
    the location of the header files of the CUDD package and the location
    of the lib directory where the libextra.a will be copied after it is
    successfully compiled.

(5) Execute "make" and (optional) "make clean".


LINKING YOUR PROJECT TO EXTRA LIBRARY:

(1) Make sure your application source code #include's "extra.h".

(2) Add "libextra.a" to Makefile in the same way you add "libcudd.a" 
    and other libraries in the CUDD distribution.

(3) Compile and link the debug(release) versions of your application.



Good luck!

Alan Mishchenko
Portland State University
Electrical and Computer Engineering
<alanmi@ee.pdx.edu>

May 19, 2001

This file can be found at http://www.ee.pdx.edu/~alanmi/research/extra.htm

