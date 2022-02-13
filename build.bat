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
	set CommonCompilerFlags=-DQUI_INTERNAL=1 %CommonCompilerFlags%
)

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

del *.pdb > NUL 2> NUL
echo WAITING FOR PDB > lock.tmp

if "%~1"=="RELEASE" (

	echo DO RELEASE BUILD

) else (

	REM kengine.dll
	cl %CommonCompilerFlags% -MTd -Od -I..\kurl\library\kengine\code\ ..\kurl\code\kurl.cpp /LD /link %CommonLinkerFlags% -out:kengine.dll -PDB:kengine_%random%.pdb -EXPORT:InitApp -EXPORT:HandleCommand -EXPORT:CheckboxChanged -EXPORT:GetListViewText -EXPORT:HandleListViewItemChanged -EXPORT:LogWorkCallback -EXPORT:EditChanged -EXPORT:ComboChanged

	REM Skip trying to build GalaQ.exe if currently running
	FOR /F %%x IN ('tasklist /NH /FI "IMAGENAME eq win32_shell.exe"') DO IF %%x == win32_shell.exe goto FOUND
	
	REM win32_shell.exe
	rc -nologo ..\kurl\library\kengine\code\win32_gui_resource.rc
	cl %CommonCompilerFlags% -MTd -Od ..\kurl\library\kengine\code\win32_gui_kengine.cpp ..\kurl\library\kengine\code\win32_gui_resource.res /link -out:kurl.exe %CommonLinkerFlags%
	del /q ..\kurl\library\kengine\code\win32_gui_resource.res

)

:FOUND
del /q *.obj
del lock.tmp
popd