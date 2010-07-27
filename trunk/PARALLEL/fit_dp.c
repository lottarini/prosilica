#include "fit_dp.h"

static unsigned char *crop = NULL;

/***************************************************************************************************************
							Centroid
 ****************************************************************************************************************/

/** image need to be the output of a createMask function
 both the center position and the dimension of the centroid depends
 on the filter parameter of the createMask function previously used*/

void centroid(unsigned char *image, int w, int h, double *x, double *y, double *sigma_x, double *sigma_y) {	
	int npixels = w * h;
	int* counth = (int*) malloc (sizeof(int) * h);
	int* countw = (int*) malloc (sizeof(int) * w);	
	int count = 0, i = 0;
	double w_center = 0.0, h_center = 0.0;
	int left_border = 0, right_border = 0;
	int down_border = 0, up_border = 0;

    	for (i = 0; i < h; i++)
		counth [i] = 0;
    	for (i = 0; i < w; i++)
		countw [i] = 0;
	for (i = 0; i < npixels; i++) {
		if (image [i]) {
			++count;
			++countw [i % w];
			++counth [i / w];
		}
    	}
	
	/* W CENTER */
	for (i = 0; i < w; i++) {
		w_center = w_center + ((double) countw [i] / count) * (i + 1);
		if (!left_border && countw [i])
			left_border = i;
		if (left_border && !right_border && !countw [i])
			right_border = i;
	}
	
	/* H CENTER */
	for (i = 0; i < h; i++) {
		h_center = h_center + ((double) counth [i] / count) * (i + 1);
		if (!down_border && counth[i])
			down_border = i;
		if (down_border && !up_border && !counth [i])
			up_border = i;
	}
	*x = w_center;
	*y = h_center;
	*sigma_x = (right_border - left_border) / 2.0;
	*sigma_y = (up_border - down_border) / 2.0;
	/* free the space */
	free(counth);
	free(countw);
}


/***************************************************************************************************************
 Matrix
 ****************************************************************************************************************/
unsigned char * createMatrix (int length, int width, double* result){
	int i = 0, j = 0;	
	int dim = length * width;
	unsigned char* matrix = (unsigned char*) malloc (dim);
	unsigned char* p = matrix;	
	/* build the image */
	for (i = 0;i < length; i++)
		for(j = 0; j < width; j++)
			*p++ = (unsigned char) evaluateGaussian(result, j, i);
	return matrix;
}

/***************************************************************************************************************
 Cookie
 ****************************************************************************************************************/

unsigned char *createMask(unsigned char *image, int w, int h, int max, int min, double filter) {
	int threshold = (int) (filter * (max - min));
	int i = 0;
    	int npixels = w * h;
	unsigned char *cookie = (unsigned char*) malloc(npixels);
	for (i = 0; i < npixels; i++) {
		unsigned char temp = image[i];
		if (temp > threshold)
			cookie[i] = MAXIMUM;
		else
			cookie[i] = 0;
	}
	return cookie;
}

/***************************************************************************************************************
 Crop function
 ****************************************************************************************************************/

unsigned char *cropImage(const unsigned char *input, int w, int h, int x1, int x2, int y1, int y2) {
	int count = 0, i = 0;
	int limit = w * (y2 + 1);
	int dimension = (x2 - x1 + 1) * (y2 - y1 + 1);
	if (crop == NULL)
		crop = (unsigned char*) malloc(dimension);
	for (i = 0; i < limit; i++)
		if (i % w >= x1 && i % w <= x2 && i / w >= y1 && i / w <= y2)
			crop[count++] = input[i];
	return crop;
}

/***************************************************************************************************************
 Evaluate Gaussian at coordinates (x,y)
 ****************************************************************************************************************/

double evaluateGaussian(double* gaussian, int x, int y) {
	double slope = gaussian[PAR_a] * x + gaussian[PAR_b] * y + gaussian[PAR_c];
	double x_arg = pow(((double) x - gaussian[PAR_X]), 2.0) / pow(gaussian[PAR_SX], 2.0);
	double y_arg = pow(((double) y - gaussian[PAR_Y]), 2.0) / pow(gaussian[PAR_SY], 2.0);
	double arg = -(x_arg + y_arg);
	return gaussian[PAR_A] * exp(arg) + slope;
}

/***************************************************************************************************************
								fit algorithm
 ****************************************************************************************************************/

