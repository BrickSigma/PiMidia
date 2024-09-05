#include "pimidia.h"
#include "audio/AudioIO.h"

#include <QApplication>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // AudioIO test
    // TODO: Remove later...
    // *****************************************
    AudioIO audioIO;
    audioIO.initialize();
    std::cout << "Start" << std::endl;
    audioIO.startRecording();
    std::cout << "Wait" << std::endl;
    Pa_Sleep(5000);
    audioIO.stopRecording();
    audioIO.writeRecording("test.raw");
    audioIO.close();
    // *****************************************

    PiMidia w;
    w.show();
    return a.exec();
}
