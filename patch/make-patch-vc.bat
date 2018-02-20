@echo off
setlocal
pushd .
%~d0
cd %~dp0/..
if not exist patch\ups mkdir patch\ups
gru patch/lua/make-patch-vc.lua %*
popd
