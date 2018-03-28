CmdDeck - An Arduino-based command deck with a serial interface

Credits
=======

This code is based in part on the ArduinoDeck code by MikeJewski.  The original
can be found at the following URL:

    https://github.com/MikeJewski/ArduinoDeck

The code has undergone a bit of changes with further changes planned.

Purpose
=======

To provide a hand-held command interface that can control activities on a PC.
The interface to the PC is via usb-to-serial connection and a simple command
set exists to control the deck and receive button touches.


Reading from the serial port
============================

If a line is read with just a numeric value, that indicates that a button
pass pressed.

Other data is also available from the serial port, including logging,
status from the commnads as OK or NOK, etc.

Writing to the serial port/commands
===================================

Commands are expected to have either LF or CR/LF endings

freeze
    This freeze all processsing other than the command processing.  This can
    be used to make many image changes at once

unfreeze
    This unfreezes the processing and shows the updates.

msg1 <msg>
msg2 <msg>
msg3 <msg>
    These will show a message at the message are on the top.
    To show a blank message include the space but no message text.

buttonX <file>
    Where X = 1..15, this will set the file used for a button.


An application program can dynamically process the input/send output in order
to provided the desired response.

