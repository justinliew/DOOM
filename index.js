function UserAction() {
	var xhttp = new XMLHttpRequest();
	xhttp.onload = function(oEvent) {
		var resp = JSON.parse(xhttp.responseText);
		var frame_b64 = resp.framebuffer;

		var frame = new Image();
		frame.src = 'data:image/png;base64,'+frame_b64;
		const canvas = document.getElementById('framebuffer');
		canvas.width = 1600;
		canvas.height = 1200;
		const ctx = canvas.getContext('2d');
		ctx.drawImage(frame,0,0,600,400);
	};
	// Access-Control-Allow-Origin
	xhttp.open("POST", "https://definitely-shining-penguin.edgecompute.app/", true);
	xhttp.responseType = "text";
//	xhttp.setRequestHeader("Content-type", "application/json");
    xhttp.send("Your JSON Data Here");
}

var Game = {
	stopMain: {}
};

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

	UserAction();

	// hit endpoint
	// https://definitely-shining-penguin.edgecompute.app/

	// render to canvas

	// time

}

	Game.lastTick = performance.now();
	Game.lastRender = Game.lastTick; // Pretend the first draw was on first update.
	Game.tickLength = 50; // This sets your simulation to run at 20Hz (50ms)

	//setInitialState();
	main(performance.now());
})();