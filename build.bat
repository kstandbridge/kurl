@echo off

if not defined DevEnvDir (
	call C:\"Program Files (x86)"\"Microsoft Visual Studio"\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat x64
)

set CommonCompilerFlags=-DUNICODE -D_UNICODE -WL -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -FC -Z7
set CommonLinkerFlags=-incremental:no -opt:ref 

REM Unreferenced formal parameter
set CommonCompilerFlags=-wd4100 %CommonCompilerFlags%
REM Local variable is initialized but not referenced
set CommonCompilerFlags=-wd4189 %CommonCompilerFlags%
REM Unreferenced function with internal linkage has been removed
set CommonCompilerFlags=-wd4505 %CommonCompilerFlags%

if "%~1"=="RELEASE" (
	set CommonCompilerFlags=-DVERSION=%REVISION% %CommonCompilerFlags%
) else (
	set CommonCompilerFlags=-DKENGINE_INTERNAL=1 %CommonCompilerFlags%
)

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

del *.pdb > NUL 2> NUL
echo WAITING FOR PDB > lock.tmp

if "%~1"=="RELEASE" (

	echo DO RELEASE BUILD

) else (

	REM kengine_http.dll
	REM cl %CommonCompilerFlags% -MTd -Od -I..\kurl\library\kengine\code\ ..\kurl\code\krest.cpp /LD /link %CommonLinkerFlags% -out:kengine_http.dll -PDB:kengine_http_%random%.pdb -EXPORT:HandleHttpCallback


	REM krest.exe
	REM cl %CommonCompilerFlags% -MTd -Od ..\kurl\library\kengine\code\win32_kengine_http.cpp /link -out:krest.exe %CommonLinkerFlags%


	REM kengine_gui.dll
	cl %CommonCompilerFlags% -MTd -Od -I..\kurl\library\kengine\code\ ..\kurl\code\kurl.cpp /LD /link %CommonLinkerFlags% -out:kengine_gui.dll -PDB:kengine_gui_%random%.pdb -EXPORT:InitApp -EXPORT:HandleCommand -EXPORT:CheckboxChanged -EXPORT:GetListViewText -EXPORT:HandleListViewItemChanged -EXPORT:LogWorkCallback -EXPORT:EditChanged -EXPORT:ComboChanged

	REM kurl.exe
	rc -nologo ..\kurl\library\kengine\code\win32_kengine_resource.rc
	cl %CommonCompilerFlags% -MTd -Od ..\kurl\library\kengine\code\win32_kengine_gui.cpp ..\kurl\library\kengine\code\win32_kengine_resource.res /link -out:kurl.exe %CommonLinkerFlags%
	del /q ..\kurl\library\kengine\code\win32_kengine_resource.res


)

del /q *.obj
del lock.tmp
popd