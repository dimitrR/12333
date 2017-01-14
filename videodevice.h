#ifndef VIDEODEVICE_H
#define VIDEODEVICE_H

#include <iostream>

using namespace std;

struct buffer {
    void   *start;
    size_t  length;
};

class videodevice{
public:
    videodevice();

    void openDevice(string dev_name);
    void closeDevice();

    void setPath(string sPath);
    void getFrame(int iTime);

private:
    void errno_exit(string err_str);

    void initMMAP();
    void freeMMAP();
    void setFormatcam();

    void startCapturing();
    void stopCapturing();

    void savetojpeg(const char *filename);
    bool readFrame(string file_name);

    int  xioctl(int fd, int request, void *arg);

    int             fd;
    string          fileDevicePath;
    struct buffer   *devbuffer;
    char            *mypixels;
    string          sPath;
};

#endif // VIDEODEVICE_H
