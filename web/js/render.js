window.TopsbotRender = (function () {
  const palette = ['#0066ff', '#10b981', '#f59e0b', '#ef4444', '#8b5cf6'];

  // MediaPipe Hands connections (21 landmarks).
  const HAND_CONNECTIONS = [
    [0, 1], [1, 2], [2, 3], [3, 4],
    [0, 5], [5, 6], [6, 7], [7, 8],
    [0, 9], [9, 10], [10, 11], [11, 12],
    [0, 13], [13, 14], [14, 15], [15, 16],
    [0, 17], [17, 18], [18, 19], [19, 20],
    [5, 9], [9, 13], [13, 17],
  ];

  // MediaPipe Pose connections (33 landmarks, subset used for drawing).
  const POSE_CONNECTIONS = [
    [11, 12], [11, 13], [13, 15], [12, 14], [14, 16],
    [11, 23], [12, 24], [23, 24],
    [23, 25], [25, 27], [24, 26], [26, 28],
    [15, 17], [15, 19], [15, 21], [16, 18], [16, 20], [16, 22],
    [27, 29], [27, 31], [28, 30], [28, 32],
    [0, 1], [1, 2], [2, 3], [3, 7],
    [0, 4], [4, 5], [5, 6], [6, 8],
  ];

  function colorForType(type, index) {
    if (!type) return palette[index % palette.length];
    let hash = 0;
    for (let i = 0; i < type.length; i += 1) {
      hash = (hash * 31 + type.charCodeAt(i)) >>> 0;
    }
    return palette[hash % palette.length];
  }

  /** Map image pixel coords to canvas, matching CSS object-fit: contain on .cell img */
  function containTransform(canvasW, canvasH, imgW, imgH) {
    const iw = imgW > 0 ? imgW : canvasW;
    const ih = imgH > 0 ? imgH : canvasH;
    const scale = Math.min(canvasW / iw, canvasH / ih);
    const dw = iw * scale;
    const dh = ih * scale;
    return {
      scale,
      offsetX: (canvasW - dw) / 2,
      offsetY: (canvasH - dh) / 2,
    };
  }

  function connectionsForType(type) {
    if (!type) return [];
    if (type.indexOf('hand') >= 0) return HAND_CONNECTIONS;
    if (type.indexOf('body') >= 0 || type.indexOf('pose') >= 0) return POSE_CONNECTIONS;
    return [];
  }

  function drawOverlay(ctx, overlay, imgW, imgH, layers) {
    if (!overlay || !overlay.targets) return;
    const cw = ctx.canvas.width;
    const ch = ctx.canvas.height;
    ctx.clearRect(0, 0, cw, ch);
    const { scale, offsetX, offsetY } = containTransform(cw, ch, imgW, imgH);

    overlay.targets.forEach((target, ti) => {
      (target.boxes || []).forEach((box, bi) => {
        if (!layers.boxes) return;
        const x = box.x * scale + offsetX;
        const y = box.y * scale + offsetY;
        const w = box.w * scale;
        const h = box.h * scale;
        ctx.strokeStyle = colorForType(box.type || target.type, ti + bi);
        ctx.lineWidth = 2;
        ctx.strokeRect(x, y, w, h);
        if (layers.labels) {
          const label = `${box.type || target.type || 'obj'} ${(box.score || 0).toFixed(2)}`;
          ctx.fillStyle = 'rgba(10,22,40,0.75)';
          ctx.fillRect(x, Math.max(0, y - 18), ctx.measureText(label).width + 10, 18);
          ctx.fillStyle = '#fff';
          ctx.font = '12px sans-serif';
          ctx.fillText(label, x + 4, Math.max(12, y - 5));
        }
        if (layers.trackId && target.track_id) {
          ctx.fillStyle = '#fff';
          ctx.font = '11px sans-serif';
          ctx.fillText(`id:${target.track_id}`, x + 4, y + 14);
        }
      });

      if (!layers.keypoints) return;
      (target.points || []).forEach((pointSet, pi) => {
        const pts = pointSet.points || [];
        const color = colorForType(pointSet.type || target.type, ti + pi + 3);
        const connections = connectionsForType(pointSet.type || '');
        ctx.strokeStyle = color;
        ctx.fillStyle = color;
        ctx.lineWidth = 2;
        connections.forEach(([a, b]) => {
          if (a >= pts.length || b >= pts.length) return;
          const pa = pts[a];
          const pb = pts[b];
          if ((pa.score || 1) < 0.3 || (pb.score || 1) < 0.3) return;
          ctx.beginPath();
          ctx.moveTo(pa.x * scale + offsetX, pa.y * scale + offsetY);
          ctx.lineTo(pb.x * scale + offsetX, pb.y * scale + offsetY);
          ctx.stroke();
        });
        pts.forEach((p) => {
          if ((p.score || 1) < 0.3) return;
          ctx.beginPath();
          ctx.arc(p.x * scale + offsetX, p.y * scale + offsetY, 3, 0, Math.PI * 2);
          ctx.fill();
        });
      });
    });
  }

  return { drawOverlay, colorForType, containTransform };
})();
