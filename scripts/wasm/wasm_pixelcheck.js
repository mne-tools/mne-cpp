// wasm_pixelcheck.js — Automated WebGL canvas rendering verification
// Injects preserveDrawingBuffer into WebGL context creation, then
// periodically samples the canvas to verify that rendered content exists.
// Results are logged to the browser console with [PIXEL_CHECK] prefix.
//
// Enable via URL parameter:  ?pixelcheck=1
// Or call window.pixelCheck() manually from the dev console.

(function () {
  "use strict";

  var enabled =
    new URLSearchParams(window.location.search).get("pixelcheck") === "1";

  // Patch getContext to force preserveDrawingBuffer when pixel checking
  if (enabled) {
    var origGetContext = HTMLCanvasElement.prototype.getContext;
    HTMLCanvasElement.prototype.getContext = function (type, attrs) {
      if (type === "webgl2" || type === "webgl") {
        attrs = Object.assign({}, attrs || {}, { preserveDrawingBuffer: true });
      }
      return origGetContext.call(this, type, attrs);
    };
  }

  var frameIdx = 0;

  function sampleCanvas() {
    var canvas = document.querySelector("canvas");
    if (!canvas || canvas.width === 0 || canvas.height === 0) {
      return { status: "NO_CANVAS", nonBlack: 0, total: 0 };
    }

    // Draw the WebGL canvas onto a small 2D canvas for cheap pixel reads
    var w = Math.min(canvas.width, 64);
    var h = Math.min(canvas.height, 64);
    var tmp = document.createElement("canvas");
    tmp.width = w;
    tmp.height = h;
    var ctx = tmp.getContext("2d");
    ctx.drawImage(canvas, 0, 0, w, h);
    var imgData = ctx.getImageData(0, 0, w, h);
    var px = imgData.data;

    var nonBlack = 0;
    var total = w * h;
    for (var i = 0; i < px.length; i += 4) {
      if (px[i] > 2 || px[i + 1] > 2 || px[i + 2] > 2) {
        nonBlack++;
      }
    }

    return {
      status: nonBlack > 0 ? "PASS" : "FAIL",
      nonBlack: nonBlack,
      total: total,
      pct: ((nonBlack / total) * 100).toFixed(1),
      canvasW: canvas.width,
      canvasH: canvas.height,
    };
  }

  // Manual check from console
  window.pixelCheck = function () {
    var r = sampleCanvas();
    console.log(
      "[PIXEL_CHECK] " +
        r.status +
        " — " +
        r.nonBlack +
        "/" +
        r.total +
        " pixels have color (" +
        r.pct +
        "%) canvas: " +
        r.canvasW +
        "x" +
        r.canvasH
    );
    return r;
  };

  if (!enabled) return;

  // Automatic periodic check
  var checkInterval = setInterval(function () {
    frameIdx++;
    var r = sampleCanvas();
    if (r.status === "NO_CANVAS") {
      if (frameIdx > 30) {
        console.warn("[PIXEL_CHECK] No canvas after 30 checks — stopping");
        clearInterval(checkInterval);
      }
      return;
    }
    console.log(
      "[PIXEL_CHECK #" +
        frameIdx +
        "] " +
        r.status +
        " — " +
        r.nonBlack +
        "/" +
        r.total +
        " (" +
        r.pct +
        "%)"
    );
    if (r.status === "FAIL" && frameIdx > 3) {
      console.warn(
        "[PIXEL_CHECK] ⚠️ Canvas appears BLANK — possible rendering issue"
      );
    }
    if (frameIdx >= 60) {
      console.log("[PIXEL_CHECK] Stopping after 60 checks");
      clearInterval(checkInterval);
    }
  }, 2000);
})();
