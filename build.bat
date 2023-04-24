@echo off

REM Navigate to out dir and create the output folder.
if not exist out mkdir out
pushd out

if not exist include mkdir include

echo Building Preprocessor
cl /nologo /Zi ..\code\preprocessor.cpp /EHsc /std:c++20 /link /DEBUG:FULL
if %ERRORLEVEL% neq 0 goto :error
echo:

echo Running preprocessor
preprocessor.exe ../code/client.cpp include/output.hpp
if %ERRORLEVEL% neq 0 goto :error

echo Building client
cl /nologo  /Zi ..\code\client.cpp -Iinclude /std:c++20  /EHsc /std:c++20 /link /DEBUG:FULL
if %ERRORLEVEL% neq 0 goto :error
echo:

goto :end

:error
echo There was an error. Idiot. Error was %ERRORLEVEL%
goto :end

:end

popd
