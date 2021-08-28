

cls
@echo off
echo.

:: Preprocess with GCC
:: Run this in PowerShell. Easier than trying to run a CMD shell in unicode mode...
:: UCS-2 output is needed for kas2c.exe to read. Otherwise you'll get stupid parsing errors.
echo Preprocessing .kas files...
powershell -executionpolicy remotesigned -File BuildPreprocess.ps1
echo.

:: Now use kas2c to generate the files.
echo Generating mission files...
call BuildMission.bat Mission01
call BuildMission.bat Mission02
call BuildMission.bat Mission03
call BuildMission.bat Mission04
call BuildMission.bat Mission05
call BuildMission.bat Mission05_OEM
call BuildMission.bat Mission06
call BuildMission.bat Mission07
call BuildMission.bat Mission08
call BuildMission.bat Mission09
call BuildMission.bat Mission10
call BuildMission.bat Mission11
call BuildMission.bat Mission12
call BuildMission.bat Mission13
call BuildMission.bat Mission14
call BuildMission.bat Mission15
call BuildMission.bat Mission16
call BuildMission.bat Tutorial1
echo.

:: Copy them where they need to be.
echo Copying files...
xcopy *.h /Y /Q Generated
xcopy *.c /Y /Q Generated
echo.

:: Then delete the temporary files.
echo Deleting temporary files...
del *.kp
del *.c
del *.h
echo.

echo Done.
pause

