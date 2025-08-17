# OpenMW Extremely ~~Shit~~ Simple Mod Manager

A simple-yet-powerful, controller-friendly mod manager for **OpenMW**, designed primarily for Linux-based systems and handheld devices. ESMM handles mod extraction, configuration, and load order management through an intuitive, tabbed interface.

## About

Managing an OpenMW mod list on a handheld device or a Linux desktop can be cumbersome. ESMM (Extremely Simple Mod Manager) aims to simplify the process by providing the essential tools in a single, easy-to-navigate application with full gamepad support. It automates mod extraction, helps configure complex mods, and gives you full control over your data and content load orders.

## Features

-   **Tabbed Interface:** All features are organized into clean, accessible tabs.
-   **Archive Management:**
    -   Automatically scans your `mods` directory for new archives (`.7z`, `.rar`, `.zip`).
    -   Detects mod status: **[New]**, **[Installed]**, or **[Update Available]** with color-coding.
    -   Extracts, updates, or deletes selected mods with the press of a button.
    -   "Toggle New" and "Toggle Updates" buttons for easy batch management.
-   **Mod Configuration:**
    -   A clear, indented-list view of each mod's available options.
    -   Enable or disable entire mods with a single checkbox.
    -   Configure individual components, including support for single-choice (e.g., `00 Core`, `01 Option A`) and multiple-choice groups.
-   **Data Path & Content File Ordering:**
    -   Dedicated tabs to view and reorder your active `data=` paths and `content=` files.
    -   **Controller-Friendly Reordering:** Use **L1/R1** on a focused item to move it up or down in the load order.
    -   Enable or disable individual plugins (`.esp`/`.esm`/`*.omwscripts`/`*.omwaddon`) in the content list.
-   **Smart Auto-Sorting:**
    -   Uses a simple, powerful `openmw_esmm.ini` rules file to apply a community-or-user-defined load order.
    -   Supports wildcard matching for flexible and powerful rules.

### Default File Structure

By default, ESMM looks for files and directories relative to its own location. For the easiest setup, use the following structure:

```
openmw/
├── 7zzs                  # (The 7-zip executable)
├── mods/                 # (Place your mod archives like .zip, .7z, .rar here)
├── mod_data/             # (ESMM will extract mods into this directory)
├── openmw/openmw.cfg     # (Your main OpenMW configuration file)
├── openmw_esmm.ini       # (Optional auto-sorting rules file)
└── openmw_esmm           # (The executable you just built)
```

### Command-Line Arguments

You can override the default paths using command-line arguments.

| Argument         | Description                                        |
| ---------------- | -------------------------------------------------- |
| `--help`         | Show the help message.                             |
| `--7zz`          | Path to the `7zzs` executable.                     |
| `--mod-archives` | Path to the directory containing your mod archives. |
| `--mod-data`     | Path to the directory where mods are extracted.    |
| `--config-file`  | Path to your `openmw.cfg` file.                    |
| `--rules-file`   | Path to your `openmw_esmm.ini` sorting rules file. |

**Example:**
```bash
./openmw_esmm --mod-data /home/user/games/openmw/mod_data/ --config-file /home/user/.config/openmw/openmw.cfg
```

## Auto-Sorting (`openmw_esmm.ini`)

The auto-sorting feature allows you to define a prioritized load order via a simple INI file. This file should be named `openmw_esmm.ini` and placed next to the executable.

The file is split into a `[data]` section for `data=` paths and a `[content]` section for `content=` files.

-   Rules are processed from top to bottom.
-   Wildcards (`*`, `?`) are supported.
-   The first rule that matches a file determines its priority.
-   You can define a priority level with a comment (`# 100`). All subsequent rules will use that priority until a new level is defined.
-   You can also set priority explicitly with an equals sign (`=`).
-   Files with the same priority are sorted alphabetically.

### Example `openmw_esmm.ini`
```ini
##
## Data Path Sorting Rules
##
[data]
# 1000 - Essential Patches
Patch for Purists=1000

# 2000 - Core Fixes & Optimizations
Morrowind Optimization Patch/*=2000

# 3000 - Major Overhauls
Project Atlas/*=3000

# 7000 - Body & Armor Mods
Better Bodies/*/Data Files=7000
Better *=7001

# 9000 - Everything else (catch-all)
*=9000

##
## Content File (Plugin) Sorting Rules
##
[content]
# Core files.
Morrowind.esm=0
Tribunal.esm=1
Bloodmoon.esm=2
# 1000 - Master Files and their patches
Patch for Purists.esm=1000
Patch for Purists*.ESP=1001

# 7000 - Body & Armor Plugins
Better*=7000

# 9000 - Everything else (catch-all)
*=9000
```

## Dependencies

To build ESMM, you will need the following development libraries:

-   **SDL2** (`libsdl2-dev`)
-   **SDL2_ttf** (`libsdl2-ttf-dev`)
-   **Boost** (`libboost-all-dev` or specifically `libboost-filesystem-dev` and `libboost-program-options-dev`)

On a Debian-based system (like Ubuntu, Raspberry Pi OS, etc.), you can install them with:
```bash
sudo apt-get install libsdl2-dev libsdl2-ttf-dev libboost-filesystem-dev libboost-program-options-dev
```

## Building

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/your-username/openmw-esmm.git
    cd openmw-esmm
    ```

2.  **Get Dear ImGui:**
    This project uses Dear ImGui as a dependency. You will need to download it and place the required files in the `libs/imgui/` directory.
    -   Download the latest release from the [Dear ImGui GitHub repository](https://github.com/ocornut/imgui/releases).
    -   Create the directories: `mkdir -p libs/imgui/backends`
    -   From the ImGui zip file, copy the following files into `libs/imgui/`:
        -   `imconfig.h`
        -   `imgui.h`, `imgui.cpp`
        -   `imgui_draw.cpp`
        -   `imgui_internal.h`
        -   `imgui_tables.cpp`
        -   `imgui_widgets.cpp`
        -   `imstb_rectpack.h`, `imstb_textedit.h`, `imstb_truetype.h`
    -   Copy the following backend files into `libs/imgui/backends/`:
        -   `imgui_impl_sdl2.h`, `imgui_impl_sdl2.cpp`
        -   `imgui_impl_sdlrenderer2.h`, `imgui_impl_sdlrenderer2.cpp`

3.  **Compile with CMake:**
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```
    The final executable, `openmw_esmm`, will be located in the `build` directory.

## Credits & Third-Party Libraries

This project would not be possible without the following amazing open-source libraries:

-   [**Dear ImGui**](https://github.com/ocornut/imgui): For the entire graphical user interface.
-   [**SDL2** & **SDL2_ttf**](https://www.libsdl.org/): For windowing, rendering, and input.
-   [**Boost**](https://www.boost.org/): For robust filesystem operations and command-line parsing.
-   [**DejaVu Sans Mono**](https://dejavu-fonts.github.io/): The default font used for the UI, included in the `assets/` directory.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
