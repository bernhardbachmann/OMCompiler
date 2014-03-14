/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-CurrentYear, Linköping University,
 * Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3
 * AND THIS OSMC PUBLIC LICENSE (OSMC-PL).
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES RECIPIENT'S
 * ACCEPTANCE OF THE OSMC PUBLIC LICENSE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from Linköping University, either from the above address,
 * from the URLs: http://www.ida.liu.se/projects/OpenModelica or
 * http://www.openmodelica.org, and in the OpenModelica distribution.
 * GNU version 3 is obtained from: http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS
 * OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 */

/*
 * Developed by:
 * FH-Bielefeld
 * Developer: Vitalij Ruge
 * Contact: vitalij.ruge@fh-bielefeld.de
 */

#include "../ipoptODEstruct.h"
#include "../localFunction.h"

#ifdef WITH_IPOPT

/* static  int jac_struc(Index *iRow, Index *iCol, long int nx, long int nv, int nsi); */
static  int radauJac1(double *a, double *J, double dt, double * values, int nv, int *k, int j,IPOPT_DATA_ *iData);
static  int lobattoJac1(double *a, double *J, double *J0, double dt, double * values, int nv, int *k, int j, long double tmp,IPOPT_DATA_ *iData);
static  int radauJac2(double *a, double *J, double dt, double * values, int nv, int *k, int j,IPOPT_DATA_ *iData);
static  int lobattoJac2(double *a, double *J, double *J0, double dt, double * values, int nv, int *k, int j, long double tmp,IPOPT_DATA_ *iData);
static  int radauJac3(double *a, double *J, double dt, double * values, int nv, int *k, int j,IPOPT_DATA_ *iData);
static  int lobattoJac3(double *a, double *J, double *J0, double dt, double * values, int nv, int *k, int j, long double tmp,IPOPT_DATA_ *iData);
static int jac_struc(IPOPT_DATA_ *iData,int *iRow, int *iCol);
static int conJac(double *J, double * values, int nv, int *k, int j,IPOPT_DATA_ *iData);


/*!
 *  eval derivation of s.t.
 *  author: Vitalij Ruge
 **/
Bool evalfDiffG(Index n, double * x, Bool new_x, Index m, Index njac, Index *iRow, Index *iCol, Number *values, void * useData)
{
  IPOPT_DATA_ *iData;
  iData = (IPOPT_DATA_ *) useData;
  if(values == NULL){
   jac_struc(iData, iRow, iCol);

   /*
    printf("\n m = %i , %i",m ,iData->NRes);
    printf("\nk = %i , %i" ,k ,njac);
    for(i = 0; i< njac; ++i)
      printf("\nJ(%i,%i) = 1; i= %i;",iRow[i]+1, iCol[i]+1,i);

    assert(0);
    */

#if 0
    {
  int i;
    FILE *pFile;
    char buffer[4096];
    pFile = fopen("jac_struct.m", "wt");
    if(pFile == NULL)
      printf("\n\nError");
    fprintf(pFile, "%s", "clear J\n");
    fprintf(pFile, "%s", "%%%%%%%%%%%%%%%%%%%%%%\n");
    fprintf(pFile, "%s", "nz = ");
    fprintf(pFile, "%i", njac);
    fprintf(pFile, "%s", "\nnumberVars = ");
    fprintf(pFile, "%i", n);
    fprintf(pFile, "%s", "\nnumberconstraints = ");
    fprintf(pFile, "%i", m);
    fprintf(pFile, "%s", "\nNumberOfIntervalls = ");
    fprintf(pFile, "%i", iData->nsi);
    fprintf(pFile, "%s", "\nH=[];\n");
    fprintf(pFile, "%s", "%%%%%%%%%%%%%%%%%%%%%%\n");
    for(i=0; i< njac; ++i){
      sprintf(buffer, "H(%i,%i) = 1;\n", iRow[i]+1, iCol[i]+1);
      fprintf(pFile,"%s", buffer);
    }
    fprintf(pFile, "%s", "%%%%%%%%%%%%%%%%%%%%%%\n");
    fprintf(pFile, "%s", "spy(H)\n");
    }
#endif

  }else{
    int i,j,k,l,ii;
    long double tmp[3];
    int id;
    int nng = iData->nJ;

    ipoptDebuge(iData,x);

    tmp[0] = iData->dt[0]*iData->d1[4];
    tmp[1] = iData->dt[0]*iData->d2[4];
    tmp[2] = iData->dt[0]*iData->d3[4];

    diff_functionODE(x, iData->t0 , iData, iData->J0);

    for(i = 0, id = iData->nv, k = 0; i<1; ++i){
      for(l=0; l<iData->deg; ++l, id += iData->nv){
        diff_functionODE(x+id , iData->time[i*iData->deg + l] , iData, iData->J);
        for(j=0; j<iData->nx; ++j){
          switch(l){
          case 0:
            lobattoJac1(iData->d1, iData->J[j], iData->J0[j], iData->dt[i], values, iData->nv, &k, j, tmp[0], iData);
            break;
          case 1:
            lobattoJac2(iData->d2, iData->J[j], iData->J0[j], iData->dt[i], values, iData->nv, &k, j, tmp[1], iData);
            break;
          case 2:
            lobattoJac3(iData->d3, iData->J[j], iData->J0[j], iData->dt[i], values, iData->nv, &k, j, tmp[2], iData);
            break;
          }
        }
        for(;j<nng; ++j){
          conJac(iData->J[j], values, iData->nv, &k, j, iData);
        }
      }
    }

    for(; i<iData->nsi; ++i){
      for(l=0; l<iData->deg; ++l, id += iData->nv){
        diff_functionODE(x+id, iData->time[i*iData->deg + l], iData, iData->J);
        for(j=0; j<iData->nx; ++j){
          switch(l){
          case 0:
            radauJac1(iData->a1, iData->J[j], iData->dt[i], values, iData->nv, &k, j, iData);
            break;
          case 1:
            radauJac2(iData->a2, iData->J[j], iData->dt[i], values, iData->nv, &k, j, iData);
            break;
          case 2:
            radauJac3(iData->a3, iData->J[j], iData->dt[i], values, iData->nv, &k, j, iData);
            break;
          }
        }
        for(;j<nng; ++j)
          conJac(iData->J[j], values, iData->nv, &k, j, iData);
      }
    }
     /*assert(k == njac);*/
  }
  return TRUE;
}

