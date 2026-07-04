window.TopsbotRender = (function () {
  const palette = ['#0066ff', '#10b981', '#f59e0b', '#ef4444', '#8b5cf6'];

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
      });
    });
  }

  return { drawOverlay, colorForType, containTransform };
})();
