@echo off
setlocal
pushd .
%~d0
cd %~dp0
set PATH=./bin;%PATH%
gru lua/patch-vc.lua %*
popd
