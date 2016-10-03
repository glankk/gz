@echo off
pushd .
cd %~dp0
gru lua/patch.lua %*
popd
pause
