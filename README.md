# Downstream Keyer with Thumbnails for OBS Studio

> **This plugin is a fork of [Downstream Keyer](https://github.com/exeldro/obs-downstream-keyer) by [Exeldro](https://github.com/exeldro).**
> All credit for the original Downstream Keyer goes to Exeldro. This fork only adds a thumbnail view and tally borders to the dock.
>
> **Ta wtyczka jest forkiem wtyczki [Downstream Keyer](https://github.com/exeldro/obs-downstream-keyer) autorstwa [Exeldro](https://github.com/exeldro).**
> Fork dodaje jedynie widok miniatur i ramkę tally w panelu DSK.

Plugin for OBS Studio to add a Downstream Keyer dock.

Inspired by: https://github.com/obsproject/obs-studio/pull/4491/

## What this fork adds

- **Thumbnail view** – the scene list of every DSK tab can be shown as a grid of live thumbnails
  (gear menu → *Thumbnails*). Transparent areas of a scene are shown on a checkerboard.
  In the thumbnail view **left click** shows the DSK with that scene, **right click** on the scene
  that is shown hides it.
- **Adjustable thumbnail size** – slider in the DSK toolbar, or **Ctrl + mouse wheel** over the list
  (48–480 px). The setting is saved per DSK in the scene collection.
- **Tally** – a red border is drawn around a scene (thumbnail, or row in list mode) while it is visible
  in the program output: shown by the DSK (also during its transition), being the current program scene,
  or nested in what is on air.

Thumbnails are rendered on the OBS graphics thread at a reduced size (max. ~10 fps per scene, a few
scenes per frame) and only while the DSK tab is visible, so the extra GPU load is small.

## Co dodaje ten fork (PL)

- **Widok miniatur** – lista scen każdej zakładki DSK może być wyświetlana jako siatka podglądów na żywo
  (menu zębatki → *Miniatury*). Przezroczyste obszary sceny widać na szachownicy.
  W widoku miniatur **lewy przycisk** włącza DSK z tej sceny, **prawy przycisk** na włączonej scenie
  ją wyłącza.
- **Regulowana wielkość miniatur** – suwak w pasku narzędzi DSK lub **Ctrl + kółko myszy** nad listą
  (48–480 px). Ustawienie zapisuje się dla każdego DSK w kolekcji scen.
- **Tally** – czerwona ramka wokół sceny (miniatury lub wiersza listy), gdy scena jest widoczna w programie:
  wywołana przez DSK (również w trakcie przejścia), jest bieżącą sceną programu albo jest zagnieżdżona
  w tym, co jest na antenie.

## Fixes compared to upstream

- New keyers no longer get a random *Hide After* value (the field was not initialized). A random tiny
  value made the DSK hide immediately, so no scene could be selected. Saved values below 100 ms are
  treated as 0 (off).
- Thumbnails never get wider than the dock; a cell wider than the list used to leave an empty row above the scenes.

## Versions

Fork versions continue from the upstream version they are based on, with a fork number appended:
`0.4.4.2` = upstream Downstream Keyer 0.4.4 + fork release 2.

## Install (Windows)

Download the zip from the releases page and extract it into the OBS Studio installation folder
(e.g. `C:\Program Files\obs-studio`), so that the files end up in:

- `obs-plugins/64bit/downstream-keyer.dll`
- `data/obs-plugins/downstream-keyer/locale/*.ini`

This replaces the original Downstream Keyer if it is installed (same module name, compatible settings).
Requires OBS Studio 31.1 or newer.

## Build

```
cmake --preset windows-x64
cmake --build build_x64 --config RelWithDebInfo
cmake --install build_x64 --config RelWithDebInfo --prefix release
```

The preset uses the Visual Studio 2022 generator; with only Visual Studio 2026 installed, add
`-G "Visual Studio 18 2026"` to the first command. OBS sources, obs-deps and Qt6 are downloaded
automatically as defined in `buildspec.json`.

## License

GPL-2.0, same as the original plugin – see [LICENSE](LICENSE).

## Original author / donations

Original plugin by Exeldro: https://obsproject.com/forum/resources/downstream-keyer.1254/ –
donations to the original author: https://www.paypal.me/exeldro
