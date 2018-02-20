@echo off
setlocal
pushd .
%~d0
cd %~dp0
set PATH=./bin;%PATH%
echo 45e | gzinject -a genkey -k common-key.bin > nul
gru lua/patch-wad.lua %*
del common-key.bin
rmdir /s /q wadextract
popd
