# OpenMW Extremely ~~Simple~~ Stupid Mod Manager


Mods are extracted into the `ports/openmw/mod_data` directory with their own subfolder. (ie: `ports/openmw/mod_data/Patch for Purists/` is a single mod).

The mod itself can have multiple `data=` directories.

Currently the rules are for data directories:

- Contains a `Data Files/` directory at the top level, ***and*** other directories with `Data Files/` as a subdirectory.
  The main `Data Files/` directory is mandatory, the other directories are optional.
- Contains a `Data Files/` directory at the top level, ***and*** other directories without `Data Files/`.
  The man `Data Files/` directory is mandatory, the other directories can be ignored for now.
- Contains directories with numbers at the beginning as you can see below with "Project Atlas".
  If there are multiple directories with the same number, then only one of them can be enabled at a time. If it contains `00` it must be enabled for any of the other directories to be enabled.
- Contains a multiple top level directories with a sub `Data Files` directory.
  Any or all of the directories can be enabled, show the name of the 

The `content=` files can only be enabled if the directory that contains it is enabled.


## Patch for Purists

This has 1 `data=` directory, and 3 `content=` files.

- mod_data/Patch for Purists
  - Patch for Purists - Book Typos.ESP
  - Patch for Purists - Semi-Purist Fixes.ESP
  - Patch for Purists.esm

## Real Signposts

This has 1 `data=` directory, and 1 `content=` file.

- mod_data/Real Signposts
  - RealSignposts.esp

## Better Morrowind Armor ENG

This has 1 `data=` mandatory directory, and 7 optional `data=` directories, and 7 `content=` files.

- mod_data/Better Morrowind Armor ENG/Data Files
  - Better Morrowind Armor DeFemm(a).ESP
  - Better Morrowind Armor DeFemm(o).ESP
  - Better Morrowind Armor DeFemm(r).ESP
  - Better Morrowind Armor.esp
- mod_data/Better Morrowind Armor ENG/Complete Armor Joints/Data Files
  - Complete Armor Joints.esp
- mod_data/Better Morrowind Armor ENG/Imperial Steel Cuirass with belt/Data Files
- mod_data/Better Morrowind Armor ENG/female Steel Cuirass from LeFemm Armor/Data Files
- mod_data/Better Morrowind Armor ENG/patch for HiRez Armors- Native Styles V2/Data Files
- mod_data/Better Morrowind Armor ENG/patch for LeFemmArmor/Data Files
  - LeFemmArmor.esp
- mod_data/Better Morrowind Armor ENG/patch for Snow Prince Armor Redux/Data Files
  - Snow Prince Armor Redux.ESP
- mod_data/Better Morrowind Armor ENG/patch for installation without Tribunal/Data Files

## Graphic Herbalism MWSE - OpenMW

This has 1 mandatory `data=` directory and 1 optional `data=` directory, and 4 `content=` files.

- mod_data/Graphic Herbalism MWSE - OpenMW/00 Core + Vanilla Meshes
- mod_data/Graphic Herbalism MWSE - OpenMW/01 Optional - Smoothed Meshes

## Morrowind Optimization Patch

This has 1 mandator `data=` directory and 4 optional `data=` directories, with 2 `content=` files.

- mod_data/Morrowind Optimization Patch/00 Core
- mod_data/Morrowind Optimization Patch/01 Lake Fjalding Anti-Suck
  - Lake Fjalding Anti-Suck.ESP
- mod_data/Morrowind Optimization Patch/02 Weapon Sheathing Patch
- mod_data/Morrowind Optimization Patch/03 Chuzei Fix
  - chuzei_helm_no_neck.esp
- mod_data/Morrowind Optimization Patch/04 Better Vanilla Textures
- mod_data/Morrowind Optimization Patch/05 Graphic Herbalism Patch

## Morrowind Optimization Patch

This has 1 `data=` directory, and 1 `content=` file.

- mod_data/Morrowind Weapons Expansion/Data Files
  - Weapons Expansion Morrowind.esp

# OpenMW Containers Animated

This has 1 `data=` directory, and 1 `content=` file.

