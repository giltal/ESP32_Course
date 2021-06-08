#ifndef OTAutil_h
#define OTAutil_h

#include <FS.h>
#include <SD.h>

bool updateFromSD(fs::FS &fs, char * fileName);

#endif
