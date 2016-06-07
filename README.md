# sFS - A steganographic filesystem
Create a filesystem within the steganographic space of other files. This is still very much a POC but it is functional. This was just a side project I did based on a concept I thought was cool. Enjoy.

# Build Status
![Build Status](https://travis-ci.org/jedi22/sfs.svg?branch=master)

# Disclaimer
The encryption employed in sFS does not currently use authenticated encryption so there is no protection against bit flipping attacks. Also the implemention has not be audited.

# Usage
```bash
./sfs -d /path/to/mount/point
```
The ```-d``` starts the FUSE system in debug mode so it's clear what is going on. This is my prefered way of running the software in its current state.