- mod_data/OpenMW Containers Animated/Containers Animated
  - Containers Animated.esp

# OpenMW Containers Animated

This has 1 mandatory `data=` directory and 9 optional `data=` directories, with 0 `content=` files.

- mod_data/OpenMW Containers Animated/Containers Animated
  - Containers Animated.esp
- mod_data/OpenMW Containers Animated/Optional/kollops

- mod_data/Project Atlas/00 Core
- mod_data/Project Atlas/01 Textures - Intelligent Textures
- mod_data/Project Atlas/01 Textures - MET
- mod_data/Project Atlas/01 Textures - Vanilla
- mod_data/Project Atlas/02 Urns - Smoothed
- mod_data/Project Atlas/03 Redware - Smoothed
- mod_data/Project Atlas/04 Emperor Parasols - Smoothed
- mod_data/Project Atlas/05 Wood Poles - Hi-Res Texture
- mod_data/Project Atlas/06 Glow in the Dahrk Patch
- mod_data/Project Atlas/07 Graphic Herbalism Patch
- mod_data/Project Atlas/08 ILFAS Patch
- mod_data/Project Atlas/09 BC Mushrooms - Normal - Glowing Bitter Coast Patch
- mod_data/Project Atlas/09 BC Mushrooms - Smoothed
- mod_data/Project Atlas/09 BC Mushrooms - Smoothed - Glowing Bitter Coast Patch

# WeaponSheathing1.6-OpenMW

This has one `data=` directory.

- mod_data/WeaponSheathing1.6-OpenMW/Data Files
- mod_data/WeaponSheathing1.6-OpenMW/Extras/alternate draw animations/animation compilation
- mod_data/WeaponSheathing1.6-OpenMW/Extras/alternate draw animations/vanilla

# Better Bodies

Technically has only 1 selectable `data=` directory, but we are going to play by the rules as stated above.

- mod_data/Better Bodies/Nude/Data Files
  - Better Bodies.esp
- mod_data/Better Bodies/Peanut Gallery/Data Files
  - Better Bodies.esp
- mod_data/Better Bodies/Underwear/Data Files
  - Better Bodies.esp



# openmw.cfg

This file is broken up into 3 parts:

## 1. Engine Data

The top of the file has a bunch of settings that the engine needs to run, they are possibly configurable but they are beyond the scope of this project.

```ini
fallback=LightAttenuation_UseConstant,0
fallback=LightAttenuation_ConstantValue,0.0
fallback=LightAttenuation_UseLinear,1
fallback=LightAttenuation_LinearMethod,1
fallback=LightAttenuation_LinearValue,3.0
fallback=LightAttenuation_LinearRadiusMult,1.0
fallback=LightAttenuation_UseQuadratic,0
fallback=LightAttenuation_QuadraticMethod,2
fallback=LightAttenuation_QuadraticValue,16.0
fallback=LightAttenuation_QuadraticRadiusMult,1.0
fallback=LightAttenuation_OutQuadInLin,0

```

This part goes on.

## 2. Data files

The `data=` directives need to be able to be ordered correctly. The core `ports/openmw/data/` directory is always first.