/* static  int jac_struc(Index *iRow, Index *iCol, long int nx, long int nv, int nsi) */

/*!
 *  special jacobian struct
 *  author: Vitalij Ruge
 **/
static  int radauJac1(double *a, double *J, double dt, double * values, int nv, int *k, int j,IPOPT_DATA_ *iData)
{
  int l;
  values[(*k)++] = a[0];
  /*1*/
  for(l=0; l<nv; ++l){
    if(iData->knowedJ[j][l]){
      values[(*k)++] = (j == l) ? dt*J[l]-a[1] : dt*J[l];
    }
  }

  /*2*/
  values[(*k)++] = -a[2];

  /*3*/
  values[(*k)++] = a[3];
  return 0;
}

/*!
 *  special jacobian struct
 *  author: Vitalij Ruge
 **/
static  int lobattoJac1(double *a, double *J, double *J0, double dt, double * values, int nv, int *k, int j, long double tmp,IPOPT_DATA_ *iData)
{
  int l;
  /*0*/
  for(l = 0; l< nv; ++l){
    if(j == l) {
      values[(*k)++] = tmp*J0[l] + a[0];
    } else if(iData->knowedJ[j][l] == 1) {
      values[(*k)++] = tmp*J0[l];
    }
  }
  /*1*/
  for(l = 0; l< nv; ++l){
    if(iData->knowedJ[j][l]){
      values[(*k)++] = ((j == l)? dt*J[l] - a[1] : dt*J[l]);
    }
  }
  /*2*/
  values[(*k)++] = -a[2];

  /*3*/
  values[(*k)++] = a[3];
  return 0;
}


/*!
 *  special jacobian struct
 *  author: Vitalij Ruge
 **/
static  int radauJac2(double *a, double *J, double dt, double * values, int nv, int *k, int j,IPOPT_DATA_ *iData)
{
  int l;
  /*0*/
  values[(*k)++] = -a[0];

  /*1*/
  values[(*k)++] = a[1];

  /*2*/
  for(l = 0; l< nv; ++l){
    if(iData->knowedJ[j][l]){
      values[(*k)++] = ((j == l)? dt*J[l] - a[2] : dt*J[l]);
    }
  }

  /*3*/
  values[(*k)++] = -a[3];
  return 0;
}

/*!
 *  special jacobian struct
 *  author: Vitalij Ruge
 **/
