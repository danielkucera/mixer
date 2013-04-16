int main(int argc, char** argv) 
{
    cvNamedWindow("xsample", CV_WINDOW_AUTOSIZE);

    CvCapture* capture = cvCreateFileCapture("movie.avi");
    if (!capture)
    {
      printf("!!! cvCreateFileCapture didn't found the file !!!\n");
      return -1; 
    }

    IplImage* frame;
    while(1) 
    {
        frame = cvQueryFrame( capture );
        if (!frame) 
            break;

        cvShowImage( "xsample", frame );

        char c = cvWaitKey(33);
        if (c == 27) 
            break; // ESC was pressed
    }

    cvReleaseCapture(&capture);
    cvDestroyWindow("xsample");

    return 0;
}
