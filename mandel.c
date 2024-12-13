/// 
//  mandel.c
//  Based on example code found here:
//  https://users.cs.fiu.edu/~cpoellab/teaching/cop4610_fall22/project3.html
//
//  Converted to use jpg instead of BMP and other minor changes
//  
///
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "jpegrw.h"
#include <time.h>
#include <pthread.h>
#include <string.h>

// local routines
static int iteration_to_color( int i, int max );
static int iterations_at_point( double x, double y, int max );
static void compute_image( imgRawImage *img, double xmin, double xmax,
									double ymin, double ymax, int max, int sector);
void *compute_image_thread(void *args);
static void show_help();

//arg struct
struct argument {
	imgRawImage *img;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	int max;
	int sector;
	int sCount;
	char fOut[50];
};


int main( int argc, char *argv[] )
{
	struct timespec begin, end;
	clock_gettime(CLOCK_MONOTONIC_RAW, &begin);

	char c;

	// These are the default configuration values used
	// if no command line arguments are given.
	const char *outfile = "mandelbrot.jpg";
	double xcenter = 0;
	double ycenter = 0;
	double xscale = 4;
	double yscale = 0; // calc later
	int    image_width = 2048;
	int    image_height = 2048;
	int    max = 2048;
	int    makeMovie = 0;
	double zoomRate = 0.05;
	int procRuns = 5;
	int a = 0; //does not change
	int b = 0; //does not change
	int d = 0; //does not change
	char fileout[50]; //does not change
	int imgThreads = 1;
	int framesGen = 50;
	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:h:M:z:p:t:F:"))!=-1) {
		switch(c) 
		{
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				xscale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'o':
				outfile = optarg;
				break;
			case 'h':
				show_help();
				exit(1);
				break;
			case 'M':
				makeMovie = atoi(optarg);
				break;
			case 'z':
				zoomRate = atof(optarg);
				break;
			case 'p':
				procRuns = atoi(optarg);
				break;
			case 't':
				imgThreads = atoi(optarg);
				break;
			case 'F':
				framesGen = atoi(optarg);
		}
	}

	if (makeMovie == 0) {
		// Calculate y scale based on x scale (settable) and image sizes in X and Y (settable)
		yscale = xscale / image_width * image_height;

		// Display the configuration of the image.
		printf("mandel: x=%lf y=%lf xscale=%lf yscale=%1f max=%d outfile=%s\n",xcenter,ycenter,xscale,yscale,max,outfile);

		// Create a raw image of the appropriate size.
		imgRawImage* img = initRawImage(image_width,image_height);

		// Fill it in with black
		setImageCOLOR(img,0);

		// Compute the Mandelbrot image
		compute_image(img,xcenter-xscale/2,xcenter+xscale/2,ycenter-yscale/2,ycenter+yscale/2,max, imgThreads);

		// Save the image in the stated file.
		storeJpegImageFile(img,outfile);

		// free the mallocs
		freeRawImage(img);
	} else {
		pid_t pid;
		for (a = 0; a < procRuns; a++) {
			pid = fork();
			//printf("a: %d\ta + 1: %d\tgetpid: %d\tpid: %d\n", a, a + 1, getpid(), pid);
			if (pid == 0) {
				//printf("pid 0!\n");
				int leftOver;
				if (framesGen % procRuns != 0 && a == procRuns - 1) {
					leftOver = 50 % procRuns;
				} else {
					leftOver = 0;
				}
				for (b = a * (int)(framesGen / procRuns); b <(((int)(framesGen / procRuns)) * (a + 1)) + leftOver; b++) {
					//int b = 0;
					//printf("b: %d\n", b);
					//make a new xscale from the original, and calculate it
					double nuxscale = xscale + (b * zoomRate);
					// Calculate y scale based on x scale (settable) and image sizes in X and Y (settable)
					yscale = nuxscale / image_width * image_height;
					// write a filename
					sprintf(fileout, "mandel%d.jpg", b);
					// Display the configuration of the image.
					printf("mandel: x=%lf y=%lf xscale=%lf yscale=%1f max=%d fileout=%s\n",xcenter,ycenter,nuxscale,yscale,max,fileout);
					// Create a raw image of the appropriate size.
					imgRawImage* img = initRawImage(image_width,image_height);
					//struct to pass args
					
					// Fill it in with black
					setImageCOLOR(img,0);
					// Compute the Mandelbrot image
					pthread_t tid[imgThreads];
					struct argument arg[imgThreads];
					for (d = 0; d < imgThreads; d++) {
						//int sparePixel;
						//if ((int)((nuxscale / imgThreads) - (int)(nuxscale / imgThreads)) != 0 && d == imgThreads - 1) {
						//	sparePixel = ((nuxscale / imgThreads) - (int)(nuxscale / imgThreads));
						//} else {
						//	sparePixel = 0;
						//}
						//printf("b4 %f %f %f %f\n", (xcenter-nuxscale/2),(xcenter+nuxscale/2),(ycenter-yscale/2),(ycenter+yscale/2));
						arg[d].img = img;
						arg[d].xmin = (xcenter-nuxscale/2) + ((nuxscale / imgThreads) * (d));
						arg[d].xmax = (xcenter-nuxscale/2) + ((nuxscale / imgThreads) * (d + 1));
						arg[d].ymin = (ycenter-yscale/2);
						arg[d].ymax = (ycenter+yscale/2);
						arg[d].max = max;
						arg[d].sector = d;
						arg[d].sCount = imgThreads;
						strcpy(arg[d].fOut, fileout);
						printf("go %f %f %f %f %d %d %s %d %d\n",arg[d].xmin,arg[d].xmax,arg[d].ymin,arg[d].ymax,arg[d].max,arg[d].sector,arg[d].fOut,arg[d].img->width,arg[d].img->height);
						struct argument *argPoint = &arg[d];
						pthread_create(&tid[d], NULL, &compute_image_thread, (void *)argPoint);
						img = arg[d].img;
					}
					for (d = 0; d < imgThreads; d++) {
						void *tRet;
						imgRawImage* fImg[imgThreads];
						//char temp[50];
						pthread_join(tid[d], &tRet);
						fImg[d] = (imgRawImage *)tRet;
						//sprintf(temp, "test%d.jpg", d);
						//storeJpegImageFile(img, temp);
					}
					// Save the image in the stated file.
					storeJpegImageFile(img,fileout);
					freeRawImage(img);
				}
				return 1;
			}
		}
		for (a = 0; a < procRuns; a++) {
			wait(NULL);
		}
		printf("\nType\n\tffmpeg -i mandel%cd.jpg [example file name].mpeg\nto compile the movie!\n", 37);
	}
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
	printf ("Total time = %f seconds\n",(end.tv_nsec - begin.tv_nsec) / 1000000000.0 +(end.tv_sec  - begin.tv_sec));
	return 0;
}




