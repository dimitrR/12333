#ifndef SETTINGDEMON_H
#define SETTINGDEMON_H

#include <iostream>

using namespace std;

class settingDemon{
public:
    settingDemon();

    bool openSetting(string fileSetting);
    void closeSetting();
    void readSetting();

    string  path();
    int     time();
private:
    int findItem(string name);

    bool isOpen;
    FILE *fs;
};

#endif // SETTINGDEMON_H
