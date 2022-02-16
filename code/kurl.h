#ifndef QUI_H

#define ID_MAIN 1000

#define ID_GROUP_HISTORY 1100
#define ID_LIST_HISTORY  1101
#define ID_PANEL_MAIN    1200

#define ID_GROUP_URL    1300
#define ID_COMBO_METHOD 1301
#define ID_EDIT_URL     1302
#define ID_BUTTON_SEND  1303

#define ID_GROUP_REQUEST    1400
#define ID_EDIT_REQUEST_RAW 1401

#define ID_GROUP_RESPONSE                1500
#define ID_PANEL_RESPONSE                1501
#define ID_GROUP_RESPONSE_COMMON_HEADERS 1502

#define ID_PANEL_RESPONSE_VERSION        1510
#define ID_STATIC_RESPONSE_VERSION       1511
#define ID_EDIT_RESPONSE_VERSION         1512

#define ID_PANEL_RESPONSE_CODE           1520
#define ID_STATIC_RESPONSE_CODE          1521
#define ID_EDIT_RESPONSE_CODE            1522

#define ID_PANEL_RESPONSE_CONTENT_TYPE   1530
#define ID_STATIC_RESPONSE_CONTENT_TYPE  1531
#define ID_EDIT_RESPONSE_CONTENT_TYPE    1532

#define ID_PANEL_RESPONSE_CONTENT_LENGTH   1540
#define ID_STATIC_RESPONSE_CONTENT_LENGTH  1541
#define ID_EDIT_RESPONSE_CONTENT_LENGTH    1542

#define ID_GROUP_RESPONSE_MISC_HEADERS   1700
#define ID_LIST_RESPONSE_MISC_HEADERS    1701

#define ID_GROUP_RESPONSE_RAW            1800
#define ID_EDIT_RESPONSE_RAW             1801

#include "kengine_platform.h"
#include "kengine_shared.h"
#include "kengine_log.h"
#include "kengine_http.h"

enum http_method
{
    HttpMethod_Post,
    HttpMethod_Get,
    HttpMethod_Put,
    HttpMethod_Delete,
    
    HttpMethod_Count
};

inline wchar_t *
HttpMethodToString(http_method HttpMethod)
{
    wchar_t *Result = 0;
    
    switch(HttpMethod)
    {
        case HttpMethod_Post:
        {
            Result = L"POST";
        } break;
        case HttpMethod_Get:
        {
            Result = L"GET";
        } break;
        case HttpMethod_Put:
        {
            Result = L"PUT";
        } break;
        case HttpMethod_Delete:
        {
            Result = L"DELETE";
        } break;
        
        InvalidDefaultCase;
    }
    
    return Result;
}

struct request_history
{
    struct request_history *Next;
    
    string Method;
    string Url;
    string RequestRaw;
    string ResponseRaw;
    
    http_response Response;
};

struct app_state
{
    b32 IsInitialized;
    
    memory_arena PermanentArena;
    memory_arena TransientArena;
    
    request_history RequestHistory;
    request_history *SelectedRequestHistory;
};

#define QUI_H
#endif //QUI_H