static  int lobattoJac2(double *a, double *J, double *J0, double dt, double * values, int nv, int *k, int j, long double tmp,IPOPT_DATA_ *iData)
{
  int l;
  /*0*/
  for(l = 0; l< nv; ++l){
    if( j==l){
      values[(*k)++] = -(tmp*J0[l] + a[0]);
    } else if(iData->knowedJ[j][l]) {
      values[(*k)++] = -tmp*J0[l];
    }
  }
  /*1*/
  values[(*k)++] = a[1];

  /*2*/
  for(l = 0; l< nv; ++l){
    if(iData->knowedJ[j][l]){
      values[(*k)++] = ((j == l)? dt*J[l]-a[2] : dt*J[l]);
    }
  }
  /*3*/
  values[(*k)++] = -a[3];
  return 0;
}

/*!
 *  special jacobian struct
 *  author: Vitalij Ruge
 **/
static  int radauJac3(double *a, double *J, double dt, double * values, int nv, int *k, int j,IPOPT_DATA_ *iData)
{
  int l;
  /*0*/
  values[(*k)++] = a[0];
  /*1*/
  values[(*k)++] = -a[1];
  /*2*/
  values[(*k)++] = a[2];
  /*3*/
  for(l = 0; l< nv; ++l){
    if(iData->knowedJ[j][l]){
      values[(*k)++] = ((j == l)? dt*J[l] - a[3] : dt*J[l]);
    }
  }
  return 0;
}

/*!
 *  special jacobian struct
 *  author: Vitalij Ruge
 **/
static  int lobattoJac3(double *a, double *J, double *J0, double dt, double * values, int nv, int *k, int j, long double tmp,IPOPT_DATA_ *iData)
{
  int l;
  /*0*/
  for(l=0; l<nv; ++l){
    if(j==l){
      values[(*k)++] = tmp*J0[l] + a[0];
    }else if(iData->knowedJ[j][l]) {
      values[(*k)++] = tmp*J0[l];
    }
  }
  /*1*/
  values[(*k)++] = -a[1];
  /*2*/
  values[(*k)++] = a[2];
  /*3*/
  for(l=0; l<nv; ++l){
    if(iData->knowedJ[j][l]){
      values[(*k)++] = ((j == l)? dt*J[l] - a[3] : dt*J[l]);
    }
  }
  return 0;
}

static int conJac(double *J, double *values, int nv, int *k, int j,IPOPT_DATA_ *iData)
{
  int l;
  for(l=0; l<nv; ++l)
    if(iData->knowedJ[j][l])
      values[(*k)++] = J[l];

  return 0;
}

/*!
 *  special jacobian struct
 *  author: Vitalij Ruge
 **/
