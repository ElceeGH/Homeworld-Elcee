


:: Generates files for building the mission.
:: Pass the name, for example Mission01

set kas=..\..\tools\kas2c\project\KAS2C\Debug\kas2c.exe
%kas% %1.kp   %1.c   %1.h   %1.func.c

