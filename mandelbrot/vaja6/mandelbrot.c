// "s"cepec preberemo iz datoteke

#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include "FreeImage.h"
#include <omp.h>
#include <math.h>

#define HEIGHT  (2160)
#define WIDTH   (3840)
#define WORKGROUP_SIZE	(512)
#define MAX_SOURCE_SIZE 16384
double gpuTime;

void mandelbrotGPU(unsigned char *image, int height, int width) {
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

    // Alokacija pomnilnika na napravi
    /*
    cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									  vectorSize*sizeof(int), A, &ret);
			// kontekst, na"cin, koliko, lokacija na hostu, napaka	
    cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									  vectorSize*sizeof(int), B, &ret);
    */
    
    
  
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
    double startTime = omp_get_wtime();
    cl_kernel kernel = clCreateKernel(program, "mandelbrot", &ret);
			// program, ime "s"cepca, napaka

    cl_mem image_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
									  imageSize* 4 *sizeof(unsigned char), NULL, &ret);
    // "s"cepec: argumenti
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&image_mem_obj);
    ret |= clSetKernelArg(kernel, 1, sizeof(cl_int), (void *)&width);
    ret |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&height);
    // "s"cepec, "stevilka argumenta, velikost podatkov, kazalec na podatke

	// "s"cepec: zagon
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,						
								 &global_item_size, &local_item_size, 0, NULL, NULL);	
			// vrsta, "s"cepec, dimenzionalnost, mora biti NULL, 
			// kazalec na "stevilo vseh niti, kazalec na lokalno "stevilo niti, 
			// dogodki, ki se morajo zgoditi pred klicem
																						
    // Kopiranje rezultatov
    ret = clEnqueueReadBuffer(command_queue, image_mem_obj, CL_TRUE, 0,						
							  imageSize* 4 *sizeof(unsigned char), image, 0, NULL, NULL);				
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
    ret = clReleaseMemObject(image_mem_obj);
    /*
    ret = clReleaseMemObject(b_mem_obj);
    ret = clReleaseMemObject(c_mem_obj);
    */
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);

}

void mandelbrotCPU(unsigned char *image, int height, int width) {
	float x0, y0, x, y, xtemp;
	int i, j;
	int color;
	int iter;
	int max_iteration = 800;   //max stevilo iteracij
	unsigned char max = 255;   //max vrednost barvnega kanala

	//za vsak piksel v sliki							
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			x0 = (float)j / width * (float)3.5 - (float)2.5; //zacetna vrednost
			y0 = (float)i / height * (float)2.0 - (float)1.0;
			x = 0;
			y = 0;
			iter = 0;
			//ponavljamo, dokler ne izpolnemo enega izmed pogojev
			while ((x*x + y * y <= 4) && (iter < max_iteration))
			{
				xtemp = x * x - y * y + x0;
				y = 2 * x*y + y0;
				x = xtemp;
				iter++;
			}
			//izracunamo barvo (magic: http://linas.org/art-gallery/escape/smooth.html)
			color = 1.0 + iter - log(log(sqrt(x*x + y * y))) / log(2.0);
			color = (8 * max * color) / max_iteration;
			if (color > max)
				color = max;
			//zapisemo barvo RGBA (v resnici little endian BGRA)
			image[4 * i*width + 4 * j + 0] = 0; //Blue
			image[4 * i*width + 4 * j + 1] = color; // Green
			image[4 * i*width + 4 * j + 2] = 0; // Red
			image[4 * i*width + 4 * j + 3] = 255;   // Alpha
		}
}


int main(void) 
{
    int imageSize = HEIGHT * WIDTH;
    printf("IMAGE SIZE: %d X %d \n", WIDTH, HEIGHT);
    printf("---------------------------");
	unsigned char *image = (unsigned char *)calloc(imageSize * 4,sizeof(unsigned char));	
    int pitch = ((32 * WIDTH + 31) / 32) * 4;

    mandelbrotGPU(image, HEIGHT, WIDTH);
    
    printf("GPU time : %f\n", gpuTime);
    

    FIBITMAP *dst = FreeImage_ConvertFromRawBits(image, WIDTH, HEIGHT, pitch,
		32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
	FreeImage_Save(FIF_PNG, dst, "mandelbrot-gpu.png", 0);
    
    double startTime = omp_get_wtime();
    mandelbrotCPU(image, HEIGHT, WIDTH);
    double endTime = omp_get_wtime();
    double cpuTime = endTime - startTime;

    printf("CPU time: %f\n", cpuTime);
    printf("---------------------------\n");
    printf("SPEEDUP: %f\n", cpuTime / gpuTime);
    printf("---------------------------\n");

    dst = FreeImage_ConvertFromRawBits(image, WIDTH, HEIGHT, pitch,
		32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
	FreeImage_Save(FIF_PNG, dst, "mandelbrot-cpu.png", 0);
    
    free(image);

    return 0;
}


/*
IMAGE SIZE: 640 X 480       |   IMAGE SIZE: 800 X 600   |   IMAGE SIZE: 1920 X 1080     |   IMAGE SIZE: 3840 X 2160
--------------------------- |    GPU time : 0.002555    |   GPU time : 0.007073         |   GPU time : 0.023697
                            |   CPU time: 0.442824      |   CPU time: 1.911587          |   CPU time: 7.639385
--------------------------- |   SPEEDUP: 173.321094     |   SPEEDUP: 270.253409         |   SPEEDUP: 322.378655
                            |                           |                               |
GPU time : 0.001936         |                           |                               |
CPU time: 0.283565          |                           |                               |
--------------------------- |                           |                               |
SPEEDUP: 146.435726         |                           |                               |

*/