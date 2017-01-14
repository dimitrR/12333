#include "settingdemon.h"

#include <sys/stat.h>
#include <fcntl.h>// Открытие и вывод каталогов,опции управления файлами
#include <unistd.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SETTING_TIME    5
#define SETTING_PATH    "/home/dimitr/temp/"

struct strItem{
    char *name;
    char *value;
};

vector <strItem*> vSetting;

settingDemon::settingDemon(){
    isOpen=false;
}

bool settingDemon::openSetting(string fileSetting){
    //открываем файл с настройками
    fs=fopen(fileSetting.c_str(), "r");
    if(fs==NULL){   //если не удалось открыть (нет файла, доступа)
        cout<<"Cannot open '"<<fileSetting<<"'";
        isOpen=false;
        return false;
    }

    isOpen=true;
    return true;
}

void settingDemon::closeSetting(){
    if(fs!=NULL){
        isOpen=false;
        fclose(fs);
    }
}

void settingDemon::readSetting(){
    if(!isOpen)
        return;

    //очищаем вектор от старых данных и уст. указатель на начало файла
    vSetting.clear();
    fseek(fs, 0, SEEK_SET);

    //читаем файла по строчно
    ssize_t read;
    size_t len=0;
    char *line=NULL;

    while((read=getline(&line, &len, fs))!=-1){
        //разбиваем строку на параметр и значение
        string sTemp=line;
        int iIndex=sTemp.find("=");
        if(iIndex>0){
            //сохраняем в вектор считанные данные
            int sizeValue=sTemp.size()-iIndex-2;

            strItem *pItem=new strItem;
            pItem->name=new char[iIndex+1];
            pItem->name[iIndex]=0;
            pItem->value=new char[sizeValue];
            pItem->value[sizeValue]=0;

            sTemp.copy(pItem->name, iIndex, 0);
            sTemp.copy(pItem->value, sizeValue, iIndex+1);
            vSetting.push_back(pItem);
        }
    }
}

string settingDemon::path(){
    string sTemp=SETTING_PATH;

    int index;
    if((index=findItem("PATH"))!=-1){
        strItem *pItem=vSetting[index];
        sTemp=pItem->value;
    }

    return sTemp;
}

int settingDemon::time(){
    int iTemp=SETTING_TIME;

    int index;
    if((index=findItem("TIME"))!=-1){
        strItem *pItem=vSetting[index];
        iTemp=atoi(pItem->value);
    }

    return iTemp;
}

int settingDemon::findItem(string name){
    //поиск в векторе необходимого параметра
    for(int i=0; i<vSetting.size(); i++){
        strItem *pItem=vSetting.at(i);
        if(strcmp(pItem->name, name.c_str())==0)
            return i;
    }

    return -1;
}
