#include "kurl.h"

global_variable platform_api *Platform;

internal void
GenerateRequest(app_state *AppState)
{
    wchar_t UrlBuffer[280];
    Platform->GetControlText(ID_EDIT_URL, UrlBuffer, sizeof(UrlBuffer));
    
    wchar_t PathBuffer[280] = {0};
    wchar_t *Path = FindFirstOccurrence(UrlBuffer, L"/");
    wchar_t HostBuffer[280] = {0};
    if(Path == 0)
    {
        wchar_t *DefaultPath = L"/";
        Copy(StringLength(DefaultPath)*sizeof(wchar_t), DefaultPath, PathBuffer);
    }
    else
    {
        Copy(StringLength(Path)*sizeof(wchar_t), Path, PathBuffer);
        Path[0] = '\0';
    }
    Copy(StringLength(UrlBuffer)*sizeof(wchar_t), UrlBuffer, HostBuffer);
    
    
    temporary_memory TempMemory = BeginTemporaryMemory(&AppState->TransientArena);
    
    wchar_t *At;
    string RequestRaw = BeginPushString(TempMemory.Arena, &At);
    s32 SelectedIndex = Platform->GetComboSelectedIndex(ID_COMBO_METHOD);
    ContinuePushString(TempMemory.Arena, &RequestRaw, HttpMethodToString((http_method)SelectedIndex), &At);
    ContinuePushString(TempMemory.Arena, &RequestRaw, L" ", &At);
    ContinuePushString(TempMemory.Arena, &RequestRaw, PathBuffer, &At);
    ContinuePushString(TempMemory.Arena, &RequestRaw, L" HTTP/1.1\r\nHost: ", &At);
    ContinuePushString(TempMemory.Arena, &RequestRaw, HostBuffer, &At);
    ContinuePushString(TempMemory.Arena, &RequestRaw, L"\r\nUser-Agent: kurl/debug\r\nAccept: */*\r\n\r\n", &At);
    EndPushString(TempMemory.Arena, &RequestRaw);
    
    Platform->SetControlText(ID_EDIT_REQUEST_RAW, RequestRaw.Data);
    
    EndTemporaryMemory(TempMemory);
    CheckArena(&AppState->TransientArena);
}

struct http_parse_state;
typedef void http_parse_function(http_parse_state *State);

internal void ParseHttpValue_(http_parse_state *State);
internal void ParseHttpKey_(http_parse_state *State);
internal void ParseHttpHeader_(http_parse_state *State);

struct key_value
{
    struct key_value *Next;
    
    string Key;
    string Value;
};

struct http_response
{
    string Version;
    s32 Code;
    string CodeValue;
    
    key_value KeyValues;
    
    u32 ContentLength;
    string Content;
    
};

struct http_parse_state
{
    http_parse_function *Next;
    
    memory_arena *Arena;
    
    wchar_t *Source;
    u32 SourceLength;
    u32 SourceIndex;
    
    wchar_t Lexer[256];
    u32 LexerIndex;
    
    http_response *Context;
    key_value *CurrentKeyValue;
};

internal void
ParseHttpValue_(http_parse_state *State)
{
    while((State->SourceIndex <= State->SourceLength) &&
          !IsEndOfLine(State->Source[State->SourceIndex]))
    {
        State->Lexer[State->LexerIndex++] = State->Source[State->SourceIndex++];
    }
    State->Lexer[State->LexerIndex] = '\0';
    State->SourceIndex += 2;
    
    State->CurrentKeyValue->Value = PushString(State->Arena, State->Lexer);
    State->LexerIndex = 0;
    
    if((State->SourceIndex <= State->SourceLength) &&
       IsEndOfLine(State->Source[State->SourceIndex]))
    {
        State->SourceIndex += 2;
        // TODO(kstandbridge): Parse data
        State->Next = 0;
    }
    else
    {
        State->Next = ParseHttpKey_;
    }
}

