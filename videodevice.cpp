#include <sys/ioctl.h>          //ioctrl()   манипулирует базовыми параметрами устройств, представленных в виде специальных файлов. 
#include <linux/videodev2.h>    //struct v4l2_capability
#include <unistd.h>             //close()

#include <sys/mman.h>//Отображение файлов в память
#include <sstream>//работы со строками
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sstream>

#include "videodevice.h"
#include "jpeglib.h"

#define IMAGE_WIDTH     640
#define IMAGE_HEIGHT    480

videodevice::videodevice(){
}

void videodevice::errno_exit(string err_str){
    cout<<"\""<<err_str<<"\": "<<errno<<", "<<strerror(errno)<<endl;
    exit(EXIT_FAILURE);
}

int videodevice::xioctl(int fd, int request, void *arg){
    int r;

    r=ioctl(fd, request, arg);
    if(r==-1){
        if(errno==EAGAIN)
            return EAGAIN;

        stringstream ss;
        ss<<"ioctl code "<<request<<" ";

        errno_exit(ss.str());
    }

    return r;
}

void videodevice::openDevice(string dev_name){
    fileDevicePath=dev_name;

    fd=open(fileDevicePath.c_str(), O_RDWR /* required */ | O_NONBLOCK, 0);
    if(fd==-1){
        stringstream str;
        str << "Cannot open '" << fileDevicePath << "'";
        errno_exit(str.str());
    }

    cout<<"Open device"<< fileDevicePath<<endl;
}

void videodevice::setFormatcam(){
    struct v4l2_format fmt;
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = IMAGE_WIDTH;
    fmt.fmt.pix.height      = IMAGE_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV422P;

    if(-1==xioctl(fd, VIDIOC_S_FMT, &fmt))
        errno_exit("VIDIOC_S_FMT");
}

void videodevice::closeDevice(){
    if(close(fd)==-1)
        errno_exit("close");
    fd=-1;

    cout<<"Close device "<<fileDevicePath<<endl;
}

void videodevice::getFrame(int iTime){//метод readFrame — отвечает за чтение и обработку полученого изображения//
    //методы startCapturing(), stopCapturing() — включение/выключение режима streaming у видеоустройства. Наличие этих функций, у камеры, можно проверить флагом V4L2_CAP_STREAMING [*].//
    setFormatcam();
    initMMAP();//создание буфера памяти устройства.

    long int i=1;
    while(true){
        startCapturing();

        ostringstream str;
         
        str<<i;
        string sFile=str.str();
        for(;;){
            if(readFrame(sPath+sFile))
               break;
        }
        ++i;
        stopCapturing();

        cout<<"next frame to "<<iTime<<" sec, frame - "<<i<<endl;
        sleep(iTime);
    }

    freeMMAP();//чистка буфера памяти устройства.
}

void videodevice::setPath(string sPath){
    this->sPath=sPath;
}

void videodevice::initMMAP(){
    //Инициализация буфера
    struct v4l2_requestbuffers req;
    req.count=1;
    req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory=V4L2_MEMORY_MMAP;

    xioctl(fd, VIDIOC_REQBUFS, &req);//VIDIOC_REQBUFS  позволяет проинициализировать буфер памяти внутри устройства. 

    //выделить память
    devbuffer=(buffer*)calloc(req.count, sizeof(*devbuffer));

    //отоброжаем буфер на память
    struct v4l2_buffer buf;
    buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.index=0;

    xioctl(fd, VIDIOC_QUERYBUF, &buf);//Функция VIDIOC_QUERYBUF  позволяет считать параметры буфера, которые будут использоваться для создания memory-mapping области

    //отоброзить память устройства в оперативную память
    devbuffer->length=buf.length;
    devbuffer->start=mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if(devbuffer->start==MAP_FAILED)
        errno_exit("mmap");

    cout<<"Init mmap"<<endl;
}

void videodevice::startCapturing(){//переключить камеру в режим захвата. 
    struct v4l2_buffer buf;
    buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory=V4L2_MEMORY_MMAP;
    buf.index=0;

    //буфер в очередь обработки драйвером устройства
    xioctl(fd, VIDIOC_QBUF, &buf);

    //камеру в режим захвата
    enum v4l2_buf_type type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMON, &type);//Функция VIDIOC_STREAMON[в†“] включает камеру в режим захвата.

    cout<<"Start capturing"<<endl;
}

bool videodevice::readFrame(string file_name){
    struct v4l2_buffer buf;
    buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory=V4L2_MEMORY_MMAP;

    //освобождаем буфер
    if(xioctl(fd, VIDIOC_DQBUF, &buf)==EAGAIN)//Функция VIDIOC_DQBUF[в†“] освобождает буфер из очереди обработки драйвера. 
        return false;

    buffer *temp=devbuffer;

    //сохраняем в файл RAW
    string rawFile=file_name+string(".raw");
    FILE *out_file=fopen(rawFile.c_str(), "w");
    fwrite(temp->start, temp->length, 1, out_file);
    fclose(out_file);

    //сохраняем фото в jpg
    cout<<"save jpeg"<<endl;
    string jpegFile=file_name+string(".jpeg");
    savetojpeg(jpegFile.c_str());

    return true;
}

void videodevice::savetojpeg(const char *filename){
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    FILE *outfile;                //выходной файл jpeg
    JSAMPROW row_pointer[1];      //указатель на JSAMPLE

    //инициализация jpeg компресора
    cinfo.err=jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    //открытия файла для сохраннения
    if((outfile=fopen(filename, "wb")) == NULL) {
        fprintf(stderr, "can't open %s\n", filename);
        exit(1);
    }

    jpeg_stdio_dest(&cinfo, outfile);
    cinfo.image_width=IMAGE_WIDTH & -1;         //размер изображения
    cinfo.image_height=IMAGE_HEIGHT  & -1;
    cinfo.input_components=3;                //количество цветовых компанентов
    cinfo.in_color_space=JCS_YCbCr;          //цветовая схема

    //установка параметров изображения
    jpeg_set_defaults(&cinfo);

    //установка качества изображения
    jpeg_set_quality(&cinfo, 120, TRUE);

    //Начало компрессии изображения
    jpeg_start_compress(&cinfo, TRUE);

    JSAMPLE *image_buffer=(JSAMPLE*)devbuffer->start;
    
    unsigned char *buf=new unsigned char[IMAGE_WIDTH*3]; 
    while (cinfo.next_scanline<IMAGE_HEIGHT) { 
        for(int i=0; i<cinfo.image_width; i+=2){ 
            buf[i*3]=image_buffer[i*2]; 
            buf[i*3+1]=image_buffer[i*2+1]; 
            buf[i*3+2]=image_buffer[i*2+3]; 
            buf[i*3+3]=image_buffer[i*2+2]; 
            buf[i*3+4]=image_buffer[i*2+1]; 
            buf[i*3+5]=image_buffer[i*2+3]; 
        } 
        row_pointer[0]=buf; 
        image_buffer+=IMAGE_WIDTH*2; 
        jpeg_write_scanlines(&cinfo, row_pointer, 1); 
    }
    jpeg_finish_compress(&cinfo);
    delete [] buf;
    
    fclose(outfile);
    jpeg_destroy_compress(&cinfo);
}

void videodevice::stopCapturing(){
    enum v4l2_buf_type type;

    type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type);

    cout<<"stop Capturing"<<endl;
}

void videodevice::freeMMAP(){
    if(munmap(devbuffer->start, devbuffer->length)==-1)
        errno_exit("munmap");

    free(devbuffer);

    cout<<"free mmap"<<endl;
}
