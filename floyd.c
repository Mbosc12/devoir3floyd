#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include <CL/cl.h>

//def
#define SOURCE_CL "floydGraph.cl"

//lecture de fichier source
char* load_program_source(const char *filename);

int main(int argc, char *argv[]) {	
	
	// step 0 : configuration and initialization
	int N = 0;

	printf(">> CONFIG\n");
	printf("Input N value:" );
	scanf_s("%d", &N);
	
	int *A = NULL;
	int x = 0; j = 0;
	size_t sizeData = sizeof(int) * N * N;
	
	A = (int*) malloc(sizeData);
	//initialization matrix
	for(x = 0; x < N; x++) {
		for(y = 0; y < N; y++) {
			//default
			A[x*N+y] = N + 1;
			//arc i à i
			if(x == y) A[x*N+y] = 0;
			//arc i à i+1
			if(x == y-1) A[x*N+y] = 1;
			//arc de n-1 à 0
			if((x == N-1) && (y == 0)) A[x*N+y] = 1;
		}
	}
	
	//execute on the OpenCL host
	char* programSource = load_program_source(SOURCE_CL);

	if(programSource == NULL) printf("ERROR\n");
	
	// step 1 :  Discover and initialize the platforms
	
	cl_uint numPlatforms = 0;
	cl_platform_id *platforms = NULL;
	
	//calc nb platforms
	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	printf("Number of platforms : %d\n", numPlatforms);

	//memory
	platforms = (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));

	//find platforms
	status = clGetPlatformIDs(numPlatforms, platforms, NULL);

	char Name[1000];
	clGetPlatformInfo(platforms[0], CL_PLATFORM_NAME, sizeof(Name), Name, NULL);
	printf("Name of platform : %s\n", Name);
	fflush(stdout);
	
	// step 3 : discover and initialize devices
	cl_uint numDevices = 0;
    cl_device_id *devices = NULL;
	
	status = clGetDeviceIDs(
                            platforms[0],
                            CL_DEVICE_TYPE_ALL,
                            0,
                            NULL,
                            &numDevices);

    printf("Number of devices = %d\n", (int)numDevices);
    
	// memory
    devices = (cl_device_id*)malloc(
                          numDevices*sizeof(cl_device_id));
    
    // find devices
    status = clGetDeviceIDs(
                            platforms[0],
                            CL_DEVICE_TYPE_ALL,
                            numDevices,
                            devices,
                            NULL);

    for (int i = 0; i < numDevices; i++){
        clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(Name), Name, NULL);
        printf("Name of device %d: %s\n\n", i, Name);
    }
	
	//step 3 : create context
	printf("create context\n");
    fflush(stdout);
	
    cl_context context = NULL;
    context = clCreateContext(
                              NULL,
                              numDevices,
                              devices,
                              NULL,
                              NULL,
                              &status);
	
	//step 4 : create a command queue
	printf("create command queue\n");
	
    fflush(stdout);
    cl_command_queue cmdQueue;
	
    cmdQueue = clCreateCommandQueue(
                                    context,
                                    devices[1],
                                    0,
                                    &status);
		
	//step 5 create buffer
	
	printf("create buffer\n");
    fflush(stdout);
	
    cl_int bufferA;  

    bufferA = clCreateBuffer(
                             context,
                             CL_MEM_READ_WRITE,
                             datasize,
                             NULL,
                             &status);

	//step 6 write host data to device buffer
	printf("write to buffer\n");

    fflush(stdout);

    status = clEnqueueWriteBuffer(
                                  cmdQueue,
                                  bufferA,
                                  CL_TRUE,
                                  0,
                                  datasize,
                                  A,
                                  0,
                                  NULL,
                                  NULL);

	// step 7 create et compile programm
    printf("CreateProgramWithSource\n");
    fflush(stdout);

    cl_program program = clCreateProgramWithSource(
                                                   context,
                                                   1,
                                                   (const char**)&programSource,
                                                   NULL,
                                                   &status);
	
    printf("Compilation\n");
    fflush(stdout);
	
    status = clBuildProgram(
                            program,
                            numDevices,
                            devices,
                            NULL,
                            NULL,
                            NULL);
    if (status) printf("COMPILATION ERROR: %d\n", status);
	
	// step 8 : create kernel

	cl_kernel kernel = NULL;
    
    printf("Create kernel\n");
    fflush(stdout);
    kernel = clCreateKernel(program, "floyd algorithm", &status);
	
	// step 9 : kernel arguments
	
		printf("clSetKernelArg : ");
	fflush(stdout);
	int K = 0;
	status = clSetKernelArg(kernel, 0, sizeof(cl_int), (void*)&bufferA);
	status = clSetKernelArg(kernel, 1, sizeof(int), &N);
	status = clSetKernelArg(kernel,	2, sizeof(int), &K);

	// step 10 : configure work-item structure
	
	// max work-item in a work-group
	size_t MaxGroup;
	clGetDeviceInfo(devices[1], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &MaxGroup, NULL);
	printf("CL_DEVICE_MAX_WORK_GROUP_SIZE : %d\n", (int)MaxGroup);

	// max work-item in a work-group > dimensions
	size_t MaxItems[3];
	clGetDeviceInfo(devices[1], CL_DEVICE_MAX_WORK_ITEM_SIZES, 3 * sizeof(size_t), MaxItems, NULL);
	printf("CL_DEVICE_MAX_WORK_ITEM_SIZES : (%d, %d, %d)\n", (int)MaxItems[0], (int)MaxItems[1], (int)MaxItems[2]);

	size_t globalWorkSize[2] = { N, N };
	size_t localWorkSize[3] = { 3, 3 };
	
	// step 11 : enqueue kernel for execution
	
	printf("Kernels call\n");
	clock_t begin = clock();

	int gstatus = 0;
	printf("\rKernel call : ... ");
	for (K = 0; K < N; K++) {
		clSetKernelArg(kernel, 2, sizeof(int), &K);
		gstatus += clEnqueueNDRangeKernel(cmdQueue, kernel, 2, NULL, globalWorkSize, USE_LOCAL_GROUP ? localWorkSize : NULL, 0, NULL, NULL);
	}
	clFinish(cmdQueue);

	//Debug
	clock_t end = clock();
	double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("Execution time : %f sec\n", time_spent);
	
	// step 12 : read output buffer back
	
	//Debug
	printf("result \n");

	// read new matrix
	printf("clEnqueueReadBuffer : ", time_spent);
	status = clEnqueueReadBuffer(cmdQueue, bufferA, CL_TRUE, 0, datasize, A, 0, NULL, NULL);
	clFinish(cmdQueue);
	if (status) printf("ERROR (code %d)\n", status); else printf("OK\n");
	printf("\n");

	//Affichage de la matrice
	while (1) {
		printf("See value of A[i][j] :\n");
		printf("[i] : ");
		scanf_s("%d", &i);
		printf("[j] : ");
		scanf_s("%d", &j);
		if ((i >= 0)&(j >= 0)&&(i < N)&&(j < N)) printf("A[%d][%d] = %d\n\n", i, j, A[i*N+j]);
		else printf("A[%d][%d] = NULL\n\n", i, j);
	}
	
	// step 13 : release openCL resources
	clReleaseKernel(kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(cmdQueue);
	clReleaseMemObject(bufferA);
	clReleaseContext(context);

	//Libération des ressources de l'hôte
	free(A);
	free(platforms);
	free(devices);

	return 0;
}

char load_program_source(const char *filename) {
    //initialisation
	FILE *fp;
    char *source;
    int sz=0;

    struct stat status;

	//open
    fp = fopen(filename, "r");
    if (fp == 0){

        printf("Echec\n");

        return 0;

    }

    if (stat(filename, &status) == 0) {
        sz = (int) status.st_size;
	}

    source = (char *) malloc(sz + 1);

    fread(source, sz, 1, fp);

    source[sz] = '\0';

    

    return source;
}
