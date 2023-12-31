[![Build status](https://ci.appveyor.com/api/projects/status/ksqojxpjtqo3k4bk?svg=true)](https://ci.appveyor.com/project/RelativisticMechanic/crterm) [![Download zip](https://custom-icon-badges.demolab.com/badge/-Download-blue?style=for-the-badge&logo=download&logoColor=white "Download zip")](https://github.com/RelativisticMechanic/CRTerm/releases/tag/0.5.0)


<img align="right" width="200" height="200" src="images/crterm-logo.png">



# Gautam's CRTerm ![Windows 11](https://img.shields.io/badge/Windows%2011-%230079d5.svg?style=for-the-badge&logo=Windows%2011&logoColor=white) ![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white) ![OpenGL](https://img.shields.io/badge/OpenGL-%23FFFFFF.svg?style=for-the-badge&logo=opengl)

A CRT style terminal for Windows. Watch it in [action](https://www.youtube.com/watch?v=2HmNPndbZS4).

<b>Note: This is a WIP, it is not ready for production!</b>

| Amber with WSL          | Green with cmd.exe      | White with Powershell    | XTerm 256 Color Support    |
| ----------------------  | ----------------------- | ----------------------   | -------------------------- |
|![](images/wsl-amber.png)|![](images/cmd-green.png)| ![](images/winfetch.png) | ![](./images/256color.png) |


| UTF-8 Support | Vim                 | Matrix                  | Mapscii                 |
| ------------------   | ------------------- | ----------------------- | ----------------------- |
| ![](images/utf8.png) | ![](images/vim.png) | ![](images/cmatrix.png) | ![](images/mapscii.png) |

## Motivation

This project is inspired from SwordFish90's [Cool-Retro-Term](https://github.com/Swordfish90/cool-retro-term). Unfortunately, this program while being cool, was only available for Linux / MacOS, so I decided to write one for Windows. 

Note that <i>while it is inspired from Cool-Retro-Term, it is not Cool-Retro-Term!</i> Cool-Retro-Term has undergone ~10 years of development and this does not use any code from that. This has gone about ~10 days in development in comparison, so once again it is in active development and will see many changes. :P

## Description

The terminal supports a subset of VT220, and uses SDL-GPU for rendering. For the UI part, it employs Dear ImGui. It uses Nlohmann's JSON library and Sam Hocevar's portable file dialogs as well. It has a 16 color palette. The mouse is used for selection, copying and pasting. It also implements a scrollback buffer which can be used to view the terminal's history.

The terminal reads the "default" file in its directory which should just contain the path to a JSON file. An example is given in default and config/ located in src/resources. 

The current version is 0.5.0, it is still in active development and may not replace your terminal application.

## Features

* Subset of VT220 is supported
* Mouse selection (copy, and paste)
* 16-color palette
* Scrollback history
* TrueType support
* 256 Color (in upcoming release)
* UTF-8 Support (Work in progress)
* Drag and drop files (plays MP3 in the terminal itself!)

Hey, it looks nice, if you wanted functionality, go for Windows Terminal.

## Upcoming

For the road to 1.0.0, we have the following,

* Support for certain iTerm sequences to allow graphics rendering in the terminal
* Support for terminal resizing
* Support for coloured glyphs
* Full TrueType (including bold and italics)

## Building

Visual Studio 2017/2022 is required, it uses C++17. Windows 10 SDK (June 2018)+ is needed as it uses CreatePseudoConsole() and ConPTY. 

All dependencies are present in the repo, they are:

- ImGui
- SDL2
- SDL_mixer
- SDL2-GPU
- FreeType

The header only dependencies are:

- Nlohmann/JSON
- Hocevar/PDF

Don't forget to add the DLLs to the Debug/ folder whilst running!

Have fun!
