#include "ardrone/ardrone.h"
#define KEY_DOWN(key) (GetAsyncKeyState(key) & 0x8000);
#define KEY_PUSH(key) (GetAsyncKeyState(key) & 0x0001);
// %%%%%%%%%%%%%%%%%%%%%%
// %%%%  Variables  %%%%%
// %%%%%%%%%%%%%%%%%%%%%%

// ArDrone
ARDrone ardrone;								// ARDrone type
int option;										// option to run the program

// time variables
int ts=33;										// sample time [ms]
int kwait=0;									// time between frames (32)
clock_t startClock,finishClock,diff_1,diff_2;	// control time variables

// pant and image variables
int x_pant, y_pant;								// pant size [px]								
CvSize size_image;								// image size (width,height)
int x_ima, y_ima;								// image size [px]

// reference point and correction
CvPoint ref;									// reference point for control
CvPoint correct_ref;							// reference correction

// relationship between pixels and meters
double rate1;									// image meter by ArDrone heigh in meters (m/m)
double rate2;									// rate referenced by the square (cm/px)

// text variables .txt
FILE *fp;										// text file
char txtfilename[20];							// text file name
char txt_buffer[500];							// text buffer
clock_t time_count=0;							// time counter
int row_count=0;								// column counter
int file_count=0;								// file counter

// photos variables
int photo_count=0;								// photo counter 
char photoname[20];								// photo name

// video recording variables
int flag1_video=0;								// flag1 video to initialize the video recording
int flag2_video=0;								// flag2 video to record video
CvVideoWriter *video;							// initialize video file writer
char videofilename[40];							// video file name
SYSTEMTIME file_date;							// get date and time to write the file name

// motion variables for the ArDrone (systems inputs)
double pitch_in=0.0;							// pitch angle input, normalized data between [-1 1]
double roll_in=0.0;								// roll angle input, normalized data between [-1 1]
double vz_in=0.0;								// z axis speed, lifting speed, normalized data between [-1 1]
double vr_in=0.0;								// r axis speed, rotation speed, normalized data between [-1 1]
double pond;

// navigation data from the ArDrone (systems outputs)
int battery;									// battery level in percent [0-100 %]
double pitch_out;								// pitch angle [deg]
double roll_out;								// roll angle [deg]
double yaw_out;									// yaw angle [deg]
double altitude_out;							// altitude [m]
double speed_out;					 			// average speed in all three axes
double vx_out, vy_out, vz_out;					// speed in each axis x,y,z [m/s] (speed_z no return)
double x_position=0, y_position=0;				// position, obtain by the integral of the speed
double yaw,last_yaw;							// angle correction
signed long int turn_counter=0;					// turn counter for yaw angle correction

// control Board for the ArDrone : trackbar
int TB_camera=1;								// [0 1] bottom camera, top camera
int TB_landing=0;								// [0 1] take off, land
int TB_pitch_angle=1;							// [-1 0 1] pitch angle 
int TB_roll_angle=1;							// [-1 0 1] roll angle 
int TB_vz=1;									// [-1 0 1] vz 
int TB_vr=1;									// [-1 0 1] vr 
int TB_move_3d=1;								// [0 1] enable and disable motion

// image and video variables
IplImage *image;								// original image 
IplImage *final_image;							// edited and analized image 
IplImage *hsv;									// image in hsv format 
IplImage *grayscale;							// image in grayscale
IplImage *b_w1;									// image in black & white by Segmentation Color
IplImage *b_w2;									// image in black & white by Segmentation grayscale
IplImage *filter_1;								// filtered image 
IplImage *edges;								// edged image
IplImage *edit_1;								// edited image
IplImage *equali;								// equalized image by histogram
IplImage *affine_t;								// transformed image (deformation,scale,rotation)
IplImage *i_contours;							// contoured image
IplImage *i_contours_aprox;						// contoured approximate image
IplImage *i_contours_analy;						// contoured analyzed image
CvCapture *capture;								// to capture video

// editing variables
int band_edition_1;								// enable or disable editing
CvScalar red=CV_RGB(250,0,0);					// red color
CvScalar green=CV_RGB(0,250,0);					// green color
CvScalar blue=CV_RGB(0,0,250);					// blue color
CvScalar white=CV_RGB(255,255,255);				// white color
CvScalar black=CV_RGB(0,0,0);					// black color
int minH=0,maxH=100;							// H range to binarizing image
int minS=0,maxS=200;							// S range to binarizing image
int minV=0,maxV=255;							// V range to binarizing image
CvScalar lower=cvScalar(minH,minS,minV);		// minimum range 
CvScalar upper=cvScalar(maxH,maxS,maxV);		// maximum range
int thresh=207;									// threshold from grayscale to black and white image
int canny1=100, canny2=10;						// canny edge thresholds
int ite=1;										// interactions to erode and dilate
int filter_i=2;									// filter parameter
CvMat* warp_mat=cvCreateMat(2,3,CV_32FC1);		// affine transform matrix (deformation)
CvMat* rot_mat=cvCreateMat(2,3,CV_32FC1);		// affine transform matrix (scale and rotation)
CvPoint2D32f srcTri[3],dstTri[3];				// three points for the deformation
double angle=0.0,scale=1.0;						// angle and scale value [°deg]
CvPoint2D32f center_affine;						// center point for rotation

