# Koru Browser
![Koru browser icon](https://github.com/mjdave/katipoBrowser/blob/main/katipoBrowser-iOS-256.png)

Katipo is a minimal internet alternative. A free and open platform that lives entirely outside of the existing HTML/CSS/JS based web.

This is the repository for the client browser, an app that can be installed to connect to sites hosted on a Katipo network. The Katipo equivalent of Safari, Chrome or Firefox.

For more information about Katipo networks generally and what this is all about, have a look at the [Katipo core networking library repository](https://github.com/mjdave/katipo).

Check the [releases](https://github.com/mjdave/katipoBrowser/releases) page for the latest builds.

# How to install and run
Here is an example for for building and running the waraki client on a raspberry pi, it should work for all linux based systems. There are also xcode and visual studio projects provided for Windows, macOS, and iOS.

```
# most of the required packages are to build SDL3, as well as lib-vulkan-dev

sudo apt update
sudo apt-get install build-essential git make pkg-config cmake ninja-build gnome-desktop-testing libasound2-dev libpulse-dev libaudio-dev libfribidi-dev libjack-dev libsndio-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev libthai-dev libusb-1.0-0-dev lib-vulkan-dev

cd katipoBrowser
git submodule update --init --recursive

#to build koru browser:
cd linux
./build.sh
./bin/koru

#to build waraki:
cd waraki/linux
./build.sh
./bin/waraki

```