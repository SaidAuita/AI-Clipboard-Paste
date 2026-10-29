(function() {
    if (app.documents.length === 0) return;
    var host = (typeof $.global.ClipboardPasteHost !== "undefined") ? $.global.ClipboardPasteHost : (typeof ClipboardPasteHost !== "undefined" ? ClipboardPasteHost : null);
    if (!host || !host.buffer || !host.buffer.items) {
        alert("Clipboard Paste:\nБуфер пуст! Сначала скопируйте объект (Шаг A).");
        return;
    }

    var buf = host.buffer;
    try {
        var test = buf.items[0].name;
    } catch (e) {
        alert("Clipboard Paste:\nИсходный документ был закрыт! Скопируйте объекты заново.");
        host.buffer = null;
        return;
    }

    var destDoc = app.activeDocument;
    var targetLayer = destDoc.activeLayer;
    if (targetLayer.locked) targetLayer.locked = false;
    if (!targetLayer.visible) targetLayer.visible = true;

    var destAb = destDoc.artboards[destDoc.artboards.getActiveArtboardIndex()];
    var destAbRect = destAb.artboardRect;

    var selBounds = buf.bounds;
    var selMidX = (selBounds[0] + selBounds[2]) / 2;
    var selMidY = (selBounds[1] + selBounds[3]) / 2;

    var destMidX = (destAbRect[0] + destAbRect[2]) / 2;
    var destMidY = (destAbRect[1] + destAbRect[3]) / 2;

    var dx = destMidX - selMidX;
    var dy = destMidY - selMidY;

    destDoc.selection = null;
    var newSel = [];
    for (var i = 0; i < buf.items.length; i++) {
        var item = buf.items[i];
        var dup = item.duplicate(targetLayer, ElementPlacement.PLACEATBEGINNING);
        dup.left = item.left + dx;
        dup.top = item.top + dy;
        newSel.push(dup);
    }
    destDoc.selection = newSel;

    // Мгновенное оповещение CEP-панели через CSXSEvent
    try {
        var xLib = new ExternalObject("lib:PlugPlugExternalObject");
        if (xLib) {
            var eventObj = new CSXSEvent();
            eventObj.type = "com.clipboardpaste.bufferChanged";
            eventObj.data = '{"count":' + newSel.length + ',"docName":"' + destDoc.name.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '"}';
            eventObj.dispatch();
        }
    } catch (eEvt) {}
})();
