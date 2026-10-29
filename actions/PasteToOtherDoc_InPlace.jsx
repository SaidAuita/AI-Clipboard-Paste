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

    var srcAb = srcDoc.artboards[srcDoc.artboards.getActiveArtboardIndex()];
    var srcAbRect = srcAb.artboardRect;

    var destAb = targetDoc.artboards[targetDoc.artboards.getActiveArtboardIndex()];
    var destAbRect = destAb.artboardRect;

    var targetLayer = targetDoc.activeLayer;
    if (targetLayer.locked) targetLayer.locked = false;
    if (!targetLayer.visible) targetLayer.visible = true;

    var dx = destAbRect[0] - srcAbRect[0];
    var dy = destAbRect[1] - srcAbRect[1];

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
