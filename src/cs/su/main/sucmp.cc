/* Copyright (c) Colorado School of Mines, 2011.*/
/* All rights reserved.                       */

/* SUCMP: $Revision: 1.5 $ ; $Date: 2011/11/16 17:24:58 $    */

#include "csException.h"
#include "csSUTraceManager.h"
#include "csSUArguments.h"
#include "csSUGetPars.h"
#include "su_complex_declarations.h"
#include "cseis_sulib.h"
#include <string>

extern "C" {
  #include <pthread.h>
}
extern "C" {
  #include <string.h>

}
extern "C" {
  #include <math.h>

}

#include "header.h"
#include "su.h"
#include "segy.h"

/*********************** self documentation ***************************/

std::string sdoc_sucmp =
"									"
" SUCMP   - CoMPare two seismic data sets, returns 0 to the shell	" 
"             if the same and 1 if different				"
"									"
"  sucmp file_A file_B							"
"									"
" Required parameters:							"
"      none								"
"									"
"   Optional parameters:						"
"      limit=1.0e-4    normalized difference threshold value		"
"									"
" Notes:								"
" This program is the seismic equivalent of the Unix cmp(1)		"
" command.  However, unlike cmp(1), it understands seismic data		"
" and will consider files which have only small numerical		"
" differences to be the same.						"
"									"
" Sucmp first checks that the number of traces and number of samples	"
" are the same. It then compares the trace headers bit for bit.		"
" Finally it checks that the fractional difference of A & B is		"
" less than limit.							"
"									"
" This program is intended as an aid in regression testing changes to	"
" seismic processing programs.						"
"									"
" Expected usage is in shell scripts or Makefiles, e.g.			"
"   #!/bin/sh								"
"    #-------------------------------------------------------		"
"    # Run a test data set and verify the result is correct		"
"    # If the data doesn't match show the data on the screen.		"
"   #-------------------------------------------------------		"
"									"
"  ./fubar par=tst1.par							"
"   sucmp tst1.su ref/tst1.su						"
"   if [ $? ]								"
"      then								"
"      suxwigb <tst1.su &						"
"      suxwigb <ref/tst1.su & "
"   fi									"
;

/*  Sun Feb 24 13:30:07 2013
  Automatically modified for usage in SeaSeis  */
namespace sucmp {



/**********************************************************************
 * Author:  Reginald H. Beardsley 
 *		rhb@acm.org
 *
 *  sucmp - compare two seismic files in CWP/SU format to see if they
 *         are the same within the user specified limit.
 *
 *  Algorithm:
 *
 *  Loop over both input files comparing data values.  To be 
 *  considered the same files must have:
 *
 *    - same number of traces
 *    - same number of samples per trace
 *    - trace values within limits of each other
 *
 * Note that the program exits as soon as the files fail to match.
 *
 * For readability, explicit temporary variables are used which the
 * compiler will optimize away. Braces are used on all conditionals
 * so that a breakpoint can be set to stop the debugger only if the 
 * condition is true.
 *
 * Because of the overloading of trace header fields in CWP/SU, the
 * headers are compared bit for bit.
**********************************************************************/
/************************** end self doc ******************************/
      

segy traceA;
segy traceB;

void* main_sucmp( void* args )
{

   FILE *fpA;     /* file pointer for first file      */
   FILE *fpB;     /* file pointer for second file     */

   int nA;        /* bytes read from fpA */
   int nB;        /* bytes read from fpB */

   int nt;        /* number of sample points in traces   */

   int i;
   int j;

   float A;
   float B;

   float upper;
   float lower;

   float limit;              /* fractional difference limit */

   /*------------*/
   /* Initialize */
   /*------------*/

   cseis_su::csSUArguments* suArgs = (cseis_su::csSUArguments*)args;
   cseis_su::csSUTraceManager* cs2su = suArgs->cs2su;
   cseis_su::csSUTraceManager* su2cs = suArgs->su2cs;
   int argc = suArgs->argc;
   char **argv = suArgs->argv;
   cseis_su::csSUGetPars parObj;

   void* retPtr = NULL;  /*  Dummy pointer for return statement  */
   su2cs->setSUDoc( sdoc_sucmp );
   if( su2cs->isDocRequestOnly() ) return retPtr;
   parObj.initargs(argc, argv);

   try {  /* Try-catch block encompassing the main function body */

	
   if(!parObj.getparfloat("limit",&limit)) limit = 1.0e-4;

   parObj.checkpars();

   upper = 1.0 + limit;
   lower = 1.0 - limit;

   /*------------------*/
   /* Open input files */
   /*------------------*/
   if (!(fpA=efopen(argv[1], "rb")))
	throw cseis_geolib::csException("unable to open first file");
   if (!(fpB=efopen(argv[2], "rb"))) 
	throw cseis_geolib::csException("unable to open second file");


   /*---------------*/
   /* compare files */
   /*---------------*/
   j = 0;

   while( !feof(fpA) && !feof(fpB) ){

      nA = fgettr(fpA, &traceA);
      nB = fgettr(fpB, &traceB); 

      j++;

      if( nA != nB ){

         printf( "Files %s & %s differ at trace %d\n"
                 ,argv[1] ,argv[2] ,j );
         return( 1 );

      }else if( memcmp( &traceA ,&traceB ,HDRBYTES ) ){

         printf( "Files %s & %s differ in headers at trace %d\n"
                 ,argv[1] ,argv[2] ,j );
         return( 1 );

      }else{

         nt = traceA.ns;

         for (i=0; i<nt; i++){

            A = traceA.data[i];
            B = traceB.data[i];

            if( (A > 0.0 && (A < B*lower || A > B*upper ) )
              ||(A < 0.0 && (A > B*lower || A < B*upper ) ) ){	

               printf( "Files %s & %s differ at"   ,argv[1] ,argv[2] );
               printf( " Trace: %d Sample: %d\n"   ,j       ,i       );
               printf( "   A: %15g   B: %15g\n"    ,A       ,B       );
               return( 1 );

            }
         }

      }
              
   }

	su2cs->setEOF();
	pthread_exit(NULL);
	return retPtr;
}
catch( cseis_geolib::csException& exc ) {
  su2cs->setError("%s",exc.getMessage());
  pthread_exit(NULL);
  return retPtr;
}
}

} // END namespace