internal void
ParseHttpKey_(http_parse_state *State)
{
    while((State->SourceIndex <= State->SourceLength) &&
          State->Source[State->SourceIndex] != ':')
    {
        State->Lexer[State->LexerIndex++] = State->Source[State->SourceIndex++];
    }
    State->Lexer[State->LexerIndex] = '\0';
    State->SourceIndex += 2;
    
    if(State->CurrentKeyValue == 0)
    {
        State->CurrentKeyValue = &State->Context->KeyValues;
    }
    else
    {
        State->CurrentKeyValue->Next = PushStruct(State->Arena, key_value);
        State->CurrentKeyValue = State->CurrentKeyValue->Next;
    }
    
    State->CurrentKeyValue->Key = PushString(State->Arena, State->Lexer);
    State->LexerIndex = 0;
    
    State->Next = ParseHttpValue_;
    
}

internal void
ParseHttpHeader_(http_parse_state *State)
{
    Assert(State->Source[State->SourceIndex] == 'H');
    Assert(State->Source[State->SourceIndex + 1] == 'T');
    Assert(State->Source[State->SourceIndex + 2] == 'T');
    Assert(State->Source[State->SourceIndex + 3] == 'P');
    State->SourceIndex += 5;
    
    for(s32 Index = 0;
        Index < 3;
        ++Index)
    {
        while((State->SourceIndex <= State->SourceLength) &&
              !IsWhitespace(State->Source[State->SourceIndex]))
        {
            State->Lexer[State->LexerIndex++] = State->Source[State->SourceIndex++];
        }
        State->Lexer[State->LexerIndex] = '\0';
        ++State->SourceIndex;
        
        if(Index == 0)
        {
            State->Context->Version = PushString(State->Arena, State->Lexer);
        }
        else if(Index == 1)
        {
            State->Context->Code = S32FromZ(State->Lexer);
        }
        else if(Index == 2)
        {
            State->Context->CodeValue = PushString(State->Arena, State->Lexer);
        }
        else
        {
            InvalidCodePath;
        }
        State->LexerIndex = 0;
    }
    ++State->SourceIndex;
    
    State->Next = ParseHttpKey_;
}

internal http_response
ParseHttp(memory_arena *Arena, wchar_t *Source, u32 SourceLength)
{
    http_response Result;
    
    http_parse_state State = {0};
    State.Next = ParseHttpHeader_;
    State.Arena = Arena;
    State.Source = Source;
    State.SourceLength = SourceLength;
    State.Context = &Result;
    while(State.Next)
    {
        State.Next(&State);
    }
    
    return Result;
}


extern "C" void
InitApp(app_memory *AppMemory)
{
    Platform = &AppMemory->PlatformApi;
    
    app_state *AppState = (app_state *)AppMemory->PermanentStorage;
    Assert(!AppState->IsInitialized);
    
    if(!AppState->IsInitialized)
    {
        InitializeArena(&AppState->PermanentArena, 
                        AppMemory->PermanentStorageSize - sizeof(app_state), 
                        (u8 *)AppMemory->PermanentStorage + sizeof(app_state));
        InitializeArena(&AppState->TransientArena,
                        AppMemory->TransientStorageSize,
                        AppMemory->TransientStorage);
        
        InitTasks(Platform, &AppState->TransientArena, 128, Kilobytes(8));
        
        Platform->AddPanel(ID_WINDOW, ID_MAIN, SIZE_FILL, ControlLayout_Horizontal);
        
        Platform->AddGroupBox(ID_MAIN, ID_GROUP_HISTORY, L"History", 314.0f, ControlLayout_Verticle);
        Platform->SetControlMargin(ID_GROUP_HISTORY, 8.0f, 8.0f, 4.0f, 8.0f);
        Platform->AddListView(ID_GROUP_HISTORY, ID_LIST_HISTORY, SIZE_FILL);
        Platform->AddListViewColumn(ID_LIST_HISTORY, 0, L"Code");
        Platform->AddListViewColumn(ID_LIST_HISTORY, 1, L"Method");
        Platform->AddListViewColumn(ID_LIST_HISTORY, 2, L"Url");
        Platform->AddPanel(ID_MAIN, ID_PANEL_MAIN, SIZE_FILL, ControlLayout_Verticle);
        
        Platform->AddGroupBox(ID_PANEL_MAIN, ID_GROUP_URL, L"Url", 58.0f, ControlLayout_Horizontal);
        Platform->SetControlMargin(ID_GROUP_URL, 8.0f, 4.0f, 8.0f, 4.0f);
        Platform->AddCombobox(ID_GROUP_URL, ID_COMBO_METHOD, 64.0f);
        
        for(s32 Method = HttpMethod_Post;
            Method != HttpMethod_Count;
            ++Method)
        {
            Platform->InsertComboText(ID_COMBO_METHOD, HttpMethodToString((http_method)Method));
        }
        
        
        Platform->SetComboSelectedIndex(ID_COMBO_METHOD, 1);
        Platform->AddEdit(ID_GROUP_URL, ID_EDIT_URL, L"echo.jsontest.com/key/value/one/two", SIZE_FILL);
        Platform->SetControlMargin(ID_EDIT_URL, 2.0f, 8.0f, 8.0f, 2.0f);
        Platform->AddButton(ID_GROUP_URL, ID_BUTTON_SEND, L"Send", 48.0f);
        
        Platform->AddGroupBox(ID_PANEL_MAIN, ID_GROUP_REQUEST, L"Request", 128.0f, ControlLayout_Verticle);
        Platform->SetControlMargin(ID_GROUP_REQUEST, 4.0f, 4.0f, 8.0f, 4.0f);
        Platform->AddEditMultiline(ID_GROUP_REQUEST, ID_EDIT_REQUEST_RAW, L"", SIZE_FILL);
        
        Platform->AddGroupBox(ID_PANEL_MAIN, ID_GROUP_RESPONSE, L"Response", SIZE_FILL, ControlLayout_Verticle);
        Platform->SetControlMargin(ID_GROUP_RESPONSE, 4.0f, 4.0f, 8.0f, 8.0f);
        Platform->AddEditMultiline(ID_GROUP_RESPONSE, ID_EDIT_RESPONSE_RAW, L"", SIZE_FILL);
        
        LogDebug(L"Controls Loaded...");
        
        AppState->IsInitialized = true;
        
        GenerateRequest(AppState);
    }
    
    
}


