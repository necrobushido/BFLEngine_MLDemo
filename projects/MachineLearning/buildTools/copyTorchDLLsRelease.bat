setlocal

rem Ensure LIBTORCH_RELEASE is set
if not defined LIBTORCH_RELEASE (
    echo LIBTORCH_RELEASE environment variable is not set.
    exit /b 1
)

xcopy %LIBTORCH_RELEASE%\libtorch\lib ..\x64\Release /D /Y
if %errorlevel% neq 0 goto :cmEnd

:cmEnd
endlocal
exit /b 0