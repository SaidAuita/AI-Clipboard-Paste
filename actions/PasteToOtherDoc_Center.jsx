#target illustrator
(function() {
    if (app.documents.length < 2) {
        alert("Clipboard Paste:\nОткрыт только 1 документ!\nОткройте второй документ для вставки.");
        return;
    }

    var srcDoc = app.activeDocument;
    var rawSel = srcDoc.selection;
    if (!rawSel || rawSel.length === 0) {
        alert("Clipboard Paste:\nВыделите объекты для копирования!");
        return;
    }

    var sel = [];
    for (var s = 0; s < rawSel.length; s++) {
        sel.push(rawSel[s]);
    }

    var targetDoc = null;
    for (var k = 0; k < app.documents.length; k++) {
        if (app.documents[k] !== srcDoc) {
            targetDoc = app.documents[k];
            break;
        }
    }

    if (!targetDoc) return;

    var destAb = targetDoc.artboards[targetDoc.artboards.getActiveArtboardIndex()];
    var destAbRect = destAb.artboardRect;

    var targetLayer = targetDoc.activeLayer;
    if (targetLayer.locked) targetLayer.locked = false;
    if (!targetLayer.visible) targetLayer.visible = true;

    // Центр выделения
    var minL = Infinity, maxT = -Infinity, maxR = -Infinity, minB = Infinity;
    for (var b = 0; b < sel.length; b++) {
        var bounds = sel[b].geometricBounds;
        if (bounds[0] < minL) minL = bounds[0];
        if (bounds[1] > maxT) maxT = bounds[1];
        if (bounds[2] > maxR) maxR = bounds[2];
        if (bounds[3] < minB) minB = bounds[3];
    }
    var selMidX = (minL + maxR) / 2;
    var selMidY = (maxT + minB) / 2;

    var destMidX = (destAbRect[0] + destAbRect[2]) / 2;
    var destMidY = (destAbRect[1] + destAbRect[3]) / 2;

    var dx = destMidX - selMidX;
    var dy = destMidY - selMidY;

    targetDoc.selection = null;
    var newSel = [];
    for (var i = 0; i < sel.length; i++) {
        var item = sel[i];
        var dup = item.duplicate(targetLayer, ElementPlacement.PLACEATBEGINNING);
        dup.left = item.left + dx;
        dup.top = item.top + dy;
        newSel.push(dup);
    }

    targetDoc.selection = newSel;
    targetDoc.activate();
})();