// contour variables
CvSeq *s_contours_origi;						// original contours
CvSeq *s_contours_aprox;						// polynomial approximate contours
CvSeq *s_contours_copy,*s_contours_analy;		// contour analysis, copy required
CvMemStorage* g_storage=cvCreateMemStorage(0);	// memory storage for the contours
int Nc=0;										// number of contours
int N_aprox=10;									// type of polynomial approximation
int N_circles=0;								// number of circles inside the each square
double area=0,real_area=0;						// contour area [px^2] and contour real area [m^2]
CvBox2D square_1;								// square box to get the height, width and the center
CvBox2D square_2;								// box for the center circle to get the orientation reference
double dist_pts;								// distance between two points
double angulo;									// angle of each square
int flag_orientation=0;							// flag for only one orientation

// drawing variables
CvFont text_font;								// text font to put in the image
CvFont text_font2;								// text font to put in the image for the altitude
CvPoint pt1,pt2;								// center points to put circles and shapes
char *text,text_1[100];							// text to put in the image

// struct - robot recognition
struct robot
{	int enable;
	double angle;
	CvPoint center;
} rb1,rb2,rb3,rb4,rb5;


// control variables
double Ts;				// sampling time for the controllers
int jump_flag=0;		// twice time, 66 [ms] controller sampling time

// altitude controller variables
double Wt_alt=0;							// setpoint
double Yt_alt;							// plant output
double Kp_alt,Ti_alt,Td_alt;			// PID parameters
double Umax_alt,Umin_alt;				// threshold to the controller output
double Up_alt,Ui_alt,Ud_alt;			// Proportional, Integral and differencial control action
double Ut_alt;							// Total control action
double Wt1_alt,Yt1_alt,Ui1_alt,Ud1_alt;	// store for next sampling time

// orientation controller variables
double Wt_ori=0;						// setpoint
double Yt_ori;							// plant output
double Kp_ori,Ti_ori,Td_ori;			// PID parameters
double Umax_ori,Umin_ori;				// threshold to the controller output
double Up_ori,Ui_ori,Ud_ori;			// Proportional, Integral and differencial control action
double Ut_ori;							// Total control action
double Wt1_ori,Yt1_ori,Ui1_ori,Ud1_ori;	// store for next sampling time

// x position controller variables
double Wt_xpos=0;						// setpoint
double Yt_xpos;							// plant output
double Kp_xpos,Ti_xpos,Td_xpos;			// PID parameters
double Umax_xpos,Umin_xpos;				// threshold to the controller output
double Up_xpos,Ui_xpos,Ud_xpos;			// Proportional, Integral and differencial control action
double Ut_xpos;							// Total control action
double Wt1_xpos,Yt1_xpos,Ui1_xpos,Ud1_xpos;	// store for next sampling time

// y position controller variables
double Wt_ypos=0;						// setpoint
double Yt_ypos;							// plant output
double Kp_ypos,Ti_ypos,Td_ypos;			// PID parameters
double Umax_ypos,Umin_ypos;				// threshold to the controller output
double Up_ypos,Ui_ypos,Ud_ypos;			// Proportional, Integral and differencial control action
double Ut_ypos;							// Total control action
double Wt1_ypos,Yt1_ypos,Ui1_ypos,Ud1_ypos;	// store for next sampling time

// enable control : trackbar
int TB_alt_control=1;					// [0 1] enable/disable altitude control 
int TB_ori_control=0;					// [0 1] enable/disable orientation control
int TB_xpos_control=0;					// [0 1] enable/disable x position control 
int TB_ypos_control=0;					// [0 1] enable/disable y position control 

// %%%%%%%%%%%%%%%%%%%%%%
// %%%%%  Funtions  %%%%%
// %%%%%%%%%%%%%%%%%%%%%%
void ts_control(void) // time control
	{
		finishClock = clock();													// end of the cycle
		// Look out! in these lines we have a slight time error
		diff_1=difftime(finishClock, startClock);								// cycle time without delay
		if(diff_1<ts) {msleep(ts-diff_1);}										// add remainder time to be the setpoint (ts)
		finishClock = clock();													// new end of the cycle
		diff_2=difftime(finishClock, startClock);								// cycle time without delay (real)
		// Look out! in these lines we have a slight time error
		startClock = clock();													// start of the cycle
		//if(diff_2>ts) diff_1=diff_2; // peak correction in cycle time
	}

void write_txt(int i) // text file to save the measurements
	{
		switch(i)
			{
				case 0: sprintf(txtfilename,"C:/data_%d.txt",file_count);						// text file name
						fp=fopen(txtfilename,"w+");												// open file to create
						row_count=0;															// column counter initializing
						break;

				case 1:	//sprintf(txt_buffer,"%d	%d	%d	%d	%d	%d	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%d	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f\n",time_count,diff_1,diff_2,ts,option,file_count,real_area,pitch_out,roll_out,yaw_out,altitude_out,vx_out,vy_out,vz_out,battery,pitch_in,roll_in,vz_in,vr_in,x_position,y_position);	// ordered data in each column
						sprintf(txt_buffer,"%d	%d	%d	%d	%d	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%d	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f\n",file_count,time_count,diff_1,diff_2,ts,pitch_in,roll_in,vz_in,vr_in,pitch_out,roll_out,yaw_out,altitude_out,vx_out,vy_out,battery,x_position,y_position,Wt_alt,Wt_ori,Wt_xpos,Wt_ypos,real_area);	// ordered data in each column
						time_count=time_count+ts;												// time counter runs (ideal)
						//time_count=time_count+diff_2;											// time counter runs (real)
						row_count++;															// column counter runs
						fp=fopen(txtfilename,"r+");												// open file for writing
						fseek(fp,0,SEEK_END);													// it's positioned at the end of the file
						fprintf(fp,txt_buffer);													// writes in the text file
						fclose(fp);																// close file
						break;
				
				case 2:	file_count++;															// increase file counter
						//sprintf(txt_buffer,"%d	%d	%d	%d	%d	%d	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%d	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f\n",time_count,diff_1,diff_2,ts,option,file_count,real_area,pitch_out,roll_out,yaw_out,altitude_out,vx_out,vy_out,vz_out,battery,pitch_in,roll_in,vz_in,vr_in,x_position,y_position);	// write the last column to reference the following file
						sprintf(txt_buffer,"%d	%d	%d	%d	%d	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%d	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f	%3.3f\n",file_count,time_count,diff_1,diff_2,ts,pitch_in,roll_in,vz_in,vr_in,pitch_out,roll_out,yaw_out,altitude_out,vx_out,vy_out,battery,x_position,y_position,Wt_alt,Wt_ori,Wt_xpos,Wt_ypos,real_area);	// ordered data in each column
						fp=fopen(txtfilename,"r+");												// open file for writing
						fseek(fp,0,SEEK_END);													// it's positioned at the end of the file
						fprintf(fp,txt_buffer);													// writes in the text file
						fclose(fp);																// close file
													
						sprintf(txtfilename,"C:/data_%d.txt",file_count);						// text file name
						fp=fopen(txtfilename,"w+");												// open file to create
						row_count=0;														    // column counter initializing
						break;
			}
	}