static int jac_struc(IPOPT_DATA_ *iData, int *iRow, int *iCol)
{
  int nr, nc, r, c, nv, nsi, nx, nJ, mv, ir, ic;
  int i, j, k=0, l;

  nv = (int) iData->nv;
  nx = (int) iData->nx;
  nsi = (int) iData->nsi;
  nJ = (int) iData->nJ;

  /*=====================================================================================*/
  /*=====================================================================================*/
  
  for(i=0, r = 0, c = nv ; i < 1; ++i){
  /******************************* 1  *****************************/
    for(j=0; j <nx; ++j){
      /* 0 */
      ir = r + j;
      for(l=0; l < nv; ++l){
        if(iData->knowedJ[j][l]){
          iRow[k] = ir;
          iCol[k++] = l;
        }
      }
      /* 1 */
      for(l = 0; l <nv; ++l){
        if(iData->knowedJ[j][l]){
          iRow[k] = ir;
          iCol[k++] = c + l;
        }
      }

      /* 2 */
      ic = c + j + nv;
      iRow[k] = ir;
      iCol[k++] = ic;

      /* 3 */
      iRow[k] = ir;
      iCol[k++] = ic + nv;
    }

    for(;j<nJ; ++j){
      /*1*/
      for(l=0; l<nv; ++l){
        if(iData->knowedJ[j][l]){
          iRow[k] = r + j;
          iCol[k++] = c + l;
        }
      }
    }

  /******************************* 2 *****************************/
    r += nJ;
    c += nv;

    for(j=0; j<nx; ++j){
      ir = r + j;
      /*0*/
      for(l=0; l<nv; ++l){
        if(iData->knowedJ[j][l]){
          iRow[k] = ir;
          iCol[k++] = l;
        }
      }

      /*1*/
      ic = c + j;
      iRow[k] = ir;
      iCol[k++] =  ic - nv;

      /*2*/
      for(l=0; l<nv; ++l){
        if(iData->knowedJ[j][l]){
          iRow[k] = ir;
          iCol[k++] = c + l;
        }
      }

      /*3*/
       iRow[k] = ir;
       iCol[k++] = ic + nv;
    }

    for(;j<nJ; ++j){
    /*2*/
      for(l=0; l<nv; ++l){
        if(iData->knowedJ[j][l]){
          iRow[k] = r + j;
          iCol[k++] = c + l;
        }
      }
    }

  /******************************* 3 *****************************/
    r += nJ;
    c += nv;

    for(j=0; j<nx; ++j){
      ir = r + j;
      /*0*/
      for(l=0; l<nv; ++l){
        if(iData->knowedJ[j][l]){
          iRow[k] = ir;
          iCol[k++] = l;
        }
      }

      /*1*/
      ic = c + j;
      iRow[k] = ir;
      iCol[k++] = ic - 2*nv;

      /*2*/
      iRow[k] = ir;
      iCol[k++] = ic - nv;

      /*3*/
      for(l=0; l<nv; ++l){
        if(iData->knowedJ[j][l]){
          iRow[k] = ir;
          iCol[k++] = c + l;
        }
      }
    }

    for(;j<nJ; ++j){
        /*3*/
        for(l=0; l<nv; ++l){
          if(iData->knowedJ[j][l]){
            iRow[k] = r + j;
            iCol[k++] = c + l;
          }
        }
    }
    r += nJ;
    c += nv;
  } /* end i*/

  /*********************************************/
  /*********************************************/
  /*********************************************/

  for(; i<nsi; ++i){
    /*1*/
    for(j=0; j<nx; ++j){
      /*0*/
      iRow[k] = r + j;
      iCol[k++] = c - nv + j;

      /*1*/
      for(l=0; l<nv; ++l){
        if(iData->knowedJ[j][l]){
          iRow[k] = r + j;
          iCol[k++] = c + l;
        }
      }

      /*2*/
      iRow[k] = r + j;
      iCol[k++] = c + nv + j;
      /*3*/
      iRow[k] = r + j;
      iCol[k++] = c + 2*nv + j;
    }

    for(;j<nJ; ++j){
        /*1*/
        for(l=0; l<nv; ++l){
          if(iData->knowedJ[j][l]){
            iRow[k] = r + j;
            iCol[k++] = c + l;
          }
        }
    }

    /*2*/
    r += nJ;
    c += nv;

    for(j=0; j<nx; ++j){
      /*0*/
      iRow[k] = r + j;
      iCol[k++] = c - 2*nv + j;

      /*1*/
      iRow[k] = iRow[k-1];
      iCol[k++] =  c - nv + j;

      /*2*/
      for(l=0; l<nv; ++l){
        if(iData->knowedJ[j][l]){
          iRow[k] = iRow[k-1];
          iCol[k++] = c + l;
        }
      }

      /*3*/
       iRow[k] = iRow[k-1];
       iCol[k++] = c + nv + j;
    }
    for(;j<nJ; ++j){
        /*2*/
        for(l=0; l<nv; ++l){
          if(iData->knowedJ[j][l]){
            iRow[k] = r + j;
            iCol[k++] = c + l;
          }
        }
    }

    /*3*/
    r += nJ;
    c += nv;

    for(j=0; j<nx; ++j){
      /*0*/
      iRow[k] = r + j;
      iCol[k++] = c - 3*nv + j;

      /*1*/
      iRow[k] = iRow[k-1];
      iCol[k++] = c - 2*nv + j;

      /*2*/
      iRow[k] = iRow[k-1];
      iCol[k++] = c - nv + j;

      /*3*/
      for(l=0; l<nv; ++l){
        if(iData->knowedJ[j][l]){
          iRow[k] = r + j;
          iCol[k++] = c + l;
        }
      }
    }

    for(;j<nJ; ++j){
        /*3*/
        for(l=0; l<nv; ++l){
          if(iData->knowedJ[j][l]){
            iRow[k] = r + j;
            iCol[k++] = c + l;
          }
        }
    }

    r += nJ;
    c += nv;
  }

    /*
    printf("\n\n%i = %i",iData->njac,k);
    assert(0);
    */


  return 0;
}

#endif
