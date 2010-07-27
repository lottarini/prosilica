#include "fit_dp.h"
#include "d_parallel.h"

int main(int argc, char* argv[]){
	
	/* MPI Variables */
	int my_rank = 0, p = 0; /* p is the number of processes */

	MPI_Status status;

 	/* dimension of cropped image */
	int dim = 0;
	
	/* pixels per worker */
	int ppw;

	/* time variables */
	struct timeval tv1, tv2;	

	/* Dimension of the cropped image */
	int dimx = 0, dimy = 0;
	
	/* two fits of Gaussian */
	double result [DIM_FIT], fit [DIM_FIT];
	
	/* image representing Gaussian fit */	
	unsigned char *matrix = NULL;
	
	/* cropped image taken from a Gaussian image */
	unsigned char *cropped = NULL;
	
	/* local partition */
	unsigned char* partition;

	/* indexes */
	int i = 0;
	
	/* error status of gsl_LU */
	int error;
	
	/* check the input parameters */
	if(argc != 2){
		fprintf(stderr, "Invalid number of parameters\n");
		exit(EXIT_FAILURE);
	}

	/* data for the LU solver */
	gsl_vector *delta = gsl_vector_alloc(DIM_FIT);
	gsl_permutation* permutation = gsl_permutation_alloc(DIM_FIT);
			
	/* Initialize of MPI */
	MPI_Init (&argc, &argv);
	/* Every process takes their own rank*/
	MPI_Comm_rank (MPI_COMM_WORLD, &my_rank);
	/* Total number of processes */
	MPI_Comm_size (MPI_COMM_WORLD, &p);

/*********************************************************************
						INIT
*********************************************************************/		

	// angosciante
	double* data = (double*) malloc(sizeof(double)*DIM_FIT*(DIM_FIT+1));
	gsl_matrix_view matrice = gsl_matrix_view_array(data,DIM_FIT,DIM_FIT);
	gsl_vector_view vettore = gsl_vector_view_array(data + (DIM_FIT*DIM_FIT),DIM_FIT);

	//usato solo per la reduce... si fa in place? loop unrolling?
	double* ret = (double*) malloc(sizeof(double)*DIM_FIT*(DIM_FIT+1));
	
	if(my_rank == EMITTER){
		/* THIS IS THE TASK OF THE EMITTER PROCESS */
#if DEBUG
		printf("EMITTER: RANK %d\n",my_rank);
#endif		
		/* initialization of the fit */
		initialization(argv[1], result, fit, &matrix, &cropped, &dimx, &dimy);
	
		/* send to the workers the parameters and images */
		dim = dimx * dimy;
		for(i = PS; i < p; i++){
			MPI_Send(&dimx, 1, MPI_INT, i, PARAMETERS, MPI_COMM_WORLD);
			MPI_Send(&dimy, 1, MPI_INT, i, PARAMETERS, MPI_COMM_WORLD);
			MPI_Send(fit, DIM_FIT, MPI_DOUBLE, i, RESULTS, MPI_COMM_WORLD);
#if DEBUG
			printf("EMITTER: DATI INIZIALI INVIATI A %d\n",i);
#endif
		}
		
		/* I have to determine the number of pixels per worker (risky since they must be multiples of the processes) */
		ppw = (dimx*dimy)/p;
		printf("Pixel per worker: %f",(double) (dimx*dimy)/ (double )p);
		fflush(stdout);
	}
	else{
#if DEBUG
		printf("WORKER: RANK %d\n",my_rank);
#endif
		/* receive the dimension of the cropped image */
		MPI_Recv(&dimx, 1, MPI_INT, EMITTER, PARAMETERS, MPI_COMM_WORLD, &status);
		MPI_Recv(&dimy, 1, MPI_INT, EMITTER, PARAMETERS, MPI_COMM_WORLD, &status);
		/* receive the fit of the Gaussian */
		MPI_Recv(fit, DIM_FIT, MPI_DOUBLE, EMITTER, RESULTS, MPI_COMM_WORLD, &status);
		
#if DEBUG
		printf("PROCESSO %d, the initial fit: %f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", my_rank, fit[PAR_A], fit[PAR_X],
		fit[PAR_Y], fit[PAR_SX], fit[PAR_SY], fit[PAR_a], fit[PAR_b], fit[PAR_c]);
#endif

	}
	
	
	/*********************************************************************
	 LOOP on ELEMENTS
	 *********************************************************************/		
	
	ppw = (dimx*dimy)/p;
	partition = (unsigned char*) malloc(sizeof(unsigned char)*ppw);
	
	printf("processo %d sopravvissuto alla init\n",my_rank);
	MPI_Barrier(MPI_COMM_WORLD);

	for (i=0;i<STREAMLENGTH;i++){
	
		printf("LOOP %d\n",i);
		
		MPI_Scatter ( cropped, ppw, MPI_UNSIGNED_CHAR, 
						 partition, 
						 ppw, 
						 MPI_UNSIGNED_CHAR, 
						 EMITTER, 
						 MPI_COMM_WORLD);
						 
		printf("processo %d sopravvissuto alla scatter\n",my_rank);
		MPI_Barrier(MPI_COMM_WORLD);				 
		
		procedure (partition, dimx/p, dimy/p, fit,matrice,vettore);
		
		printf("processo %d sopravvissuto alla PROCEDURE\n",my_rank);
		MPI_Barrier(MPI_COMM_WORLD);
		
		MPI_Reduce (data,ret,DIM_FIT*(DIM_FIT+1),MPI_DOUBLE,MPI_SUM,EMITTER,MPI_COMM_WORLD);
		
		printf("processo %d sopravvissuto alla REDUCE\n",my_rank);
		fflush(stdout);
		MPI_Barrier(MPI_COMM_WORLD);
		
		// da cambiare con il collettore?
		if (my_rank == EMITTER){
			gsl_linalg_LU_decomp(&matrice.matrix, permutation, &error); /* TEST ERRORE--> TODO*/
			gsl_linalg_LU_solve(&matrice.matrix, permutation, &vettore.vector, delta);
			//printf("%f \n",gsl_vector_get(delta, 7));
			//printf("%f \n" ,result[0]);

			int j;
			for(j = 0; j < DIM_FIT; j++){
				printf("%d: %f + %f =" ,j,result[j],gsl_vector_get(delta, j));
				fit[j]  = fit[j]  + gsl_vector_get(delta, j);
				printf("%f\n",fit[j]);
			}
			printf("finito l'update\n");
		}
		fflush(stdout);
		MPI_Barrier(MPI_COMM_WORLD);
		
	}
	
	
	/* Finalize of MPI */
	MPI_Finalize();
	
	return 0;
}