/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	//printf("(%f, %f)\n",x,y);
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	//printf("iter: %d\n", iter);

	return iter;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

void *compute_image_thread(void *args)
{
	struct argument *argz =  (struct argument *)args;
	
	int i,j;

	int width = argz->img->width;
	int height = argz->img->height;

	printf("in %f %f %f %f %d %d %s %d %d\n",argz->xmin,argz->xmax,argz->ymin,argz->ymax,argz->max,argz->sector,argz->fOut,argz->img->width,argz->img->height);

	// For every pixel in the image...
	printf("scount: %d from %d to %d\n",argz->sCount, (width * argz->sector)/argz->sCount, (width * ((argz->sector) + 1))/argz->sCount);	

	for(j=0;j<height;j++) {
		//double xPrev = argz->xmin - 1;
		//double yPrev = argz->ymin + j*(argz->ymax-argz->ymin)/height;;

		for(i = (width * argz->sector)/argz->sCount;i< (width * ((argz->sector) + 1))/argz->sCount;i++) {

			// Determine the point in x,y space for that pixel.
			double x = argz->xmin + i*(argz->xmax-argz->xmin)/width;
			double y = argz->ymin + j*(argz->ymax-argz->ymin)/height;

			//if (x <= xPrev) {
			//	printf("x error! (%f,%f) sect %d prev (%f,%f)\n",x,y,argz->sector,xPrev,yPrev);
			//} else if (y != yPrev && yPrev != 0) {
			//	printf("y error! (%f,%f) sect %d prev (%f,%f)\n",x,y,argz->sector,xPrev,yPrev);
			//}
			//xPrev = x;
			//yPrev = y;
			//if (x < argz-> xmin || i < (width * argz->sector)/argz->sCount) {
			//	printf("(%f,%f) sect %d start %d\n", x, y, argz->sector,(width * argz->sector)/argz->sCount);
			//}

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,argz->max);
			//printf("iters: %d (%f, %f)\n", iters, x, y);
			//if (argz->sector == 1 && iters == 0) {
			//	printf("(%f,%f) sect %d iters %d\n", x, y, argz->sector,iters);
			//}

			// Set the pixel in the bitmap
			setPixelCOLOR(argz->img,i,j,iteration_to_color(iters,argz->max));
		}
	}
	//args->img = argz->img;
	// Save the image in the stated file.
	//storeJpegImageFile(argz->img,argz->fOut);
	pthread_exit(argz->img);
	return NULL;
}

void compute_image(imgRawImage *img, double xmin, double xmax, double ymin, double ymax, int max, int sector)
{
	int i,j;

	int width = img->width;
	int height = img->height;

	// For every pixel in the image...

	for(j=0;j<height;j++) {

		for(i=0;i<width;i++) {

			// Determine the point in x,y space for that pixel.
			double x = xmin + i*(xmax-xmin)/width;
			double y = ymin + j*(ymax-ymin)/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,max);

			// Set the pixel in the bitmap.
			setPixelCOLOR(img,i,j,iteration_to_color(iters,max));
		}
	}
}


/*
Convert a iteration number to a color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/
int iteration_to_color( int iters, int max )
{
	int color = 0xFFFFFF*iters/(double)max;
	return color;
}


// Show help message
void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=2048)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates (X-axis). (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=2048)\n");
	printf("-H <pixels> Height of the image in pixels. (default=2048)\n");
	printf("-M          Make a video zooming out of the mandelbrot set. (default=0)\n");
	printf("-z          Set the rate the video zooms out at. (default=0.05)\n");
	printf("-p          Set the amount of child processes to generate the movie frames. If your chosen number is not divisible by 50, it will revert to default. (default=5)\n");
	printf("-o <file>   Set output file. (default=mandelbrot.jpg)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}