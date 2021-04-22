# Reframe360 XL

This plugin is an extension of the excellent Reframe360 (http://reframe360.com/) plugin by Stefan Sietzen.  When Stefan discontinued the project and released the source code, I got to work to add features and fix bugs and it became Reframe360 XL.  XL is used to point out that new features were put in place and is also a nudge to the excellent SkaterXL game.

New features and bug fixes:
- New animation curves implemented (Sine, Expo, Circular)
- Apply animation curves to main camera parameters
- Fix black dot in center of output
- Update to latest libraries and Resolve 17

Enjoy!

# Installation on MacOS (Intel and Apple Silicon)
* Build tested on MacOS 11.2.3 / XCode 12.4
* Install latest XCode from Apple App store
* Install Blackmagic DaVinci Resolve from Blackmagic website (free or studio version)
* clone glm repository https://github.com/bclarksoftware/glm.git
* clone this repository
* and build

````
cd reframe360XL
make
````

# Binary for MacOS
* install [bundle](https://github.com/eltorio/reframe360XL/blob/master/Reframe360.ofx.bundle.zip?raw=true)
* uncompress it and puts it to /Library/OFX/Plugins
