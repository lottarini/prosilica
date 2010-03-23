include ../conf

CC = mpic++
FLAGS = -DDEBUG
GSL_LIB=-lgsl -lgslcblas
LIB=$(GSL_LIB) -lm -ltiff

NP=4

.PHONY : parallel test clean

parallel:
	$(CC)$(CVER) -D_$(ARCH) -D_$(OS) $(FLAGS) $(GSL_INCLUDE_DIR) $(LIB) ./parallel.cpp ./tiffPostElaboration.cpp ./fit.cpp $(GSL_LIB_DIR) -o parallel

test:
	mpirun -np $(NP) ./parallel ./parameters

clean:
	rm -f RESULTS* *.tiff FIT Centroid *~