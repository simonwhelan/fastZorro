#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <float.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>


#define EXTERN
#include "trim.h"
#undef EXTERN

#define EXTERN extern
#include "utils.h"
#include "hmm.h"
#include "matrices.h"
#undef EXTERN


int main(int argc,char **argv){ 
  int i;
  if(argc < 2){
    printf("Input file not specified\n");
    exit(EXIT_FAILURE);
  }

  // Hard coded options
  JTT = 0;
  PMB = 0;
  PAM = 1;
  MATRICES = 0;

  readSeq(argv[argc-1]);

  initHMM(alen);
  calc_posterior(alen);

  return 1;
  
}
