<h1 align="center">Crepe Vamp Plugin Manual</h1>

<p align="center">
<i>Version APPVERSION for Windows, Mac & Linux</i><br>
<i>Manual by Pierre Guillot</i><br>
<a href="www.ircam.fr">www.ircam.fr</a><br><br>
</p>

<p align="center">
<img src="../resource/Screenshot.png" alt="Example" width="400"/>
</p>

## Table of contents

1. [Introduction](#introduction)
2. [Requirements](#system-requirements)
3. [Installation](#installation)
5. [Credits](#credits)

## Introduction

The Crepe plugin is an implementation of the [CREPE](https://github.com/marl/crepe) monophonic pitch tracker, based on a deep convolutional neural network operating directly on the time-domain waveform input, as a [Vamp plugin](https://www.vamp-plugins.org/). The Crepe plugin analyses the pitch in the audio stream and generates curves corresponding to the fequencies.

The Crepe Vamp Plugin has been designed for use in the free audio analysis application [Partiels](https://forum.ircam.fr/projects/detail/partiels/).

## Requirements

- Windows 10
- MacOS 10.15 (ARM)
- Linux

## Installation

Use the installer for your operating system. The plugin dynamic library (*crepe.dylib* for MacOS, *crepe.dll* for Windows and *crepe.so* for Linux) and the category file (*crepe.cat*) will be installed in your operating system's Vamp plugin installation directory:
- Linux: `~/vamp`
- MacOS: `/Library/Audio/Plug-Ins/Vamp`
- Windows: `C:\Program Files\Vamp`

## Credits

- **[Crepe Vamp plugin](https://www.ircam.fr/)** by Pierre Guillot at IRCAM IMR Department
- **[Crepe](https://github.com/marl/crepe)** model by Jong Wook Kim, Justin Salamon, Peter Li & Juan Pablo Bello
- **[Vamp SDK](https://github.com/vamp-plugins/vamp-plugin-sdk)** by Chris Cannam, copyright (c) 2005-2024 Chris Cannam and Centre for Digital Music, Queen Mary, University of London.
- **[Ircam Vamp Extension](https://github.com/Ircam-Partiels/ircam-vamp-extension)** by Pierre Guillot at [IRCAM IMR department](https://www.ircam.fr/).  

