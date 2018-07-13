# Advanced Image Downsampling
***

![banner image](http://www.interactive-biology.com/wp-content/uploads/2012/04/Human-Lung-1280x640.jpg)


## Overview
---

Advanced Image Downsampling project demonstrates tricky approach for very hihj quality image downsampling.
At the current stage demo application shows 2d case, but in future it will be 3d demonstration.

## Features
---

Additional downsampling methods, implemented here for compare:

- Simplest pixel sub sampling
- Smooth Gauss sub sampling
- Bilateral filter based sub sampling
- Advanced downsampling method (proposed)


## Build with Visual Studio
---

Open **downsample2d.sln** and build all targets inside Visual Studio


## Build with CMAKE
---

```shell
md build
cd build
cmake .. -G "Visual Studio 14"
make
```

### Source code quality checks
---

By Cpp check: run command
```
cppcheck.exe --enable=all ./src/universal ./src/win ./src/dwnsmpl ./src/test
```
By KWStyle:
1. Get project KWStyle from [https://github.com/Kitware/KWStyle](https://github.com/Kitware/KWStyle) and build KWStyle.exe
2. Run in command line mode:
```
kwstyle.cmd
```


## References
---
J.Diaz-Garcia, P.Brunet, I.Navazo, P.Vazquez, "Downsampling Methods for Medical Datasets", 2017
https://upcommons.upc.edu/bitstream/handle/2117/111411/IADIS-CGVCV2017-.pdf;jsessionid=876F408154FD5973D28806BF5BDB635C?sequence=1