void video_control_board(void)  // video control board
	{	
		cvNamedWindow("VideoControlBoard",1);									// create window for the control panel
		cvResizeWindow("VideoControlBoard",300,220);							// Resize the window (300,220)
		cvMoveWindow("VideoControlBoard",x_pant-300,0);							// move the window to a desired position on the screen
		cvCreateTrackbar("cycle time","VideoControlBoard",&ts,300);				// cycle time
		cvCreateTrackbar("kwait","VideoControlBoard",&kwait,300);				// kwait
		cvCreateTrackbar("filter_p","VideoControlBoard",&filter_i,9);			// filter parameter
		cvCreateTrackbar("thresh bw","VideoControlBoard",&thresh,255);			// threshold B&W
		cvCreateTrackbar("N_aprox","VideoControlBoard",&N_aprox,30);			// contour approximation parameter	
	}

void start_program(int i) //initialize variables according to the option chosen
	{	
		cvInitFont(&text_font,CV_FONT_HERSHEY_SIMPLEX,1,1,0,2,8);		// text font initialization
		cvInitFont(&text_font2,CV_FONT_HERSHEY_SIMPLEX,0.4,0.4,0,1,8);	// text font initialization
		x_pant=GetSystemMetrics(SM_CXSCREEN);							// obtaining screen width [px]
		y_pant=GetSystemMetrics(SM_CYSCREEN);							// obtaining screen heigh [px]
				
		switch(i)
			{
				case 0: capture=cvCreateFileCapture("C:/FlightVideo_26_06_2013__14_40_12.avi");	// get video from a file
						break;

				case 1:	capture=cvCreateCameraCapture(0);										// get video from the webcam
						break;
				
				case 2:	if (!ardrone.open()) {printf("Failed to initialize.\n"); return;}		// initialize Ar Drone
						ardrone.setCamera(1); msleep(500);										// initialize bottom camera
						break;

				case 3: break;																	// initialize nothing for the option 3, working with image file
			}
	}

void upload_image(int i) // upload image
	{	
		switch(i)
			{
				case 0: image=cvQueryFrame(capture);		// get image from video file
						break;

				case 1:	image=cvQueryFrame(capture);		// get image from the webcam
						break;
				
				case 2:	image=ardrone.getImage();			// get image from the ArDrone			 
						break; 
				
				case 3: image=cvLoadImage( "C:/prueba_4_1.jpg");	// get image from the image file		
						break;
			}
	}

void take_photo(void)  // toma foto
	{
		msleep(300);												// delay to take only one 
		sprintf(photoname,"C:/photo_%d.jpg",photo_count);			// name photo
		cvSaveImage(photoname,image);								// save photo 
		photo_count++;												// run photo counter
	}

void switch_camera(int x)
{ 
	ardrone.setCamera(x);								// change the front and bottom camera
}

void switch_land(int x)
{ 
	//x=2;
	if (x==1 && ardrone.onGround()) ardrone.takeoff();  // takeoff
	if (x==0 && !ardrone.onGround()) ardrone.landing();	// landing
}

void switch_pitch(int x)
{ 
    if (x==0)   pitch_in=-1.0;				// *goes backward
	if (x==1)   pitch_in= 0.0;				// stands still
	if (x==2)   pitch_in= 1.0;				// *goes forward
    if (TB_move_3d==0) pitch_in= 0.0;		// stands still
}

void switch_roll(int x)
{ 
    if (x==0)   roll_in=1.0;				// goes to the left side
	if (x==1)   roll_in=0.0;				// stands still
	if (x==2)   roll_in=-1.0;				// goes to the right side
	if (TB_move_3d==0) roll_in=0.0;			// stands still
}

void switch_vz(int x)
{ 
    if (x==0)   vz_in=-1.0;					// goes down			
	if (x==1)   vz_in= 0.0;					// stands still
	if (x==2)   vz_in= 1.0;					// goes up
	if (TB_move_3d==0) vz_in= 0.0;			// stands still
}

void switch_vr(int x)
{ 
    if (x==0)   vr_in= 1.0;					// rotates in anticlockwise sense
	if (x==1)   vr_in= 0.0;					// stands still
	if (x==2)   vr_in=-1.0;					// rotates in clockwise sense
	if (TB_move_3d==0) vr_in= 0.0;			// stands still
}

