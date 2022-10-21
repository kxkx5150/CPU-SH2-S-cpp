
#ifndef MOVIE_H
#define MOVIE_H

#include "core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define Stopped   1
#define Recording 2
#define Playback  3

#define RunNormal   0
#define Paused      1
#define NeedAdvance 2

void DoMovie(void);

struct MovieStruct
{
    int         Status;
    FILE       *fp;
    int         ReadOnly;
    int         Rerecords;
    int         Size;
    int         Frames;
    const char *filename;
};

extern struct MovieStruct Movie;

struct MovieBufferStruct
{
    int   size;
    char *data;
};

struct MovieBufferStruct *ReadMovieIntoABuffer(FILE *fp);

void MovieLoadState(void);

void SaveMovieInState(void **stream);
void ReadMovieInState(const void *stream);

void TestWrite(struct MovieBufferStruct tempbuffer);

void MovieToggleReadOnly(void);

void TruncateMovie(struct MovieStruct Movie);

void DoFrameAdvance(void);

int  SaveMovie(const char *filename);
int  PlayMovie(const char *filename);
void StopMovie(void);

const char *MakeMovieStateName(const char *filename);

void MovieReadState(const void *stream);

void PauseOrUnpause(void);

int IsMovieLoaded(void);

extern int  framecounter;
extern int  LagFrameFlag;
extern int  lagframecounter;
extern char MovieStatus[40];
extern char InputDisplayString[40];
extern int  FrameAdvanceVariable;

#ifdef __cplusplus
}
#endif

#endif
