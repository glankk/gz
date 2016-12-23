@echo off
pushd .
cd %~dp0
gs -w bin 0x80400060 1024 ../bin/gz/oot-1.1/gz.bin -w text text_hook.gsc -u
popd
pause
