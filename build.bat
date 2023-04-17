@echo off

REM Compile preprocessor
if not exist out mkdir out
pushd out

if not exist include mkdir include

echo ####################################
echo ##          PREPROCESSOR          ##
echo ####################################

cl /nologo ..\code\preprocessor.cpp /EHsc /std:c++20
if %ERRORLEVEL% neq 0 goto :error

preprocessor.exe ..\code\client.cpp include\output.hpp
if %ERRORLEVEL% neq 0 goto :error

if not "%~1" == "full" goto :end

echo:

echo ####################################
echo ##             CLIENT             ##
echo ####################################
cl /nologo  /Zi ..\code\client.cpp -Iinclude /std:c++20  /EHsc /std:c++20 /link /DEBUG:FULL
if %ERRORLEVEL% neq 0 goto :error

client.exe
if %ERRORLEVEL% neq 0 goto :error

goto :end

:error
echo There was an error. Idiot.

:end

popd