extern "C" void
HandleCommand(app_memory *AppMemory, s64 ControlId)
{
    LogDebug(L"HandleCommand(ConrolId: %d)", ControlId);
    app_state *AppState = (app_state *)AppMemory->PermanentStorage;
    
    if(ControlId == ID_BUTTON_SEND)
    {
        request_history *Tail = &AppState->RequestHistory;
        s32 RequestHistoryCount = 1;
        while(Tail->Next != 0)
        {
            Tail = Tail->Next;
            ++RequestHistoryCount;
        }
        Tail->Next = PushStruct(&AppState->TransientArena, request_history);
        Tail = Tail->Next;
        Tail->Next = 0;
        
        
        local_persist u8 Code = 100;
        wchar_t Buffer[2048];
        FormatString(sizeof(Buffer), Buffer, L"%d", ++Code);
        Tail->Code = PushString(&AppState->TransientArena, Buffer);;
        Platform->GetControlText(ID_COMBO_METHOD, Buffer, sizeof(Buffer));
        Tail->Method = PushString(&AppState->TransientArena, Buffer);
        Platform->GetControlText(ID_EDIT_URL, Buffer, sizeof(Buffer));
        wchar_t *HostnameEnd = FindFirstOccurrence(Buffer, L"/");
        if(HostnameEnd)
        {
            HostnameEnd[0] = '\0';
        }
        Tail->Url = PushString(&AppState->TransientArena, Buffer);
        char Url[256];
        Utf16ToChar((char *)Buffer, Url, StringLength(Buffer));
        Platform->GetControlText(ID_EDIT_REQUEST_RAW, Buffer, sizeof(Buffer));
        Tail->RequestRaw = PushString(&AppState->TransientArena, Buffer);
        char RequestRaw[256];
        Utf16ToChar((char *)Buffer, RequestRaw, StringLength(Buffer));
        s32 RequestLength = StringLength(RequestRaw);
        
        
        Tail->ResponseRaw = Platform->SendHttpRequest(&AppState->TransientArena, Url, RequestRaw, RequestLength);
        
        http_response Response = ParseHttp(&AppState->TransientArena, Tail->ResponseRaw.Data, Tail->ResponseRaw.Length);
        LogDebug(L"Got response code: %d", Response.Code);
        
        for(key_value *KeyValue = &Response.KeyValues;
            KeyValue != 0;
            KeyValue = KeyValue->Next)
        {
            LogDebug(L"%S: %S", KeyValue->Key, KeyValue->Value);
        }
        
        Platform->SetListViewItemCount(ID_LIST_HISTORY, RequestHistoryCount);
        Platform->AutoSizeListViewColumn(ID_LIST_HISTORY, 0);
        Platform->AutoSizeListViewColumn(ID_LIST_HISTORY, 1);
        Platform->AutoSizeListViewColumn(ID_LIST_HISTORY, 2);
    }
    else
    {
        Platform->ShowMessageBox(L"Error", L"Unknown ControlId");
        __debugbreak();
    }
}

