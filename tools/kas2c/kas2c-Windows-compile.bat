
cls
@echo off
echo.


SET PATH=%PATH%;C:\Projects\HomeworldSDL\tools\kas2c\bison\bin
SET PATH=%PATH%;C:\Projects\HomeworldSDL\tools\kas2c\flex\bin

flex -i -olexer.c lexer.l
bison -d -o parser.c parser.y

pause
