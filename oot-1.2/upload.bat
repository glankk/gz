@echo off
pushd .
cd %~dp0
gs -w bin 0x80600000 1024 ../bin/gz/oot-1.2/gz.bin -w text text_hook.gsc -u
popd
pause
