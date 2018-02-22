@echo off
setlocal
pushd .
%~d0
cd %~dp0/..
gru patch/lua/make-rom-vc.lua %*
popd