void switch_on(int x)
{ 
	if (x==0)   
		{	pitch_in = 0.0; roll_in = 0.0; vz_in = 0.0; vr_in = 0.0;	// disables moving
			TB_pitch_angle=1;											// updated trackbar value
			TB_roll_angle=1;											// updated trackbar value
			TB_vz=1;													// updated trackbar value
			TB_vr=1;													// updated trackbar value
		}
}

void ArDrone_control_board(void)	// ArDrone control board
	{	
		cvDestroyWindow("ArDrone_Control");
		cvDestroyWindow("Control Window");

		cvNamedWindow("ArDrone_Control",1);												// create window
		cvResizeWindow("ArDrone_Control",300,350);										// resize the window (300,350)
		cvMoveWindow("ArDrone_Control",x_pant-300,220+39);								// move the window to a desired position on the screen
		cvCreateTrackbar("Camera","ArDrone_Control",&TB_camera,1,switch_camera);		// switch camera
		cvCreateTrackbar("Landing","ArDrone_Control",&TB_landing,1,switch_land);		// switch take off / landing
		cvCreateTrackbar("< Pitch","ArDrone_Control",&TB_pitch_angle,2,switch_pitch);	// switch <pitch
		cvCreateTrackbar("< Roll","ArDrone_Control",&TB_roll_angle,2,switch_roll);		// switch <roll
		cvCreateTrackbar("Vz","ArDrone_Control",&TB_vz,2,switch_vz);					// switch vz
		cvCreateTrackbar("Vr","ArDrone_Control",&TB_vr,2,switch_vr);					// switch vr
		cvCreateTrackbar("Move3D","ArDrone_Control",&TB_move_3d,1,switch_on);			// switch motion enable / disable


		cvNamedWindow("Control Window",1);												// create window
		cvResizeWindow("Control Window",300,180);										// resize the window (300,350)
		cvMoveWindow("Control Window",x_pant-623,0);									// move the window to a desired position on the screen
		cvCreateTrackbar("Alt_C","Control Window",&TB_alt_control,1);					// enable/disable altitude control
		cvCreateTrackbar("Ori_C","Control Window",&TB_ori_control,1);					// enable/disable altitude control
		cvCreateTrackbar("Xpos_C","Control Window",&TB_xpos_control,1);					// enable/disable altitude control
		cvCreateTrackbar("Ypos_C","Control Window",&TB_ypos_control,1);					// enable/disable altitude control
	}


void start_video_recording(void)  // start video recording
	{
		GetLocalTime(&file_date);																		// get local time
		sprintf(videofilename,"C:/FlightVideo_%02d_%02d_%d__%02d_%02d_%02d.avi",file_date.wDay,file_date.wMonth,file_date.wYear,file_date.wHour,file_date.wMinute,file_date.wSecond);	// give file name
		video=cvCreateVideoWriter(videofilename,CV_FOURCC('D','I','V','X'),30,cvGetSize(image));		// create video writer with desired format
	}


void record_video(void)
	{
		cvWriteFrame(video,image);		// write frame in the video file
	}

void start_edition(void)
	{	
		upload_image(option);											// upload image
		size_image=cvGetSize(image);									// obtain image size
		x_ima=size_image.width/3;										// ideal image size to enlarge or reduce
		y_ima=size_image.height/3;										// ideal image size to enlarge or reduce
		
		final_image=cvCreateImage(cvGetSize(image),IPL_DEPTH_8U,3);		// initialize final_image with identical properties to the original image
		hsv=cvCloneImage(image);										// initialize HSV image with identical properties to the original image
		b_w1=cvCreateImage(cvGetSize(image),8,1);						// initialize B&W1 image with desired properties
		grayscale=cvCreateImage(cvGetSize(image),IPL_DEPTH_8U,1);		// initialize grayscale image with desired properties
		equali=cvCreateImage(cvGetSize(image),IPL_DEPTH_8U,1);			// initialize equali image with desired properties
		b_w2=cvCreateImage(cvGetSize(image),8,1);						// initialize B&W2 image with desired properties
		filter_1=cvCreateImage(cvGetSize(image),IPL_DEPTH_8U,3);		// initialize filter_1 image with desired properties
		edges=cvCreateImage(cvGetSize(image),image->depth,1);			// initialize edges image with desired properties
		i_contours=cvCreateImage(cvGetSize(image),IPL_DEPTH_8U,1);		// initialize i_contours with desired properties
		i_contours_aprox=cvCreateImage(cvGetSize(image),8,3);			// initialize i_contours_aprox with desired properties
		i_contours_analy=cvCreateImage(cvGetSize(image),8,3);			// initialize i_contours_analy with desired properties
		affine_t=cvCloneImage(b_w2);									// initialize affine_t image with desired properties
	

		// WINDOWS INITIALIZATION

		cvNamedWindow("Original",0);									// original_image window
		cvResizeWindow("Original",x_ima,y_ima);							// resize the window 
		cvMoveWindow("Original",0*(x_ima+17),0*(y_ima+39));				// move the window to a desired position on the screen

		cvNamedWindow("Filter",0);										// filtered_image window
		cvResizeWindow("Filter",x_ima,y_ima);							// resize the window 
		cvMoveWindow("Filter",0*(x_ima+17),1*(y_ima+39));				// move the window to a desired position on the screen

		cvNamedWindow("GrayScale",0);										// grayscale_image window
		cvResizeWindow("GrayScale",x_ima,y_ima);							// resize the window 
		cvMoveWindow("GrayScale",0*(x_ima+17),2*(y_ima+39));				// move the window to a desired position on the screen

		cvNamedWindow("BlackWhite",0);									// binarized image window
		cvResizeWindow("BlackWhite",x_ima,y_ima);						// resize the window 
		cvMoveWindow("BlackWhite",1*(x_ima+17),0*(y_ima+39));			// move the window to a desired position on the screen

		cvNamedWindow("W_Contours",0);									// contoured image window
		cvResizeWindow("W_Contours",x_ima,y_ima);						// resize the window 
		cvMoveWindow("W_Contours",1*(x_ima+17),1*(y_ima+39));			// move the window to a desired position on the screen

		cvNamedWindow("W_Contours_Aprox",0);							// contoured_aprox image window
		cvResizeWindow("W_Contours_Aprox",x_ima,y_ima);					// resize the window 
		cvMoveWindow("W_Contours_Aprox",1*(x_ima+17),2*(y_ima+39));		// move the window to a desired position on the screen

		cvNamedWindow("W_Contours_Analy",0);							// contoured_analy image window
		cvResizeWindow("W_Contours_Analy",x_ima,y_ima);					// resize the window 
		cvMoveWindow("W_Contours_Analy",2*(x_ima+17),0*(y_ima+39));		// move the window to a desired position on the screen

		cvNamedWindow("Final",0);										// final image window
		cvResizeWindow("Final",x_ima,y_ima);					// resize the window 
		cvMoveWindow("Final",2*(x_ima+17),1*(y_ima+39));		// move the window to a desired position on the screen

		video_control_board();											// start manual video controller (trackbars)
	}

