# The Communist Dogifesto
Thanks for downloading The Communist Dogifesto! I hope you enjoy it.

## Credits and Licensing
### Code
All the code here is distributed under the GPLv3 license, unless noted
otherwise. All non-GPL source files contain a copyright notice denoting the
particular license it is distributed under. For convenience I have also listed
each of them below. Thanks to:

*   **Lode Vandevenne** for *LodePNG* (public domain)
*   **David Reid** for *dr_wav* (public domain)
*   **Stefan Gustavson** for *noise1234* (public domain)
*   **Sean Barrett** for *stbvorbis* (public domain)
*   **Randy Gaul** for *tinyfiles.h* (zlib) which I have modified slightly
*   **datenwolf** for *linmath.h* (WTFPL) to which I have added several modifications

### Assets
All the audio and graphics (**except for the music**) are distributed under the
Creative Commons BY-NC-SA license: https://creativecommons.org/licenses/by-nc-sa/4.0/

All the music was made by **Eric Matyas**, of www.soundimage.org, and is
licensed under the Soundimage International Public License which can be found
there.

## Compiling
Compiling the project should be a fairly straight-forward process, but you will
likely need to edit the makefile to prevent it trying to static link with libs
in the `src/libs/*` directory. Or for other reasons. I never tried compiling
the project on any computer besides my own so your mileage may vary. Either way
the only dependencies are SDL, OpenGL 3.3, and GLEW so it can't be too hard.
