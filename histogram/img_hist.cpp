#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeImage.h"
#include <CL/cl.h>
#include <omp.h>

#define WORKGROUP_SIZE	256
#define MAX_SOURCE_SIZE 16384
#define BINS 256

double gpuTime;

struct histogram {
	cl_uint *R;
	cl_uint *G;
	cl_uint *B;
};

void histogramCPU(unsigned char *imageIn, histogram H, int width, int height)
{

    //Each color channel is 1 byte long, there are 4 channels BLUE, GREEN, RED and ALPHA
    //The order is BLUE|GREEN|RED|ALPHA for each pixel, we ignore the ALPHA channel when computing the histograms
	for (int i = 0; i < (height); i++)
		for (int j = 0; j < (width); j++)
		{
			H.B[imageIn[(i * width + j) * 4]]++;
			H.G[imageIn[(i * width + j) * 4 + 1]]++;
			H.R[imageIn[(i * width + j) * 4 + 2]]++;
		}
}

void histogramGPU(unsigned char *image_in, histogram H, int width, int height, int pitch)
{
    char ch;
    int i;
	cl_int ret;

	int imageSize = height * width;
    // Branje datoteke
    FILE *fp;
    char *source_str;
    size_t source_size;


    fp = fopen("kernel.cl", "r");
    if (!fp) 
	{
		fprintf(stderr, ":-(#\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	source_str[source_size] = '\0';
    fclose( fp );

	// Rezervacija pomnilnika
	//unsigned char *image = (unsigned char *)malloc(imageSize * sizeof(unsigned char) * 4);
    // Inicializacija vektorjev
 
    // Podatki o platformi
    cl_platform_id	platform_id[10];
    cl_uint			ret_num_platforms;
	char			*buf;
	size_t			buf_len;
	ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);
			// max. "stevilo platform, kazalec na platforme, dejansko "stevilo platform
	
	// Podatki o napravi
	cl_device_id	device_id[10];
	cl_uint			ret_num_devices;
	// Delali bomo s platform_id[0] na GPU
	ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 10,	
						 device_id, &ret_num_devices);				
			// izbrana platforma, tip naprave, koliko naprav nas zanima
			// kazalec na naprave, dejansko "stevilo naprav

    // Kontekst
    cl_context context = clCreateContext(NULL, 1, &device_id[0], NULL, NULL, &ret);
			// kontekst: vklju"cene platforme - NULL je privzeta, "stevilo naprav, 
			// kazalci na naprave, kazalec na call-back funkcijo v primeru napake
			// dodatni parametri funkcije, "stevilka napake
 
    // Ukazna vrsta
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id[0], 0, &ret);
			// kontekst, naprava, INORDER/OUTOFORDER, napake

	// Delitev dela

	size_t local_item_size = WORKGROUP_SIZE;
	size_t num_groups = ((imageSize-1)/local_item_size+1);		
    size_t global_item_size = num_groups*local_item_size;



    size_t datasize = sizeof(unsigned char) * pitch * height; 		
  	
  
    // Priprava programa
    cl_program program = clCreateProgramWithSource(context,	1, (const char **)&source_str,  
												   NULL, &ret);
	
			// kontekst, "stevilo kazalcev na kodo, kazalci na kodo,		
    		// stringi so NULL terminated, napaka

    													
    // Prevajanje
    ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);
			// program, "stevilo naprav, lista naprav, opcije pri prevajanju,
			// kazalec na funkcijo, uporabni"ski argumenti

    
	// Log
	size_t build_log_len;
	char *build_log;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 
								0, NULL, &build_log_len);

			// program, "naprava, tip izpisa, 
			// maksimalna dol"zina niza, kazalec na niz, dejanska dol"zina niza
    
	build_log =(char *)malloc(sizeof(char)*(build_log_len+1));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 
							    build_log_len, build_log, NULL);
	printf("%s\n", build_log);
	free(build_log);

    // "s"cepec: priprava objekta

    cl_kernel kernel = clCreateKernel(program, "histogram", &ret);
	//printf("KERNEL %d\n", ret);
			// program, ime "s"cepca, napaka

    double startTime = omp_get_wtime();
    
    cl_mem image_mem_obj_in = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
									  datasize, image_in, &ret);
    //printf("IMAGE PASS %d\n", ret);    
	cl_mem hist_r_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
											BINS * sizeof(unsigned int), NULL, &ret);
	cl_mem hist_b_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
											BINS * sizeof(unsigned int), NULL, &ret);
	cl_mem hist_g_mem_obj = clCreateBuffer(context, CL_MEM_READ_WRITE,
											BINS * sizeof(unsigned int), NULL, &ret);
	
	//printf("HISTOGRAM PASS %d\n", ret);
    // "s"cepec: argumenti

    
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&image_mem_obj_in);
	ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&hist_r_mem_obj);
	ret |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&hist_g_mem_obj);
	ret |= clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&hist_b_mem_obj);
    ret |= clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&width);
    ret |= clSetKernelArg(kernel, 5, sizeof(cl_int), (void *)&height);
	//printf("HEIGHT SET %d\n", ret);
    // "s"cepec, "stevilka argumenta, velikost podatkov, kazalec na podatke

	// "s"cepec: zagon

    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,						
								 &global_item_size, &local_item_size, 0, NULL, NULL);
	//printf("ENQUEUE KERNEL %d\n", ret);	
			// vrsta, "s"cepec, dimenzionalnost, mora biti NULL, 
			// kazalec na "stevilo vseh niti, kazalec na lokalno "stevilo niti, 
			// dogodki, ki se morajo zgoditi pred klicem
																						
    // Kopiranje rezultatov
	
    ret = clEnqueueReadBuffer(command_queue, hist_r_mem_obj, CL_TRUE, 0,						
							  BINS * sizeof(unsigned int), H.R, 0, NULL, NULL);

	ret = clEnqueueReadBuffer(command_queue, hist_g_mem_obj, CL_TRUE, 0,						
							  BINS * sizeof(unsigned int), H.G, 0, NULL, NULL);
	
	ret = clEnqueueReadBuffer(command_queue, hist_b_mem_obj, CL_TRUE, 0,						
							  BINS * sizeof(unsigned int), H.B, 0, NULL, NULL);		
			// branje v pomnilnik iz naparave, 0 = offset
			// zadnji trije - dogodki, ki se morajo zgoditi prej

    double endTime = omp_get_wtime();
    gpuTime = endTime - startTime;

    // Prikaz rezultatov
 
    // "ci"s"cenje
    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(image_mem_obj_in);
    ret = clReleaseMemObject(hist_r_mem_obj);
	ret = clReleaseMemObject(hist_g_mem_obj);
	ret = clReleaseMemObject(hist_b_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);

	
}

