# DOOM on Fastly Compute

There is a multi-step process to building DOOM for Compute, as it is cobbled together from pieces of different eras.

1. If you need to change the HTML that drives the app, you should modify `index.html` or `index_sp.html` depending on whether you are building the single player or (unsupported) multiplayer version. Then, compile and run `prepweb` by doing `cc prepweb.c -o prepweb` and then running `./prepweb <index>` with either 1 (multiplayer) or 2 (singleplayer). So `./prepweb 2` would prep `index_sp.html` into the `linuxdoom-1.10` directory. The reason this is done is to append newlines onto it so that the C can more easily load it when it needs to serve it. (see `memorywad.`). If there's a better way to handle that load, please feel free to fix this.

2. navigate to the `linuxdoom-1.10` directory and run `make`.

3. navigate to the `compute` folder and run `fastly compute pack -w ../linuxdoom-1.10/linux/linuxxdoom && fastly compute deploy`
NOTE: jliew@fastly.com currently has access to the Compute service so you will need to talk to them.

# References

[Fastly DOOM Demo Documentation](https://www.fastly.com/documentation/solutions/demos/doom/)

[Fastly Blog: Porting DOOM to Compute@Edge](https://www.fastly.com/blog/compute-edge-porting-the-iconic-video-game-doom)