void editing_image(void)
	{	
		cvCopy(image,final_image);										// copy the original image
			
		cvSmooth(image,filter_1,CV_GAUSSIAN,filter_i*2+1,filter_i*2+1);	// get filtered image 
		cvShowImage("Filter",filter_1);									// show the filtered image

		cvCvtColor(filter_1,grayscale,CV_RGB2GRAY);						// get grayscale image
		cvShowImage("GrayScale",grayscale);								// show the grayscale image

		cvThreshold(grayscale,b_w2,thresh,255,CV_THRESH_BINARY);		// get binalizared image with a threshold
		cvShowImage("BlackWhite",b_w2);									// show the black and white image

		cvZero(i_contours);															// need to clear the contour image but don't show
		cvZero(i_contours_aprox);													// need to clear the contour image but don't show
		cvZero(i_contours_analy);													// need to clear the contour image but don't show
		
		Nc=cvFindContours(b_w2,g_storage,&s_contours_origi);						// find contours
		s_contours_analy=s_contours_origi;											// sequence to analyze contours
		s_contours_copy=s_contours_origi;											// sequence copy to analyze contours

		if(s_contours_origi)																								// if it finds at least one contour
			{	
				s_contours_aprox=cvApproxPoly(s_contours_origi,sizeof(CvContour),g_storage,CV_POLY_APPROX_DP,N_aprox,1);	// get approxploy from the contours
				cvDrawContours(i_contours,s_contours_origi,white,white,1,1,16);												// draw contours original
				cvDrawContours(i_contours_aprox,s_contours_aprox,red,blue,1,1,16);											// draw contours approx
			}
		else
			{	
				s_contours_aprox=NULL;																						// not find any contour
			}
		

		rb1.enable=0;	rb2.enable=0;	rb3.enable=0;	rb4.enable=0;	rb5.enable=0;
		rb1.angle=0;	rb2.angle=0;	rb3.angle=0;	rb4.angle=0;	rb5.angle=0;
		rb1.center.x=0;	rb2.center.x=0;	rb3.center.x=0;	rb4.center.x=0;	rb5.center.x=0;
		rb1.center.y=0;	rb2.center.y=0;	rb3.center.y=0;	rb4.center.y=0;	rb5.center.y=0;

		while(s_contours_aprox)
			{	
				if(s_contours_aprox->total==4 && cvCheckContourConvexity(s_contours_aprox))									// discrimination by number of sides and convexity for squares
					{	
						area=cvContourArea(s_contours_aprox);			// get contour area
						square_1=cvMinAreaRect2(s_contours_aprox,0);	// finds minimum area rotated rectangle bounding a set of points

						if (square_1.size.height>0.9*square_1.size.width && square_1.size.height<1.1*square_1.size.width)	// discrimination by equal sides for squares
							{	
								//altitude_out=1.4;
								//rate1=(52.272*altitude_out+1.6001)/360;		//[cm/px]	
								real_area=area*rate1*rate1;				// get real area by rate1 [cm^2]
								real_area=100;							// comment it when quadrotor flies

								if (real_area>80 && real_area<120)															// discrimination by contour area for squares
									{
										rate2=10/square_1.size.width;	// get rate2 from one side of the contour[cm/px] 
										real_area=area*rate2*rate2;		// get real area by rate2 [cm^2][cm^2]

										if (real_area>80 && real_area<120)													// discrimination by contour area for squares
											{	
												N_circles=0;																// reset circles counter
												s_contours_analy=s_contours_copy;											// reset sequence of contours
												flag_orientation=0;															// reset orientation flag for the next contour
												angulo=0;																	// reset angle of orientation
												
												while(s_contours_analy)		//	scanning all contours for the square number	
													{  	
														if(cvContourArea(s_contours_analy)*rate2*rate2<10)		// discrimination by contour area for circles
															{	
																square_2=cvMinAreaRect2(s_contours_analy,0);	// get box form contourn
																dist_pts=sqrt(pow(square_2.center.x-square_1.center.x,2)+pow(square_2.center.y-square_1.center.y,2));	//	find distance between points
																	
																if (dist_pts<0.6*square_1.size.height)			// find contours inside the radius
																	{	
																		N_circles++;							// runs circles counter
																	}
																else if (dist_pts<1.3*square_1.size.height && flag_orientation==0)		// find orientatio circle
																	{										
																		angulo=atan2((square_1.center.x-square_2.center.x),(square_2.center.y-square_1.center.y))*RAD_TO_DEG;	// find the orientation with two center points, square and orientatio circle [°deg]
																		pt1=cvPoint(square_2.center.x,square_2.center.y);				// get center point of that circle
																		cvCircle(final_image,pt1,3,blue,2,1);							// draw circle in final_image
																		flag_orientation=1;												// active orientation flag for just one circle
																	}
															}
														s_contours_analy=s_contours_analy->h_next;						// advance to the next contour for search of circles 
													}

												if (flag_orientation==0) {angulo=181;}			// for squares that doesn't have orientation point

												if (N_circles>0 && N_circles<=5)				// correct number of circles
													{	
														if (N_circles==1) 
															{	
																rb1.enable=1;
																rb1.angle=angulo;
																rb1.center.x=square_1.center.x;
																rb1.center.y=square_1.center.y;
															} 
														if (N_circles==2) 
															{	
																rb2.enable=1;
																rb2.angle=angulo;
																rb2.center.x=square_1.center.x;
																rb2.center.y=square_1.center.y;
															} 
														if (N_circles==3) 
															{	
																rb3.enable=1;
																rb3.angle=angulo;
																rb3.center.x=square_1.center.x;
																rb3.center.y=square_1.center.y;
															} 
														if (N_circles==4) 
															{	
																rb4.enable=1;
																rb4.angle=angulo;
																rb4.center.x=square_1.center.x;
																rb4.center.y=square_1.center.y;
															} 
														if (N_circles==5) 
															{	
																rb5.enable=1;
																rb5.angle=angulo;
																rb5.center.x=square_1.center.x;
																rb5.center.y=square_1.center.y;
															} 

														cvDrawContours(i_contours_analy,s_contours_aprox,red,blue,-1,1,16);			// draw square contour in i_contours_analy - penultimate value (-1) to graph only the present contour
														cvDrawContours(final_image,s_contours_aprox,red,blue,-1,1,16);				// draw square contour in final_image - penultimate value (-1) to graph only the present contour
																
														sprintf(text_1,"N. %d",N_circles); text=text_1;					// write the square number 												
														pt2=cvPoint(square_1.center.x,square_1.center.y);				// point for write the square number
														cvPutText(final_image,text,pt2,&text_font,red);					// draw that square number in the final image	
											
														sprintf(text_1,"Angle:%2.0f ",angulo); text=text_1;				// write the orientation 	
														pt2=cvPoint(square_1.center.x,square_1.center.y+25);			// point for write the orientation
														cvPutText(final_image,text,pt2,&text_font,green);				// draw that orientation value in the final image	
													}

											}
									}
							}
					}
				s_contours_aprox=s_contours_aprox->h_next;				// advance to the next contour for search of squares 
			}

		sprintf(text_1,"R1:%00.0f R2:%00.0f R3:%00.0f R4:%00.0f R5:%00.0f",rb1.angle,rb2.angle,rb3.angle,rb4.angle,rb5.angle); text=text_1;	// write the robot that quadrotor can see												
		pt2=cvPoint(10,25);																								// point for write the state robots
		cvPutText(final_image,text,pt2,&text_font,red);	

		if(option==2)
			{
				sprintf(text_1,"Altitude:%2.2f[m]",altitude_out); text=text_1;	// write the altitude value 												
				pt2=cvPoint(size_image.width-120,size_image.height-15);			// get point to draw the altitude
				cvPutText(final_image,text,pt2,&text_font2,red);				// draw the altitude in the final image
				cvPutText(image,text,pt2,&text_font2,red);						// draw the altitude in the image
				
				ref=cvPoint(size_image.width/2,size_image.height/2);			// reference point in the image center
				correct_ref=cvPoint(ref.x-(int((altitude_out*100)*tan(roll_out*DEG_TO_RAD)*(1/rate1))),ref.y+(int((altitude_out*100)*tan(pitch_out*DEG_TO_RAD)*(1/rate1))));	// find reference point correction 
				cvCircle(final_image,ref,2,blue,5,8);							// draw reference point without correction
				cvCircle(final_image,correct_ref,2,black,5,8);					// draw reference point with correction
			}

		cvShowImage("Final",final_image);											// show final image
		cvShowImage("W_Contours",i_contours);										// show contours original image
		cvShowImage("W_Contours_Aprox",i_contours_aprox);							// show contours approx
		cvShowImage("W_Contours_Analy",i_contours_analy);							// show contours analy
	}


