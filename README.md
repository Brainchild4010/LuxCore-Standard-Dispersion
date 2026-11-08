# LuxCore Standard Dispersion

**MiniHero** is a compact experimental modification of
[LuxCoreRender](https://github.com/LuxCoreRender/LuxCore) 2.11.2. It improves
the existing Cauchy dispersion of the standard Glass material while keeping
the established LuxCore material nodes and scene controls.

This repository is a community fork and is not an official LuxCoreRender
release.

## Why this fork exists

The original RGB dispersion estimator selects and reconstructs wavelengths at
individual glass events. At stronger Cauchy values and in deeper glass paths,
that can produce an unattractive red bias and inconsistent results between
render engines.

MiniHero uses one wavelength consistently along a complete path and performs
the color reconstruction with a shared, normalized CIE 1931 approximation.
CPU, CUDA and OpenCL use the same wavelength range, conversion matrix and
normalization.

## Main features

- Path-consistent wavelength sampling from 380 to 780 nm
- Normalized CIE 1931 color reconstruction with neutral unit response
- Shared CPU, CUDA and OpenCL implementation
- Low Sobol boot dimension reserved for the path wavelength
- Existing LuxCore Glass material and Cauchy control remain unchanged
- Default Russian Roulette start depth raised from 3 to 16
- Python wheel-builder compatibility fixes for untagged Windows `.pyd` names

## Supported render paths

| Render path | Status |
| --- | --- |
| BiDirCPU | Supported |
| PathCPU | Supported |
| PATHOCL native CPU threads | Supported |
| CUDA | Supported |
| OpenCL | Supported |
| CUDA/OpenCL plus native CPU | Supported |

## Scope and limitations

- The modification currently targets the standard **Glass** material.
- RoughGlass, volumes and full spectral transport are outside this small
  fork's scope.
- This is a one-wavelength path estimator, not the multi-wavelength HERO
  renderer developed in the separate ML HERO project.
- Dispersion still requires additional samples compared with non-dispersive
  glass.
- The implementation is experimental and should be validated with production
  scenes before critical use.

## Source baseline

- Upstream repository: `LuxCoreRender/LuxCore`
- Upstream tag: `wheels-v2.11.2`
- Upstream commit: `b872186d2fe995a762f73708c606847eed55843e`
- Development branch: `luxcore-mod`

## Building a Windows test wheel

Use a Python version matching the target Blender installation. For example,
Blender 5.2 using Python 3.13 requires a `cp313` build; a module linked against
`python314.dll` cannot load in that Blender version.

```bat
git clone --branch luxcore-mod ^
https://github.com/Brainchild4010/LuxCore-Standard-Dispersion.git

cd LuxCore-Standard-Dispersion
make deps
set LUX_BUILD_TYPE=Release
make pyluxcore
make wheel-test
```

The test wheel is written below `out\install\Release\wheel`.

## BlendLuxCore note

No modified Glass node is required. Install the matching `pyluxcore` wheel and
use the normal Glass material with its existing Cauchy dispersion setting.

The LuxCore default for `path.russianroulette.depth` is 16 in this fork. If a
front end explicitly exports another value, the exported value takes priority.

## Upstream project and license

LuxCoreRender is the original rendering engine and remains the authoritative
upstream project. Copyright and attribution stay with the respective LuxCore
contributors. This fork is distributed under the same Apache License 2.0;
see `COPYING.txt` and the original project information below.

---

## Original LuxCoreRender README

### LuxCoreRender

![LuxCoreRender alt text](https://luxcorerender.org/wp-content/uploads/2017/12/wallpaper_lux_05_rend1b.jpg)

LuxCoreRender is a physically correct, unbiased rendering engine. It is built on
physically based equations that model the transportation of light. This allows
it to accurately capture a wide range of phenomena which most other rendering
programs are simply unable to reproduce.

You can find more information about at https://www.luxcorerender.org


### Building

#### Build documentation
Complete build documentation can be found in the wiki: https://wiki.luxcorerender.org/Building_LuxCoreRender.

Here is a short extract, please refer to link above for more information.


#### Tool requirements
[Mandatory] First, ensure you have a suitable toolchain:
- Windows: MSVC >= 194x _latest version_
- Linux: gcc 14
- MacOS Intel: XCode 15.2
- MacOS Arm: XCode 15.4

[Mandatory] Then, ensure the following software is also installed and available in
the PATH:
- Git
- Python 3
- Conan (`pip install conan`)
- CMake

[Optional, but recommended] In addition, you may install the following
software:
- [Github CLI](https://cli.github.com/) (for dependency signature checking)
- [repairwheel](https://pypi.org/project/repairwheel/) (to build test wheels)
- [nektos/act](https://github.com/nektos/act) (to test Github scripts locally)
- [Doxygen](https://www.doxygen.nl/download.html) (to generate development documentation)

[Mandatory, Windows only] For Windows, ensure the command line is configured
for building (`vcvarsall.bat`).


#### Quick build

```
git clone https://github.com/LuxCoreRender/LuxCore.git

cd LuxCore
git checkout for_v2.10

make deps
make
```

This will download LuxCore source code and LuxCore precompiled dependencies,
configure CMake and start the build.

Nota: second `make` statement can also name a specific target. Examples:
`make luxcore` `make pyluxcore` `make luxcoreconsole` `make luxcoreui`

#### Build type

Build type can be controlled by environment variable `LUX_BUILD_TYPE`.
Available build types are `Release` and `Debug` (case sensitive). Default is
`Release`.

#### Other commands

- `make clean`: clean build tree (delete intermediate files)
- `make clear`: remove build tree
- `make config`: configure/reconfigure project
- `make deps`: update dependencies
- `make doc`: build Doxygen documentation



### LuxCore library

LuxCore is the new LuxCoreRender v2.x C++ and Python API. It is released under
Apache Public License v2.0 and can be freely used in open source and commercial
applications.

You can find more information about the API at
https://wiki.luxcorerender.org/LuxCore_API

### LuxCoreUI

This is the most complete example of LuxCore API usage and it is available in
the [`samples/luxcoreui`](samples/luxcoreui) directory.

To see how it works, just run `luxcoreui` from the root directory:

Linux/MacOS:
```
./out/install/Release/bin/luxcoreui scenes/cornell/cornell.cfg
```

Windows:
```
out\install\Release\bin\luxcoreui scenes\cornell\cornell.cfg
```

(assuming you selected Release as a build type)

### LuxCoreConsole

This is a simple example of a command line renderer written using LuxCore API
and it is available in the [`samples/luxcoreconsole`](samples/luxcoreconsole)
directory.
Just run `luxcoreconsole` from the root directory with:

Linux/MacOS:
```
./out/install/Release/bin/luxcoreconsole -D batch.halttime 10 scenes/cornell/cornell.cfg
```

Windows:
```
out\install\Release\bin\luxcoreconsole -D batch.halttime 10 scenes\cornell\cornell.cfg
```

(assuming you selected Release as a build type)
### Authors

See AUTHORS.txt file.

### Credits

A special thanks goes to:

- Alain "Chiaroscuro" Ducharme for Blender 2.5 exporter and several scenes provided;
- Sladjan "lom" Ristic for several scenes provided;
- Riku "rikb" Walve for source patches;
- David "livuxman" Rodriguez for source patches;
- [Daniel "ZanQdo" Salazar](http://www.3developer.com) for Sala scene and Michael "neo2068" Klemm for SLG2 adaptation;
- [Mourelas Konstantinos "Moure"](http://moure-portfolio.blogspot.com) for Room Scene;
- Diego Nehab for PLY reading/writing library;
- [HDR Labs sIBL archive](http://www.hdrlabs.com/sibl/archive.html) and
  [SHT Lab](http://shtlab.blogspot.com/2009/08/hdri-panoramic-skies-for-free.html) for HDR maps;
- [Chronosphere](http://chronosphere.home.comcast.net/~chronosphere/radiosity.htm) for Cornell Blender scene;
- [libPNG](http://www.libpng.org) authors;
- [zlib](http://www.zlib.net) authors;
- [OpenEXR](http://www.openexr.com) authors;
- [OpenImageIO](http://www.openimageio.org) authors;
- [Tomas Davidovic](http://www.davidovic.cz) for [SmallVCM](http://www.smallvcm.com), an endless source of hints;
- [GLFW](http://www.glfw.org) authors;
- [ImGUI](https://github.com/ocornut/imgui) authors;
- [Cycles](https://www.blender.org) authors for HSV/RGB conversion code;
- [OpenVDB](http://www.openvdb.org) authors;
- [Eigen](http://eigen.tuxfamily.org) authors;
- Yangli Hector Yee, Steven Myint and Jeff Terrace for [perceptualdiff](https://github.com/myint/perceptualdiff);
- Michael Labbe for [Native File Dialog](https://github.com/mlabbe/nativefiledialog);
- Sven Forstmann's [quadric mesh simplification code](https://github.com/sp4cerat/Fast-Quadric-Mesh-Simplification).
- [SpdLog](https://github.com/gabime/spdlog) authors

### License

This software is released under Apache License Version 2.0 (see COPYING.txt file).
