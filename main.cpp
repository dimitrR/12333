#include <errno.h>      //errno объявление макроса для идентификации ошибок через их код
#include <string.h>     //string
#include <stdio.h>      //cout
#include <unistd.h>     //fork(); setsid()
#include <sys/stat.h>   //umask();

#include "videodevice.h"
#include "settingdemon.h"

void demonBody();

int main(){
    //создаем потомка
    int pid=fork();

    if(pid==-1){ // если не удалось запустить потомка
        cout<<"Error: Start Daemon failed ("<<strerror(errno)<<")"<<endl;//, strerror(errno));
        return -1;
    }
    else
        if(!pid){
            //потомок
            // разрешаем выставлять все биты прав на создаваемые файлы, иначе у нас могут быть проблемы с правами доступа.Функция _umask задает маску разрешений для файлов текущего процесса в режим, указанный в pmode.
            umask(0);

            //создаём новый сеанс, чтобы не зависеть от родителя
            setsid();

            //переходим в корень диска, если мы этого не сделаем
            chdir("/");

            //закрываем дискрипторы ввода/вывода/ошибок, так как нам они больше не понадобятся
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);

            //выполняем основной код демона
            demonBody();

            return 0;
       }
       else{
            //родитель, завершим процес, т.к. основную свою задачу (запуск демона) мы выполнили
            return 0;
       }

    return 0;
}

void demonBody(){
    //имя устройства (веб камеры)
    string device_name="/dev/video0";

    //чтение настроек
    settingDemon *pSetting=new settingDemon;
    if(pSetting->openSetting("/home/dimitr/etc/mydemon.conf"))
        pSetting->readSetting();
    else
        cout<<"error open setting file, set default setting"<<endl;

    string sPath=pSetting->path();
    int iTime=pSetting->time();
    pSetting->closeSetting();

    videodevice *pVideoDevice=new videodevice;
    //открывае дескриптор камеры
    pVideoDevice->openDevice(device_name);

    //настраиваем путь
    pVideoDevice->setPath(sPath);

    //сохраняемкадр с камеры
    pVideoDevice->getFrame(iTime);

    //закрываем дескриптор устройства
    pVideoDevice->closeDevice();
}
