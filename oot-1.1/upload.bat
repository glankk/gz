@echo off
setlocal

set PATH=%CD%/../bin;%PATH%
gs -w bin 0x80400060 1024 ../bin/gz/oot-1.1/gz.bin -w text hooks.gsc -u
pause
