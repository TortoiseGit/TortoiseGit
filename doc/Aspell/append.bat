@echo off
rem NAnt doesn't seem able to append one file to another
type %1 >> %2
