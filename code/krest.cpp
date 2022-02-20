#include "kengine_platform.h"

#include "kengine_shared.h"

#include "kengine_http.h"
#include "kengine_http.cpp"


extern "C" char *
HandleHttpCallback(memory_arena *Arena, char *Request, s32 RequestLength)
{
    char *Result = 0;
    
    temporary_memory TempMemory = BeginTemporaryMemory(Arena);
    
    string RequestString = PushString(TempMemory.Arena, Request);
    
    http_parsed Parsed = ParseHttp(Arena, RequestString.Data, RequestString.Length, HttpParsedType_Request);
    
    http_parsed Response = {0};
    Response.Version = PushString(Arena, L"1.1");
    Response.ContentType = HttpContentType_TextHttp;
    
    key_value *CurrentHeader = &Response.Headers;
    CurrentHeader->Key = PushString(Arena, L"Server");
    CurrentHeader->Value = PushString(Arena, L"Krest/debug");
    CurrentHeader->Next = PushStruct(Arena, key_value);
    CurrentHeader = CurrentHeader->Next;
    
    CurrentHeader->Key = PushString(Arena, L"Date");
    CurrentHeader->Value = PushString(Arena, L"Fri, 18 Feb 2022 15:02:22 GMT");
    CurrentHeader->Next = 0;
    
    Response.Content = PushString(Arena, L"<html><head><title>localhost - /</title></head><body><H1>localhost - /</H1>Foo bar</body></html>");
    Response.ContentLength = Response.Content.Length;
    
    Response.StatusCode = HttpStatusCode_OK;
    
    format_string_state StringState = BeginFormatString(TempMemory.Arena);
    AppendStringFormat(&StringState, L"HTTP/%S %d %s\r\n", Response.Version, Response.StatusCode, HttpStatusCodeToString(Response.StatusCode));
    AppendStringFormat(&StringState, L"Content-Type: %s\r\n", HttpContentTypeToString(Response.ContentType));
    for(key_value *Header = &Response.Headers;
        Header != 0;
        Header = Header->Next)
    {
        AppendStringFormat(&StringState, L"%S: %S\r\n", Header->Key, Header->Value);
    }
    AppendStringFormat(&StringState, L"Content-Length: %d\r\n\r\n", Response.ContentLength);
    AppendStringFormat(&StringState, L"%S", Response.Content);
    string StringResult = EndFormatString(&StringState);
    
    Result = PushArray(Arena, StringResult.Length + 1, char);
    Utf16ToChar((char *)StringResult.Data, Result, StringResult.Length);
    
    EndTemporaryMemory(TempMemory);
    CheckArena(Arena);
    
    return Result;
}