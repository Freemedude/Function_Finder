@echo off

REM Navigate to out dir and create the output folder.
if not exist out (
    echo "Missing build directory! Make sure you built the process"
    goto :end
)

pushd out
echo Running client
client.exe
if %ERRORLEVEL% neq 0 goto :end
echo:

popd


:error
echo There was an error. Idiot. Error was %ERRORLEVEL%
goto :end

:end
