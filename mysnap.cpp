/** Simple program to store argv[1] images taken with argv[2] delay on the Image folder with argv[3] name*/

#include <time.h>
#include "mysnap.hpp"

#define MAXSNAP 1000

#if defined(_LINUX) || defined(_QNX) || defined(_OSX)
void Sleep(unsigned int time)
{
    struct timespec t,r;
    
    t.tv_sec    = time / 1000;
    t.tv_nsec   = (time % 1000) * 1000000;    
    
	//molto cool!!
    while(nanosleep(&t,&r)==-1)
        t = r;
}
#endif

// wait for a camera to be plugged
void WaitForCamera()
{
    printf("waiting for a camera ");
	fflush(stdout);
    while(!PvCameraCount())
    {
        printf(".");
        Sleep(250);
    }
    printf("\n");
}

// wait for the user to press q
bool WaitForUserToQuitOrSnap()
{
    char c;

    do
    {
        c = getc(stdin);

    } while(c != 'q' && c != 's');

    return c == 's';
}

// get the first camera found
bool CameraGet(tCamera* Camera)
{
    tPvUint32 count,connected;
    tPvCameraInfo list;

    count = PvCameraList(&list,1,&connected);
    if(count == 1)
    {
        Camera->UID = list.UniqueId;
        printf("got camera %s\n",list.SerialString);
        return true;
    
    }
    else
        return false;
}

// open the camera
bool CameraSetup(tCamera* Camera)
{
    return !PvCameraOpen(Camera->UID,ePvAccessMaster,&(Camera->Handle));   
}

// setup and start streaming
bool CameraStart(tCamera* Camera)
{
    unsigned long FrameSize = 0;

    // Auto adjust the packet size to max supported by the network, up to a max of 8228.
    // NOTE: In Vista, if the packet size on the network card is set lower than 8228,
    //       this call may break the network card's driver. See release notes.
    //
    //PvCaptureAdjustPacketSize(Camera->Handle,8228);

    // how big should the frame buffers be?
    if(!PvAttrUint32Get(Camera->Handle,"TotalBytesPerFrame",&FrameSize))
    {
        bool failed = false;

        // allocate the buffer for the single frame we need
        Camera->Frame.Context[0]  = Camera;
        Camera->Frame.ImageBuffer = new char[FrameSize];
        if(Camera->Frame.ImageBuffer)
            Camera->Frame.ImageBufferSize = FrameSize;
        else
            failed = true;

        if(!failed)
        {
            // set the camera is capture mode
            if(!PvCaptureStart(Camera->Handle))
            {
		        // set the camera in continuous acquisition mode
		        if(!PvAttrEnumSet(Camera->Handle,"FrameStartTriggerMode","Freerun"))
		        {			
                	        // and set the acquisition mode into continuous
                	        if(PvCommandRun(Camera->Handle,"AcquisitionStart"))
                	        {
                    		        // if that fail, we reset the camera to non capture mode
                    		        PvCaptureEnd(Camera->Handle) ;
                    		        return false;
                	        }
                	        else
                    		        return true;
		        }
		        else
		            return false;
            }
            else
                return false;
        }
        else
            return false;
    }
    else
        return false;
}

// stop streaming
void CameraStop(tCamera* Camera)
{
    PvCommandRun(Camera->Handle,"AcquisitionStop");
    PvCaptureEnd(Camera->Handle);  
}

// snap and save a frame from the camera
void CameraSnap(tCamera* Camera)
{
    if(!PvCaptureQueueFrame(Camera->Handle,&(Camera->Frame),NULL))
    {
        printf("waiting for the frame to be done ...\n");
        while(PvCaptureWaitForFrameDone(Camera->Handle,&(Camera->Frame),100) == ePvErrTimeout)
            printf("still waiting ...\n");
        if(Camera->Frame.Status == ePvErrSuccess)
        {
            if(!ImageWriteTiff("./snap.tiff",&(Camera->Frame)))
                printf("Failed to save the grabbed frame!");
            else
                printf("frame saved\n");
        }
        else
            printf("the frame failed to be captured ...\n");
    } 
    else
        printf("failed to enqueue the frame\n");
}

void CameraSnapFile(tCamera* Camera, char *dest){
	if(!PvCaptureQueueFrame(Camera->Handle,&(Camera->Frame),NULL))
    {
        printf("waiting for the frame to be done ...\n");
        while(PvCaptureWaitForFrameDone(Camera->Handle,&(Camera->Frame),100) == ePvErrTimeout)
            printf("still waiting ...\n");
        if(Camera->Frame.Status == ePvErrSuccess)
        {
            if(!ImageWriteTiff(dest,&(Camera->Frame)))
                printf("Failed to save the grabbed frame!");
            else
                printf("frame saved\n");
        }
        else
            printf("the frame failed to be captured ...\n");
    } 
    else
        printf("failed to enqueue the frame\n");
}

// unsetup the camera
void CameraUnsetup(tCamera* Camera)
{
    PvCameraClose(Camera->Handle);
    // and free the image buffer of the frame
    delete [] (char*)Camera->Frame.ImageBuffer;
}

