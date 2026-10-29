#target illustrator
(function() {
    if (app.documents.length === 0) return;
    var doc = app.activeDocument;
    var rawSel = doc.selection;
    if (!rawSel || rawSel.length === 0) {
        alert("Clipboard Paste:\nВыделите хотя бы один объект для копирования!");
        return;
    }

    var srcLayer = doc.activeLayer;
    var targetLayer = doc.layers.add();
    targetLayer.name = srcLayer ? (srcLayer.name + " Copy") : "Layer Copy";
    if (srcLayer) {
        try { targetLayer.move(srcLayer, ElementPlacement.PLACEBEFORE); } catch(e){}
    }

    var sel = [];
    for (var s = 0; s < rawSel.length; s++) {
        sel.push(rawSel[s]);
    }

    doc.selection = null;
    var newSel = [];
    for (var i = 0; i < sel.length; i++) {
        var item = sel[i];
        var origLeft = item.left;
        var origTop = item.top;
        var dup = item.duplicate(targetLayer, ElementPlacement.PLACEATBEGINNING);
        dup.left = origLeft;
        dup.top = origTop;
        newSel.push(dup);
    }
    doc.activeLayer = targetLayer;
    doc.selection = newSel;
})();
