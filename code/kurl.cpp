#include "kurl.h"

global_variable platform_api *Platform;

extern "C" void
InitApp(app_memory *AppMemory)
{
    Platform = &AppMemory->PlatformApi;
    
    app_state *AppState = (app_state *)AppMemory->PermanentStorage;
    if(!AppState->IsInitialized)
    {
        InitializeArena(&AppState->PermanentArena, 
                        AppMemory->PermanentStorageSize - sizeof(app_state), 
                        (u8 *)AppMemory->PermanentStorage + sizeof(app_state));
        InitializeArena(&AppState->TransientArena,
                        AppMemory->TransientStorageSize,
                        AppMemory->TransientStorage);
        
        AppState->IsInitialized = false;
    }
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
    Platform->AddCombobox(ID_GROUP_URL, ID_COMBO_METHOD, 58.0f);
    Platform->InsertComboText(ID_COMBO_METHOD, L"GET");
    Platform->InsertComboText(ID_COMBO_METHOD, L"POST");
    Platform->SetComboSelectedIndex(ID_COMBO_METHOD, 1);
    Platform->AddEdit(ID_GROUP_URL, ID_EDIT_URL, L"www.google.co.uk", SIZE_FILL);
    Platform->SetControlMargin(ID_EDIT_URL, 2.0f, 8.0f, 8.0f, 2.0f);
    Platform->AddButton(ID_GROUP_URL, ID_BUTTON_SEND, L"Send", 48.0f);
    
    Platform->AddGroupBox(ID_PANEL_MAIN, ID_GROUP_REQUEST, L"Request", 128.0f, ControlLayout_Verticle);
    Platform->SetControlMargin(ID_GROUP_REQUEST, 4.0f, 4.0f, 8.0f, 4.0f);
    Platform->AddEdit(ID_GROUP_REQUEST, ID_EDIT_REQUEST_RAW, L"GET / HTTP/1.1\r\n\r\n", SIZE_FILL);
    
    Platform->AddGroupBox(ID_PANEL_MAIN, ID_GROUP_RESPONSE, L"Response", SIZE_FILL, ControlLayout_Verticle);
    Platform->SetControlMargin(ID_GROUP_RESPONSE, 4.0f, 4.0f, 8.0f, 8.0f);
    Platform->AddEdit(ID_GROUP_RESPONSE, ID_EDIT_RESPONSE_RAW, L"{}", SIZE_FILL);
    
    Log(L"Controls Loaded...");
}


extern "C" void
HandleCommand(app_memory *AppMemory, s64 ControlId)
{
    Log(L"HandleCommand(ConrolId: %d)", ControlId);
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
        
        
        local_persist u8 Code = 100;
        wchar_t Buffer[256];
        FormatString(sizeof(Buffer), Buffer, L"%d", ++Code);
        Tail->Code = PushString(&AppState->TransientArena, Buffer);;
        
        Platform->GetControlText(ID_COMBO_METHOD, Buffer, sizeof(Buffer));
        Tail->Method = PushString(&AppState->TransientArena, Buffer);
        Platform->GetControlText(ID_EDIT_URL, Buffer, sizeof(Buffer));
        Tail->Url = PushString(&AppState->TransientArena, Buffer);
        char Url[256];
        Utf16ToChar((char *)Buffer, Url, StringLength(Buffer));
        Platform->GetControlText(ID_EDIT_REQUEST_RAW, Buffer, sizeof(Buffer));
        Tail->RequestRaw = PushString(&AppState->TransientArena, Buffer);
        char RequestRaw[256];
        Utf16ToChar((char *)Buffer, RequestRaw, StringLength(Buffer));
        s32 RequestLength = StringLength(RequestRaw);
        Tail->ResponseRaw = Platform->SendHttpRequest(&AppState->TransientArena, Url, RequestRaw, RequestLength);
        
        
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
    Log(L"HandleListViewItemChanged(ControlId: %d, Row: %d)", ControlId, Row);
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