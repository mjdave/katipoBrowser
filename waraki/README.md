# Waraki

Waraki is a standalone local music/mp3 player app, built on top of Katipo. It is a minimal, fully functioning unique and useful cross platform music player. 

Waraki is here in the Katipo Browser repository because it is an example of how to create a standalone application using the Katipo Browser engine.

## What's in this repository?

### waraki-site/ 
Submodule which points to the waraki "katipo site" repository. [This repository](https://github.com/mjdave/waraki-site) contains everything needed to host a waraki instance either via the Waraki app, the Katipo Browser, or katipoHost command line, there is a README with more info in that repository

### waraki-app
This is where you'll find the runtime tui code and resources that make up the application layer between the waraki site and the katipo engine. Here you'll find code for the initial setup process, and for managing the bundled hosting and connections. Waraki.tui is the tui entry point for waraki, it is loaded instead of app/katipoBrowser/code.tui

### mac/ 
Contains xcode projects and macOS specific resources to build the Waraki app for mac. (more platforms soon)

### Waraki.cpp/h
C++ entry point for the Waraki application, replaces KatipoBrowser.cpp/h in the browser source