void start_controllers(void)
	{
		Ts=(ts*2)/1000.0;	// sampling time for the controller

		// start altitude control
		Wt1_alt=0;   // Setpoint
		Yt1_alt=0;   // Plant output
		Ui1_alt=0;   // controller output, integral part
		Ud1_alt=0;   // controller output, differential part

		// start orientation control
		Wt1_ori=0;   // Setpoint
		Yt1_ori=0;   // Plant output
		Ui1_ori=0;   // controller output, integral part
		Ud1_ori=0;   // controller output, differential part

		// start x position control
		Wt1_xpos=0;   // Setpoint
		Yt1_xpos=0;   // Plant output
		Ui1_xpos=0;   // controller output, integral part
		Ud1_xpos=0;   // controller output, differential part

		// start y position control
		Wt1_ypos=0;   // Setpoint
		Yt1_ypos=0;   // Plant output
		Ui1_ypos=0;   // controller output, integral part
		Ud1_ypos=0;   // controller output, differential part
	}

void altitude_control(void)
	{	
		Wt_alt=1.25;			// Set point [1.25 m]
		Yt_alt=altitude_out;	// Plant Output
		
		Kp_alt=3.834;			// controller parameter Kp 
		Ti_alt=164.7615;		// controller parameter Ti
		Td_alt=0.1826;			// controller parameter Td
		
		Umax_alt=2.5;			// extra parameters (threshold max)
		Umin_alt=-2.5;			// extra parameters (threshold min)
		
		Up_alt=Kp_alt*(Wt_alt-Yt_alt);										// Proportional action
		Ui_alt=Ui1_alt+(Kp_alt/Ti_alt)*(Ts*(Wt_alt-Yt_alt));				// Integral action
		Ud_alt=(Kp_alt*Td_alt)*((Wt_alt-Yt_alt)-(Wt1_alt-Yt1_alt))/(Ts);	// Differential action
		//Ud_alt=(Kp_alt*Td_alt)*((0-Yt_alt)-(0-Yt1_alt))/Ts;				// Differential action  (Opcion 2)
		
		Ut_alt=Up_alt+Ui_alt+Ud_alt;		// Total control action
		
		// Restriction
		if (Ut_alt<Umin_alt) Ut_alt=Umin_alt; 
		if (Ut_alt>Umax_alt) Ut_alt=Umax_alt;
		
		// Anti Reset-Windup
		//Ui_alt=Ut_alt-Up_alt-Ud_alt;
		
		// Store for next sampling time
		Wt1_alt=Wt_alt; 
		Yt1_alt=Yt_alt; 
		Ui1_alt=Ui_alt; 
		Ud1_alt=Ud_alt;

		// Plant Input
		vz_in=Ut_alt;
	}


