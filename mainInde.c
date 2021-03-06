#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#include "normals.h"
#include "mpiutil.h"
#include "matrixBlockStore.h"

/* Main program 
  a straight forward implementation to read the input matrices and build the reduced normal matrix for the global block
  created 7/09/2014
  author Alex Bombrun
  see TN APB-009
  
  mpirun -n 4 bigmatrix.mpi 4
 */
int main(int argc, char **argv) {
  
    // Normal matrix block AB
    const char* profileAB_file_name= "./data/NormalsAB/profile.txt";
    const char* valuesAB_file_name = "./data/NormalsAB/values.txt";
    //const char* AB_location = ".data/NormalsAB";
    
    int* profileAB = NULL;
    int profileAB_length,dimensionAB;
    double* matrixAB = NULL;
    
    // Normal matrix block G
    const char* profileG_file_name= "./data/NormalsG/profile.txt";
    const char* valuesG_file_name = "./data/NormalsG/values.txt";
    //const char* G_location = ".data/NormalsG";
      
    int* profileG = NULL;
    int profileG_length, dimensionG;
    double* matrixG = NULL;
    
    const char* matrixGtAB_location = "./data/SparseGtAB";
      
    int i, t; 	 // loop indices
    int i0, i1;  // main row numbers of matrix (CABG)' to be processed
    int j0, j1;  // secondary row numbers
    int idim,jdim;
    
    int rank, p;  // rank and number of mpi process
    
    int n_blocks; // block size or the number of diagonal blocks
    int i_block, j_block; // the diagonal block index
    
    int n_pTasks, n_blockTasks;
    
    int ierr = 0; // process error
   
    double* matrixCor = NULL;
    double* matrixCGABi = NULL;
    double* matrixCGABj = NULL;
    
    FILE* store = NULL;
    
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    n_blocks = get_n_blocks(argc,argv,p);
    n_pTasks = get_n_pTasks(p,rank,n_blocks);
  
    // set up the input matrices AB and G
    profileAB_length = getNumberOfLine(profileAB_file_name);
    profileAB = malloc( sizeof(int) * profileAB_length );
    readMatrixInt(profileAB,profileAB_file_name);
    dimensionAB = sumVectorInt(profileAB,profileAB_length);
    matrixAB = calloc(dimensionAB,sizeof(double));
    readMatrixDouble(matrixAB,valuesAB_file_name);
    
    profileG_length = getNumberOfLine(profileG_file_name);
    profileG = malloc( sizeof(int) * profileG_length );
    readMatrixInt(profileG,profileG_file_name);
    dimensionG = sumVectorInt(profileG,profileG_length);
    matrixG = calloc(dimensionG,sizeof(double));
    readMatrixDouble(matrixG,valuesG_file_name);
    
    printf("%d/%d: inde, number of attitude parameters=%d , number of source global parameters=%d , number of blocks=%d\n", rank, p, profileAB_length, profileG_length,n_blocks);
    
   for(t=0;t<n_pTasks;t++){
      i_block = (rank+t*p); // the diagonal block index depends on the rank index and the task index
      n_blockTasks = get_n_blockTasks(i_block,n_blocks);
      
      // diagonal block
      i0=mpi_get_i0(profileG_length,i_block,n_blocks);
      i1=mpi_get_i1(profileG_length,i_block,n_blocks);
      printf("%d/%d: block %d/%d (%d,%d), %d tasks\n",rank,p,i_block,n_blocks,i0,i1,n_blockTasks);
      
      openStore(&store,i_block,"./data/ReducedNormals");
      
      idim = i1-i0;
      jdim = i1-i0;
      printf("%d/%d: process rows from %d to %d\n",rank,p,i0,i1);
      matrixCGABi = calloc((i1-i0)*profileAB_length,sizeof(double));
      reduce(profileAB_length, profileAB, matrixAB, matrixGtAB_location, matrixCGABi, i0, i1);
      
      matrixCor = calloc(idim*jdim,sizeof(double));
      setBlockMatrix(matrixCor,i0,i1,i0,i1,matrixG,profileG_length,profileG);   
      dgemmAlex(matrixCGABi,idim,profileAB_length,matrixCGABi,jdim,profileAB_length,matrixCor,idim,jdim);
      saveBlock(i0,i1,i0,i1,matrixCor,store);
      free(matrixCor);
      printf("%d/%d: finished computing block (%d,%d)x(%d,%d) of the correction\n", rank, p, i0, i1, i0, i1);
      
      for(i=1;i<n_blockTasks;i++){  // off diagonal blocks
	  j_block = mpi_get_diag_block_id(i_block, i, n_blocks);
	  j0 = mpi_get_i0(profileG_length, j_block, n_blocks);
	  j1 = mpi_get_i1(profileG_length, j_block, n_blocks);
	  printf("%d/%d: block %d (%d,%d) linked with block %d (%d,%d) \n",rank, p, i_block, i0, i1, j_block, j0, j1);
	  jdim =j1-j0;
	  matrixCGABj = calloc((j1-j0)*profileAB_length,sizeof(double));
	  reduce(profileAB_length, profileAB, matrixAB, matrixGtAB_location, matrixCGABj, j0, j1);
      
	  matrixCor = calloc(idim*jdim,sizeof(double));
	  setBlockMatrix(matrixCor,i0,i1,j0,j1,matrixG,profileG_length,profileG);
	  dgemmAlex(matrixCGABi,idim,profileAB_length,matrixCGABj,jdim,profileAB_length,matrixCor,idim,jdim);
	  saveBlock(i0,i1,j0,j1,matrixCor,store);
	  printf("%d/%d: block %d (%d,%d) linked with block %d (%d,%d) finished \n",rank, p, i_block, i0, i1, j_block, j0, j1);
	  free(matrixCor);
	  free(matrixCGABj);
      }
      fclose(store);
      free(matrixCGABi); 
    }
    MPI_Finalize(); // the process are independent no blocking
    return ierr;
}

