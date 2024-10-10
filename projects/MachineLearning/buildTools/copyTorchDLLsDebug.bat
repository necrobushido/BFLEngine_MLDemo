setlocal

rem Ensure LIBTORCH_DEBUG is set
if not defined LIBTORCH_DEBUG (
    echo LIBTORCH_DEBUG environment variable is not set.
    exit /b 1
)

xcopy %LIBTORCH_DEBUG%\libtorch\lib ..\x64\Debug /D /Y
if %errorlevel% neq 0 goto :cmEnd

:cmEnd
endlocal
exit /b 0