void orientation_control(void)
	{			
		Wt_ori=0;				// Set point [0 °deg]
		Yt_ori=yaw_out;			// Plant Output
		
		Kp_ori=3.834;			// controller parameter Kp 
		Ti_ori=164.7615;		// controller parameter Ti
		Td_ori=0.1826;			// controller parameter Td
		
		Umax_ori=2.5;			// extra parameters (threshold max)
		Umin_ori=-2.5;			// extra parameters (threshold min)
		
		Up_ori=Kp_ori*(Wt_ori-Yt_ori);										// Proportional action
		Ui_ori=Ui1_ori+(Kp_ori/Ti_ori)*(Ts*(Wt_ori-Yt_ori));				// Integral action
		Ud_ori=(Kp_ori*Td_ori)*((Wt_ori-Yt_ori)-(Wt1_ori-Yt1_ori))/(Ts);	// Differential action
		//Ud_ori=(Kp_ori*Td_ori)*((0-Yt_ori)-(0-Yt1_ori))/Ts;				// Differential action  (Opcion 2)
		
		Ut_ori=Up_ori+Ui_ori+Ud_ori;		// Total control action
		
		// Restriction
		if (Ut_ori<Umin_ori) Ut_ori=Umin_ori; 
		if (Ut_ori>Umax_ori) Ut_ori=Umax_ori;
		
		// Anti Reset-Windup
		//Ui_ori=Ut_ori-Up_ori-Ud_ori;
		
		// Store for next sampling time
		Wt1_ori=Wt_ori; 
		Yt1_ori=Yt_ori; 
		Ui1_ori=Ui_ori; 
		Ud1_ori=Ud_ori;

		// Plant Input
		vr_in=Ut_ori;
	}

void x_position_control(void)
	{	
		Wt_xpos=0;				// Set point [0 m]
		Yt_xpos=x_position;		// Plant Output
		
		Kp_xpos=3.834;			// controller parameter Kp 
		Ti_xpos=164.7615;		// controller parameter Ti
		Td_xpos=0.1826;			// controller parameter Td
		
		Umax_xpos=2.5;			// extra parameters (threshold max)
		Umin_xpos=-2.5;			// extra parameters (threshold min)
		
		Up_xpos=Kp_xpos*(Wt_xpos-Yt_xpos);										// Proportional action
		Ui_xpos=Ui1_xpos+(Kp_xpos/Ti_xpos)*(Ts*(Wt_xpos-Yt_xpos));				// Integral action
		Ud_xpos=(Kp_xpos*Td_xpos)*((Wt_xpos-Yt_xpos)-(Wt1_xpos-Yt1_xpos))/(Ts);	// Differential action
		//Ud_xpos=(Kp_xpos*Td_xpos)*((0-Yt_xpos)-(0-Yt1_xpos))/Ts;				// Differential action  (Opcion 2)
		
		Ut_xpos=Up_xpos+Ui_xpos+Ud_xpos;		// Total control action
		
		// Restriction
		if (Ut_xpos<Umin_xpos) Ut_xpos=Umin_xpos; 
		if (Ut_xpos>Umax_xpos) Ut_xpos=Umax_xpos;
		
		// Anti Reset-Windup
		//Ui_xpos=Ut_xpos-Up_xpos-Ud_xpos;
		
		// Store for next sampling time
		Wt1_xpos=Wt_xpos; 
		Yt1_xpos=Yt_xpos; 
		Ui1_xpos=Ui_xpos; 
		Ud1_xpos=Ud_xpos;

		// Plant Input
		pitch_in=Ut_xpos;
	}

