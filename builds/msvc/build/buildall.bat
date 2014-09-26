@ECHO OFF

CALL buildbase.bat ..\vs2013\zyre.sln 12
ECHO.
CALL buildbase.bat ..\vs2012\zyre.sln 11
ECHO.

PAUSE