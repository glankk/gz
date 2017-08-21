@echo off
setlocal
set PATH=./bin;%PATH%
pushd .
%~d0
cd %~dp0
gru lua/patch.lua %*
popd
