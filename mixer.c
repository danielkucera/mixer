/*This sample program was made by:

   Aquiles Yáñez C. 
   (yanez<at>elo<dot>utfsm<dot>cl)

   Under the design guidance of:

   Agustín González V.

version 0.1 - Lanzada en Enero del 2005
version 0.2 - Lanzada en Febrero del 2005
version 0.3 - Lanzada en Octubre del 2006
version 0.4 - The same of 0.3 but in English (November 2009)
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>
#include <time.h> 		//profiling

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define MAX_INPUT   16
#define MAX_NORM    16

//#define MPLAYER "mplayer -demuxer rawvideo - -rawvideo w=720:h=576:format=bgr16 -name TEST 2>/dev/null >/dev/null"
#define MPLAYER "mplayer -demuxer rawvideo - -rawvideo w=720:h=576:format=rgb24 -name TEST 2>/dev/null >/dev/null"

//info needed to store one video frame in memory
struct buffer {    
	void *                  start;
	size_t                  length;
};

int		Bpp;
int		fd[4];
int		width		= 720;
int		height		= 576;
struct buffer	buffers[7];
int		pixel_format	= 2;
int		devs[]		= {0,1,2,3};
int		out;
int		prev_fps	= 5;
int		frame[4];

struct timespec	start;


static void logtime(char * text, int id){
struct timespec now;
struct timespec temp;

clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &now);

if ((now.tv_nsec-start.tv_nsec)<0){
temp.tv_sec = now.tv_sec - start.tv_sec - 1;
temp.tv_nsec = 1000000000 + now.tv_nsec - start.tv_nsec;
} else {
temp.tv_sec = now.tv_sec - start.tv_sec;
temp.tv_nsec = now.tv_nsec - start.tv_nsec;
}


//printf("%lld.%.9ld: %d %s\n", (long long)temp.tv_sec, temp.tv_nsec, id, text);

}

static void errno_exit (const char *s)
{
	fprintf (stderr, "%s error %d, %s\n",s, errno, strerror (errno));
	exit (EXIT_FAILURE);
}

//a blocking wrapper of the ioctl function
static int xioctl (int fd, int request, void *arg)
{
	int r;

	do r = ioctl (fd, request, arg);
	while (-1 == r && EINTR == errno);

	return r;
}

//read one frame from memory and throws the data to standard output
static int read_frame  (int * fd, int width, int height, int * n_buffers,
						struct buffer * buffers, int pixel_format)
{
/*
	if (-1 == read (*fd, buffers[0].start-1, buffers[0].length)) 
	{
		switch (errno) 
		{
			case EAGAIN:
				return 0;

			case EIO:
								//EIO ignored
			default:
				errno_exit ("read");
		}
		return 0;
	}

*/
	while (width*height*Bpp != read (*fd, buffers[0].start, buffers[0].length)) 
	{
		usleep(1000);
	}

	return 1;
}

//dummy function, that represents the stop of capturing 
static void stop_capturing (int * fd)
{
	enum v4l2_buf_type type;
	// Nothing to do. 

}

//dummy function, that represents the start of capturing 
static void start_capturing (int * fd, int * n_buffers )
{
	unsigned int i;
	enum v4l2_buf_type type;
	// Nothing to do. 

}

//free the buffers
static void uninit_device (int * n_buffers, struct buffer * buffers)
{
	unsigned int i;

	free (buffers[0].start);
	free (buffers);
}

//allocate memory for buffers, the buffer must have capacity for one video frame. 
static struct buffer *init_read (unsigned int buffer_size)
{
	struct buffer *buffers = NULL;
	buffers = calloc (1, sizeof (*buffers));

	if (!buffers) 
	{
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}
	buffers[0].length = buffer_size;
	buffers[0].start = malloc (buffer_size);
	
	if (!buffers[0].start) 
	{
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}
	return buffers;
}

