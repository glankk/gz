@echo off
setlocal
set PATH=../bin;%PATH%
pushd .
%~d0
cd %~dp0
gs -w bin 0x80400060 1024 ../bin/gz/oot-1.2/gz.bin -w text main_hook.gsc -u
popd
pause
