@echo off
setlocal

set PATH=%CD%/bin;%PATH%
echo 45e | gzinject -a genkey -k common-key.bin > nul
gru lua/patch-wad.lua %*
del common-key.bin
rmdir /s /q wadextract
