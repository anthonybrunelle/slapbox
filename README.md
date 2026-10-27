# Slapbox

A slapback delay plugin. It gives you short, punchy echoes for vocals, guitars and drums, with saturation, stereo width and a built-in filter on the echo.

Builds as a **VST3** plugin and a **standalone app**, using [JUCE](https://juce.com).

## Controls

| Control | Range | Default | Description |
| --- | --- | --- | --- |
| **Time** | 40–120 ms | 80 ms | How long until the echo arrives. |
| **Feedback** | 0–60 % | 10 % | How much of the echo repeats. |
| **Mix** | 0–100 % | 35 % | Blend between dry and echo. |
| **Width** | 0–100 % | 50 % | Moves the left and right echoes apart by up to 12 ms. At 0 % the echo is mono and sits in the centre. Works on mono sources too. |
| **Saturation** | 0–100 % | 30 % | Tape-style warmth on the echoes, which builds up with each repeat. |
| **Low Cut / High Cut** | 20 Hz–20 kHz | 80 Hz / 12 kHz | Two-thumb slider that filters the echo only. The dry signal isn't touched. |
| **Bypass** | on / off | off | Fades smoothly to the dry signal without clicking. |

Hover over or drag any knob to see its value.

## Build

You need:

- [CMake](https://cmake.org) 3.22 or newer
- C++17 compiler (Visual Studio 2022 on Windows, Xcode on macOS)
- [JUCE](https://github.com/juce-framework/JUCE)

```sh
cmake -B build -DJUCE_PATH=/path/to/JUCE
cmake --build build --config Release
```

Leave out `-DJUCE_PATH` if JUCE is already installed with `cmake --install`.

The output goes to `build/Slapbox_artefacts/Release/`:

- `VST3/Slapbox.vst3`: copy it into your VST3 folder (on Windows, `C:\Program Files\Common Files\VST3`) and rescan in your DAW.
- `Standalone/`: run it directly with no DAW needed.

## Credits

The interface uses the [Poppins](https://fonts.google.com/specimen/Poppins) font under the SIL Open Font License (see [Resources/OFL.txt](Resources/OFL.txt)).
