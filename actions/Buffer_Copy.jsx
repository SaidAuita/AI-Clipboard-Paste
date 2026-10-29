(function() {
    if (app.documents.length === 0) return;
    var doc = app.activeDocument;
    var rawSel = doc.selection;
    if (!rawSel || rawSel.length === 0) {
        alert("Clipboard Paste:\nВыделите объекты для копирования!");
        return;
    }

    var sel = [];
    var minL = Infinity, maxT = -Infinity, maxR = -Infinity, minB = Infinity;
    for (var s = 0; s < rawSel.length; s++) {
        var itm = rawSel[s];
        sel.push(itm);
        var b = itm.geometricBounds;
        if (b[0] < minL) minL = b[0];
        if (b[1] > maxT) maxT = b[1];
        if (b[2] > maxR) maxR = b[2];
        if (b[3] < minB) minB = b[3];
    }

    var ab = doc.artboards[doc.artboards.getActiveArtboardIndex()];

    // Сохраняем в глобальный контекст сессии Illustrator
    if (typeof $.global.ClipboardPasteHost === "undefined") {
        $.global.ClipboardPasteHost = {};
    }
    var host = $.global.ClipboardPasteHost;

    host.buffer = {
        srcDoc: doc,
        docName: doc.name,
        items: sel,
        count: sel.length,
        bounds: [minL, maxT, maxR, minB],
        artboardRect: ab.artboardRect
    };

    if (typeof ClipboardPasteHost === "undefined") {
        ClipboardPasteHost = host;
    } else {
        ClipboardPasteHost.buffer = host.buffer;
    }

    // Мгновенное оповещение CEP-панели через CSXSEvent
    try {
        var xLib = new ExternalObject("lib:PlugPlugExternalObject");
        if (xLib) {
            var eventObj = new CSXSEvent();
            eventObj.type = "com.clipboardpaste.bufferChanged";
            eventObj.data = '{"count":' + sel.length + ',"docName":"' + doc.name.replace(/\\/g, "\\\\").replace(/"/g, '\\"') + '"}';
            eventObj.dispatch();
        }
    } catch (eEvt) {}
})();