```ini
##
## Core `ports/openmw/data/` directory.
##
data=/mnt/mmc/roms/ports/openmw/data/

## 
## Mod `data=` directives.
## --BEGIN DATA--
data=/mnt/mmc/ports/openmw/mod_data/Morrowind Optimization Patch/00 Core
data=/mnt/mmc/ports/openmw/mod_data/Morrowind Optimization Patch/01 Lake Fjalding Anti-Suck
data=/mnt/mmc/ports/openmw/mod_data/Morrowind Optimization Patch/02 Weapon Sheathing Patch
data=/mnt/mmc/ports/openmw/mod_data/Morrowind Optimization Patch/03 Chuzei Fix
data=/mnt/mmc/ports/openmw/mod_data/Morrowind Optimization Patch/04 Better Vanilla Textures
data=/mnt/mmc/ports/openmw/mod_data/Morrowind Optimization Patch/05 Graphic Herbalism Patch
data=/mnt/mmc/ports/openmw/mod_data/Better Bodies/Underwear/Data Files
data=/mnt/mmc/ports/openmw/mod_data/Better Morrowind Armor ENG/Complete Armor Joints/Data Files
data=/mnt/mmc/ports/openmw/mod_data/Better Morrowind Armor ENG/Data Files
data=/mnt/mmc/ports/openmw/mod_data/Better Morrowind Armor ENG/Imperial Steel Cuirass with belt/Data Files
data=/mnt/mmc/ports/openmw/mod_data/Better Morrowind Armor ENG/female Steel Cuirass from LeFemm Armor/Data Files
data=/mnt/mmc/ports/openmw/mod_data/Better Morrowind Armor ENG/patch for HiRez Armors- Native Styles V2/Data Files
data=/mnt/mmc/ports/openmw/mod_data/Better Morrowind Armor ENG/patch for LeFemmArmor/Data Files
data=/mnt/mmc/ports/openmw/mod_data/Better Morrowind Armor ENG/patch for Snow Prince Armor Redux/Data Files
data=/mnt/mmc/ports/openmw/mod_data/Graphic Herbalism MWSE - OpenMW/00 Core + Vanilla Meshes
data=/mnt/mmc/ports/openmw/mod_data/Graphic Herbalism MWSE - OpenMW/01 Optional - Smoothed Meshes
data=/mnt/mmc/ports/openmw/mod_data/Morrowind Weapons Expansion/Data Files
data=/mnt/mmc/ports/openmw/mod_data/OpenMW Containers Animated/Containers Animated
data=/mnt/mmc/ports/openmw/mod_data/Patch for Purists
data=/mnt/mmc/ports/openmw/mod_data/Project Atlas/00 Core
data=/mnt/mmc/ports/openmw/mod_data/Project Atlas/01 Textures - Vanilla
data=/mnt/mmc/ports/openmw/mod_data/Project Atlas/02 Urns - Smoothed
data=/mnt/mmc/ports/openmw/mod_data/Project Atlas/03 Redware - Smoothed
data=/mnt/mmc/ports/openmw/mod_data/Project Atlas/04 Emperor Parasols - Smoothed
data=/mnt/mmc/ports/openmw/mod_data/Project Atlas/05 Wood Poles - Hi-Res Texture
data=/mnt/mmc/ports/openmw/mod_data/Project Atlas/07 Graphic Herbalism Patch
data=/mnt/mmc/ports/openmw/mod_data/Project Atlas/08 ILFAS Patch
data=/mnt/mmc/ports/openmw/mod_data/Project Atlas/09 BC Mushrooms - Smoothed
data=/mnt/mmc/ports/openmw/mod_data/Real Signposts
data=/mnt/mmc/ports/openmw/mod_data/WeaponSheathing1.6-OpenMW/Data Files
## --END DATA--

fallback-archive=Morrowind.bsa
fallback-archive=Tribunal.bsa
fallback-archive=Bloodmoon.bsa
```


## 3. Content files.

After the `fallback-archive=` directives we then need to define the `content=` directives. The three core morrowind files must be present, then mod `content=` files need to be specified.

```
##
## Core morrowind files.
##
content=Morrowind.esm
content=Tribunal.esm
content=Bloodmoon.esm
## --BEGIN CONTENT--
content=Patch for Purists - Book Typos.ESP
content=Patch for Purists - Semi-Purist Fixes.ESP
content=Patch for Purists.esm
content=RealSignposts.esp
content=Better Morrowind Armor DeFemm(a).ESP
content=Better Morrowind Armor DeFemm(o).ESP
content=Better Morrowind Armor DeFemm(r).ESP
content=Better Morrowind Armor.esp
content=Complete Armor Joints.esp
content=LeFemmArmor.esp
content=Snow Prince Armor Redux.ESP
content=Lake Fjalding Anti-Suck.ESP
content=chuzei_helm_no_neck.esp
content=Weapons Expansion Morrowind.esp
content=Containers Animated.esp
content=Containers Animated.esp
content=Better Bodies.esp
## --END CONTENT--

```

## Finally.

Now the order of all these files are important.

So we need to be able to adjust the order of the `data=` directives, and the order of the `content=` directives.

