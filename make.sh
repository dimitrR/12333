rm webdemon
g++ -c -o settingdemon.o settingdemon.cpp
g++ -c -o videodevice.o videodevice.cpp
g++ -c -o main.o main.cpp
g++ settingdemon.o videodevice.o main.o -o webdemon -ljpeg
