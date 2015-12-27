@ECHO OFF
ECHO Generating native JNI headers...
IF EXIST ..\..\..\src\native\include\org_zeromq_zyre_Zyre.h GOTO HAVE_Zyre
"%JAVA_HOME%\bin\javah.exe" -d ..\..\..\src\native\include -classpath ..\..\..\src\main\java org.zeromq.zyre.Zyre
:HAVE_Zyre
IF EXIST ..\..\..\src\native\include\org_zeromq_zyre_ZyreEvent.h GOTO HAVE_ZyreEvent
"%JAVA_HOME%\bin\javah.exe" -d ..\..\..\src\native\include -classpath ..\..\..\src\main\java org.zeromq.zyre.ZyreEvent
:HAVE_ZyreEvent