void printHistogram(histogram H) {
	printf("Colour\tNo. Pixels\n");
	for (int i = 0; i < BINS; i++) {
		if (H.B[i]>0)
			printf("%dB\t%d\n", i, H.B[i]);
		if (H.G[i]>0)
			printf("%dG\t%d\n", i, H.G[i]);
		if (H.R[i]>0)
			printf("%dR\t%d\n", i, H.R[i]);
	}
}


int main(void)
{

    //Load image from file
	FIBITMAP *imageBitmap = FreeImage_Load(FIF_PNG, "test.png", 0);
	//Convert it to a 32-bit image
    FIBITMAP *imageBitmap32 = FreeImage_ConvertTo32Bits(imageBitmap);
	
    //Get image dimensions
    int width = FreeImage_GetWidth(imageBitmap32);
	int height = FreeImage_GetHeight(imageBitmap32);
	int pitch = FreeImage_GetPitch(imageBitmap32);
	//Preapare room for a raw data copy of the image
    unsigned char *imageIn = (unsigned char *)malloc(height*pitch * sizeof(unsigned char));
	
    //Initalize the histogram
    histogram H;
	H.B = (unsigned int*)calloc(BINS, sizeof(unsigned int));
	H.G = (unsigned int*)calloc(BINS, sizeof(unsigned int));
	H.R = (unsigned int*)calloc(BINS, sizeof(unsigned int));
	
    //Extract raw data from the image
	FreeImage_ConvertToRawBits(imageIn, imageBitmap32, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);

    //Free source image data
	FreeImage_Unload(imageBitmap32);
	FreeImage_Unload(imageBitmap);

	printf("IMAGE SIZE: %d x %d\n", width, height);
    //Compute and print the histogram
	histogramGPU(imageIn, H, width, height, pitch);
	printf("GPU TIME: %f\n", gpuTime);
	double startCPU = omp_get_wtime();
	histogramCPU(imageIn, H, width, height);
	double endCPU = omp_get_wtime();
	double cpuTime = endCPU - startCPU;
	printf("CPU TIME: %f\n", cpuTime);
	printf("SPEED UP: %f\n", cpuTime / gpuTime);
	//printHistogram(H);

	return 0;
}

/* PROFILING
---------------------------
IMAGE SIZE: 640 x 480

GPU TIME: 0.001496
CPU TIME: 0.000642
SPEED UP: 0.428885
------------------------------
IMAGE SIZE: 800 x 600

GPU TIME: 0.002149
CPU TIME: 0.000978
SPEED UP: 0.455040
------------------------------
IMAGE SIZE: 1600 x 900

GPU TIME: 0.004571
CPU TIME: 0.003063
SPEED UP: 0.670041
------------------------------
IMAGE SIZE: 1920 x 1080

GPU TIME: 0.005895
CPU TIME: 0.004333
SPEED UP: 0.735091
-------------------------------
IMAGE SIZE: 3840 x 2400


GPU TIME: 0.020751
CPU TIME: 0.019231
SPEED UP: 0.926747
-------------------------------
*/