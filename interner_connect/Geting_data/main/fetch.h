#ifndef _FETCH_H_
#define _FETCH_H_

struct FetchParams 
{
    void (*onGotData)(char *incomingBuffer);
    char message[300];
};


void fetch(char* url, struct FetchParams *fetchParams);















#endif /* _FETCH_H_ */