int procedure (const unsigned char *data, int w, int h,double * results, gsl_matrix_view matrice,gsl_vector_view vettore) {
	int npixels = w * h;
	double *diff = (double*) malloc (sizeof(double) * npixels);
	int i = 0, x = 0, y = 0, base = 0;
	
	/* vedtors for calculations */

	gsl_matrix_view gsl_M;
	gsl_vector_view differenze;


	/** variables used to keep track of the square error*/
	double *M = (double*) malloc(sizeof(double) * DIM_FIT * npixels);
	
	double square = 0.0, diff_x = 0.0, diff_y = 0.0, frac_x = 0.0, frac_y = 0.0, sig2x = 0.0, sig2y = 0.0, dexp = 0.0;
		
	/* Task over the image */
	for (i = 0; i < npixels; i++) {
		x = (i + 1) % w;
		y = (i + 1) / w;
			
		base = i * DIM_FIT;
		diff[i] = data[i] - evaluateGaussian(results, x, y);
			
		diff_x = x - results[PAR_X];
		diff_y = y - results[PAR_Y];
		sig2x = pow(results[PAR_SX], 2);
		sig2y = pow(results[PAR_SY], 2);
		frac_x = pow(diff_x, 2) / sig2x;
		frac_y = pow(diff_y, 2) / sig2y;
		dexp = exp(frac_x + frac_y);
			
		M[base] = 1 / dexp;
		M[base + 1] = (results[PAR_A] * (2 * x - 2 * results[PAR_X])) / (sig2x * dexp);
		M[base + 2] = (results[PAR_A] * (2 * y - 2 * results[PAR_Y])) / (sig2y * dexp);
		M[base + 3] = (2 * results[PAR_A] * pow(diff_x, 2)) / (pow(results[PAR_SX], 3) * dexp);
		M[base + 4] = (2 * results[PAR_A] * pow(diff_y, 2)) / (pow(results[PAR_SY], 3) * dexp);
		M[base + 5] = x;
		M[base + 6] = y;
		M[base + 7] = 1.0;
	}
		
	/* square calculation and array adjustment */
	for (i = 0; i < npixels; i++)
		square = square + pow(diff[i], 2);
		
	gsl_M = gsl_matrix_view_array(M, npixels, DIM_FIT);
	differenze = gsl_vector_view_array(diff, npixels);
		
	/* Compute matrix = M'*M */
	gsl_blas_dgemm(CblasTrans, CblasNoTrans, 1.0, &gsl_M.matrix, &gsl_M.matrix, 0.0, &matrice.matrix);
	/* Compute vector = M'*diff */
	gsl_blas_dgemv(CblasTrans, 1.0, &gsl_M.matrix, &differenze.vector, 0.0, &vettore.vector);
	/* Compute the delta vector of deviation */

	return (int) square;
}

/***************************************************************************************************************
 Calculate Max & Min of an image
 ****************************************************************************************************************/

void maxmin(unsigned char *image, int w, int h, int *max, int *min) {
    int npixels = w * h, i = 0;
    *max = 0;
    *min = MAXIMUM;
    for (i = 0; i < npixels; i++) {
		if (image[i] > *max)
			*max = image[i];
		if (image[i] < *min)
			*min = image[i];
    }
}

/***************************************************************************************************************
				Writing mono8 black and white tiff function
****************************************************************************************************************/

void writeImage(unsigned char* image, char* dest, int w, int h) {
		
		TIFF* out = TIFFOpen(dest, "w");
		tsize_t linebytes;
		uint32 row = 0;
		/* buffer used to store the row of pixel information for writing to file */
		unsigned char *buf = NULL;
		/* 8bit image */
		int sampleperpixel = 1;
		
		TIFFSetField (out, TIFFTAG_IMAGEWIDTH, w);  /* set the width of the image */
		TIFFSetField(out, TIFFTAG_IMAGELENGTH, h);    /* set the height of the image */
		TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel);   /* set number of channels per pixel */
		TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);    /* set the size of the channels */
		TIFFSetField(out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);    /* set the origin of the image */
		/* Some other essential fields to set that you do not have to understand for now */
		TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
		TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

		/* a quanto pare si scrive una row alla volta!!! quindi una row e` lunga: */
		linebytes = sampleperpixel * w;     /* length in memory of one row of pixel in the image */
		
		/* Allocating memory to store the pixels of current row */
		if (TIFFScanlineSize(out) == linebytes)
			buf =(unsigned char *)_TIFFmalloc(linebytes);
		else
			buf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(out));
		
		
		/* We set the strip size of the file to be size of one row of pixels */
		TIFFSetField(out, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(out, w*sampleperpixel));
		
		/* Now writing image to the file one strip at a time */
		for (row = 0; row < h; row++) {
			memcpy(buf, &image[(h-row-1)*linebytes], linebytes);    /* check the index here, and figure out why not using h*linebytes */
			if (TIFFWriteScanline(out, buf, row, 0) < 0) break;
		}
		(void) TIFFClose(out);
		if (buf) _TIFFfree(buf);
}

