#ifndef _FETCH_H_
#define _FETCH_H_

typedef struct
{
    char *key;
    char *value;
} Header;

typedef enum
{
    Get,
    Post
} HttpMethod;

struct FetchParams
{
    void(*OnGotData)(char *inCumingBuffer, char *outPut);
    char message[300];
    Header header[3];
    int headCount;
    HttpMethod method;
    char *body;
    int status;
};

void fetch(char *url, struct FetchParams *fetchParams);





#endif /* _FETCH_H_ */