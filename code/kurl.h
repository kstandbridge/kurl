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

#define ID_GROUP_RESPONSE    1500
#define ID_EDIT_RESPONSE_RAW 1501


#define Log(Format, ...) { \
u64 TimeStamp = Platform->GetSystemTimeStamp(); \
wchar_t LogBuffer[1024]; \
FormatString(sizeof(LogBuffer), LogBuffer, Format, __VA_ARGS__); \
string LogMessage; \
LogMessage.Length = StringLength(LogBuffer); \
LogMessage.Size = (LogMessage.Length + 1)*sizeof(wchar_t); \
LogMessage.Data = LogBuffer; \
Platform->DebugLog(TimeStamp, L"debug", LogMessage); } \


#include "kengine_platform.h"
#include "kengine_shared.h"

struct request_history
{
    struct request_history *Next;
    
    // TODO(kstandbridge): code as enum and converstation to string
    string Code;
    
    string Method;
    string Url;
    string RequestRaw;
    string ResponseRaw;
};

struct app_state
{
    b32 IsInitialized;
    
    memory_arena PermanentArena;
    memory_arena TransientArena;
    
    request_history RequestHistory;
};

#define QUI_H
#endif //QUI_H
