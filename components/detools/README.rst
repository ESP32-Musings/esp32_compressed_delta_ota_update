About
=====

An implementation of detools in the C programming language.

Features:

- Incremental apply of `sequential`_ and `in-place`_ patches.

- bsdiff algorithm.

- LZMA, `heatshrink`_ or CRLE compression.

- Dump (and store) the apply patch state at any time. Restore it
  later, possibly after a system reboot or program crash. Only
  `heatshrink`_ and CRLE compressions are currently supported.

Goals:

- Easy to use.

- Low RAM usage.

- Small code size.

- Portable.

.. _heatshrink: https://github.com/atomicobject/heatshrink

.. _sequential: https://detools.readthedocs.io/en/latest/#id1

.. _in-place: https://detools.readthedocs.io/en/latest/#id3

.. _detools.h: https://github.com/eerimoq/detools/blob/master/src/c/detools.h

.. _examples folder: https://github.com/eerimoq/detools/tree/master/src/c/examples
