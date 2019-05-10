@echo off

if not exist patch\ups mkdir patch\ups
gru patch/lua/make-patch-vc.lua %*
