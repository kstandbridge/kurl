#include "kurl.h"
#include "kengine_http.cpp"

global_variable platform_api *Platform;

internal void
GenerateRequest(app_state *AppState)
{
    wchar_t UrlBuffer[280];
    Platform->GetControlText(ID_EDIT_URL, UrlBuffer, sizeof(UrlBuffer));
    
    wchar_t *UrlPtr = UrlBuffer;
    if(StringBeginsWith(UrlBuffer, L"https://"))
    {
        UrlPtr += 8;
    }
    else if(StringBeginsWith(UrlBuffer, L"http://"))
    {
        UrlPtr += 7;
    }
    wchar_t PathBuffer[280] = {0};
    wchar_t *Path = FindFirstOccurrence(UrlPtr, L"/");
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
    Copy(StringLength(UrlPtr)*sizeof(wchar_t), UrlPtr, HostBuffer);
    
    
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
        Platform->AddEdit(ID_GROUP_URL, ID_EDIT_URL, L"https://localhost", SIZE_FILL);
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
        
        b32 IsHttps = false;
        wchar_t Buffer[2048];
        Platform->GetControlText(ID_COMBO_METHOD, Buffer, sizeof(Buffer));
        Tail->Method = PushString(&AppState->TransientArena, Buffer);
        Platform->GetControlText(ID_EDIT_URL, Buffer, sizeof(Buffer));
        Tail->Url = PushString(&AppState->TransientArena, Buffer);
        wchar_t *UrlPtr = Buffer;
        if(StringBeginsWith(Buffer, L"https://"))
        {
            UrlPtr += 8;
            IsHttps = true;
        }
        else if(StringBeginsWith(Buffer, L"http://"))
        {
            UrlPtr += 7;
        }
        
        wchar_t *HostnameEnd = FindFirstOccurrence(UrlPtr, L"/");
        if(HostnameEnd)
        {
            HostnameEnd[0] = '\0';
        }
        char Url[256];
        Utf16ToChar((char *)UrlPtr, Url, StringLength(UrlPtr));
        Platform->GetControlText(ID_EDIT_REQUEST_RAW, Buffer, sizeof(Buffer));
        Tail->RequestRaw = PushString(&AppState->TransientArena, Buffer);
        char RequestRaw[32768];
        Utf16ToChar((char *)Buffer, RequestRaw, StringLength(Buffer));
        s32 RequestLength = StringLength(RequestRaw);
        if(IsHttps)
        {
            Tail->ResponseRaw = Platform->SendHttpsRequest(&AppState->TransientArena, Url, RequestRaw, RequestLength);
        }
        else
        {
            Tail->ResponseRaw = Platform->SendHttpRequest(&AppState->TransientArena, Url, RequestRaw, RequestLength);
        }
        
        http_response Response = ParseHttp(&AppState->TransientArena, Tail->ResponseRaw.Data, Tail->ResponseRaw.Length);
        FormatString(sizeof(Buffer), Buffer, L"%d %s", Response.StatusCode, HttpStatusCodeToString(Response.StatusCode));
        Tail->Code = PushString(&AppState->TransientArena, Buffer);
        
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