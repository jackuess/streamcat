# Streamcat
Streamcat is a command line utility for downloading streamed media. It supports
both HTTP Live Streams and MPEG-DASH.

Streamcat can also be used as a library.

MPEG-DASH support is nowhere near complete, but many streams should already
download.

[![asciicast](https://asciinema.org/a/221190.svg)](https://asciinema.org/a/221190)

## Building
Make sure you have all dependencies installed on your system:
* [Libcurl][1]
* [Ffmpeg lib][2]
* [Mxml][3]

First initialize the Git submodules:

    git submodule init --recursive

Optionally run the test suite, to make sure everything is set up
correctly:

    make test

Then simply run make without any arguments:

    make

The executable `streamcat` should now exist and be ready for your usage.

## Usage as a library
`streamcat.c` can be used as a fairly simple, and comprehensive, example of how
to use libstreamcat. In addition to that `streamlisting_test.c` can be sourced.

In the future, I hope to be able to provide both reference documentation and
some annotated examples.

## License

[Apache License 2.0](LICENSE)

[1]: https://curl.haxx.se/libcurl/
[2]: https://www.ffmpeg.org/
[3]: https://www.msweet.org/mxml/
