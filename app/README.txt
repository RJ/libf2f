A basic app using libf2f, which implements a ping and pong message.

To test, in console 1:
$ bin/f2f-demo 5555

In console 2:
$ bin/f2f-demo 5556
> connect 127.0.0.1 5555
...
> pingall

This will run two servents, the second one will ping and the first will pong.
