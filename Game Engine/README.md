# How to install

You should get a Jetson AGX Xavier and flash it with JetPack 4.2 or later.

## Install dependencies

```bash
sudo apt update
sudo apt install libsfml-dev libglew-dev
```

## Build Game Engine

```bash
mkdir build
cd build
cmake ..
make -j4
```

When finished, `Fly` binary will be generated under build directory.

## Run!

```bash
./Fly
```

A window will show up. A service will listen to port 2300. You can go to `../Python\ Apps/` directory for next steps.

