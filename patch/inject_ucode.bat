@echo off
setlocal

set PATH=%CD%/bin;%PATH%
gru lua/inject_ucode.lua %*
