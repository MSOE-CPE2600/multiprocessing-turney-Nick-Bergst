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

// local routines
static int iteration_to_color( int i, int max );
static int iterations_at_point( double x, double y, int max );
static void compute_image( imgRawImage *img, double xmin, double xmax,
									double ymin, double ymax, int max );
static void show_help();


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
	char fileout[50]; //does not change
	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:h:M:z:p"))!=-1) {
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
		compute_image(img,xcenter-xscale/2,xcenter+xscale/2,ycenter-yscale/2,ycenter+yscale/2,max);

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
				if (50 % procRuns != 0 && a == procRuns - 1) {
					leftOver = 50 % procRuns;
				} else {
					leftOver = 0;
				}
				for (b = a * (int)(50 / procRuns); b <(((int)(50 / procRuns)) * (a + 1)) + leftOver; b++) {
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
					// Fill it in with black
					setImageCOLOR(img,0);
					// Compute the Mandelbrot image
					compute_image(img,xcenter-nuxscale/2,xcenter+nuxscale/2,ycenter-yscale/2,ycenter+yscale/2,max);
					// Save the image in the stated file.
					storeJpegImageFile(img,fileout);
				}
				return 1;
			}
		}
		for (a = 0; a < procRuns; a++) {
			wait(NULL);
		}
		printf("\nType \n\tffmpeg -i mandel%cd.jpg [exapmle file name].mpeg\n to compile the movie!\n", 37);
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

	return iter;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

void compute_image(imgRawImage* img, double xmin, double xmax, double ymin, double ymax, int max )
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