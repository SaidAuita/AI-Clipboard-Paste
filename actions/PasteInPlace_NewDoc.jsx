#target illustrator
(function() {
    if (app.documents.length === 0) return;
    var srcDoc = app.activeDocument;
    var rawSel = srcDoc.selection;
    if (!rawSel || rawSel.length === 0) {
        alert("Clipboard Paste:\nВыделите хотя бы один объект для копирования!");
        return;
    }

    var sel = [];
    for (var s = 0; s < rawSel.length; s++) {
        sel.push(rawSel[s]);
    }

    var colorSpace = srcDoc.documentColorSpace;
    var abIndex = srcDoc.artboards.getActiveArtboardIndex();
    var srcAb = srcDoc.artboards[abIndex];
    var abRect = srcAb.artboardRect;
    var abW = Math.abs(abRect[2] - abRect[0]);
    var abH = Math.abs(abRect[1] - abRect[3]);
    if (isNaN(abW) || abW <= 0) abW = 800;
    if (isNaN(abH) || abH <= 0) abH = 600;

    var newDoc = app.documents.add(colorSpace, abW, abH);
    newDoc.artboards[0].artboardRect = abRect;

    var targetContainer = newDoc.activeLayer;
    newDoc.selection = null;
    var newSel = [];
    for (var k = 0; k < sel.length; k++) {
        var item = sel[k];
        var origLeft = item.left;
        var origTop = item.top;
        var dup = item.duplicate(targetContainer, ElementPlacement.PLACEATBEGINNING);
        dup.left = origLeft;
        dup.top = origTop;
        newSel.push(dup);
    }
    newDoc.selection = newSel;
    newDoc.activate();
})();