/***************************************************************************************************************
				Initialize of the fit
****************************************************************************************************************/

int initialization(char* parameter, double* result, double* fit, unsigned char** matrix, unsigned char** cropped, int* dimx, int* dimy){
		
	/* parameters for the cookie cutter */
	double x0 = 0.0, y0 = 0.0;
	double FWHM_x = 0.0, FWHM_y = 0.0;
	int span_x = 0, span_y = 0;
	int x = 0 , y = 0;
	/* File conteining parameters*/
	FILE* parameters = NULL;
	/* width and length of the input image */
	int width = 0, length = 0;
	/* parameters fro create the mask */
	int max = 0, min = 0;
	/* pixel mask for reduce the dimension of the region to analyze */
	unsigned char *mask = NULL;

	int i = 0;
		/* reading the input parameters */		
		if((parameters = fopen(parameter, "r")) == NULL){
			fprintf(stderr, "File not valid");
			exit(EXIT_FAILURE);		
		}
	
		/* initialize the dimension of the image */
		if(fscanf(parameters, "%d\t%d\t", &width, &length) == 0){
			fprintf(stderr, "File not valid");
			exit(EXIT_FAILURE);							
		}
		/* initialize the fit of the Gaussian */
		for(i = 0; i < DIM_FIT; i++){
			if(fscanf(parameters, "%lf\t", &result[i]) == 0){
				fprintf(stderr, "File not valid");
				exit(EXIT_FAILURE);							
			}		
			fit[i] = 0;
		}

		/* image representing the Gaussian fit */
		*matrix = createMatrix( length, width, result);
	
		/* writing the image to be fitted in a FIT file */
#if DEBUG
		writeImage((unsigned char *) *matrix,(char *) "gaussiana.tiff", width, length);
#endif	
		maxmin( (unsigned char*) *matrix, width, length, &max, &min);

#if DEBUG	
		printf("MAX: %d MIN: %d\n", max, min);
#endif
	
		/* a pixel mask is created in order to reduce the dimensione of the region to analyze with the centroid */
		mask = createMask( (unsigned char*) *matrix, width, length, max, min, CROP_PARAMETER);
	
#if DEBUG
		writeImage(mask, (char *) "mask.tiff", width, length);
#endif
	
		centroid(mask, width, length, &x0, &y0, &FWHM_x, &FWHM_y);
	
#if DEBUG
		printf("centro in %f - %f\nCon ampiezza %f e %f\n", x0, y0, FWHM_x, FWHM_y);
#endif
	
		free(mask);
	
		/* inizialization for the diameter of the gaussian*/
		span_x = (int) (2 * FWHM_x);
		span_y = (int) (2 * FWHM_y);
	
		/* determination of the dimension of the crop */
		*dimx = 2 * span_x + 1;
		*dimy = 2 * span_y + 1;
	
		/* inizialization of the position coordinates */
		x = (int) x0;
		y = (int) y0;
	
		/**
		 inizialization of the fit of the Gaussian
		 NOTE: the coordinates of the position (x,y) are relative to the cropped portion of the image.
		 The value of span_x, which is approximately the diameter of the gaussian, is generally not as bad
		 as you may think to start the fit.
		 */
		fit[PAR_A] = max;
		fit[PAR_X] = span_x;
		fit[PAR_Y] = span_y;
		fit[PAR_SX] = FWHM_x;
		fit[PAR_SY] = FWHM_y;
		fit[PAR_c] = min;	

#if DEBUG
	printf("%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", fit[PAR_A], fit[PAR_X] + x - span_x, fit[PAR_Y] + y - span_y, fit[PAR_SX], fit[PAR_SY], fit[PAR_a], fit[PAR_b], fit[PAR_c]);
#endif
		*cropped = cropImage((unsigned char*) *matrix, width, length, x - span_x, x + span_x, y - span_y, y + span_y);
	
#if DEBUG
		writeImage(*cropped, (char *) "./CROP.tiff", *dimx, *dimy);
#endif

	return 0;
}