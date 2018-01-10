@echo off
setlocal
pushd .
%~d0
cd %~dp0
set PATH=../bin;%PATH%
gs -w bin 0x80400060 1024 ../bin/gz/oot-1.0/gz.bin -w text main_hook.gsc -u
popd
pause
