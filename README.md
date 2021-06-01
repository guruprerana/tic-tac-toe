# Tic-Tac-Toe!

Helloooo this is Gugu and Marie welcome you to the game of tic-tac-toe. We always end up drawing the game between ourselves but we challenge you to find someone dumb enough whom you can win against!

Here's the guidelines to compile and run this game:
1. Run `make`
2. Run the server with `./server <PORT_NUMBER>`
3. Run two clients with `./client 127.0.0.1 <PORT_NUMBER>`
4. Follow the instructions on your screen and play!

Cheers

### Limitations

1. Currently, we have no mechanism implemented for a graceful exit when a client disconnects (because, well, we do not know when a client disconnects because we are using UDP). We could implement this by the means of a timer: the game exits when there has been no response for a certain period of time.
2. Does not support the playing of multiple games at the same time.