//configure and initialize the hardware device 
static int init_device (int * fd, char * dev_name, int width,
								int height, int * n_buffers, int pixel_format)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct buffer * buffers = NULL;
	unsigned int min;

	if (-1 == xioctl (*fd, VIDIOC_QUERYCAP, &cap)) 
	{
		if (EINVAL == errno) 
		{
			fprintf (stderr, "%s is no V4L2 device\n",dev_name);
			exit (EXIT_FAILURE);
		} else 
		{
			errno_exit ("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) 
	{
		fprintf (stderr, "%s is not a video capture device\n",dev_name);
		exit (EXIT_FAILURE);
	}

	if (!(cap.capabilities & V4L2_CAP_READWRITE)) 
	{
		fprintf (stderr, "%s does not support read i/o\n",dev_name);
		exit (EXIT_FAILURE);
	}
	//select video input, standard(not used) and tuner(not used) here
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl (*fd, VIDIOC_CROPCAP, &cropcap)) 
	{
				/* Errors ignored. */
	}
	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	crop.c = cropcap.defrect; /* reset to default */

	if (-1 == xioctl (*fd, VIDIOC_S_CROP, &crop)) 
	{
		switch (errno) 
		{
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
		}
	}
	CLEAR (fmt);
	//set image properties
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = width; 
	fmt.fmt.pix.height      = height;
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
//	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
	fmt.fmt.pix.colorspace  = V4L2_COLORSPACE_SRGB;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

/*	switch (pixel_format) 
	{
		case 0:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
			break;
		case 1:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
			break;
		case 2:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
			break;
	}
	
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565; //lasdlalsdaslda
*/

	if (-1 == xioctl (*fd, VIDIOC_S_FMT, &fmt))
		errno_exit ("VIDIOC_S_FMT");

	/* Note VIDIOC_S_FMT may change width and height. */

	//check the configuration data
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	fprintf(stderr,"Video bytespreline = %d\n",fmt.fmt.pix.bytesperline);
	fprintf(stderr,"Using READ IO Method\n");
//	buffers=init_read (fmt.fmt.pix.sizeimage);

//	return buffers;
	return fmt.fmt.pix.sizeimage;
}

static void close_device (int * fd)
{
	if (-1 == close (*fd))
		errno_exit ("close");

	*fd = -1;
}

static int open_device (int * fd, char * dev_name)
{
	struct stat st; 

	if (-1 == stat (dev_name, &st)) 
	{
		fprintf (stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno, strerror (errno));
		return 0;
	}

	if (!S_ISCHR (st.st_mode)) 
	{
		fprintf (stderr, "%s is no device\n", dev_name);
		return 0;
	}

	*fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (-1 == *fd) 
	{
		fprintf (stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror (errno));
		return 0;
	}
	return 1;
}

//show the usage
static void usage (FILE *fp, int argc, char **argv)
{
	fprintf (fp,
				"Usage: %s [options]\n\n"
				"Options:\n"
				"-D | --device       name               Select device name [/dev/video0]\n"
				"-d | --device-info  name               Show device info\n"
				"-i | --input        number             Video input number \n"
				"-s | --standard     number             Video standard \n"
				"-w | --window-size  <640*480|          Video size\n"
				"                      320*240>\n"
				"-p | --pixel-format number             Pixel Format (0 = YUV420)\n"
				"                                                    (1 = RGB565)\n"
				"                                                    (2 = RGB32 )\n"
				"-h | --help                            Print this message\n"
				"\n",
				argv[0]);
}

//used by getopt_long to know the possible inputs
static const char short_options [] = "D:d:i:s:w:p:h";

//long version of the previous function
static const struct option
long_options [] = 
{
		{ "device",      required_argument,      NULL,           'D' },
		{ "device-info", required_argument,      NULL,           'd' },	
		{ "input",       required_argument,      NULL,           'i' },
		{ "standard",    required_argument,      NULL,           's' },
		{ "window-size", required_argument,      NULL,           'w' },
		{ "pixel-format",required_argument,      NULL,           'p' },
		{ "help",        no_argument,            NULL,           'h' },
		{ 0, 0, 0, 0 }
};

//show all the available devices
static void enum_inputs (int * fd)
{
	int  ninputs;
	struct v4l2_input  inp[MAX_INPUT];
	printf("Available Inputs:\n");
	for (ninputs = 0; ninputs < MAX_INPUT; ninputs++) 
	{
		inp[ninputs].index = ninputs;
		if (-1 == ioctl(*fd, VIDIOC_ENUMINPUT, &inp[ninputs]))
			break;
		printf("number = %d      description = %s\n",ninputs,inp[ninputs].name); 
	}
}

//show the available standards(norms) for capture 
static void enum_standards (int * fd )
{
	struct v4l2_standard  std[MAX_NORM];
	int  nstds;
	printf("Available Standards:\n");
	for (nstds = 0; nstds < MAX_NORM; nstds++) 
	{
		std[nstds].index = nstds;
		if (-1 == ioctl(*fd, VIDIOC_ENUMSTD, &std[nstds]))
			break;
		printf("number = %d     name = %s\n",nstds,std[nstds].name);
	}
}

//configure the video input
static void set_input(int * fd, int dev_input)
{
	struct v4l2_input input;
	//set the input
	int index = dev_input;
	if (-1 == ioctl (*fd, VIDIOC_S_INPUT, &index)) 
	{
		perror ("VIDIOC_S_INPUT");
		exit (EXIT_FAILURE);
	}
	//check the input
	if (-1 == ioctl (*fd, VIDIOC_G_INPUT, &index)) 
	{
		perror ("VIDIOC_G_INPUT");
		exit (EXIT_FAILURE);
	}
	memset (&input, 0, sizeof (input));
	input.index = index;
	if (-1 == ioctl (*fd, VIDIOC_ENUMINPUT, &input)) 
	{
		perror ("VIDIOC_ENUMINPUT");
		exit (EXIT_FAILURE);
	}
	fprintf (stderr,"input: %s\n", input.name); 
}

//configure the capture standard
static void set_standard(int * fd, int dev_standard)
{
	struct v4l2_standard standard;
	v4l2_std_id st;
	standard.index = dev_standard;;
	if (-1 == ioctl (*fd, VIDIOC_ENUMSTD, &standard)) 
	{
		perror ("VIDIOC_ENUMSTD");
	}
	st=standard.id;
	
	if (-1 == ioctl (*fd, VIDIOC_S_STD, &st)) 
	{
		perror ("VIDIOC_S_STD");
	}
	fprintf (stderr,"standard: %s\n", standard.name); 
}

//enum for pixel format
typedef enum 
{      
	PIX_FMT_YUV420P,
	PIX_FMT_RGB565,
	PIX_FMT_RGB32
} pix_fmt;

//just the main loop of this program 
static void mainloop (void *arg)
{
	FILE *fp;
	int buffer = (int) arg;

	fp = popen(MPLAYER, "w");
	printf ("thread %d started\n", buffer);
//	return 0;

	unsigned int count;

	count = 100;
	frame[buffer]=0;

	int r;
	fd_set fds;
	struct timeval tv;

	for (;;) 
	{
//		printf("loop");

		/* needed for select timeout */
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		FD_ZERO (&fds);
		FD_SET (fd[buffer], &fds);

		//the classic select function, who allows to wait up to 2 seconds, until we have captured data,
		r = select (fd[buffer] + 1, &fds, NULL, NULL, &tv);

		if (-1 == r) 
		{//error
			if (EINTR == errno)
				continue;
			errno_exit ("select");
		}

		if (0 == r) 
		{
			fprintf (stderr, "select timeout\n");
			exit (EXIT_FAILURE);
		}

		//read one frame from the device and put on the buffer
		logtime ("pred read", buffer);

		if(read_frame(&fd[buffer], width, height, &buffer, &buffers[buffer], pixel_format))
			frame[buffer]++;

		fwrite(buffers[buffer].start,1, width*height*Bpp, fp);

		logtime ("po read", buffer);

//		printf("skipped frame?\n");
//		printf("frejm %d\n", buffer);
	}       

	stop_capturing (&fd[buffer]);

	uninit_device (&buffers, buffers);

	close_device (&fd[buffer]);

}

void preview_thread(void *arg){
	FILE *fp = NULL;
	struct buffer preview;

	preview.length = width*height*Bpp;
	preview.start = malloc (width*height*Bpp);

	printf("preview thread started\n");
//	fp = fopen("/tmp/preview", "w");
//	pipe (fp);

	int x, y;

	while (1){
		usleep(1000*1000/prev_fps); //= 5 fps
	
		if (devs[0]!=-1){
			for (y=0; y<height/2; y++){
				for (x=0; x<width/2; x++){
					memcpy((preview.start+(width*y*Bpp)+x*Bpp), (buffers[0].start+(width*y*Bpp*2)+x*Bpp), Bpp);
				}
			}
		}

		if (devs[1]!=-1){
			for (y=0; y<height/2; y++){
				for (x=0; x<width/2; x++){
					memcpy((preview.start+(width*y*Bpp)+(x+width/2)*Bpp), (buffers[1].start+(width*y*Bpp*2)+x*Bpp), Bpp);
				}
			}
		}

		if (devs[2]!=-1){
			for (y=0; y<height/2; y++){
				for (x=0; x<width/2; x++){
					memcpy((preview.start+(width*(y+height/2)*Bpp)+x*Bpp), (buffers[2].start+(width*y*Bpp*2)+x*Bpp), Bpp);
				}
			}
		}

		if (devs[3]!=-1){
			for (y=0; y<height/2; y++){
				for (x=0; x<width/2; x++){
					memcpy((preview.start+(width*(y+height/2)*Bpp)+(x+width/2)*Bpp), (buffers[3].start+(width*y*Bpp*2)+x*Bpp), Bpp);
				}
			}
		}

		if (fp){
			fwrite(preview.start,1, width*height*Bpp, fp);
		} else {
//			fp = popen("cat > /tmp/kokosy", "w");
			fp = popen(MPLAYER, "w");
		}

//		printf("*");
//		fflush(stdout);
	}
}

void output_thread(void *arg){
	FILE *fp = NULL;
	int u_frame=0;
	int len=Bpp*width*height;
	struct buffer output;
        output.length = width*height*Bpp;
        output.start = malloc (width*height*Bpp);


	printf("output thread started\n");
//	fp = fopen("/tmp/preview", "w");
//	pipe (fp);
	fp = popen(MPLAYER, "w");

	while (1){

//		usleep(1000*1000/25); // 25fps

		if (frame[out]!=u_frame){
//			usleep(10*1000);
			u_frame=frame[out];

	//		if (fp){
				memcpy(output.start, buffers[out].start, len);
				fwrite(output.start,Bpp, width*height, fp);
//			} else {
	//			fp = popen("cat > /tmp/kokosy", "w");
//			}

	//		printf("*");
	//		fflush(stdout);

		} else {
			usleep(3000);
		}
	}
}
int main (int argc, char ** argv)
{

	int                 dev_standard; 
	int                 dev_input;
	int                 set_inp              = 0;
	int                 set_std              = 0;
	char                dev_name[20];//            = "/dev/video0";
//	int                 fd[4]                ;//= -1;
//	int                 width                = 720;
//	int                 height               = 576;
	int                 n_buffers;
//	int                 pixel_format         = 2;
//	struct buffer       buffers[7];//             = NULL;

	//process all the command line arguments
/*
	for (;;) 
	{	
		int index;
		int c;
				
		c = getopt_long (argc, argv,short_options, long_options,&index);

		if (-1 == c)
			break; //no more arguments (quit from for)

		switch (c) 
		{
			case 0: // getopt_long() flag
				break;

			case 'D':
				dev_name = optarg;
				break;
					
			case 'd':
				dev_name = optarg;
				open_device (&fd,dev_name);
				printf("\n");
				printf("Device info: %s\n\n",dev_name);
				enum_inputs(&fd);
				printf("\n");
				enum_standards(&fd);
				printf("\n");
				close_device (&fd);
				exit (EXIT_SUCCESS);
				//break;
				
			case 'i':  
				dev_input = atoi(optarg);              
				set_inp=1;
				break;

			case 's':
				dev_standard = atoi(optarg);
				set_std=1;
				break;

			case 'w':
				if (strcmp(optarg,"640*480")==0)
				{
					printf("window size 640*480\n");
					width=640;
					height=480;
				}
				if (strcmp(optarg,"320*240")==0)
				{
					printf("window size 320*240\n");
					width=320;
					height=240;
				}
				if ((strcmp(optarg,"320*240")!=0)&&(strcmp(optarg,"640*480")!=0))
				{
					printf("\nError: window size not supported\n");
					exit(EXIT_FAILURE);
				}                
				break;
				
			case 'p':
				pixel_format=atoi(optarg);
				break;

			case 'h':
				usage (stdout, argc, argv);
				exit (EXIT_SUCCESS);

			default:
				usage (stderr, argc, argv);
				exit (EXIT_FAILURE);
		}
	}
*/	
	int 	i;
	int	have_dev = 0;
	pthread_t thread[3];
	pthread_t prev_thread;
	pthread_t out_thread;

	Bpp = 3;

	for (i=0; i<4; i++){
		sprintf(dev_name, "/dev/video%d", devs[i]);
		printf("initializing %s\n", dev_name);
		

		if (!open_device (&fd[i], dev_name)){
			devs[i]=-1;
			continue;
		}
	
		//set the input if needed
		if (set_inp==1)
			set_input(&fd[i], dev_input);
	
		//set the standard if needed
		if (set_std==1)
			set_standard(&fd[i], dev_standard);

		int buffer_size = init_device (&fd[i], dev_name, width, height, &n_buffers, pixel_format);

		if (!buffers)
		{
			fprintf (stderr, "Out of memory\n");
			exit (EXIT_FAILURE);
		}
		
		buffers[i].length = buffer_size;
		buffers[i].start = malloc (buffer_size);

		if (!buffers[i].start)
		{
			fprintf (stderr, "Out of memory\n");
			exit (EXIT_FAILURE);
		}

		start_capturing (&fd[i], &n_buffers);

		pthread_create(&thread[i], NULL, mainloop, (void *)i);
		
		have_dev = 1;
	}

	if (!have_dev){
		printf("No available device found!\n");
//		exit(2);
	}
		
	out=0;

//	thread_create(&prev_thread, NULL, preview_thread, NULL);
	pthread_create(&out_thread, NULL, output_thread, NULL);

	int input;
	char in;

	while(1){
		printf("0-3 change input; q/Q - exit; k,o - preview fps up,down\n");
		printf("Preview: %d fps Input: %d\n",prev_fps,out);
		system("/bin/stty raw");
//		scanf("%d",&input);
		in=getchar();
		system("/bin/stty cooked");
//		input=atoi(getchar());
		
		if ((in< 52) && (in>47)){
			out=in-48;
		}
		switch(in){
			case 81:
			case 113:
				exit(0); break;
			case 107:
				prev_fps--; break;
			case 111:
				prev_fps++; break;
			
		}
/*		if ((in==81) || (in==113))
			exit(0);
		if (in==107){
			prev_fps--;
		}
		if (in==111){
			prev_fps++;
		}
*/
		printf("\n%d\n", in);
//		out=input-1;
		system("clear");
	}

	pthread_join(thread[0], NULL);
	pthread_join(thread[1], NULL);
	pthread_join(thread[2], NULL);
	pthread_join(thread[3], NULL);

	exit(0);
//	mainloop (&fd[0], width, height, &n_buffers, buffers, pixel_format);

	write(STDOUT_FILENO, buffers[0].start, width*height*Bpp);
					
	//TODO: main loop never exits, a break method must be implemented to execute 
	//the following code
	
	exit (EXIT_SUCCESS);

}
