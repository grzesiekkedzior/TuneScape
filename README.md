# TuneScape

**A free and open-source internet radio player for Windows, built with C++ and Qt 6.**

Browse thousands of stations from around the world, search by country, save your favorites, record streams and control playback directly from the Windows system tray.

[Download TuneScape](https://sourceforge.net/projects/tunescape/files/latest/download) · [Homepage](https://grzesiekkedzior.github.io/TuneScape-site/) · [Releases](https://github.com/grzesiekkedzior/TuneScape/releases) · [Report an issue](https://github.com/grzesiekkedzior/TuneScape/issues)

![TuneScape 6.0.0 with the Dark theme](https://grzesiekkedzior.github.io/TuneScape-site/assets/app-dark.png)

## Features

- Browse thousands of community-maintained stations provided by [Radio Browser](https://www.radio-browser.info/).
- Search for stations by name and browse stations by country.
- Save favorite stations and quickly return to them later.
- View station details and open the broadcaster's homepage.
- See live track metadata, cover artwork and available artist information.
- Record the currently playing radio stream locally.
- Control playback and open Top, Popular, New and Favorite station lists from the system tray.
- Use the compact **Now Playing** panel while the main window is minimized.
- Choose from five interface themes: **System, Light, Dark, Classic and Cyberpunk**.

## What's new in 6.0.0

TuneScape 6.0.0 is a major release focused on a cleaner and more consistent Radio Browser experience.

- A new list-based station browser with station icons and clearer information.
- Improved country browsing, favorites and station actions.
- Consistent highlighting of the currently playing station.
- Expanded system tray controls and a refreshed Now Playing panel.
- Five interface themes.
- A major internal refactoring that makes the application easier to maintain and extend.
- Removal of the legacy **Tiles**, **IceCast** and **Spectrum/FFT** modes.

For earlier versions, see [GitHub Releases](https://github.com/grzesiekkedzior/TuneScape/releases).

## Download and installation

TuneScape is currently developed and tested on Windows.

1. Download the latest release from [SourceForge](https://sourceforge.net/projects/tunescape/files/latest/download).
2. Run the installer.
3. Start TuneScape and choose a station.

## Building from source

To build TuneScape, you need:

- Qt 6;
- a C++ compiler supported by your Qt installation;
- Qt Creator or an equivalent Qt development environment.

Clone the repository, open the project in Qt Creator, select a compatible Qt 6 kit and build the project.

```bash
git clone https://github.com/grzesiekkedzior/TuneScape.git
cd TuneScape
```

## Data and third-party services

TuneScape uses open services and community-maintained data:

- [Radio Browser](https://www.radio-browser.info/) for the station directory;
- [MusicBrainz](https://musicbrainz.org/) for music metadata;
- [Cover Art Archive](https://coverartarchive.org/) for cover artwork;
- [Wikipedia](https://www.wikipedia.org/) for available artist information.

Availability and accuracy of station streams and metadata depend on their respective providers.

## Contributing

Contributions are welcome. You can help by:

- reporting a bug through [GitHub Issues](https://github.com/grzesiekkedzior/TuneScape/issues);
- suggesting an improvement or a new feature;
- improving the documentation;
- submitting a pull request;
- starring the repository and sharing TuneScape with other internet-radio listeners.

Before making a larger change, please open an issue so the idea can be discussed first.

## Support TuneScape

TuneScape is developed in free time. If you enjoy using it, you can support its continued development with a donation:

[![Donate via PayPal](https://img.shields.io/badge/Donate%20via%20PayPal-00457C?logo=paypal&logoColor=white&style=for-the-badge)](https://www.paypal.com/donate/?hosted_button_id=MW4VMJ8YHSZF2)

A GitHub star, useful bug report or thoughtful feedback also helps the project grow.

## License

TuneScape is released under the [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.html).

Additional components retain their original licenses:

- the modified **Combinear** theme is based on [Qt Style Sheets Templates](https://qss-stock.devsecstudio.com/) and is licensed under the MIT License;
- the application icon originates from [Iconduck](https://iconduck.com/icons/105034/radio) and is licensed under GPLv2.

See the repository's license and attribution files for complete details.
