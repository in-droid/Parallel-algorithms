#define HIST_SIZE 256

__kernel void histogram(__global unsigned char *image,
                        __global unsigned int *R,
                        __global unsigned int *G,
                        __global unsigned int *B,
						int width,
						int height)						
{														
							
    int Gid = get_global_id(0);
    int Lid = get_local_id(0);

    __local unsigned int rLocal[HIST_SIZE];
    __local unsigned int gLocal[HIST_SIZE];
    __local unsigned int bLocal[HIST_SIZE];

    int i = Gid / width;
    int j = Gid % width;
    int size = width * height;
    // izracun											
    if( Gid < size ) {
  
        rLocal[Lid] = 0;
        gLocal[Lid] = 0;
        bLocal[Lid] = 0;
        
        barrier(CLK_LOCAL_MEM_FENCE);
  
        atomic_inc(&bLocal[image[(i * width + j) * 4]]);
        atomic_inc(&gLocal[image[(i * width + j) * 4 + 1]]);
        atomic_inc(&rLocal[image[(i * width + j) * 4 + 2]]);
    
        barrier(CLK_LOCAL_MEM_FENCE);

        atomic_add(&R[Lid], rLocal[Lid]);
        atomic_add(&G[Lid], gLocal[Lid]);
        atomic_add(&B[Lid], bLocal[Lid]);																							
    }													
}					