void y_position_control(void)
	{
		Wt_ypos=0;				// Set point [0 m]
		Yt_ypos=y_position;		// Plant Output
		
		Kp_ypos=3.834;			// controller parameter Kp 
		Ti_ypos=164.7615;		// controller parameter Ti
		Td_ypos=0.1826;			// controller parameter Td
		
		Umax_ypos=2.5;			// extra parameters (threshold max)
		Umin_ypos=-2.5;			// extra parameters (threshold min)
		
		Up_ypos=Kp_ypos*(Wt_ypos-Yt_ypos);										// Proportional action
		Ui_ypos=Ui1_ypos+(Kp_ypos/Ti_ypos)*(Ts*(Wt_ypos-Yt_ypos));				// Integral action
		Ud_ypos=(Kp_ypos*Td_ypos)*((Wt_ypos-Yt_ypos)-(Wt1_ypos-Yt1_ypos))/(Ts);	// Differential action
		//Ud_ypos=(Kp_ypos*Td_ypos)*((0-Yt_ypos)-(0-Yt1_ypos))/Ts;				// Differential action  (Opcion 2)
		
		Ut_ypos=Up_ypos+Ui_ypos+Ud_ypos;		// Total control action
		
		// Restriction
		if (Ut_ypos<Umin_ypos) Ut_ypos=Umin_ypos; 
		if (Ut_ypos>Umax_ypos) Ut_ypos=Umax_ypos;
		
		// Anti Reset-Windup
		//Ui_ypos=Ut_ypos-Up_ypos-Ud_ypos;
		
		// Store for next sampling time
		Wt1_ypos=Wt_ypos; 
		Yt1_ypos=Yt_ypos; 
		Ui1_ypos=Ui_ypos; 
		Ud1_ypos=Ud_ypos;

		// Plant Input
		roll_in=Ut_ypos;
	}


// %%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%  MAIN  %%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%
void main(void)
	{ 
		option=0;										// (0)video_file (1)webcam (2)Ar.Drone (3)image_file , option to run the program 
		band_edition_1=1;								// enable edition(0)off (1)on
		start_program(option);							// start_program initialize variables
		write_txt(0);									// create text file
		if(band_edition_1==1) start_edition();			// start edition initialize variables
		start_controllers();							// start controllers


		while (!GetAsyncKeyState(VK_ESCAPE)) 
			{	cvWaitKey(kwait+1);									// wait to move the windows	
				cvShowImage("Original",image);						// show original image
				upload_image(option);								// upload image
				if (option==0 && image==NULL) return;				// close the program when finish the video 
				if(band_edition_1==1) editing_image();				// editing image

				if (option==2)
					{	if(!ardrone.update()) break;							// ArDrone update 
						pitch_out=ardrone.getPitch()*RAD_TO_DEG;				// get pitch 
						roll_out=ardrone.getRoll()*RAD_TO_DEG;					// get roll
						last_yaw=yaw;											// get last yaw
						yaw=ardrone.getYaw()*RAD_TO_DEG;						// get yaw 
						altitude_out=ardrone.getAltitude();						// get altitude 
						speed_out=ardrone.getVelocity(&vx_out,&vy_out,&vz_out);	// get speed
						battery=ardrone.getBatteryPercentage();					// get battery level 
						

						if((yaw>0 && last_yaw<0) && ((yaw>150) || (yaw<-150))) turn_counter--;
						if((yaw<0 && last_yaw>0) && ((yaw>150) || (yaw<-150))) turn_counter++;
						yaw_out=-((360*turn_counter)+yaw);						// get speed pintch angle correction

						pitch_out=-pitch_out;									// get speed pintch angle correction
						roll_out=-roll_out;										// get speed roll angle correction
						vy_out=-vy_out;											// get speed Vy correction

						rate1=(52.272*altitude_out+1.6001)/360;					// calculating rate1 from altitude and look at table [cm/px]
						x_position=x_position+(vx_out*(ts/1000.0));				// calculating the x position by the integral of vx_out 
						y_position=y_position+(vy_out*(ts/1000.0));				// calculating the y position by the integral of vy_out

						
						printf("YAW_angle: %2.2f \n",yaw_out);

						// Control
						if (jump_flag%2==0)										// twice time, 66 [ms] controller sampling time
  							{ 
								if (TB_alt_control==1) altitude_control();		// altitude control
								if (TB_ori_control==1) orientation_control();	// orientation control
								if (TB_xpos_control==1) x_position_control();	// x position control
								if (TB_ypos_control==1) y_position_control();	// y position control
								jump_flag=0;									// reset jump flag
							}
						jump_flag++;											// increase jump flag
		
						// Motion
						pond=1;
						ardrone.move3D(pitch_in,roll_in,vz_in,vr_in);			// send the manipulated variable to the plant
					}



				

				if (option==2 && (cvGetWindowProperty("ArDrone_Control",1)==-1 || cvGetWindowProperty("Control Window",1)==-1)) ArDrone_control_board(); // if not Ar.drone control window (open)
				if (band_edition_1==1 && cvGetWindowProperty("VideoControlBoard",1)==-1) video_control_board();	// if not video control window (open)

				if (GetAsyncKeyState('S') && option!=2) {msleep(100); cvWaitKey(0);}		// pause the video
				if (GetAsyncKeyState('P')) take_photo();								// take a photo					
				if (GetAsyncKeyState('R')){if(flag1_video==0){start_video_recording();flag1_video=1;}flag2_video=1;} if (flag2_video==1) record_video(); // record video
			
				write_txt(1);															// write datas in .txt file
				if (row_count==303){write_txt(2);}										// after a number of rows, create a new file
				ts_control();															// time control
			}

		cvReleaseCapture(&capture);
		ardrone.landing();
		ardrone.close();
		fclose(fp);
		cvReleaseImage(&affine_t);
		cvReleaseMat(&rot_mat);
		cvReleaseMat(&warp_mat);
		cvReleaseVideoWriter(&video);
	}