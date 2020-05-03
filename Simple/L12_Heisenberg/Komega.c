#include "Def.h"
#include "mpi.h"

double complex **cd_2d_allocate(int N,int M);
void free_cd_2d_allocate(double complex**A);

int main(int argc, char* argv[]){
    
   char sdt[256],sdt_2[256],ctmp[256];
   FILE *fp,*fp_2, *fplist;
   double complex **Ham,*tmp_Ham;
   double complex **s,*tmp_s;
   double complex **U,*tmp_U;
   double complex **V,*tmp_V;
   double complex **norm_Ham,*tmp_norm_Ham;
   double *r,*norm_r;
   double complex *vec;
   int int_z;
   int int_i,int_j,cnt,All_N;
   int int_k,int_l;
   int int_A,int_B;
   int tmp_i,tmp_j;
   int tot_i,nk;
   int rank;
   int itmp,ihermite,ham_i,ham_j;
   int cnt_nr,nr;
   double dHam_re,dHam_im;

   int itr,ndim,nl,nz,itermax,*status;
   double complex *x,*z,*v2,*v12,*rhs,*r_l;
   double complex *v14,*v4;
   double *res;
   double complex tmp,tmp_12,tmp_14;
   double threshold;

   double  norm;
   double  rho,gamma;
   unsigned long int u_long_i; 
   dsfmt_t dsfmt;
   
   u_long_i  = 120;

   /*[s] read Hamiltonian defined in Matrix Market form*/
   sprintf(sdt,"Ham.dat");
   fp = fopen(sdt, "r");
   if(fp==NULL) return 0;
   /*[s] skip header*/
   fgets(ctmp, sizeof(ctmp)/sizeof(char), fp);
   /*[e] skip header*/
   fgets(ctmp, sizeof(ctmp)/sizeof(char), fp);
   sscanf(ctmp, "%ld %ld %ld\n", &All_N, &All_N, &ihermite);
   //printf("%d \n",ihermite);
   /*[s] allocate Hamiltonian*/
   Ham      = cd_2d_allocate(All_N,All_N);
   for(int_i=0;int_i<All_N;int_i++){
     for(int_j=0;int_j<All_N;int_j++){
       Ham[int_i][int_j] = 0.0;
     }
   }
   /*[e] allocate Hamiltonian*/
   for(int_i=0; int_i < ihermite; int_i++){
     fgets(ctmp, sizeof(ctmp) / sizeof(char), fp);
     sscanf(ctmp, "%ld %ld %lf %lf\n",&ham_i, &ham_j, &dHam_re, &dHam_im);
     //printf("Ham: %d: %d %d %lf %lf \n",int_i,ham_i,ham_j,dHam_re,dHam_im);
     Ham[ham_i-1][ham_j-1]=dHam_re+I*dHam_im;           /*-1: 1 offset to 0 offset*/
     Ham[ham_j-1][ham_i-1]=conj(Ham[ham_i-1][ham_j-1]); /*-1: 1 offset to 0 offset*/
   }
   fclose(fp);
   /*[e] read Hamiltonian defined in Matrix Market form*/

   /*[s] set parameters*/
   //All_N     = ihermite;
   ndim      = All_N;
   nl        = ndim;
   dsfmt_init_gen_rand(&dsfmt,u_long_i);
   /*[e] set parameters*/

   
   /*[s]allocate for vectors*/
   status  = (int *)malloc((3)*sizeof(int));
   res     = (double *)malloc((nz)*sizeof(double));

   x       = (double complex*)malloc((nl)*sizeof(double complex));
   vec     = (double complex*)malloc((All_N)*sizeof(double complex));
   r_l     = (double complex*)malloc((All_N)*sizeof(double complex));
   v2      = (double complex*)malloc((All_N)*sizeof(double complex));
   v12     = (double complex*)malloc((All_N)*sizeof(double complex));
   v4      = (double complex*)malloc((All_N)*sizeof(double complex));
   v14     = (double complex*)malloc((All_N)*sizeof(double complex));
   rhs     = (double complex*)malloc((All_N)*sizeof(double complex));
   /*[e]allocate for vectors*/

   /*[s] generating initial vector*/
   for(int_i=0;int_i<All_N;int_i++){
     rhs[int_i]  = 2*(dsfmt_genrand_close_open(&dsfmt)-0.5);
     rhs[int_i] += 2*I*(dsfmt_genrand_close_open(&dsfmt)-0.5);
   } 
   /*[e] generating initial vector*/
   /*[s] normalize generating initial vector*/
   norm=0.0;
   for(int_i=0;int_i<All_N;int_i++){
     norm += (rhs[int_i])*conj(rhs[int_i]);  
   }
   norm=sqrt(norm);
   for(int_i=0;int_i<All_N;int_i++){
     rhs[int_i]  = rhs[int_i]/norm;
     v2[int_i]   = rhs[int_i]; 
     v12[int_i]  = 0;
     v4[int_i]   = conj(rhs[int_i]); 
     v14[int_i]  = 0;
   }
   /*[e] normalize generating initial vector*/
     
   /*[s] komega initialization*/
   komega_bicg_init(&ndim, &nl, &nz, x, z, &itermax, &threshold, NULL);
   /*[e] komega initialization*/
   /*[s] komega main loop: obtaining x solving (zI-H)x=v2*/
   //printf("ini:cnt_nr =%d \n",cnt_nr);
   for(itr=0;itr<itermax;itr++){
     for(int_i=0;int_i<All_N;int_i++){
       r_l[int_i] = v2[int_i];
     }
     /*[s]mat-vec*/
     MatVec(All_N,Ham,v2,v12);
     MatVec(All_N,Ham,v4,v14);
     /*[e]mat-vec*/
     komega_bicg_update(v12,v2,v14,v4,x,r_l,status);
     if(status[0]<0){ //converge
       break;
     }
   }
   //printf("fin:cnt_nr =%d \n",cnt_nr);
   /*[e] komega main loop: obtaining x solving (zI-H)x=v2*/
   komega_bicg_finalize();

   for(int_i=0;int_i<All_N;int_i++){
     print("%d %lf %lf\n",int_i,creal(x[int_i]),cimag(x[int_i]));
   }
  
   return 0;
}

complex double **cd_2d_allocate(int N,int M){
    int int_i;
    complex double **A;
    A     = (complex double**)calloc((N),sizeof(complex double));
    A[0]  = (complex double*)calloc((M*N),sizeof(complex double));
    for(int_i=0;int_i<N;int_i++){
      A[int_i] = A[0]+int_i*M;
    }
    return A;
}

void free_cd_2d_allocate(double complex**A){
    free(A[0]);
    free(A);
}


