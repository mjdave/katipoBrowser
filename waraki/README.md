# Waraki

Waraki is a standalone local music/mp3 player app, built on top of Katipo. It is a minimal, fully functioning unique and useful cross platform music player. 

Waraki is here in the Katipo Browser repository because it is an example of how to create a standalone application using the Katipo Browser engine.

## The files in this repository

### Waraki.cpp/h
C++ entry point for the Waraki application, replaces KatipoBrowser.cpp/h in the browser source
### waraki.tui
Tui entry point for waraki, replaces app/katipoBrowser/code.tui
### mac/ 
Contains xcode projects and macOS specific resources to build the Waraki app for mac. (more platforms soon)
### waraki-site/ 
Submodule which points to the waraki "katipo site" repository. [This repository](https://github.com/mjdave/waraki-site) contains everything needed to host a waraki instance either via the Waraki app, the Katipo Browser, or katipoHost command line, there is a README with more info in that repository