extern "C" void
HandleListViewItemChanged(app_memory *AppMemory, s64 ControlId, s32 Row)
{
    LogDebug(L"HandleListViewItemChanged(ControlId: %d, Row: %d)", ControlId, Row);
    app_state *AppState = (app_state *)AppMemory->PermanentStorage;
    
    if(ControlId == ID_LIST_HISTORY)
    {
        if(Row == -1)
        {
            // TODO(kstandbridge): Select the correct index of the Method? I really need an emum for that
            // TODO(kstandbridge): Prehaps a clear all function, or set to some defauls
            Platform->SetControlText(ID_EDIT_URL, L"");
            Platform->SetControlText(ID_EDIT_REQUEST_RAW, L"");
            Platform->SetControlText(ID_EDIT_RESPONSE_RAW, L"");
        }
        else
        {
            request_history *RequestHistory = &AppState->RequestHistory;
            s32 Index = 0;
            do
            {
                RequestHistory = RequestHistory->Next;
                ++Index;
            } while(Index <= Row);
            
            // TODO(kstandbridge): Select the correct index of the Method? I really need an emum for that
            Platform->SetControlText(ID_EDIT_URL, RequestHistory->Url.Data);
            Platform->SetControlText(ID_EDIT_REQUEST_RAW, RequestHistory->RequestRaw.Data);
            Platform->SetControlText(ID_EDIT_RESPONSE_RAW, RequestHistory->ResponseRaw.Data);
        }
    }
    
}

extern "C" void
CheckboxChanged(app_memory *AppMemory, s64 ControlId, b32 IsChecked)
{
    
}

extern "C" wchar_t *
GetListViewText(app_memory *AppMemory, s64 ControlId, s32 Column, s32 Row)
{
    wchar_t *Result = 0;
    
    app_state *AppState = (app_state *)AppMemory->PermanentStorage;
    if(ControlId == ID_LIST_HISTORY)
    {
        request_history *RequestHistory = &AppState->RequestHistory;
        s32 Index = 0;
        do
        {
            RequestHistory = RequestHistory->Next;
            ++Index;
        } while(Index <= Row);
        
        if(Column == 0)
        {
            Result = RequestHistory->Code.Data;
        }
        else if(Column == 1)
        {
            Result = RequestHistory->Method.Data;
        }
        else if(Column == 2)
        {
            Result = RequestHistory->Url.Data;
        }
        else
        {
            Result = L"Unknown column";
        }
    }
    else
    {
        __debugbreak();
        Result = L"Unknown list view";
    }
    
    return Result;
}

extern "C" void
LogWorkCallback(platform_work_queue *Queue, void *Data)
{
    
}

extern "C" void
ComboChanged(app_memory *AppMemory, s64 ControlId)
{
    app_state *AppState = (app_state *)AppMemory->PermanentStorage;
    if(AppState->IsInitialized)
    {
        if(ControlId == ID_COMBO_METHOD)
        {
            GenerateRequest(AppState);
        }
        else
        {
            LogError(L"Unknown combo box changed id: %d", ControlId);
        }
    }
}

extern "C" void
EditChanged(app_memory *AppMemory, s64 ControlId)
{
    app_state *AppState = (app_state *)AppMemory->PermanentStorage;
    if(AppState->IsInitialized)
    {        
        if(ControlId == ID_EDIT_URL)
        {
            GenerateRequest(AppState);
        }
        else
        {
            LogError(L"Unknown edit changed id: %d", ControlId);
        }
    }
    
}