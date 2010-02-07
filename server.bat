@ECHO OFF

set BF_DIR=.
set BF_OPTIONS=

IF EXIST bin\bfserver.exe (
    bin\bfserver.exe %BF_OPTIONS% %* 
) ELSE (
    IF EXIST %BF_DIR%\bin\bfserver.exe (
        pushd %BF_DIR%
        bin\bfserver.exe %BF_OPTIONS% %*
        popd
    ) ELSE (
        echo Unable to find the Blood Frontier server binary
        pause
    )
)
