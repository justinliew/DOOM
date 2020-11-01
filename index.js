var Game = {
	stopMain: {},
	curState: {},
	dirty: true
};

// typedef struct
// {
//     char	forwardmove;	// *2048 for move
//     char	sidemove;	// *2048 for move
//     short	angleturn;	// <<16 for angle delta
//     short	consistancy;	// checks for net game
//     byte	chatchar;
//     byte	buttons;
// } ticcmd_t;

function GeneratePostBody() {
	if (!Game.curState.byteLength) {
		return;
	}
	var body = new ArrayBuffer(Game.curState.byteLength + 12); // 4 bytes for length of state, and then 8 bytes for ticcmd
	var view = new DataView(body);
	view.setUint32(0,Game.curState.byteLength);

	// I'm sure there's a better way to do this.
	for (i=0;i<Game.curState.byteLength;++i) {
		view.setUint8(i+4, Game.curState[i]);
	}

	let base = 4 + Game.curState.byteLength;
	view.setUint8[base+0] = 10;
	view.setUint8[base+1] = 11;
	view.setUint16[base+2] = 12;
	view.setUint16[base+4] = 13;
	view.setUint8[base+6] = 14;
	view.setUint8[base+7] = 15;

	return body;
}

// helper in case we need to download data for inspection
function download(filename, data) {
    var element = document.createElement('a');
    element.setAttribute('href', 'data:binary/octet-stream,' + data);
    element.setAttribute('download', filename);

    element.style.display = 'none';
    document.body.appendChild(element);

    element.click();

    document.body.removeChild(element);
}

function GetFrame() {
	var xhttp = new XMLHttpRequest();
	xhttp.onload = function(oEvent) {
		var arraybuffer = xhttp.response;
		if (arraybuffer) {
			var byteArray = new Uint8Array(arraybuffer);
			var dv = new DataView(arraybuffer);
			let fb_len = dv.getInt32(0,true); // framebuffer length
			let fb = new Uint8Array(arraybuffer.slice(4));

			// Obtain a blob: URL for the image data.
			var blob = new Blob( [ fb ], { type: "image/png" } );
			var imageUrl = URL.createObjectURL( blob );
			var frame = new Image();
			frame.onload = function() {
				const canvas = document.getElementById('framebuffer');
				canvas.width = 1600;
				canvas.height = 1200;
				const ctx = canvas.getContext('2d');
				ctx.drawImage(frame,0,0,600,400);
				URL.revokeObjectURL(blob);
			}
			frame.src = imageUrl;

			let gs_len = dv.getInt32(4+fb_len,true);
			Game.curState = new Uint8Array(arraybuffer.slice(8+fb_len));
			Game.dirty = true;
		}
	};
	// Access-Control-Allow-Origin	
	xhttp.open("POST", "https://definitely-shining-penguin.edgecompute.app/", true);
	xhttp.responseType = "arraybuffer";
	var body = GeneratePostBody();
    xhttp.send(body);
}

;(function () {
	function main( tFrame ) {
	Game.stopMain = window.requestAnimationFrame( main );
	var nextTick = Game.lastTick + Game.tickLength;
	var numTicks = 0;

	// If tFrame < nextTick then 0 ticks need to be updated (0 is default for numTicks).
	// If tFrame = nextTick then 1 tick needs to be updated (and so forth).
	// Note: As we mention in summary, you should keep track of how large numTicks is.
	// If it is large, then either your game was asleep, or the machine cannot keep up.
	if (tFrame > nextTick) {
		var timeSinceTick = tFrame - Game.lastTick;
		numTicks = Math.floor( timeSinceTick / Game.tickLength );
	}

	if (Game.dirty)
	{
		GetFrame();
		Game.dirty = false;
	}

	// time

}

	Game.lastTick = performance.now();
	Game.lastRender = Game.lastTick; // Pretend the first draw was on first update.
	Game.tickLength = 100; // This sets your simulation to run at 10Hz (100ms)

	//setInitialState();
	main(performance.now());
})();